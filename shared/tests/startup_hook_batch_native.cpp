#include "../StartupHookBatch.h"
#include <cstdio>
#include <cstdlib>

namespace {
    typedef int (*Target)();
    typedef int (*Immediate)(void*, void*, void**);
    Target originals[8], predecessor, successor;
    Target targets[8];

    template<int Index>
    int Detour() { return originals[Index]() + 100; }

    int Predecessor() { return predecessor() + 10; }
    int Successor() { return successor() + 1000; }

    void Check(bool success, const char* message)
    {
        if (!success) { std::fprintf(stderr, "FAIL %s\n", message); std::exit(1); }
    }
}

int wmain(int argc, wchar_t** argv)
{
    Check(argc == 3, "expected absolute SDK path and installation mode");
    const HMODULE sdk = LoadLibraryW(argv[1]);
    Check(sdk != NULL, "load actual SDK into the test process");
    const StartupHooks::Backend backend = StartupHooks::Detail::ResolveBackend();
    if (wcscmp(argv[2], L"fallback") == 0)
    {
        Check(!backend.Ready(), "unknown SDK must not use the private exports");
        void* original = NULL;
        StartupHooks::Hook rows[] = {
            StartupHooks::Hook(sdk, &Predecessor, &original)
        };
        StartupHooks::Install(rows, [sdk](void* target, void*, void** result) -> bool {
            *result = sdk;
            return target == sdk;
        });
        Check(rows[0].installed && original == sdk, "unsupported SDK goes straight to immediate installation");
        std::puts("PASS unsupported SDK falls back before touching any queue");
        return 0;
    }
    Check(backend.Ready(), "admit exact installed SDK");
    Check(backend.owner == reinterpret_cast<size_t>(GetModuleHandleW(NULL)), "queue is owned by this module");

    // Private initialization is test-only, with the SDK pinned by ResolveBackend.
    // No engine initialization or Kenshi executable is involved.
    const unsigned char* const sdkBytes = reinterpret_cast<const unsigned char*>(sdk);
    const unsigned char initPrefix[] = {
        0x40, 0x53, 0x48, 0x83, 0xec, 0x60, 0x48, 0x8b, 0x05, 0xff, 0x15, 0x0d, 0, 0x48, 0x33, 0xc4
    };
    Check(std::memcmp(sdkBytes + 0x14884, initPrefix, sizeof(initPrefix)) == 0, "exact isolated MinHook initialization");
    Check(reinterpret_cast<Target>(const_cast<unsigned char*>(sdkBytes) + 0x14884)() == 0, "initialize standalone hook engine");

    const Immediate addHook = reinterpret_cast<Immediate>(GetProcAddress(
        sdk, "?AddHook@KenshiLib@@YA?AW4HookStatus@1@PEAX0PEAPEAX@Z"));
    Check(addHook != NULL, "locate existing API");

    // Synthetic functions with different return values. The process owns these
    // functions and their trampolines until exit.
    const SIZE_T pageBytes = 4096;
    unsigned char* const page = static_cast<unsigned char*>(
        VirtualAlloc(NULL, pageBytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    Check(page != NULL, "allocate test targets");
    for (int i = 0; i != 8; ++i)
    {
        unsigned char* const body = page + i * 16;
        const unsigned char code[] = {0xb8, static_cast<unsigned char>(i), 0, 0, 0, 0x90, 0xc3};
        std::memcpy(body, code, sizeof(code));
        targets[i] = reinterpret_cast<Target>(body);
    }
    DWORD protection;
    Check(VirtualProtect(page, pageBytes, PAGE_EXECUTE_READ, &protection) != 0, "publish test targets");
    FlushInstructionCache(GetCurrentProcess(), page, pageBytes);

    const auto immediate = [addHook](void* target, void* detour, void** original) {
        return addHook(target, detour, original) == 0;
    };
    Check(immediate(page, reinterpret_cast<void*>(&Predecessor), reinterpret_cast<void**>(&predecessor)),
          "install earlier plugin's hook");
    Check(targets[0]() == 10, "earlier hook active");

    StartupHooks::Hook rows[] = {
        StartupHooks::Hook(targets[0], Detour<0>, &originals[0]),
        StartupHooks::Hook(targets[1], Detour<1>, &originals[1]),
        StartupHooks::Hook(targets[2], Detour<2>, &originals[2]),
        StartupHooks::Hook(targets[3], Detour<3>, &originals[3]),
        StartupHooks::Hook(targets[4], Detour<4>, &originals[4]),
        StartupHooks::Hook(targets[5], Detour<5>, &originals[5]),
        StartupHooks::Hook(targets[6], Detour<6>, &originals[6]),
        StartupHooks::Hook(targets[7], Detour<7>, &originals[7]),
    };
    const bool batch = wcscmp(argv[2], L"batch") == 0;
    LARGE_INTEGER start, end, frequency;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);
    if (batch)
        StartupHooks::Install(rows, immediate);
    else
        StartupHooks::InstallWithBackend(rows, 8, StartupHooks::Backend(), immediate);
    QueryPerformanceCounter(&end);

    for (int i = 0; i != 8; ++i)
    {
        Check(rows[i].installed && originals[i] != NULL, "successful row keeps its native trampoline");
        Check(originals[i]() == (i == 0 ? 10 : i), "original points into legacy chain");
        Check(targets[i]() == (i == 0 ? 110 : 100 + i), "detours are active after install returns");
    }
    Check(immediate(page, reinterpret_cast<void*>(&Successor), reinterpret_cast<void**>(&successor)),
          "install later plugin over batched hook");
    Check(successor() == 110 && targets[0]() == 1110 && predecessor() == 0, "both plugin chain neighbours preserved");
    std::printf("PASS %s eight real SDK hooks and load-order chain, %.3f ms\n",
                batch ? "batch" : "immediate", 1000.0 * (end.QuadPart - start.QuadPart) / frequency.QuadPart);
    return 0;
}
