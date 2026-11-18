#define WIN32_LEAN_AND_MEAN
#include "../EngineerMaterialsRuntime.h"
extern "C" __declspec(dllexport) bool EngineerCompile(){return EngineerMaterialsRuntime::Install();}
