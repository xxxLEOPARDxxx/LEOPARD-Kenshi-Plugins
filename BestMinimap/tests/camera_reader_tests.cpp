#define NOMINMAX

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstring>
#include <cstdio>
#include "../CameraRangePolicy.h"
__declspec(align(16)) unsigned char image[0x6000];DWORD protection[6],state[6];int queries=0,resolves=0,errors=0;bool moduleAvailable=true;unsigned char* functionAt=0;
HMODULE WINAPI FakeGetModuleHandleW(LPCWSTR){return moduleAvailable?reinterpret_cast<HMODULE>(image):0;}
SIZE_T WINAPI FakeVirtualQuery(LPCVOID address,PMEMORY_BASIC_INFORMATION info,SIZE_T){++queries;UINT_PTR p=reinterpret_cast<UINT_PTR>(address),base=reinterpret_cast<UINT_PTR>(image);if(p<base||p-base>=sizeof(image))return 0;size_t n=(p-base)/0x1000;memset(info,0,sizeof(*info));info->BaseAddress=image+n*0x1000;info->RegionSize=0x1000;info->State=state[n];info->Protect=protection[n];return sizeof(*info);}
#define GetModuleHandleW FakeGetModuleHandleW
#define VirtualQuery FakeVirtualQuery
struct CameraClass{void restrictPosition(){}};
namespace KenshiLib{template<class T>intptr_t GetRealAddress(T){++resolves;return reinterpret_cast<intptr_t>(functionAt);}}
void ErrorLog(const char*){++errors;}
struct Player{bool characterEditorMode;bool editor;Player():characterEditorMode(false),editor(false){}void*getLevelEditor(){return editor?this:0;}};struct World{Player*player;};Player player;World world={&player};World*ou=&world;int g_camRangeSet=0;
void Setup(size_t fn=0x1800,size_t radiusAt=0x2800,size_t squaredAt=0x2804){memset(image,0,sizeof(image));for(int i=0;i<6;++i){protection[i]=PAGE_READWRITE;state[i]=MEM_COMMIT;}moduleAvailable=true;IMAGE_DOS_HEADER*dos=reinterpret_cast<IMAGE_DOS_HEADER*>(image);dos->e_magic=IMAGE_DOS_SIGNATURE;dos->e_lfanew=0x100;IMAGE_NT_HEADERS*nt=reinterpret_cast<IMAGE_NT_HEADERS*>(image+0x100);nt->Signature=IMAGE_NT_SIGNATURE;nt->OptionalHeader.SizeOfImage=sizeof(image);functionAt=image+fn;BestCamera::StockProfile(functionAt);int a=static_cast<int>(radiusAt-fn-0x131),b=static_cast<int>(squaredAt-fn-0xf5);memcpy(functionAt+0x12d,&a,4);memcpy(functionAt+0xf1,&b,4);float radius=2250,squared=5062500;memcpy(image+radiusAt,&radius,4);memcpy(image+squaredAt,&squared,4);}
int checks=0,failures=0;
#define C(x) do{++checks;if(!(x)){++failures;printf("FAIL %d: %s\n",__LINE__,#x);}}while(0)
#include "camera_reader_under_test.h"
int main(){Setup();C(DetectNativeCameraRange()==2250);float radius=3000,squared=9000000;memcpy(image+0x2800,&radius,4);memcpy(image+0x2804,&squared,4);C(DetectNativeCameraRange()==3000);
Setup(0x1800,0x800,0x804);C(DetectNativeCameraRange()==2250);Setup(0x1f00);C(DetectNativeCameraRange()==2250);protection[2]=PAGE_GUARD|PAGE_READONLY;C(DetectNativeCameraRange()==0);
Setup();protection[1]=PAGE_NOACCESS;C(DetectNativeCameraRange()==0);Setup();state[2]=MEM_RESERVE;C(DetectNativeCameraRange()==0);Setup();protection[2]=PAGE_EXECUTE;C(DetectNativeCameraRange()==0);
Setup();functionAt[0]=0xe9;C(DetectNativeCameraRange()==0);Setup();functionAt[0x102]^=1;C(DetectNativeCameraRange()==0);
Setup();int d=static_cast<int>(0x7000-0x1800-0x131);memcpy(functionAt+0x12d,&d,4);C(DetectNativeCameraRange()==0);Setup();float wrong=1;memcpy(image+0x2804,&wrong,4);C(DetectNativeCameraRange()==0);
Setup();unsigned nan=0x7fc00000;memcpy(image+0x2800,&nan,4);C(DetectNativeCameraRange()==0);Setup();unsigned inf=0x7f800000;memcpy(image+0x2804,&inf,4);C(DetectNativeCameraRange()==0);
Setup();moduleAvailable=false;C(DetectNativeCameraRange()==0);Setup();float value=0;C(!ReadMainImage(image+sizeof(image)-1,&value,4));C(!ReadMainImage(image,&value,static_cast<size_t>(-1)));unsigned char outside[400];BestCamera::StockProfile(outside);functionAt=outside;C(DetectNativeCameraRange()==0);
Setup();int calls=resolves;g_camRangeSet=7777;C(CameraRange()==7777&&resolves==calls);g_camRangeSet=0;C(CameraRange()==2250&&resolves==calls+1);calls=resolves;for(int i=0;i<1000;++i)C(CameraRange()==2250);C(resolves==calls);player.editor=true;C(CameraRange()==0);player.editor=false;player.characterEditorMode=true;C(CameraRange()==0);player.characterEditorMode=false;g_camRangeSet=4444;C(CameraRange()==4444);g_camRangeSet=0;C(CameraRange()==2250&&resolves==calls);
printf("independent actual reader/policy: %d checks, %d failures\n",checks,failures);return failures?1:0;}
