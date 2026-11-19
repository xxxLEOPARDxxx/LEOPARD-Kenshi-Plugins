#include <Windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
struct AI{unsigned char unused[0x2f8];void* actor;};
struct Building;
struct hand{unsigned words[8];Building* building;Building* getBuilding()const{return building;}};
struct TaskMatch{hand subject;};
struct GameData{};struct TownBase{};struct Faction{};struct FactoryCallbackInterface{};struct Layout{};struct GameSaveState{};struct RootObjectFactory{};
namespace Ogre{struct Vector3{float x,y,z;};struct Quaternion{float w,x,y,z;};}
struct Building{unsigned index;bool productive,wall,complete,paused,dismantled;};
#define AIFIXED_ENGINEER_PRIORITY_EXTERNAL_TYPES
#include <EngineerPriorityNative.h>
using namespace AIFixedEngineerPriority;
static unsigned checks=0,failures=0;
static void Check(bool b,const char*s){++checks;if(!b){++failures;std::printf("FAIL %s\n",s);}}
static unsigned char* image;static unsigned epoch=1;static bool active=true,allow=true,invalidSnapshot=false,failedResolve=false,badResolved=false;
static LONG finderCalls=0,selectorCalls=0,slotCalls=0,factoryCalls=0;static unsigned hookCalls=0,reports=0,resolveCalls=0;static bool probeABI=false,nestedFactory=false,throwBuilder=false;
static bool wrongCaller=false,reenter=false,invalidateDuring=false;static AI* nestedAI=0;static LONG lockedCallbacks=0;static bool throwCurrent=false,throwSnapshot=false;
static Building a={100,true,false,false,false,false},b={2,true,false,false,false,false},wall={3,false,true,false,false,false},legacy={4,true,false,false,false,false};
static Building* candidates[8];static unsigned candidateCount=0;static Building* nativeResult=&b;static void* finderObject=0;static int nativeThrow=0;
static Building* noSource=NULL;
static bool inTransit=false,wrongMaterialCaller=false,wrongCache=false,expireDuringWait=false;
static unsigned materialCalls=0,coverageChecks=0;
static void CheckUnlocked(){if(PriorityNative::metadataLock.RecursionCount&&reinterpret_cast<size_t>(PriorityNative::metadataLock.OwningThread)==GetCurrentThreadId())InterlockedIncrement(&lockedCallbacks);}
static bool Current(unsigned*g){CheckUnlocked();if(throwCurrent)RaiseException(0xE0421041,0,0,0);*g=epoch;return active;}
static bool Allowed(AI*){CheckUnlocked();return allow;}
static bool MaterialsEnRoute(AI*,Building* project)
{
    CheckUnlocked();++coverageChecks;
    const bool covered=inTransit&&project==noSource;
    if(expireDuringWait)inTransit=false;
    return covered;
}
static bool Snapshot(Building*p,Candidate*c){CheckUnlocked();if(throwSnapshot)RaiseException(0xE0421042,0,0,0);if(reenter){reenter=false;hand in={},out={};PriorityNative::FinderHook(nestedAI,in,out,false);}if(invalidSnapshot||!p)return false;std::memset(c,0,sizeof(*c));c->key.type=0;c->key.container=3;c->key.containerSerial=7;c->key.index=p->index;c->key.serial=9;c->productive=p->productive;c->wall=p->wall;c->complete=p->complete;c->paused=p->paused;c->dismantled=p->dismantled;return true;}
static Building* Resolve(const Key&k){CheckUnlocked();++resolveCalls;if(failedResolve)return 0;if(badResolved)return &legacy;Building* all[]={&a,&b,&wall,&legacy};for(unsigned i=0;i<4;++i)if(all[i]->index==k.index)return all[i];return 0;}
static void Report(const char*){++reports;}
static float slotResult=0;
static float* OriginalSlot(void*,Building*const*){CheckUnlocked();InterlockedIncrement(&slotCalls);return &slotResult;}
static float OriginalMaterials(void*,const void*,const TaskMatch&,hand& out)
{CheckUnlocked();++materialCalls;out.words[0]=57;return 0.0f;}
static Building* OriginalSelector(void*){
    CheckUnlocked();InterlockedIncrement(&selectorCalls);
    for(unsigned i=0;i<candidateCount;++i)
    {
        if(candidates[i]==noSource&&PriorityNative::selectorFrame)
        {
            TaskMatch request={};request.subject.building=noSource;hand out={};
            AI* actor=PriorityNative::selectorFrame->finder->ai;
            void* cache=reinterpret_cast<unsigned char*>(actor)+(wrongCache?0x210:0x208);
            const float score=PriorityNative::MaterialsAt(cache,NULL,request,out,image+(wrongMaterialCaller?0x9A5FF4:0x9A5FF3));
            Check(score==0.0f&&out.words[0]==57,"intentional wait leaves the source score and output unchanged");
            continue;
        }
        float*p=PriorityNative::ScoreSlotAt(&slotResult,&candidates[i],image+(wrongCaller?0x9A60F4:0x9A60F3));
        Check(p==&slotResult,"score-slot native pointer preserved");
        if(invalidateDuring){invalidateDuring=false;++epoch;PriorityNative::Invalidate();}
    }
    if(nativeThrow)throw 73;return nativeResult;
}
static float OriginalFinder(AI*,const hand&,hand&out,bool asking){CheckUnlocked();InterlockedIncrement(&finderCalls);Building*p=PriorityNative::SelectorHook(finderObject);out.words[0]=p?p->index:0;out.words[1]=asking?8:9;return p?0.625f:0.0f;}
static Building* factoryResult=&a;
static void Place(Building*,const void*,GameSaveState*,bool);
static void OriginalBuilder(void*);
static bool TestDisk(){return true;}
static Building* OriginalFactory(RootObjectFactory* self,GameData* data,Ogre::Vector3 position,TownBase* town,Faction* owner,Ogre::Quaternion rotation,FactoryCallbackInterface* callback,Layout* layout,Building* door,GameSaveState* saved,Building* indoors,bool invisible,bool completed,bool foliage,int floor,bool outside){CheckUnlocked();InterlockedIncrement(&factoryCalls);if(probeABI){Check(self==reinterpret_cast<RootObjectFactory*>(1)&&town==reinterpret_cast<TownBase*>(2)&&owner==reinterpret_cast<Faction*>(3)&&callback==reinterpret_cast<FactoryCallbackInterface*>(4)&&layout==reinterpret_cast<Layout*>(5)&&door==reinterpret_cast<Building*>(6)&&saved==reinterpret_cast<GameSaveState*>(7)&&indoors==reinterpret_cast<Building*>(8)&&invisible&&completed&&foliage&&floor==23&&outside,"all factory ABI arguments forwarded exactly");}if(nestedFactory){nestedFactory=false;Place(&legacy,image+0x101,0,false);}if(position.x!=1||position.y!=2||position.z!=3||rotation.w!=1||rotation.x||rotation.y||rotation.z)RaiseException(0xE0421043,0,0,0);return data?reinterpret_cast<Building*>(data):factoryResult;}
static bool AddHook(void*where,void*,void**original){++hookCalls;size_t r=(unsigned char*)where-image;if(r==0x998190)*original=(void*)&OriginalFinder;else if(r==0x9A5D40)*original=(void*)&OriginalSelector;else if(r==0x8D9600)*original=(void*)&OriginalSlot;else if(r==0x57C1E0)*original=(void*)&OriginalFactory;else if(r==0x4D6810)*original=(void*)&OriginalBuilder;else if(r==0x60E740)*original=(void*)&OriginalMaterials;else return false;return true;}
static void Place(Building*p,const void*caller=0,GameSaveState*saved=0,bool completed=false){factoryResult=p;Ogre::Vector3 v={1,2,3};Ogre::Quaternion q={1,0,0,0};PriorityNative::FactoryAt(0,reinterpret_cast<GameData*>(p),v,0,0,q,0,0,0,saved,0,false,completed,false,0,false,caller?caller:image+0x4D6DE5);}
static void OriginalBuilder(void*){Place(&a,image+0x101,0,false);Place(&b,image+0x102,0,false);if(throwBuilder)throw 74;}
static unsigned Find(AI&ai){hand in={},out={};float f=PriorityNative::FinderHook(&ai,in,out,true);Check(f==(out.words[0]?0.625f:0.0f)&&out.words[1]==8,"finder native score and unrelated out fields preserved");return out.words[0];}
static DWORD WINAPI WorkerFind(void*p){unsigned* result=static_cast<unsigned*>(p);*result=Find(*nestedAI);return 0;}
static DWORD WINAPI WorkerPlace(void*p){for(unsigned i=0;i<200;++i)Place(static_cast<Building*>(p));return 0;}
int main(int argc,char**argv){
 Check(argc==2,"mapped image supplied");if(argc!=2)return 1;FILE*f=0;fopen_s(&f,argv[1],"rb");Check(f!=0,"mapped fixture opens");if(!f)return 1;fseek(f,0,SEEK_END);size_t size=ftell(f);rewind(f);image=(unsigned char*)std::malloc(size);Check(image&&fread(image,1,size,f)==size,"mapped fixture read");fclose(f);const unsigned thunks[5]={0x4CF5F,0x20D56,0x1E1DC,0x8918,0x5305D};for(unsigned i=0;i<5;++i)reinterpret_cast<size_t*>(image+0x1740910)[i]=reinterpret_cast<size_t>(image+thunks[i]);
 PriorityNative::Config cfg={};cfg.current=Current;cfg.allowActor=Allowed;cfg.snapshot=Snapshot;cfg.resolve=Resolve;cfg.addHook=AddHook;cfg.report=Report;cfg.diskIdentity=TestDisk;cfg.materialsEnRoute=MaterialsEnRoute;
 Check(PriorityNative::Configure(cfg,image,size),"callbacks configured");Check(PriorityNative::Install(),"exact Steam image installs native observers");if(!PriorityNative::Ready()){std::printf("%u checks, %u failures\n",checks,failures);return 1;}

 Check(hookCalls==6&&reports==0,"exact six hooks, no startup error");
 Check(PriorityNative::Install()&&hookCalls==6,"installation is one-shot");
 Check(PriorityNative::AdmitImage(image,size),"unmodified full native proof admitted");
 Check(!PriorityNative::AdmitImage(0,0)&&!PriorityNative::AdmitImage(image,64),"null or truncated image refused");
 for(unsigned pi=0;pi<sizeof(PriorityNative::pins)/sizeof(PriorityNative::pins[0]);++pi){const PriorityNative::Pin&pin=PriorityNative::pins[pi];unsigned offsets[3]={0,pin.size/2,pin.size-1};for(unsigned k=0;k<3;++k){image[pin.rva+offsets[k]]^=1;Check(!PriorityNative::AdmitImage(image,size),"mutated first/middle/last native body refused");image[pin.rva+offsets[k]]^=1;}}
 size_t* vt=reinterpret_cast<size_t*>(image+0x1740910);vt[1]++;Check(!PriorityNative::AdmitImage(image,size),"wrong construction vtable refused");vt[1]--;
 image[0x20D56]^=1;Check(!PriorityNative::AdmitImage(image,size),"wrong native vtable thunk refused");image[0x20D56]^=1;
 IMAGE_NT_HEADERS64* nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(image+reinterpret_cast<IMAGE_DOS_HEADER*>(image)->e_lfanew);nt->FileHeader.TimeDateStamp^=1;Check(!PriorityNative::AdmitImage(image,size),"other executable version refused");nt->FileHeader.TimeDateStamp^=1;
 nt->FileHeader.Machine^=1;Check(!PriorityNative::AdmitImage(image,size),"other architecture refused");nt->FileHeader.Machine^=1;
 Check(PriorityNative::AdmitImage(image,size),"restored exact native proof admitted");

 Check(PriorityNative::FileSteamIdentity(L"D:/SteamLibrary/steamapps/common/Kenshi/RE_Kenshi/Kenshi_x64.exe"),"production CryptoAPI SHA256 verifies actual on-disk Steam EXE");
 Check(!PriorityNative::FileSteamIdentity(0),"missing disk identity path is refused");
 Check(!PriorityNative::DiskSteamIdentity(),"ordinary fixture executable cannot satisfy actual Steam on-disk SHA identity");
 unsigned char factoryHead[5],builderHead[5],townPatch[13];std::memcpy(factoryHead,image+0x57C1E0,5);std::memcpy(builderHead,image+0x4D6810,5);std::memcpy(townPatch,image+0x57C9B1,13);
 image[0x57C1E0]=0xE9;std::memset(image+0x57C1E1,0x12,4);Check(PriorityNative::AdmitImage(image,size),"normal shared public factory entry with exact tail admitted");
 image[0x57C1E5]^=1;Check(!PriorityNative::AdmitImage(image,size),"shared factory entry still requires exact native tail");image[0x57C1E5]^=1;
 image[0x4D6810]=0xE9;std::memset(image+0x4D6811,0x34,4);Check(PriorityNative::AdmitImage(image,size),"normal shared builder entry with entire remaining body exact admitted");
 image[0x4D6815]^=1;Check(!PriorityNative::AdmitImage(image,size),"shared builder remaining body stays strict");image[0x4D6815]^=1;
 std::memset(image+0x57C9B1,0x90,13);Check(PriorityNative::AdmitImage(image,size),"KEO internal TownClaim patch does not reject unchanged forwarded public factory ABI");
 std::memcpy(image+0x57C1E0,factoryHead,5);std::memcpy(image+0x4D6810,builderHead,5);std::memcpy(image+0x57C9B1,townPatch,13);
 AI ai={};int actor=3;ai.actor=&actor;nestedAI=&ai;unsigned char finder[48]={};*reinterpret_cast<void**>(finder)=image+0x1740910;*reinterpret_cast<void**>(finder+0x28)=ai.actor;finderObject=finder;
 candidateCount=2;candidates[0]=&b;candidates[1]=&a;nativeResult=&b;
 Place(&a);Place(&b);LONG calls=factoryCalls;
 Check(Find(ai)==100,"production bridge chooses earlier confirmed placement from final positive candidates");
 Check(finderCalls==1&&selectorCalls==1&&slotCalls==2&&factoryCalls==calls,"one original call at each native boundary");
 noSource=&a;inTransit=true;
 Check(Find(ai)==2&&coverageChecks==0,"speculative checks cannot claim a machine");
 PriorityNative::Started(&a,epoch-1);
 Check(Find(ai)==2&&coverageChecks==0,"old-world election cannot record the current construction priority");
 PriorityNative::Started(&a,epoch);
 const unsigned materialsBefore=materialCalls;
 Check(Find(ai)==0&&materialCalls==materialsBefore+1,"waiting for a working machine does not divert to the next machine");
 nativeResult=&wall;Check(Find(ai)==3,"the same wait does not suppress native wall work");nativeResult=&b;
 candidates[2]=&wall;candidateCount=3;
 Check(Find(ai)==3,"positive wall remains available even when native first ranks the later machine");
 wall.paused=true;Check(Find(ai)==0,"a paused alternate is not substituted for the native winner");wall.paused=false;
 candidateCount=2;
 wrongMaterialCaller=true;Check(Find(ai)==2,"only the native construction scoring callsite may defer a project");wrongMaterialCaller=false;
 wrongCache=true;Check(Find(ai)==2,"another actor's cache cannot defer the selected machine");wrongCache=false;
 expireDuringWait=true;Check(Find(ai)==2,"cancelled delivery falls back within the same native selection");expireDuringWait=false;
 inTransit=false;Check(Find(ai)==2,"a genuinely unsupplied machine yields to useful work");
 noSource=NULL;nativeResult=&legacy;candidates[0]=&legacy;
 Check(Find(ai)==100,"established focus also orders legacy blueprints once elected");
 a.complete=true;Check(Find(ai)==4,"finished machine cannot retain focus");a.complete=false;
 nativeResult=&b;candidates[0]=&b;
 PriorityNative::Invalidate();Place(&a);Place(&b);
 nativeResult=&wall;Check(Find(ai)==3,"native wall selection preserved");nativeResult=&b;
 b.complete=true;Check(Find(ai)==2,"native repair selection preserved");b.complete=false;
 candidates[0]=&legacy;Check(Find(ai)==2,"unknown legacy positive candidate forces native fallback");candidates[0]=&b;
 allow=false;Check(Find(ai)==2,"coordinator actor/order/priority refusal preserves native");allow=true;
 active=false;Check(Find(ai)==2,"inactive world preserves native");active=true;
 wrongCaller=true;Check(Find(ai)==2,"helper callsite mismatch cannot manufacture positive admission");wrongCaller=false;
 invalidSnapshot=true;Check(Find(ai)==2,"invalid snapshot discards policy");invalidSnapshot=false;
 failedResolve=true;Check(Find(ai)==2,"selected handle failed resolution preserves native");failedResolve=false;
 badResolved=true;Check(Find(ai)==2,"recycled or mismatched resolved identity preserves native");badResolved=false;
 unsigned char* tableBefore=finder;*reinterpret_cast<void**>(tableBefore)=image+0x1740918;Check(Find(ai)==2,"different native finder type is refused");*reinterpret_cast<void**>(tableBefore)=image+0x1740910;
 *reinterpret_cast<void**>(finder+0x28)=0;Check(Find(ai)==2,"mismatched actor cannot reuse TLS scope");*reinterpret_cast<void**>(finder+0x28)=ai.actor;
 throwCurrent=true;Check(Find(ai)==2,"coordinator access fault falls back to native original");throwCurrent=false;
 throwSnapshot=true;Check(Find(ai)==2,"snapshot access fault falls back to native original");throwSnapshot=false;
 Check(PriorityNative::finderFrame==0&&PriorityNative::selectorFrame==0&&PriorityNative::inCallback==0,"callback faults leave no TLS state");
 reenter=true;Check(Find(ai)==2,"nested callback finder poisons parent override");
 Check(PriorityNative::finderFrame==0&&PriorityNative::selectorFrame==0,"nested scope restored");
 nativeThrow=1;bool caught=false;try{Find(ai);}catch(int e){caught=e==73;}nativeThrow=0;Check(caught,"original native C++ exception is not swallowed");
 Check(PriorityNative::finderFrame==0&&PriorityNative::selectorFrame==0,"native exception restores TLS scopes");
 Check(Find(ai)==100,"fresh scope works after native exception");
 unsigned workerResult=0;HANDLE worker=CreateThread(0,0,WorkerFind,&workerResult,0,0);Check(worker!=0&&WaitForSingleObject(worker,5000)==WAIT_OBJECT_0,"native worker fixture finishes");if(worker)CloseHandle(worker);Check(workerResult==100,"native worker thread is admitted with its own TLS");
 HANDLE workers[2]={CreateThread(0,0,WorkerPlace,&a,0,0),CreateThread(0,0,WorkerPlace,&b,0,0)};Check(workers[0]&&workers[1]&&WaitForMultipleObjects(2,workers,TRUE,5000)==WAIT_OBJECT_0,"concurrent placement metadata operations finish");for(unsigned i=0;i<2;++i)if(workers[i])CloseHandle(workers[i]);Check(Find(ai)==100,"concurrent duplicate notifications preserve proven ordinal");
 Check(lockedCallbacks==0,"all coordinator callbacks and native originals execute outside metadata lock");
 invalidateDuring=true;Check(Find(ai)==2,"mid-selector lifecycle change rejects override");
 Check(Find(ai)==2,"load transient reset makes former blueprints unknown");
 Place(&a,image+0x4D6DE6);Place(&b);Check(Find(ai)==2,"unproven placement caller cannot seed chronology");
 PriorityNative::Invalidate();Place(&a,0,reinterpret_cast<GameSaveState*>(1));Place(&b);Check(Find(ai)==2,"load reconstruction cannot seed chronology");
 PriorityNative::Invalidate();Place(&a,0,0,true);Place(&b);Check(Find(ai)==2,"completed factory object cannot seed chronology");
 PriorityNative::Invalidate();Place(&a);Place(&b);Check(Find(ai)==100,"new confirmed placements can be ordered after reset");
 Candidate dead={};Snapshot(&a,&dead);PriorityNative::Forget(dead.key);Check(Find(ai)==2,"cancellation hint retires ordinal");
 Building* key=&b;LONG before=slotCalls;Check(PriorityNative::ScoreSlotAt(&slotResult,&key,image+0x9A60F3)==&slotResult&&slotCalls==before+1&&key==&b,"unscoped helper preserves native pointer and original once");

 PriorityNative::Invalidate();Place(&a);Place(&b);a.paused=true;
 for(unsigned pass=0;pass<32;++pass){unsigned beforeResolve=resolveCalls;PriorityNative::Maintenance(999);Check(resolveCalls-beforeResolve<=8,"maintenance bounds native handle queries even with excessive requested budget");}
 a.paused=false;Check(Find(ai)==100,"paused construction retains placement ordinal");
 a.complete=true;for(unsigned pass=0;pass<32;++pass)PriorityNative::Maintenance(8);a.complete=false;
 Check(Find(ai)==2,"completed metadata is retired without retaining stale priority");
 PriorityNative::Invalidate();Place(&a);Place(&b);a.dismantled=true;for(unsigned pass=0;pass<32;++pass)PriorityNative::Maintenance();a.dismantled=false;
 Check(Find(ai)==2,"dismantled metadata is retired");
 PriorityNative::Invalidate();Place(&a);Place(&b);failedResolve=true;for(unsigned pass=0;pass<32;++pass)PriorityNative::Maintenance();failedResolve=false;
 Check(Find(ai)==2,"unavailable handle becomes unknown native fallback rather than invented restored chronology");
 unsigned beforeResolve=resolveCalls;PriorityNative::Maintenance(0);Check(resolveCalls==beforeResolve,"zero maintenance budget performs no handle query");
 probeABI=true;Ogre::Vector3 av={1,2,3};Ogre::Quaternion aq={1,0,0,0};Building* ar=PriorityNative::FactoryAt(reinterpret_cast<RootObjectFactory*>(1),reinterpret_cast<GameData*>(&a),av,reinterpret_cast<TownBase*>(2),reinterpret_cast<Faction*>(3),aq,reinterpret_cast<FactoryCallbackInterface*>(4),reinterpret_cast<Layout*>(5),reinterpret_cast<Building*>(6),reinterpret_cast<GameSaveState*>(7),reinterpret_cast<Building*>(8),true,true,true,23,true,image+0x4D6DE5);probeABI=false;
 Check(ar==&a,"factory ABI result forwarded unchanged");

 PriorityNative::Invalidate();PriorityNative::BuilderHook(0);Check(Find(ai)==100,"shared KEO factory wrapper caller preserves ordinal through proven native builder scope");
 Check(PriorityNative::placementDepth==0&&PriorityNative::factoryDepth==0,"placement factory TLS depths restored");
 throwBuilder=true;bool caughtBuilder=false;try{PriorityNative::BuilderHook(0);}catch(int e){caughtBuilder=e==74;}throwBuilder=false;
 Check(caughtBuilder&&PriorityNative::placementDepth==0&&PriorityNative::factoryDepth==0,"builder native exception propagates with clean depths");
 PriorityNative::Invalidate();nestedFactory=true;PriorityNative::BuilderHook(0);candidates[0]=&legacy;candidates[1]=&a;candidates[2]=&b;candidateCount=3;
 Check(Find(ai)==2,"recursive factory allocation callback cannot manufacture native blueprint age");candidateCount=2;candidates[0]=&b;candidates[1]=&a;
 Check(lockedCallbacks==0,"final no callback-under-lock check");

 std::printf("%u checks, %u failures\n",checks,failures);return failures?1:0;
}
