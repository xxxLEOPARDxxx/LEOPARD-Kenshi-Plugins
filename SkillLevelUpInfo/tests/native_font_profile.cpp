// Tests the one-time profile against a local loaded MyGUI DLL, not the game.
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <mygui/MyGUI.h>
#include <mygui/MyGUI_EditText.h>
#include <string>
#include <WidgetRef.h>
#include "PanelTextStyle.h"
#include <cstdio>
#include <cstring>
bool XpPanelTextStyle::g_nativeProfile=false;
int main(){
 HMODULE engine=LoadLibraryW(L"D:/SteamLibrary/steamapps/common/Kenshi/MyGUIEngine_x64.dll");
 if(!engine){printf("FAIL local MyGUI load: %lu\n",GetLastError());return 1;}
 bool result=XpPanelTextStyle::InitializeNativeProfile();
 printf("%s actual installed MyGUI font getter profile (no GUI/game getter calls)\n",result?"PASS":"FAIL");
 MyGUI::VectorWidgetPtr vector;vector.reserve(3);vector.push_back(NULL);
 UINT_PTR words[3];memcpy(words,&vector,sizeof(words));
 const bool vectorOk=sizeof(vector)==32 && sizeof(std::allocator<MyGUI::Widget*>)==1 &&
     words[0]==reinterpret_cast<UINT_PTR>(&vector[0]) && words[1]-words[0]==8 && words[2]-words[0]==24;
 printf("%s production VC10 VectorWidgetPtr size32 empty allocator1 begin/end/cap offsets0/8/16\n",vectorOk?"PASS":"FAIL");
 FreeLibrary(engine);return result && vectorOk?0:1;
}
