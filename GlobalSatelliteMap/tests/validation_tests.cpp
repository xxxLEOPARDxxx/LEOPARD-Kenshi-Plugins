#include "../CaptureValidation.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <sstream>
unsigned checks = 3;
void Require(bool ok, const char *name)
{
    ++checks;
    if (!ok)
    {
        std::cerr << "FAIL " << name << "\n";
        std::exit(1);
    }
}
void GrowAfterStat(void *context)
{
    const std::string &path = *static_cast<std::string *>(context);
    HANDLE h = CreateFileA(path.c_str(), GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                           OPEN_EXISTING, 0, NULL);
    Require(h != INVALID_HANDLE_VALUE, "grow after metadata");
    SetFilePointer(h, 0, NULL, FILE_END);
    char extra[4096] = {0};
    DWORD written = 0;
    Require(WriteFile(h, extra, sizeof(extra), &written, NULL) != FALSE && written == sizeof(extra),
            "physical concurrent growth");
    CloseHandle(h);
}
void CancelAfterEntry(void *)
{
    InterlockedIncrement(&GSM::generation);
}
int main()
{
    GSM::ValidationJob j;
    j.folder = "D:/DEV/Kenshi/research/global-satellite-20261031/no-capture-exists/";
    j.generation = InterlockedCompareExchange(&GSM::generation, 0, 0);
    GSM::ValidateCapture(&j);
    if (j.ok)
    {
        std::cerr << "Missing capture was accepted\n";
        return 1;
    }
    j.error.clear();
    InterlockedIncrement(&GSM::generation);
    GSM::ValidateCapture(&j);
    if (j.ok)
    {
        std::cerr << "Cancelled capture was accepted\n";
        return 1;
    }
    std::string text;
    if (GSM::ReadSmall(j.folder + "manifest.gsm", text, 65536))
    {
        std::cerr << "Missing file was read\n";
        return 1;
    }
    const std::string root = "D:/DEV/Kenshi/research/global-satellite-20261031/file-regressions/";
    CreateDirectoryA(root.c_str(), NULL);
    std::string path = root + "payload.bin";
    {
        std::ofstream out(path.c_str(), std::ios::binary | std::ios::trunc);
        out << "0123456789";
    }
    j.generation = InterlockedCompareExchange(&GSM::generation, 0, 0);
    GSM::FileSnapshot snapshot;
    unsigned long long budget = 10;
    Require(GSM::HashFileSnapshot(path, snapshot, budget, j) && budget == 0 && snapshot.size == 10,
            "exact actual-byte budget");

    budget = 100000;
    Require(!GSM::HashFileSnapshot(path, snapshot, budget, j, &GrowAfterStat, &path) &&
                snapshot.hash.empty() && snapshot.readBytes == 10 && budget == 99990,
            "growth after initial stat reads only original ten bytes");
    {
        std::ofstream out(path.c_str(), std::ios::binary | std::ios::trunc);
        out << "0123456789";
    }
    budget = 9;
    Require(!GSM::HashFileSnapshot(path, snapshot, budget, j) && snapshot.hash.empty(),
            "over-budget file rejected before read");
    HANDLE grower = CreateFileA(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL, OPEN_EXISTING, 0, NULL);
    Require(grower != INVALID_HANDLE_VALUE, "open concurrent grower");
    SetFilePointer(grower, 0, NULL, FILE_END);
    DWORD written = 0;
    WriteFile(grower, "growing", 7, &written, NULL);
    budget = 10;
    Require(!GSM::HashFileSnapshot(path, snapshot, budget, j) && snapshot.hash.empty() &&
                budget == 10,
            "concurrent growth rejected without reading beyond budget");
    CloseHandle(grower);
    budget = 17;
    Require(GSM::HashFileSnapshot(path, snapshot, budget, j) && budget == 0,
            "grown stable file exact accounting");
    const unsigned long long verifiedStamp = snapshot.stamp;
    HANDLE changed =
        CreateFileA(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    Require(changed != INVALID_HANDLE_VALUE, "open replacement after hash");
    FILETIME replacement;
    replacement.dwLowDateTime = DWORD(verifiedStamp + 10000000ull);
    replacement.dwHighDateTime = DWORD((verifiedStamp + 10000000ull) >> 32);
    Require(SetFileTime(changed, NULL, NULL, &replacement) != FALSE,
            "change stamp in old race gap");
    CloseHandle(changed);
    WIN32_FILE_ATTRIBUTE_DATA info;
    Require(GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &info) != FALSE,
            "read replacement stamp");
    unsigned long long newStamp =
        (static_cast<unsigned long long>(info.ftLastWriteTime.dwHighDateTime) << 32) |
        info.ftLastWriteTime.dwLowDateTime;
    Require(snapshot.stamp == verifiedStamp && snapshot.stamp != newStamp,
            "old content hash retains old verified stamp");
    for (int i = 0; i < 3; ++i)
    {
        std::ostringstream name;
        name << root << "empty" << i;
        CreateDirectoryA(name.str().c_str(), NULL);
    }
    std::vector<std::string> files;
    unsigned entries = 2;
    Require(!GSM::EnumerateNativeFiles(root, "", files, 0, entries, j) && entries == 0,
            "empty directories consume total traversal budget");
    entries = 4096;
    files.clear();
    Require(!GSM::EnumerateNativeFiles(root, "", files, 0, entries, j, &CancelAfterEntry, NULL) &&
                entries == 4095 && files.empty(),
            "cancellation during enumeration stops after first directory entry");
    InterlockedIncrement(&GSM::generation);
    entries = 4096;
    files.clear();
    Require(!GSM::EnumerateNativeFiles(root, "", files, 0, entries, j) && entries == 4096 &&
                files.empty(),
            "cancelled enumeration performs no traversal");
    budget = 100;
    snapshot.hash = "stale";
    Require(!GSM::HashFileSnapshot(path, snapshot, budget, j) && snapshot.hash.empty() &&
                budget == 100,
            "cancelled hash clears stale result");
    DeleteFileA(path.c_str());
    for (int i = 0; i < 3; ++i)
    {
        std::ostringstream name;
        name << root << "empty" << i;
        RemoveDirectoryA(name.str().c_str());
    }
    std::cout << checks << " production worker/file-race checks passed\n";
}
