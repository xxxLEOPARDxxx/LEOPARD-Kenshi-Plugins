#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <IniDefaults.h>
#include <string>
#include <cstdlib>
#include <float.h>

extern "C" IMAGE_DOS_HEADER __ImageBase;

// Settings always belong to this DLL; never depend on the game's cwd.
namespace PluginSettings
{
    inline std::string Path()
    {
        char path[MAX_PATH] = {};
        DWORD n = GetModuleFileNameA((HMODULE)&__ImageBase, path, MAX_PATH);
        if (!n || n >= MAX_PATH) return std::string();
        std::string s(path, n);
        size_t dot = s.rfind('.');
        return dot == std::string::npos ? std::string() :
               IniDefaults::Prepare(s.substr(0, dot) + ".ini");
    }
    inline std::string Text(const char* section, const char* key,
                            const char* def, const std::string& path)
    {
        char text[1024] = {};
        GetPrivateProfileStringA(section, key, def, text, sizeof(text), path.c_str());
        return text;
    }
    inline bool Bool(const char* section, const char* key, bool def,
                     const std::string& path)
    {
        const std::string s = Text(section, key, def ? "1" : "0", path);
        if (!_stricmp(s.c_str(), "true") || !_stricmp(s.c_str(), "yes") ||
            !_stricmp(s.c_str(), "on") || s == "1") return true;
        if (!_stricmp(s.c_str(), "false") || !_stricmp(s.c_str(), "no") ||
            !_stricmp(s.c_str(), "off") || s == "0") return false;
        return def;
    }
    inline float Number(const char* section, const char* key, float def,
                         float lo, float hi, const std::string& path)
    {
        std::string text = Text(section, key, "", path);
        if (text.empty()) return def;
        char* end = NULL;
        double n = strtod(text.c_str(), &end);
        if (end == text.c_str() || *end || !_finite(n)) return def;
        return n < lo ? lo : n > hi ? hi : (float)n;
    }
}
