// Tests the one-time profile against a local loaded MyGUI DLL, not the game.
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <mygui/MyGUI.h>
#include <mygui/MyGUI_EditText.h>
#include <string>
#include <WidgetRef.h>
#include "PanelTextStyle.h"
#include <cstdio>
bool XpPanelTextStyle::g_nativeProfile=false;
int main(){
 HMODULE engine=LoadLibraryW(L"D:/SteamLibrary/steamapps/common/Kenshi/MyGUIEngine_x64.dll");
 if(!engine){printf("FAIL local MyGUI load: %lu\n",GetLastError());return 1;}
 bool result=XpPanelTextStyle::InitializeNativeProfile();
 printf("%s actual installed MyGUI font getter profile (no GUI/game getter calls)\n",result?"PASS":"FAIL");
 FreeLibrary(engine);return result?0:1;
}
