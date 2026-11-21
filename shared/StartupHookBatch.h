#ifndef LEOPARD_STARTUP_HOOK_BATCH_H
#define LEOPARD_STARTUP_HOOK_BATCH_H

#include <Windows.h>
#include <wincrypt.h>
#include <cstring>
#pragma comment(lib, "advapi32.lib")

namespace StartupHooks {
    struct Hook
    {
        void* target;
        void* detour;
        void** original;
        const char* name;
        bool enabled;
        bool installed;

        template<class Target, class Detour, class Original>
        Hook(Target address, Detour callback, Original* trampoline,
             const char* label = NULL, bool wanted = true)
            : target(reinterpret_cast<void*>(address)),
              detour(reinterpret_cast<void*>(callback)),
              original(reinterpret_cast<void**>(trampoline)),
              name(label), enabled(wanted), installed(false)
        {}
    };

    struct Backend
    {
        typedef int (*Queue)(size_t, void*, void*, void**);
        typedef int (*Apply)(size_t);
        size_t owner;
        Queue queue;
        Apply apply;

        Backend(size_t id = 0, Queue add = NULL, Apply finish = NULL)
            : owner(id), queue(add), apply(finish)
        {}

        bool Ready() const { return owner && queue && apply; }
    };

    inline bool Eligible(const Hook& row)
    {
        return row.enabled && row.target && row.detour && row.original && !*row.original;
    }

    // QueueHook uses the module ID for all rows. Duplicate targets require the
    // immediate SDK path, where AddHook assigns a separate ID per installation.
    inline bool DistinctRows(const Hook* rows, size_t count)
    {
        for (size_t i = 0; i < count; ++i)
        {
            if (!Eligible(rows[i])) continue;
            for (size_t j = 0; j < i; ++j)
                if (Eligible(rows[j]) &&
                    (rows[j].target == rows[i].target || rows[j].original == rows[i].original))
                    return false;
        }
        return true;
    }

    template<class Immediate>
    void InstallWithBackend(Hook* rows, size_t count, const Backend& sdk, Immediate immediate)
    {
        if (!sdk.Ready() || !DistinctRows(rows, count))
        {
            for (size_t i = 0; i < count; ++i)
            {
                Hook& row = rows[i];
                if (Eligible(row))
                    row.installed = immediate(row.target, row.detour, row.original);
            }
            return;
        }

        // Use installed as queued-success scratch until publication below.
        // These rows are private to startPlugin and are never read by detours.
        bool queued = false;
        bool originalsReady = true;
        for (size_t i = 0; i < count; ++i)
        {
            Hook& row = rows[i];
            if (!Eligible(row)) continue;
            try { row.installed = sdk.queue(sdk.owner, row.target, row.detour, row.original) == 0; }
            catch (...) { row.installed = false; }
            if (!row.installed) continue;
            queued = true;
            originalsReady = originalsReady && *row.original != NULL;
        }

        // The SDK may keep some hooks active after an aggregate failure.
        // Their trampolines remain valid and must never be reset or rehooked.
        bool applied = false;
        if (queued && originalsReady)
        {
            try { applied = sdk.apply(sdk.owner) == 0; }
            catch (...) {}
        }
        if (!applied)
            for (size_t i = 0; i < count; ++i) rows[i].installed = false;
    }

    namespace Detail {
        inline bool KnownSdkFile(HMODULE sdk)
        {
            wchar_t path[MAX_PATH];
            const DWORD length = GetModuleFileNameW(sdk, path, MAX_PATH);
            if (!length || length >= MAX_PATH) return false;

            const HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                                            OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
            if (file == INVALID_HANDLE_VALUE) return false;

            static const unsigned char expected[] = {
                0x5b, 0x62, 0x40, 0x62, 0x2d, 0x06, 0x80, 0x1d,
                0xc3, 0x98, 0x96, 0xe3, 0x85, 0xef, 0xb8, 0x34,
                0xb8, 0xe1, 0xfe, 0x2f, 0xcd, 0xf8, 0x60, 0x67,
                0xd0, 0x71, 0xae, 0x7a, 0x04, 0xf6, 0xc8, 0x3b
            };
            const DWORD sdkBytes = 951808;
            HCRYPTPROV provider = 0;
            HCRYPTHASH hash = 0;
            bool known = false;
            if (GetFileSize(file, NULL) == sdkBytes &&
                CryptAcquireContextW(&provider, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT) &&
                CryptCreateHash(provider, CALG_SHA_256, 0, 0, &hash))
            {
                unsigned char block[16384];
                DWORD remaining = sdkBytes;
                while (remaining)
                {
                    DWORD read = 0;
                    const DWORD request = remaining < sizeof(block) ? remaining : sizeof(block);
                    if (!ReadFile(file, block, request, &read, NULL) || !read ||
                        !CryptHashData(hash, block, read, 0))
                        break;
                    remaining -= read;
                }
                unsigned char digest[sizeof(expected)];
                DWORD length = sizeof(digest);
                known = !remaining && CryptGetHashParam(hash, HP_HASHVAL, digest, &length, 0) &&
                        length == sizeof(digest) && std::memcmp(digest, expected, length) == 0;
            }
            if (hash) CryptDestroyHash(hash);
            if (provider) CryptReleaseContext(provider, 0);
            CloseHandle(file);
            return known;
        }

        inline Backend ResolveBackend()
        {
            // The SDK's static public wrappers can use different module IDs.
            // Resolve the two verified 0.5.0 exports and give both one ID.
            const HMODULE sdk = GetModuleHandleW(L"KenshiLib.dll");
            if (!sdk) return Backend();
            const unsigned char* const image = reinterpret_cast<const unsigned char*>(sdk);
            const FARPROC queue = GetProcAddress(sdk, "?QueueHookInternal@@YA?AW4HookStatus@KenshiLib@@_KPEAX1PEAPEAX@Z");
            const FARPROC apply = GetProcAddress(sdk, "?ApplyQueuedHooksInternal@@YA?AW4HookStatus@KenshiLib@@_K@Z");
            const unsigned char queuePrefix[] = {
                0x40, 0x53, 0x57, 0x48, 0x81, 0xec, 0xb8, 0, 0, 0, 0x48, 0xc7, 0x44, 0x24, 0x20, 0xfe
            };
            const unsigned char applyPrefix[] = {
                0x40, 0x53, 0x48, 0x81, 0xec, 0xb0, 0, 0, 0, 0x48, 0xc7, 0x44, 0x24, 0x20, 0xfe, 0xff
            };
            if (reinterpret_cast<const unsigned char*>(queue) != image + 0x2540 ||
                reinterpret_cast<const unsigned char*>(apply) != image + 0x26a0 ||
                std::memcmp(reinterpret_cast<const void*>(queue), queuePrefix, sizeof(queuePrefix)) != 0 ||
                std::memcmp(reinterpret_cast<const void*>(apply), applyPrefix, sizeof(applyPrefix)) != 0 ||
                !KnownSdkFile(sdk))
                return Backend();

            HMODULE owner = NULL;
            if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                    GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                    reinterpret_cast<LPCWSTR>(&ResolveBackend), &owner))
                return Backend();
            return Backend(reinterpret_cast<size_t>(owner), reinterpret_cast<Backend::Queue>(queue),
                           reinterpret_cast<Backend::Apply>(apply));
        }
    }

    template<size_t Count, class Immediate>
    void Install(Hook (&rows)[Count], Immediate immediate)
    {
        InstallWithBackend(rows, Count, Detail::ResolveBackend(), immediate);
    }
}

#endif
