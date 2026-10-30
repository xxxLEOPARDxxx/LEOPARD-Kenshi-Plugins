// Header layout only: no GUI object is constructed, opened or invoked.
#include <mygui/MyGUI.h>
#include <mygui/MyGUI_SubSkin.h>
#include <cstdio>
struct SkinUvAccess:MyGUI::SubSkin{
 static MyGUI::FloatRect MyGUI::SubSkin::* Member(){return &SkinUvAccess::mRectTexture;}
};
int main(){
 const size_t address=0x10000;
 MyGUI::Widget*w=reinterpret_cast<MyGUI::Widget*>(address);
 const size_t skin=reinterpret_cast<size_t>(static_cast<MyGUI::SkinItem*>(w))-address;
 const size_t crop=reinterpret_cast<size_t>(static_cast<MyGUI::ICroppedRectangle*>(w))-address;
 MyGUI::SubSkin*quad=reinterpret_cast<MyGUI::SubSkin*>(address);
 const size_t uv=reinterpret_cast<size_t>(&(quad->*SkinUvAccess::Member()))-address;
 printf("Widget SkinItem base=%Ix ICroppedRectangle base=%Ix protected SubSkin UV=%Ix\n",skin,crop,uv);
 return skin==0x288 && crop==8 && uv==0x50?0:1;
}
