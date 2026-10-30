"""Build/run VC10 host tests, including exact production adapter/SEH bodies. No game DLL load."""
from pathlib import Path
import os,subprocess,hashlib,json
ROOT=Path(__file__).resolve().parents[1]
OUT=Path(r'D:\DEV\Kenshi\build\ItemMarkersStolenCheck\boundary')
VC=Path(r'C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC')
SDK=Path(r'C:\Program Files\Microsoft SDKs\Windows\v7.1')
def block(source,anchor):
 start=source.index(anchor);begin=source.index('{',start);depth=1;end=begin+1
 while depth:
  if source[end]=='{':depth+=1
  elif source[end]=='}':depth-=1
  end+=1
 if source[end:end+1]==';':end+=1
 return source[start:end]
def run():
 OUT.mkdir(parents=True,exist_ok=True)
 env=os.environ.copy();env['INCLUDE']=str(VC/'include')+';'+str(SDK/'Include');env['LIB']=str(VC/'lib'/'amd64')+';'+str(SDK/'Lib'/'x64')
 source=(ROOT/'ItemMarkers.cpp').read_text(encoding='utf-8-sig')
 anchors=['bool HeldByPlayer(','struct StolenNative','bool CaptureDragRaw(','struct PickupCall','bool PickupScopedRaw(','bool Pickup_hook(','Item* TakeAmount_hook(']
 extracted='\n\n'.join(block(source,anchor) for anchor in anchors)
 header=r'''
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <cstdio>
#include "StolenPresentation.h"
struct Character;struct Item;
struct hand {Character* character;Item* item;Character* getCharacter() const{return character;}Item* getItem() const{return item;} };
struct Item { bool stolen,detached,fault; int quantity,owner; hand _whosInventoryWeAreIn;int kind;bool isStolen(bool) { if(fault)RaiseException(0xe0421234,0,0,NULL);return stolen||detached; }int getDataType() const{return kind;} };
typedef Item RootObject;enum {CONTAINER=46};
struct Character { bool player; bool isPlayerCharacter(){return player;} };
struct InventoryGUI { Character* character;Item* selected;Item* clone;bool split;Item* callback;Character* getCallbackCharacter(){return character;}Item* getSelectedItem(const std::string&){return selected;}RootObject* getCallbackObject(){return callback;} };
bool g_enabled=true,g_stolen=true,g_dragCaptureReady=true;
ItemMarkersStolen::DragState g_drag={};
bool (*g_origPickup)(InventoryGUI*,std::string)=NULL;
Item* (*g_origTakeAmount)(InventoryGUI*,Item*,int)=NULL;
int errors=0,calls=0,takes=0,checks=0,failures=0;
void ErrorLog(const char*){++errors;}
#define CHECK(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %d: %s\n",__LINE__,#x);}}while(0)
Item* TakeAmount_hook(InventoryGUI*,Item*,int);
ItemMarkersStolen::IconState made={};
bool raiseOriginal=false;
Item* OriginalTake(InventoryGUI* gui,Item* source,int amount){++takes;source->quantity-=amount;gui->clone->quantity=amount;return gui->clone;}
bool OriginalPickup(InventoryGUI* gui,std::string){++calls;if(raiseOriginal)RaiseException(0xe0421234,0,0,NULL);Item* item=gui->split?TakeAmount_hook(gui,gui->selected,1):gui->selected;item->detached=true;ItemMarkersStolen::OnIcon(made,g_drag,item);return true;}
'''
 tail=r'''
bool CatchSeh(InventoryGUI* gui,const std::string* section,const ItemMarkersStolen::DragState* next){
 __try{PickupScopedRaw(gui,section,next);}
 __except(GetExceptionCode()==0xe0421234?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return true;}
 return false;
}
int main(){
 g_origPickup=OriginalPickup;g_origTakeAmount=OriginalTake;
 Character player={true};Item clean={false,false,false,6,1};Item clone={false,false,false,0,1};InventoryGUI gui={&player,&clean,&clone,false};
 CHECK(Pickup_hook(&gui,"main"));StolenNative native;CHECK(made.frozen);CHECK(!ItemMarkersStolen::Show(made,native,&clean));CHECK(calls==1);CHECK(clean.owner==1 && clean.quantity==6);CHECK(!g_drag.active);
 clean.detached=false;gui.split=true;CHECK(Pickup_hook(&gui,"bag"));CHECK(takes==1 && clean.quantity==5 && clone.quantity==1);CHECK(!ItemMarkersStolen::Show(made,native,&clone));CHECK(!g_drag.active);
 Item realTheft={true,false,false,1,7};gui.split=false;gui.selected=&realTheft;CHECK(Pickup_hook(&gui,"main"));CHECK(ItemMarkersStolen::Show(made,native,&realTheft));CHECK(realTheft.owner==7);
 player.player=false;gui.selected=&clean;clean.detached=false;CHECK(Pickup_hook(&gui,"foreign"));CHECK(!made.frozen);CHECK(ItemMarkersStolen::Show(made,native,&clean));
 player.player=true;clean.detached=false;clean.fault=true;std::string section="main";ItemMarkersStolen::DragState captured={};CHECK(!CaptureDragRaw(&gui,&section,&captured));CHECK(!captured.active);clean.fault=false;
 ItemMarkersStolen::DragState previous={true,true,&gui,&clean,&clean};g_drag=previous;ItemMarkersStolen::DragState next={true,false,&gui,&clone,&clone};raiseOriginal=true;
 CHECK(CatchSeh(&gui,&section,&next));CHECK(g_drag.active==previous.active && g_drag.held==previous.held && g_drag.stolen==previous.stolen);raiseOriginal=false;g_drag=ItemMarkersStolen::DragState();
 g_dragCaptureReady=false;clean.detached=false;CHECK(Pickup_hook(&gui,"main"));CHECK(!made.frozen);CHECK(ItemMarkersStolen::Show(made,native,&clean));
 g_dragCaptureReady=true;Item bag={false,false,false,1,1};bag.kind=CONTAINER;bag._whosInventoryWeAreIn.character=&player;gui.character=NULL;gui.callback=&bag;clean.detached=false;
 CHECK(Pickup_hook(&gui,"unequipped_bag"));CHECK(made.frozen);CHECK(!ItemMarkersStolen::Show(made,native,&clean));
 Item outer={false,false,false,1,1};outer._whosInventoryWeAreIn.character=&player;bag._whosInventoryWeAreIn.character=NULL;bag._whosInventoryWeAreIn.item=&outer;clean.detached=false;
 CHECK(Pickup_hook(&gui,"nested_bag"));CHECK(made.frozen);CHECK(!ItemMarkersStolen::Show(made,native,&clean));
 std::printf("%d exact-production adapter/SEH checks, %d failures\n",checks,failures);return failures?1:0;
}
'''
 # The fixture has native types; all policy/adapter/exception bodies below come from production source.
 generated=OUT/'production_adapter_tests.cpp';generated.write_text(header+'\n'+extracted+'\n'+tail,encoding='ascii')
 reports=[]
 for name,path in [('policy',ROOT/'tests'/'stolen_boundary_tests.cpp'),('production_adapter',generated)]:
  obj=OUT/(name+'.obj');exe=OUT/(name+'.exe')
  args=[str(VC/'bin'/'amd64'/'cl.exe'),'/nologo','/EHsc','/MD','/O2','/W3','/I'+str(ROOT),'/Fo'+str(obj),'/Fe'+str(exe),str(path)]
  compile=subprocess.run(args,env=env,capture_output=True,text=True);(OUT/(name+'-build.txt')).write_text(compile.stdout+compile.stderr,encoding='utf-8')
  if compile.returncode:raise RuntimeError(compile.stdout+compile.stderr)
  result=subprocess.run([str(exe)],capture_output=True,text=True);(OUT/(name+'-tests.txt')).write_text(result.stdout+result.stderr,encoding='utf-8');print(result.stdout.strip())
  if result.returncode:raise RuntimeError('host test failed')
  reports.append({'name':name,'passed':True,'output':result.stdout.strip(),'exe_sha256':hashlib.sha256(exe.read_bytes()).hexdigest()})
 report={'production_source_sha256':hashlib.sha256((ROOT/'ItemMarkers.cpp').read_bytes()).hexdigest(),'extracted_bodies':anchors,'tests':reports,'game_dll_loaded':False,'process_access':False}
 (OUT/'tests-validation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
if __name__=='__main__':run()
