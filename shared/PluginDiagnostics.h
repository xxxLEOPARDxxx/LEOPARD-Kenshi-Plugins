#pragma once
#include <Windows.h>
#include <PluginSettings.h>
#include <Debug.h>
#include <string>

// One shared switch per DLL, including all of its translation units.
// Diagnostics never disable ErrorLog; errors remain visible with Debug=0.
namespace PluginDiagnostics
{
    inline volatile LONG& Flag() { static volatile LONG flag = 0; return flag; }
    inline bool Enabled() { return InterlockedCompareExchange(&Flag(), 0, 0) != 0; }
    inline void Configure(const char* section, const std::string& path)
    {
        InterlockedExchange(&Flag(), PluginSettings::Bool(section, "Debug", false, path) ? 1 : 0);
    }
    inline void Write(const std::string& message) { if (Enabled()) DebugLog(message); }
}
