#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <cstdio>
#include <cstring>
#include <cassert>

// Raw fixtures are deliberately limited to fields read by the disassembled
// native functions. No Gui/RenderManager singleton or game objects are used.
struct Size {int width,height;};
struct FloatSize {float width,height;};
struct Link {Link* next;Link* previous;void* delegate;};
struct Delegate {void** vtable;};
struct Node {void** vtable;unsigned char state[0xA0];};
struct WidgetFixture {unsigned char bytes[0x600];Link head;Link callback;};
typedef void (*SetSize)(void*,const Size&);
typedef void (*SetXY)(void*,int,int);
typedef void (*SetReal)(void*,float,float);
typedef void (*SetFloatSize)(void*,const FloatSize&);
SetSize nativeSize=NULL;
typedef void (*DirtyFn)(void*,void*);
DirtyFn nativeDirty=NULL;
int dirtyCalls=0,coordCallbacks=0,observedCalls=0;
void* observedThis=NULL;
unsigned char skin[0x200],renderItem[0x78];
void* cropVtable[8]={0};
void* skinVtable[20]={0};
void* nodeVtable[20]={0};
void* delegateVtable[8]={0};
Node node={nodeVtable};Delegate delegate={delegateVtable};
void* skins[1]={skin};
template<class T>T& Field(void* base,size_t offset){return *reinterpret_cast<T*>(static_cast<unsigned char*>(base)+offset);}
void Dirty(void* self,void* item){++dirtyCalls;assert(item==renderItem);nativeDirty(self,item);assert(renderItem[0x18]&&Field<unsigned char>(self,0xA0));}
void Changed(void*,void*){++coordCallbacks;}
void ObservedSize(void* cropped,const Size& size){++observedCalls;observedThis=cropped;nativeSize(cropped,size);}
void GuardedSize(void* cropped,const Size& size){if(Field<int>(cropped,0x20)==size.width&&Field<int>(cropped,0x24)==size.height)return;ObservedSize(cropped,size);}
void Init(WidgetFixture& widget,int width,int height,bool notify){
 memset(&widget,0,sizeof(widget));
 Field<void*>(widget.bytes,8)=cropVtable;
 Field<int>(widget.bytes,0x28)=width;Field<int>(widget.bytes,0x2c)=height;
 Field<int>(widget.bytes,0x468)=0xA; // Left|Top: no resize or reposition with parent.
 widget.head.next=widget.head.previous=&widget.head;
 if(notify){widget.callback.next=widget.callback.previous=&widget.head;widget.callback.delegate=&delegate;widget.head.next=widget.head.previous=&widget.callback;}
 Field<void*>(widget.bytes,0x3b8)=&widget.head;
 widget.bytes[0x390]=1; // SkinItem::mSubSkinsVisible; no initial visibility transition.
}
void AttachSkin(WidgetFixture& widget){
 memset(skin,0,sizeof(skin));
 Field<void*>(widget.bytes,0x310)=skins;Field<void*>(widget.bytes,0x318)=skins+1;
 Field<void*>(skin,0)=skinVtable;Field<void*>(skin,0x38)=widget.bytes+8;
 Field<int>(skin,0x20)=Field<int>(widget.bytes,0x28);Field<int>(skin,0x24)=Field<int>(widget.bytes,0x2c);
 Field<int>(skin,0x48)=0x1E; // HStretch|VStretch, follows parent dimensions.
 Field<float>(skin,0x58)=1.0f;Field<float>(skin,0x5c)=1.0f;
 Field<void*>(skin,0x90)=&node;Field<void*>(skin,0x98)=renderItem;
}
int main(int argc,char** argv){
 HMODULE lib=LoadLibraryA("D:\\SteamLibrary\\steamapps\\common\\Kenshi\\MyGUIEngine_x64.dll");assert(lib);
 nativeSize=(SetSize)GetProcAddress(lib,"?setSize@Widget@MyGUI@@UEAAXAEBU?$TSize@H@types@2@@Z");
 nativeDirty=(DirtyFn)GetProcAddress(lib,"?outOfDate@LayerNode@MyGUI@@UEAAXPEAVRenderItem@2@@Z");
 SetXY xy=(SetXY)GetProcAddress(lib,"?setSize@Widget@MyGUI@@QEAAXHH@Z");
 SetReal real=(SetReal)GetProcAddress(lib,"?setRealSize@Widget@MyGUI@@QEAAXMM@Z");
 SetFloatSize floats=(SetFloatSize)GetProcAddress(lib,"?setRealSize@Widget@MyGUI@@QEAAXAEBU?$TSize@M@types@2@@Z");
 skinVtable[0x68/8]=GetProcAddress(lib,"?_setAlign@SubSkin@MyGUI@@UEAAXAEBU?$TSize@H@types@2@@Z");
 skinVtable[0x58/8]=GetProcAddress(lib,"?_updateView@SubSkin@MyGUI@@UEAAXXZ");
 assert(nativeSize&&nativeDirty&&xy&&real&&floats&&skinVtable[13]&&skinVtable[11]);
 cropVtable[2]=(void*)ObservedSize;nodeVtable[14]=(void*)Dirty;delegateVtable[2]=(void*)Changed;
 WidgetFixture parent,child;Init(parent,100,12,false);Init(child,80,12,true);
 Field<void*>(child.bytes,0x40)=parent.bytes+8;AttachSkin(child);
 Size same={80,12};nativeSize(child.bytes+8,same);nativeSize(child.bytes+8,same);
 assert(dirtyCalls==2&&coordCallbacks==2&&Field<int>(child.bytes,0x28)==80);
 printf("Native IntSize equality: two identical calls -> SubSkin dirty=%d, changeCoord callbacks=%d; actual LayerNode+RI dirty bits set\n",dirtyCalls,coordCallbacks);
 if(argc>1&&strcmp(argv[1],"--red")==0){if(dirtyCalls!=0){puts("RED: identical pixel sizes still invalidate native SubSkin/view");return 1;}}
 dirtyCalls=coordCallbacks=observedCalls=0;
 xy(child.bytes,80,12);assert(observedThis==child.bytes+8&&observedCalls==1&&dirtyCalls==1&&coordCallbacks==1);
 real(child.bytes,0.8f,1.0f);assert(observedThis==child.bytes+8&&observedCalls==2&&dirtyCalls==2);
 FloatSize fraction={0.8f,1.0f};floats(child.bytes,fraction);assert(observedCalls==3&&dirtyCalls==3);
 puts("Native XY and both real-size wrappers: Widget* -> cropped this=Widget+8; equal sizes still dirty");
 cropVtable[2]=(void*)GuardedSize;dirtyCalls=coordCallbacks=observedCalls=0;
 for(int i=0;i<10000;++i){real(child.bytes,0.8f,1.0f);xy(child.bytes,80,12);GuardedSize(child.bytes+8,same);}
 assert(dirtyCalls==0&&coordCallbacks==0&&observedCalls==0);
 real(child.bytes,0.809f,1.0f);assert(dirtyCalls==0); // truncation, not nearest rounding.
 real(child.bytes,0.81f,1.0f);assert(Field<int>(child.bytes,0x28)==81&&dirtyCalls==1&&coordCallbacks==1);
 printf("Candidate integer guard:30000 equal calls skip; subpixel0.809 truncates80; changed0.81 reaches81 and dirties once\n");
 // Real parent resizing already updates the unchanged child's clip geometry.
 Field<int>(child.bytes,0x20)=10;
 void* children[1]={child.bytes};Field<void*>(parent.bytes,0x3d8)=children;Field<void*>(parent.bytes,0x3e0)=children+1;
 Size narrow={50,12};nativeSize(parent.bytes+8,narrow);
 assert(Field<int>(child.bytes,0x28)==81&&Field<int>(child.bytes,0x18)==41&&Field<int>(skin,0x84)==40);
 int before=dirtyCalls;Size unchangedChild={81,12};GuardedSize(child.bytes+8,unchangedChild);
 assert(dirtyCalls==before&&Field<int>(skin,0x84)==40);
 Size wide={100,12};nativeSize(parent.bytes+8,wide);
 assert(Field<int>(child.bytes,0x18)==0&&Field<int>(skin,0x84)==81&&dirtyCalls>before);
 puts("Native parent size change: unchanged child receives clip/view propagation; integer guard preserves40px crop and81px restore");
 puts("Important: equal setter still invokes changeCoord listeners and child/skin refresh; global skip changes those observable side effects");
 return 0;
}
