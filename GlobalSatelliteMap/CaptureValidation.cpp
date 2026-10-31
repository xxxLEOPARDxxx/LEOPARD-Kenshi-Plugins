#include "CaptureValidation.h"
#include <wincrypt.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cstring>
#pragma comment(lib, "advapi32.lib")
namespace GSM
{
volatile LONG generation = 0;
bool ReadSmall(const std::string &path, std::string &text, unsigned cap)
{
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in)
        return false;
    in.seekg(0, std::ios::end);
    std::streamoff n = in.tellg();
    if (n < 0 || n > cap)
        return false;
    in.seekg(0);
    text.assign(static_cast<size_t>(n), '\0');
    if (n)
        in.read(&text[0], n);
    return !in.fail();
}
namespace
{
struct Digest
{
    HCRYPTPROV provider;
    HCRYPTHASH hash;
    Digest() : provider(0), hash(0)
    {
        if (CryptAcquireContextA(&provider, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
            CryptCreateHash(provider, CALG_SHA_256, 0, 0, &hash);
    }
    ~Digest()
    {
        if (hash)
            CryptDestroyHash(hash);
        if (provider)
            CryptReleaseContext(provider, 0);
    }
    bool Add(const char *p, unsigned n)
    {
        return hash && CryptHashData(hash, reinterpret_cast<const BYTE *>(p), n, 0) != FALSE;
    }
    bool Add(const std::string &s)
    {
        return Add(s.data(), static_cast<unsigned>(s.size()));
    }
    std::string End()
    {
        BYTE b[32];
        DWORD n = 32;
        if (!hash || !CryptGetHashParam(hash, HP_HASHVAL, b, &n, 0) || n != 32)
            return std::string();
        static const char *hex = "0123456789abcdef";
        std::string out;
        for (int i = 0; i < 32; ++i)
        {
            out += hex[b[i] >> 4];
            out += hex[b[i] & 15];
        }
        return out;
    }
};
bool Cancelled(const ValidationJob &j)
{
    return InterlockedCompareExchange(&generation, 0, 0) != j.generation;
}
struct ReadHandle
{
    HANDLE value;
    explicit ReadHandle(HANDLE h) : value(h) {}
    ~ReadHandle()
    {
        if (value != INVALID_HANDLE_VALUE)
            CloseHandle(value);
    }
};
} // namespace
bool HashFileSnapshot(const std::string &path, FileSnapshot &out, unsigned long long &remaining,
                      const ValidationJob &j, void (*afterSnapshot)(void *), void *context)
{
    out = FileSnapshot();
    if (Cancelled(j))
        return false;
    // Never obstruct game saves. Read work is bounded even if another writer grows this file.
    ReadHandle file(CreateFileA(
        path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
        OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_OPEN_REPARSE_POINT, NULL));
    if (file.value == INVALID_HANDLE_VALUE)
        return false;
    BY_HANDLE_FILE_INFORMATION before, after;
    if (!GetFileInformationByHandle(file.value, &before) ||
        (before.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)))
        return false;
    const unsigned long long n =
        (static_cast<unsigned long long>(before.nFileSizeHigh) << 32) | before.nFileSizeLow;
    if (n > remaining)
        return false;
    if (afterSnapshot)
        afterSnapshot(context);
    Digest digest;
    char buffer[65536];
    unsigned long long read = 0;
    unsigned header[32] = {0};
    while (read < n)
    {
        if (Cancelled(j))
            return false;
        const DWORD request =
            static_cast<DWORD>((n - read) < sizeof(buffer) ? (n - read) : sizeof(buffer));
        if (request > remaining)
            return false;
        DWORD actual = 0;
        if (!ReadFile(file.value, buffer, request, &actual, NULL) || actual == 0)
            return false;
        // Enforce actual consumption in every iteration, before hashing or accumulating.
        if (actual > request || actual > remaining || actual > n - read)
            return false;
        remaining -= actual;
        out.readBytes += actual;
        if (read == 0 && actual >= 128)
            std::memcpy(header, buffer, 128);
        if (!digest.Add(buffer, actual))
            return false;
        read += actual;
    }
    if (Cancelled(j) || !GetFileInformationByHandle(file.value, &after) ||
        before.nFileSizeHigh != after.nFileSizeHigh || before.nFileSizeLow != after.nFileSizeLow ||
        before.dwVolumeSerialNumber != after.dwVolumeSerialNumber ||
        before.nFileIndexHigh != after.nFileIndexHigh ||
        before.nFileIndexLow != after.nFileIndexLow ||
        CompareFileTime(&before.ftLastWriteTime, &after.ftLastWriteTime))
        return false;
    const std::string hash = digest.End();
    if (!Hash(hash))
        return false;
    out.hash = hash;
    out.size = n;
    out.stamp = (static_cast<unsigned long long>(after.ftLastWriteTime.dwHighDateTime) << 32) |
                after.ftLastWriteTime.dwLowDateTime;
    std::memcpy(out.header, header, sizeof(header));
    return true;
}
namespace
{
bool FileHash(const std::string &path, std::string &out, unsigned long long &remaining,
              const ValidationJob &j)
{
    FileSnapshot snapshot;
    const bool ok = HashFileSnapshot(path, snapshot, remaining, j);
    out = snapshot.hash;
    return ok;
}
} // namespace
bool EnumerateNativeFiles(const std::string &root, const std::string &relative,
                          std::vector<std::string> &files, int depth, unsigned &entriesRemaining,
                          const ValidationJob &j, void (*afterEntry)(void *), void *context)
{
    if (Cancelled(j) || depth > 8 || files.size() > 4096)
        return false;
    WIN32_FIND_DATAA f;
    HANDLE h = FindFirstFileA((root + relative + "*").c_str(), &f);
    if (h == INVALID_HANDLE_VALUE)
        return !Cancelled(j) && GetLastError() == ERROR_FILE_NOT_FOUND;
    bool ok = true;
    do
    {
        if (Cancelled(j))
        {
            ok = false;
            break;
        }
        const std::string name = f.cFileName;
        if (name == "." || name == "..")
            continue;
        // Count directory entries too: empty directory trees cannot evade this cap.
        if (entriesRemaining == 0 || (f.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ||
            files.size() >= 4096)
        {
            ok = false;
            break;
        }
        --entriesRemaining;
        if (afterEntry)
            afterEntry(context);
        if (Cancelled(j))
        {
            ok = false;
            break;
        }
        if (f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (!EnumerateNativeFiles(root, relative + name + "/", files, depth + 1,
                                      entriesRemaining, j, afterEntry, context))
            {
                ok = false;
                break;
            }
        }
        else
            files.push_back(relative + name);
    } while (FindNextFileA(h, &f));
    if (ok && GetLastError() != ERROR_NO_MORE_FILES)
        ok = false;
    FindClose(h);
    return ok && !Cancelled(j);
}
namespace
{
bool NativeSave(const ValidationJob &j, std::string &value)
{
    std::vector<std::string> files;
    files.push_back("quick.save");
    unsigned entriesRemaining = 4096;
    if (!EnumerateNativeFiles(j.savePath, "zone/", files, 0, entriesRemaining, j) ||
        !EnumerateNativeFiles(j.savePath, "platoon/", files, 0, entriesRemaining, j))
        return false;
    std::sort(files.begin(), files.end());
    Digest d;
    unsigned long long remaining = 512ull * 1024 * 1024;
    for (size_t i = 0; i < files.size(); ++i)
    {
        std::string hash;
        if (!FileHash(j.savePath + files[i], hash, remaining, j) ||
            !d.Add(files[i] + "\n" + hash + "\n"))
            return false;
    }
    value = d.End();
    return Hash(value);
}
std::string Trim(std::string s)
{
    while (!s.empty() &&
           (s[s.size() - 1] == '\r' || s[s.size() - 1] == ' ' || s[s.size() - 1] == '\t'))
        s.erase(s.size() - 1);
    while (!s.empty() && (s[0] == ' ' || s[0] == '\t'))
        s.erase(0, 1);
    return s;
}
bool Inputs(const ValidationJob &j)
{
    Digest d;
    std::string hash;
    unsigned long long remaining = 1024ull * 1024 * 1024;
    if (!FileHash(j.engine, hash, remaining, j) || !d.Add("engine\n" + hash + "\n"))
        return false;
    std::string mods;
    if (!ReadSmall(j.game + "data/mods.cfg", mods, 65536))
        return false;
    for (size_t i = 0; i < j.manifest.sources.size(); ++i)
    {
        const Source &s = j.manifest.sources[i];
        if (!FileHash(j.game + s.file, hash, remaining, j) || hash != s.hash ||
            !d.Add(s.file + "\n" + hash + "\n"))
            return false;
    }
    std::istringstream in(mods);
    std::string mod;
    while (std::getline(in, mod))
    {
        mod = Trim(mod);
        if (mod.empty())
            continue;
        bool found = false;
        for (size_t i = 0; i < j.manifest.sources.size(); ++i)
        {
            const std::string &file = j.manifest.sources[i].file;
            if (file.size() > mod.size() && file[file.size() - mod.size() - 1] == '/' &&
                _stricmp(file.c_str() + file.size() - mod.size(), mod.c_str()) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found)
            return false;
    }
    return d.End() == j.manifest.world;
}
} // namespace
DWORD WINAPI ValidateCapture(LPVOID input)
{
    ValidationJob &j = *static_cast<ValidationJob *>(input);
    j.ok = false;
    // Lower CPU and I/O priority; the worker never competes as a render task.
    SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);
    try
    {
        j.error = "capture manifest missing or invalid";
        std::string text;
        if (!ReadSmall(j.folder + "manifest.gsm", text, 65536) || !Parse(text, j.manifest, j.error))
            return 0;
        j.error = "capture sources do not match this world";
        if (!Inputs(j))
            return 0;
        j.error = "capture does not match this saved world";
        std::string save;
        if (!NativeSave(j, save) || save != j.manifest.save)
            return 0;
        j.error = "missing, changed or invalid capture texture";
        unsigned long long remaining = 1024ull * 1024 * 1024;
        for (size_t i = 0; i < j.manifest.assets.size(); ++i)
        {
            Asset &a = j.manifest.assets[i];
            FileSnapshot snapshot;
            if (!HashFileSnapshot(j.folder + a.file, snapshot, remaining, j) ||
                snapshot.hash != a.hash || !DDSHeader(snapshot.header, snapshot.size, a.size))
                return 0;
            // Content, DDS header and stamp come from the SAME verified file handle.
            a.stamp = snapshot.stamp;
        }
        std::string after;
        if (!NativeSave(j, after) || after != save)
        {
            j.error = "save changed during validation";
            return 0;
        }
        j.ok = !Cancelled(j);
        if (j.ok)
            j.error.clear();
    }
    catch (...)
    {
        j.error = "capture validation failed";
    }
    return 0;
}
} // namespace GSM
