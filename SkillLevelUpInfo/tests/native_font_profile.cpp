// Tests the one-time profile against a local loaded MyGUI DLL, not the game.
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <mygui/MyGUI.h>
#include <mygui/MyGUI_EditText.h>
#include <mygui/MyGUI_CommonStateInfo.h>
#include <mygui/MyGUI_SubSkin.h>
#include <string>
#include <WidgetRef.h>
#include "PanelTextStyle.h"
#include "PanelPlacement.h"
#include <cstdio>
#include <cstring>
#include <cstddef>
#include <cmath>
#include "PanelUnderlay.h"
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
 const bool cacheOk=sizeof(WidgetRef)==24 && sizeof(std::string)==40 && sizeof(XpPanelPlacement::Entry)==56 &&
     offsetof(XpPanelPlacement::Cache,entries)==24 && offsetof(XpPanelPlacement::Cache,count)==3608 &&
     offsetof(XpPanelPlacement::Cache,writes)==3612 && sizeof(XpPanelPlacement::Cache)==3616;
 printf("%s production readonly label/cache ABI: ref24 string40 entry56 entries24 count3608 cache3616\n",cacheOk?"PASS":"FAIL");
 const size_t address=0x10000;
 const MyGUI::SubSkinStateInfo*state=reinterpret_cast<const MyGUI::SubSkinStateInfo*>(address);
 const MyGUI::SubSkin*quad=reinterpret_cast<const MyGUI::SubSkin*>(address);
 const bool resourceOk=sizeof(MyGUI::SubWidgetInfo)==64 && offsetof(MyGUI::SubWidgetInfo,type)==24 &&
     sizeof(MyGUI::VectorSubWidgetInfo)==32 && sizeof(MyGUI::MapWidgetStateInfo)==32 &&
     reinterpret_cast<size_t>(&state->getRect())-address==8 && sizeof(XpPanelUnderlay::Cache)==40 &&
     offsetof(XpPanelUnderlay::Cache,main)==24 && offsetof(XpPanelUnderlay::Cache,known)==32 && offsetof(XpPanelUnderlay::Cache,applied)==33 &&
     offsetof(XpPanelUnderlay::Cache,resourceReads)==36 &&
     reinterpret_cast<size_t>(&(quad->*XpPanelUnderlay::UvAccess::Member()))-address==0x50 && XpPanelUnderlay::NativeUvProfile();
 printf("%s production resource ABI: SubWidgetInfo64/type24 vector32 map32 state-rect8 underlay-cache40 protected-UV50/native-profile\n",resourceOk?"PASS":"FAIL");
 FreeLibrary(engine);return result && vectorOk && cacheOk && resourceOk?0:1;
}
