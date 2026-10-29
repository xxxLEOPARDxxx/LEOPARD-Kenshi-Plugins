"""Compile exact current UITweaks helpers and dispatch into installed MyGUI."""
from pathlib import Path
import os,re,subprocess,sys,hashlib,json,time
fixtures=Path(__file__).resolve().parent
out=fixtures/'_generated';out.mkdir(exist_ok=True)
source=Path(r'D:\DEV\Kenshi\ours\UITweaks\UITweaks.cpp')
text=source.read_text(encoding='utf-8-sig')
def function(name):
    m=re.search(r'^    void '+re.escape(name)+r'\(',text,re.M);assert m,name
    begin=m.start();opening=text.index('{',begin);end=opening+1;depth=1
    while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[begin:end]
base=(fixtures/'native_size_test.cpp').read_text(encoding='utf-8')
base=base[:base.index('int main(')]
base=base.replace('#include <cassert>','#include <cassert>\n#include <mygui/MyGUI_Widget.h>')
base=base.replace('void Changed(void*,void*){++coordCallbacks;}','void Changed(void*,void*);')
helpers='''
bool g_skipSameMoves=true;
typedef void (*SetSizePointFn)(MyGUI::ICroppedRectangle*,const MyGUI::IntSize&);
typedef void (*SetRealSizeFn)(MyGUI::Widget*,const MyGUI::FloatSize&);
SetSizePointFn g_origSetSizePoint=NULL;
SetRealSizeFn g_origSetRealSize=NULL;
__declspec(thread) MyGUI::ICroppedRectangle* g_relativeSizeTarget=NULL;
'''+function('SetSizePoint_hook')+'\n'+function('SetRealSize_hook')
extra=r'''
int originalPointCalls=0,derivedCalls=0,nestedCalls=0;
SetFloatSize nativeFloatSize=NULL;
MyGUI::Widget* nestedWidget=NULL;
MyGUI::ICroppedRectangle* expectedPrevious=NULL;
bool reentrant=false,reentered=false;
bool nestedFromCallback=false,callbackNestedDone=false;
int callbackThrow=0,callbackNestedCalls=0;
float callbackNestedFraction=0.8f;
MyGUI::Widget* callbackNestedWidget=NULL;
void Changed(void*,void* sender){
 ++coordCallbacks;
 if(callbackThrow==1){assert(g_relativeSizeTarget==NULL);throw 23;}
 if(callbackThrow==2){assert(g_relativeSizeTarget==NULL);RaiseException(0xE1234568,0,0,NULL);}
 if(nestedFromCallback&&!callbackNestedDone){
  callbackNestedDone=true;assert(g_relativeSizeTarget==NULL);
  ++callbackNestedCalls;SetRealSize_hook(callbackNestedWidget,MyGUI::FloatSize(callbackNestedFraction,1));
  assert(g_relativeSizeTarget==NULL);
  MyGUI::Widget* widget=(MyGUI::Widget*)sender;SetSizePoint_hook(static_cast<MyGUI::ICroppedRectangle*>(widget),widget->getSize());
 }
 if(reentrant&&!reentered){reentered=true;MyGUI::Widget* widget=(MyGUI::Widget*)sender;SetSizePoint_hook(static_cast<MyGUI::ICroppedRectangle*>(widget),widget->getSize());}
}
void OriginalPoint(MyGUI::ICroppedRectangle* cropped,const MyGUI::IntSize& size){++originalPointCalls;nativeSize(cropped,*reinterpret_cast<const Size*>(&size));}
void OriginalReal(MyGUI::Widget* widget,const MyGUI::FloatSize& size){nativeFloatSize(widget,*reinterpret_cast<const FloatSize*>(&size));}
void DerivedPoint(MyGUI::ICroppedRectangle* cropped,const MyGUI::IntSize& size){++derivedCalls;SetSizePoint_hook(cropped,size);}
const std::string& DerivedType(void*){static const std::string name="Button";return name;}
void NestedReal(MyGUI::Widget* widget,const MyGUI::FloatSize& size){
 assert(g_relativeSizeTarget==static_cast<MyGUI::ICroppedRectangle*>(widget));
 ++nestedCalls;
 if(widget!=nestedWidget){SetRealSize_hook(nestedWidget,size);assert(g_relativeSizeTarget==static_cast<MyGUI::ICroppedRectangle*>(widget));}
 OriginalReal(widget,size);
}
void ThrowReal(MyGUI::Widget* widget,const MyGUI::FloatSize&){assert(g_relativeSizeTarget==static_cast<MyGUI::ICroppedRectangle*>(widget));throw 17;}
void SehReal(MyGUI::Widget* widget,const MyGUI::FloatSize&){assert(g_relativeSizeTarget==static_cast<MyGUI::ICroppedRectangle*>(widget));RaiseException(0xE1234567,0,0,NULL);}
void NestedThrowReal(MyGUI::Widget* widget,const MyGUI::FloatSize& size){if(widget==nestedWidget)throw 19;SetRealSize_hook(nestedWidget,size);}
void CatchCpp(MyGUI::Widget* widget,const MyGUI::FloatSize& size,int value){bool caught=false;try{SetRealSize_hook(widget,size);}catch(int e){caught=true;assert(e==value);}assert(caught&&g_relativeSizeTarget==expectedPrevious);}
void CatchSeh(MyGUI::Widget* widget,const MyGUI::FloatSize& size){bool caught=false;__try{SetRealSize_hook(widget,size);}__except(EXCEPTION_EXECUTE_HANDLER){caught=true;}assert(caught&&g_relativeSizeTarget==expectedPrevious);}
void ResetCounts(){dirtyCalls=coordCallbacks=originalPointCalls=derivedCalls=0;}
int main(int argc,char** argv){
 HMODULE lib=LoadLibraryA("D:\\SteamLibrary\\steamapps\\common\\Kenshi\\MyGUIEngine_x64.dll");assert(lib);
 nativeSize=(SetSize)GetProcAddress(lib,"?setSize@Widget@MyGUI@@UEAAXAEBU?$TSize@H@types@2@@Z");
 nativeDirty=(DirtyFn)GetProcAddress(lib,"?outOfDate@LayerNode@MyGUI@@UEAAXPEAVRenderItem@2@@Z");
 nativeFloatSize=(SetFloatSize)GetProcAddress(lib,"?setRealSize@Widget@MyGUI@@QEAAXAEBU?$TSize@M@types@2@@Z");
 SetXY xy=(SetXY)GetProcAddress(lib,"?setSize@Widget@MyGUI@@QEAAXHH@Z");
 skinVtable[13]=GetProcAddress(lib,"?_setAlign@SubSkin@MyGUI@@UEAAXAEBU?$TSize@H@types@2@@Z");
 skinVtable[11]=GetProcAddress(lib,"?_updateView@SubSkin@MyGUI@@UEAAXXZ");
 void* objectVtable[8]={0};objectVtable[0]=GetProcAddress(lib,"?getTypeName@Widget@MyGUI@@UEBAAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ");
 void* derivedObjectVtable[8]={0};derivedObjectVtable[0]=(void*)DerivedType;
 void* derivedCropVtable[8]={0};derivedCropVtable[2]=(void*)DerivedPoint;
 assert(nativeSize&&nativeDirty&&nativeFloatSize&&xy&&objectVtable[0]);
 nodeVtable[14]=(void*)Dirty;delegateVtable[2]=(void*)Changed;cropVtable[2]=(void*)SetSizePoint_hook;
 g_origSetSizePoint=OriginalPoint;g_origSetRealSize=OriginalReal;
 WidgetFixture parent,child,other;Init(parent,100,12,false);Init(child,80,12,true);Init(other,80,12,true);
 Field<void*>(parent.bytes,0)=objectVtable;Field<void*>(child.bytes,0)=objectVtable;Field<void*>(other.bytes,0)=objectVtable;
 Field<void*>(child.bytes,0x40)=parent.bytes+8;Field<void*>(other.bytes,0x40)=parent.bytes+8;AttachSkin(child);
 MyGUI::Widget* widget=(MyGUI::Widget*)child.bytes;MyGUI::Widget* otherWidget=(MyGUI::Widget*)other.bytes;
 MyGUI::ICroppedRectangle* cropped=static_cast<MyGUI::ICroppedRectangle*>(widget);
 MyGUI::ICroppedRectangle* otherCropped=static_cast<MyGUI::ICroppedRectangle*>(otherWidget);
 assert((unsigned char*)cropped==child.bytes+8);MyGUI::FloatSize relative(0.8f,1.0f);
 for(int i=0;i<10000;++i){SetRealSize_hook(widget,relative);assert(g_relativeSizeTarget==NULL);}
 assert(!dirtyCalls&&!coordCallbacks&&!originalPointCalls);puts("PASS exact helpers:10000 plain Widget relative equal sizes skip without stale TLS");
 SetSizePoint_hook(cropped,MyGUI::IntSize(80,12));xy(widget,80,12);assert(dirtyCalls==2&&coordCallbacks==2&&originalPointCalls==2);
 puts("PASS explicit equal IntSize and XY outside relative helper retain native refresh/events");
 ResetCounts();g_relativeSizeTarget=cropped;SetSizePoint_hook(otherCropped,MyGUI::IntSize(80,12));assert(originalPointCalls==1&&coordCallbacks==1&&g_relativeSizeTarget==cropped);g_relativeSizeTarget=NULL;
 puts("PASS scope comparison: another Widget is never skipped by outer target");
 ResetCounts();Field<void*>(other.bytes,0)=derivedObjectVtable;Field<void*>(other.bytes,8)=derivedCropVtable;
 SetRealSize_hook(otherWidget,relative);assert(derivedCalls==1&&originalPointCalls==1&&coordCallbacks==1&&g_relativeSizeTarget==NULL);
 Field<void*>(other.bytes,0)=objectVtable;Field<void*>(other.bytes,8)=cropVtable;
 puts("PASS derived runtime type passes through and reaches its virtual setter/base refresh");
 ResetCounts();SetRealSize_hook(widget,MyGUI::FloatSize(0.809f,1));assert(!originalPointCalls);SetRealSize_hook(widget,MyGUI::FloatSize(0.81f,1));assert(originalPointCalls==1&&dirtyCalls==1&&coordCallbacks==1&&Field<int>(child.bytes,0x28)==81);
 puts("PASS actual native float conversion: subpixel equality skips; changed pixel reaches81");
 Field<int>(child.bytes,0x20)=10;void* children[1]={child.bytes};Field<void*>(parent.bytes,0x3d8)=children;Field<void*>(parent.bytes,0x3e0)=children+1;
 Size narrow={50,12};nativeSize(parent.bytes+8,narrow);assert(Field<int>(skin,0x84)==40);int before=dirtyCalls;
 SetRealSize_hook(widget,MyGUI::FloatSize(1.62f,1));assert(dirtyCalls==before&&Field<int>(skin,0x84)==40);
 Size wide={100,12};nativeSize(parent.bytes+8,wide);assert(Field<int>(skin,0x84)==81);before=dirtyCalls;SetRealSize_hook(widget,MyGUI::FloatSize(0.81f,1));assert(dirtyCalls==before&&Field<int>(skin,0x84)==81);
 puts("PASS actual parent crop propagation and equal relative guard preserve40px clip/81px restore");
 // Restore equal widths for nested helpers and check a non-null outer token.
 Field<void*>(parent.bytes,0x3d8)=NULL;Field<void*>(parent.bytes,0x3e0)=NULL;
 xy(widget,80,12);expectedPrevious=static_cast<MyGUI::ICroppedRectangle*>((MyGUI::Widget*)parent.bytes);g_relativeSizeTarget=expectedPrevious;
 nestedWidget=otherWidget;g_origSetRealSize=NestedReal;SetRealSize_hook(widget,relative);assert(nestedCalls==2&&g_relativeSizeTarget==expectedPrevious);
 g_origSetRealSize=ThrowReal;CatchCpp(widget,relative,17);g_origSetRealSize=SehReal;CatchSeh(widget,relative);g_origSetRealSize=NestedThrowReal;CatchCpp(widget,relative,19);
 puts("PASS nested helpers, C++ throw, SEH and nested throw restore prior TLS target");
 g_relativeSizeTarget=NULL;g_origSetRealSize=OriginalReal;ResetCounts();g_skipSameMoves=false;SetRealSize_hook(widget,relative);assert(originalPointCalls==1&&dirtyCalls==1&&coordCallbacks==1&&g_relativeSizeTarget==NULL);g_skipSameMoves=true;
 puts("PASS toggle off preserves native same-size refresh");
 ResetCounts();reentrant=true;SetRealSize_hook(widget,MyGUI::FloatSize(0.82f,1));
 assert(originalPointCalls==2&&coordCallbacks==2&&dirtyCalls==2&&g_relativeSizeTarget==NULL);reentrant=false;
 puts("PASS reentrant explicit same-size refresh from native changeCoord retains2 native calls/events");
 ResetCounts();nestedFromCallback=true;callbackNestedWidget=otherWidget;SetRealSize_hook(widget,MyGUI::FloatSize(0.83f,1));
 assert(callbackNestedCalls==1&&originalPointCalls==2&&coordCallbacks==2&&dirtyCalls==2&&g_relativeSizeTarget==NULL);nestedFromCallback=false;
 puts("PASS nested relative helper inside native callback restores consumed TLS; following explicit refresh retains2calls");
 ResetCounts();callbackNestedDone=false;nestedFromCallback=true;callbackNestedWidget=widget;callbackNestedFraction=0.86f;SetRealSize_hook(widget,MyGUI::FloatSize(0.86f,1));
 assert(callbackNestedCalls==2&&originalPointCalls==2&&coordCallbacks==2&&dirtyCalls==2&&g_relativeSizeTarget==NULL);nestedFromCallback=false;
 puts("PASS same Widget nested relative callback skips only its own equal conversion; following explicit refresh still runs");
 g_relativeSizeTarget=expectedPrevious;callbackThrow=1;CatchCpp(widget,MyGUI::FloatSize(0.84f,1),23);callbackThrow=2;CatchSeh(widget,MyGUI::FloatSize(0.85f,1));callbackThrow=0;g_relativeSizeTarget=NULL;
 puts("PASS C++ and SEH exceptions after point-token consumption restore outer TLS via relative helper finally");
 return 0;
}
'''
code=base+helpers+extra
generated=out/'actual_helpers_test.cpp';generated.write_text(code,encoding='utf-8')
(out/'actual_helpers_source.json').write_text(json.dumps({'source':str(source),'sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'SetSizePoint_hook':function('SetSizePoint_hook'),'SetRealSize_hook':function('SetRealSize_hook')},indent=2),encoding='utf-8')
vc=Path(r'C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC');sdk=Path(r'C:\Program Files\Microsoft SDKs\Windows\v7.1');kl=Path(r'D:\DEV\Kenshi\KenshiLib-src\Include')
env=os.environ.copy();env['INCLUDE']=';'.join(str(p) for p in [vc/'include',sdk/'Include',kl,kl/'ogre',Path(r'D:\DEV\Kenshi\deps\boost_1_60_0')]);env['LIB']=';'.join(str(p) for p in [vc/'lib/amd64',sdk/'Lib/x64',Path(r'D:\DEV\Kenshi\deps\KenshiLib\Libraries')]);env['PATH']=str(vc/'bin/amd64')+';'+env['PATH']
started=time.perf_counter()
result=subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2',str(generated),'/Fe'+str(out/'actual_helpers_test.exe'),'/Fo'+str(out/'actual_helpers_test.obj'),'/link','MyGUIEngine_x64.lib'],env=env,capture_output=True,text=True)
compiled=time.perf_counter()
if result.returncode:print(result.stdout,result.stderr);raise SystemExit(result.returncode)
result=subprocess.run([str(out/'actual_helpers_test.exe')]+sys.argv[1:],cwd=r'D:\SteamLibrary\steamapps\common\Kenshi',env=env,capture_output=True,text=True)
timing={'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'compile_seconds':compiled-started,'native_fixture_seconds':time.perf_counter()-compiled,'compile_exit_code':0,'native_exit_code':result.returncode,'exact_helper_count':2,'production_modified':False}
(out/'actual_helpers_verification.json').write_text(json.dumps(timing,indent=2),encoding='utf-8')
print(result.stdout,result.stderr,end='');print(json.dumps(timing));raise SystemExit(result.returncode)
