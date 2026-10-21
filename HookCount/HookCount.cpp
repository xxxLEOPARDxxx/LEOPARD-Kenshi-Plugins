// HookCount - диагностика, не для выпуска (22.10.2026).
//
// Вопрос Asur: сколько хуков ставят все плагины при запуске игры. Плагин
// грузится первым (строкой в RE_Kenshi.json первого мода по mods.cfg),
// перехватывает сам KenshiLib::AddHook и пишет в журнал каждый вызов: чей
// (модуль, из которого позвали), куда (модуль+смещение цели) и удачно ли.
// Хуки, поставленные в обход KenshiLib (свои детуры, правка байтов), сюда
// не попадают.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <intrin.h>

#include <cstdio>
#include <string>

#include <Debug.h>
#include <core/Functions.h>

#pragma intrinsic(_ReturnAddress)

namespace
{
    std::string Where(const void* a)
    {
        HMODULE m = NULL;
        char name[MAX_PATH] = "?";
        if (a != NULL &&
            GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               static_cast<LPCSTR>(a), &m) && m != NULL)
        {
            GetModuleFileNameA(m, name, sizeof(name));
            const char* slash = strrchr(name, 0x5C);
            char buf[MAX_PATH + 32];
            _snprintf_s(buf, _TRUNCATE, "%s+0x%llX", slash != NULL ? slash + 1 : name,
                        static_cast<unsigned long long>(static_cast<const char*>(a) - reinterpret_cast<char*>(m)));
            return buf;
        }
        char buf[32];
        _snprintf_s(buf, _TRUNCATE, "%p", a);
        return buf;
    }

    typedef KenshiLib::HookStatus (*AddHookFn)(void*, void*, void**);
    AddHookFn g_origAddHook = NULL;
    unsigned g_count = 0;

    KenshiLib::HookStatus AddHook_hook(void* target, void* detour, void** orig)
    {
        const void* const caller = _ReturnAddress();
        const KenshiLib::HookStatus result = g_origAddHook(target, detour, orig);
        ++g_count;
        char line[512];
        _snprintf_s(line, _TRUNCATE, "HookCount: #%u by %s -> %s status %d", g_count, Where(caller).c_str(),
                    Where(target).c_str(), static_cast<int>(result));
        DebugLog(line);
        return result;
    }
}

__declspec(dllexport) void startPlugin()
{
    HMODULE const lib = GetModuleHandleA("KenshiLib.dll");
    void* const at = lib != NULL
        ? reinterpret_cast<void*>(GetProcAddress(lib, "?AddHook@KenshiLib@@YA?AW4HookStatus@1@PEAX0PEAPEAX@Z"))
        : NULL;
    if (at == NULL ||
        KenshiLib::SUCCESS != KenshiLib::AddHook(at, reinterpret_cast<void*>(&AddHook_hook),
                                                 reinterpret_cast<void**>(&g_origAddHook)))
    {
        ErrorLog("HookCount: could not hook KenshiLib::AddHook");
        return;
    }
    DebugLog("HookCount: installed - every KenshiLib::AddHook call is logged");
}
