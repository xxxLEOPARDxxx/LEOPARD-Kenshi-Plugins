#pragma once
#include <string>
#include <cstdio>
namespace PluginDiagnostics {inline bool Enabled(){return false;}inline void Write(const std::string&){} }
inline void ErrorLog(const std::string& message){fprintf(stderr,"%s\n",message.c_str());}
