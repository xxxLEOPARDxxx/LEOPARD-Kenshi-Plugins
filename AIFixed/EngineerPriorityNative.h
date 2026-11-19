#pragma once
#include <Windows.h>
#include <intrin.h>
#include <wincrypt.h>
#pragma comment(lib,"advapi32.lib")
#include <cstddef>
#include <cstring>
#include "EngineerPriorityPolicy.h"
#ifndef AIFIXED_ENGINEER_PRIORITY_EXTERNAL_TYPES
#include <kenshi/AI/AI.h>
#include <kenshi/AI/AITaskSystem.h>
#include <kenshi/Building/Building.h>
#include <kenshi/RootObjectFactory.h>
#endif
#pragma intrinsic(_ReturnAddress)

// Include once, at global scope. No assembly/midfunction patch: the score-slot
// helper is an ordinary MS x64 function called only AFTER native score > 0.
// Config callbacks/originals always run OUTSIDE the metadata critical section.
// Host MUST call Invalidate before lifecycle epoch changes. Call Configure and
// Install once during plugin startup; no retry/teardown while hooks can run.
// `current` admits the game's own callback thread (including native AI workers).
// `snapshot` must use full identity and final native class/shared build state,
// and reject non-player/disabled/invalid objects. `allowActor` owns scheduler,
// explicit-order and player-priority gating. Query callbacks must be read-only,
// bounded and nonthrowing. The optional completion callback only releases
// plugin claims for a known full key. No ordinal survives Invalidate or load.
namespace PriorityNative
{
    using AIFixedEngineerPriority::Key;
    using AIFixedEngineerPriority::Candidate;
    using AIFixedEngineerPriority::Round;
    using AIFixedEngineerPriority::Registry;
    struct Config
    {
        bool (*current)(unsigned* generation);
        bool (*allowActor)(AI*);
        bool (*snapshot)(Building*,Candidate*);
        Building* (*resolve)(const Key&);
        bool (*addHook)(void*,void*,void**);
        void (*report)(const char*);
        bool (*diskIdentity)(); // Optional offline host override; production defaults to SHA256 of main EXE.
        bool (*materialsEnRoute)(AI*,Building*);
        void (*constructionCompleted)(const Key&,unsigned);
    };
    struct Pin {unsigned rva,size;unsigned __int64 hash;};
    static const Pin pins[] = {
        // Finder has multiple unwind fragments; pin the WHOLE semantic body.
        {0x998190,0x162,0x3997ba451874cb15ULL},
        {0x9A5A30,0x1B7,0xd0fb980765968f36ULL},
        {0x9A5D40,0x54F,0x90208c86f0a25364ULL},
        {0x8D9600,0x98,0x2cc3e82f012a04abULL},
        {0x9FA9B0,0x13E,0xce05f4fced53610bULL},
        {0x57C1E0,0x20,0x3ff5030ff1d41cf3ULL},
        {0x4D6810,0xADE,0x9d76c1b61e7a8909ULL},
        {0x60E740,0x1A5,0x8012647b3df1b88dULL}
    };
    static unsigned __int64 HashBytes(const unsigned char* p,size_t n)
    {unsigned __int64 h=14695981039346656037ULL;for(size_t i=0;i<n;++i){h^=p[i];h*=1099511628211ULL;}return h;}
    static bool AdmitImage(const unsigned char* base,size_t size)
    {
        __try
        {
            if(!base||size<sizeof(IMAGE_DOS_HEADER))return false;
            const IMAGE_DOS_HEADER* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
            if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<0||
               static_cast<size_t>(dos->e_lfanew)>size-sizeof(IMAGE_NT_HEADERS64))return false;
            const IMAGE_NT_HEADERS64* nt=reinterpret_cast<const IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
            if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64||
               nt->FileHeader.TimeDateStamp!=0x65D604D7||nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC||
               nt->OptionalHeader.SizeOfImage!=0x232C000||size<nt->OptionalHeader.SizeOfImage)return false;
            for(unsigned i=0;i<sizeof(pins)/sizeof(pins[0]);++i)
            {
                if(pins[i].rva>size||pins[i].size>size-pins[i].rva)return false;
                if(HashBytes(base+pins[i].rva,pins[i].size)==pins[i].hash)continue;
                // Both public placement entries are normal AddHook chains in
                // KEO TownClaim. Private helpers remain entirely exact. Never
                // pin unrelated createBuilding internals: KEO's town branch
                // patch at57C9B1 is forwarded through original, not called here.
                const unsigned __int64 tail=i==5?0x829310b5f95a5d45ULL:i==6?0xd13e615b1445e3fcULL:0;
                if(!tail||base[pins[i].rva]!=0xE9||HashBytes(base+pins[i].rva+5,pins[i].size-5)!=tail)return false;
            }
            const unsigned thunks[5]={0x4CF5F,0x20D56,0x1E1DC,0x8918,0x5305D};
            const unsigned destinations[5]={0x9A5A30,0x9A5D40,0x9FA150,0x9FB670,0x8CBD80};
            const size_t* table=reinterpret_cast<const size_t*>(base+0x1740910);
            for(unsigned j=0;j<5;++j)
            {
                if(table[j]!=reinterpret_cast<size_t>(base+thunks[j])||base[thunks[j]]!=0xE9)return false;
                int delta=0;std::memcpy(&delta,base+thunks[j]+1,4);
                if(static_cast<size_t>(thunks[j]+5+delta)!=destinations[j])return false;
            }
            const unsigned char finderRelay[]={0xe9,0x55,0x94,0x5c,0x00};
            const unsigned char materialsRelay[]={0xe9,0xe2,0x17,0x59,0x00};
            if(std::memcmp(base+0x452E6,finderRelay,sizeof(finderRelay))||
               std::memcmp(base+0x107E9,materialsRelay,sizeof(materialsRelay)))return false;
            return true;
        }
        __except(EXCEPTION_EXECUTE_HANDLER){return false;}
    }
    typedef float (*FinderFn)(AI*,const hand&,hand&,bool);
    typedef Building* (*SelectorFn)(void*);
    typedef void (*BuilderFn)(void*);
    typedef float* (*ScoreSlotFn)(void*,Building* const*);
    // Win64 passes the 16-byte AI member pointer by address in RDX.
    typedef float (*MaterialsFn)(void*,const void*,const TaskMatch&,hand&);
    typedef Building* (*FactoryFn)(RootObjectFactory*,GameData*,Ogre::Vector3,TownBase*,Faction*,Ogre::Quaternion,
        FactoryCallbackInterface*,Layout*,Building*,GameSaveState*,Building*,bool,bool,bool,int,bool);
    static Config config={};static const unsigned char* image=0;static size_t imageSize=0;
    static CRITICAL_SECTION metadataLock;static Registry registry;
    static bool configured=false,attempted=false;static volatile LONG ready=0,invalidation=0;
    static unsigned maintenanceCursor=0;static unsigned registryGeneration=0;static LONG registryInvalidation=-1;
    static FinderFn originalFinder=0;static SelectorFn originalSelector=0;
    static ScoreSlotFn originalScoreSlot=0;static FactoryFn originalFactory=0;static BuilderFn originalBuilder=0;
    static MaterialsFn originalMaterials=0;
    struct FinderFrame {AI* ai;unsigned generation;LONG invalidation;bool bad;};
    struct SelectorFrame {FinderFrame* finder;void* selector;void* scoreMap;Round round;unsigned failedSourceChecks;};
    static __declspec(thread) FinderFrame* finderFrame=0;
    static __declspec(thread) SelectorFrame* selectorFrame=0;
    static __declspec(thread) unsigned inCallback=0,placementDepth=0,factoryDepth=0;
    static bool Ready(){return InterlockedCompareExchange(&ready,0,0)!=0;}
    static LONG Invalid(){return InterlockedCompareExchange(&invalidation,0,0);}
    static void Invalidate(){InterlockedIncrement(&invalidation);}
    static bool Current(unsigned* generation)
    {
        if(!Ready()||inCallback)return false;
        ++inCallback;bool ok=false;
        __try {__try {ok=config.current(generation);} __except(EXCEPTION_EXECUTE_HANDLER){ok=false;}}
        __finally {--inCallback;}
        return ok;
    }
    static bool Snapshot(Building* b,Candidate* c)
    {
        ++inCallback;bool ok=false;
        __try {__try {ok=b&&config.snapshot(b,c);} __except(EXCEPTION_EXECUTE_HANDLER){ok=false;}}
        __finally {--inCallback;}
        return ok;
    }
    static bool Allowed(AI* ai)
    {++inCallback;bool ok=false;__try{__try{ok=config.allowActor(ai);}__except(EXCEPTION_EXECUTE_HANDLER){ok=false;}}__finally{--inCallback;}return ok;}
    static Building* Resolve(const Key& key)
    {++inCallback;Building* b=0;__try{__try{b=config.resolve(key);}__except(EXCEPTION_EXECUTE_HANDLER){b=0;}}__finally{--inCallback;}return b;}
    static bool SameEpoch(unsigned generation,LONG local)
    {unsigned now=0;return Current(&now)&&now==generation&&Invalid()==local;}
    static void Synchronise(unsigned generation,LONG local)
    {
        // Caller holds metadataLock. No callbacks or native functions here.
        if(registryGeneration!=generation||registryInvalidation!=local)
        {registry.Reset(generation);maintenanceCursor=0;registryGeneration=generation;registryInvalidation=local;}
    }
    static void Started(Building* project,unsigned expectedGeneration)
    {
        unsigned generation=0;const LONG local=Invalid();Candidate candidate={};
        if(!Current(&generation)||generation!=expectedGeneration||!Snapshot(project,&candidate)||!AIFixedEngineerPriority::Construction(candidate)||
           !SameEpoch(generation,local))return;
        EnterCriticalSection(&metadataLock);
        if(Invalid()==local){Synchronise(generation,local);registry.Start(candidate.key);}
        LeaveCriticalSection(&metadataLock);
    }
    static bool EnRoute(AI* ai,Building* project)
    {
        ++inCallback;bool covered=false;
        __try {__try {covered=config.materialsEnRoute(ai,project);} __except(EXCEPTION_EXECUTE_HANDLER){covered=false;}}
        __finally {--inCallback;}
        return covered;
    }
    static bool FinderIdentity(void* finder,AI* ai)
    {
        __try
        {
            return finder&&ai&&*reinterpret_cast<const void* const*>(finder)==image+0x1740910&&
                *reinterpret_cast<void* const*>(static_cast<const unsigned char*>(finder)+0x28)==
                *reinterpret_cast<void* const*>(reinterpret_cast<const unsigned char*>(ai)+0x2F8);
        }
        __except(EXCEPTION_EXECUTE_HANDLER){return false;}
    }
    static bool BeginSelector(SelectorFrame* frame,FinderFrame* parent,void* self)
    {
        if(!parent||parent->bad||!FinderIdentity(self,parent->ai)||!SameEpoch(parent->generation,parent->invalidation))return false;
        frame->finder=parent;frame->selector=self;frame->scoreMap=0;
        EnterCriticalSection(&metadataLock);
        if(Invalid()!=parent->invalidation){LeaveCriticalSection(&metadataLock);return false;}
        Synchronise(parent->generation,parent->invalidation);registry.Begin(frame->round,parent->generation);
        LeaveCriticalSection(&metadataLock);return true;
    }
    // Explicit caller-taking core lets offline fixtures execute the real bridge
    // without synthesising a game return address or executing native EXE bytes.
    static float* ScoreSlotAt(void* self,Building* const* key,const void* caller)
    {
        float* result=originalScoreSlot(self,key); // Native ownership/exception contract unchanged.
        SelectorFrame* frame=selectorFrame;
        if(!frame||caller!=image+0x9A60F3||inCallback||frame->round.bad||!Ready())return result;
        if(frame->finder->bad||!key||!SameEpoch(frame->finder->generation,frame->finder->invalidation))
        {frame->round.bad=true;return result;}
        if(frame->scoreMap&&frame->scoreMap!=self){frame->round.bad=true;return result;}
        frame->scoreMap=self;Candidate candidate={};
        if(!Snapshot(*key,&candidate)){frame->round.bad=true;return result;}
        bool completed=false;
        EnterCriticalSection(&metadataLock);
        if(registryGeneration!=frame->finder->generation||registryInvalidation!=frame->finder->invalidation||
           Invalid()!=frame->finder->invalidation)frame->round.bad=true;
        else
        {
            if(candidate.productive&&candidate.complete&&config.constructionCompleted)
                completed=registry.Forget(candidate.key);
            registry.Observe(frame->round,candidate);
        }
        LeaveCriticalSection(&metadataLock);
        if(completed&&SameEpoch(frame->finder->generation,frame->finder->invalidation))
            config.constructionCompleted(candidate.key,frame->finder->generation);
        return result;
    }
    static __declspec(noinline) float* ScoreSlotHook(void* self,Building* const* key)
    {const void* caller=_ReturnAddress();return ScoreSlotAt(self,key,caller);}
    static float MaterialsAt(void* cache,const void* token,const TaskMatch& request,hand& out,const void* caller)
    {
        const float score=originalMaterials(cache,token,request,out);
        SelectorFrame* frame=selectorFrame;
        if(!frame||caller!=image+0x9A5FF3||!(score<=0.0f)||inCallback||frame->round.bad||!frame->round.trackWaiting)return score;
        FinderFrame* parent=frame->finder;
        if(parent->bad||cache!=reinterpret_cast<unsigned char*>(parent->ai)+0x208||
           !SameEpoch(parent->generation,parent->invalidation))return score;
        if(++frame->failedSourceChecks>Registry::MaxObserved){frame->round.bad=true;return score;}
        // This call site follows native range, fortification and build checks.
        // An earlier machine with only in-flight materials can have a native
        // score of zero. The original cache contents and result stay intact.
        Building* project=NULL;
        __try {project=request.subject.getBuilding();} __except(EXCEPTION_EXECUTE_HANDLER){return score;}
        Candidate candidate={};
        if(!Snapshot(project,&candidate)||!AIFixedEngineerPriority::Construction(candidate))return score;
        EnterCriticalSection(&metadataLock);
        const bool started=registryInvalidation==parent->invalidation&&registryGeneration==parent->generation&&
            Invalid()==parent->invalidation&&registry.Started(candidate.key);
        LeaveCriticalSection(&metadataLock);
        if(!started||!EnRoute(parent->ai,project)||!SameEpoch(parent->generation,parent->invalidation))return score;
        EnterCriticalSection(&metadataLock);
        if(registryInvalidation==parent->invalidation&&registryGeneration==parent->generation&&Invalid()==parent->invalidation)
            registry.Observe(frame->round,candidate,true);
        LeaveCriticalSection(&metadataLock);
        return score;
    }
    static __declspec(noinline) float MaterialsHook(void* cache,const void* token,const TaskMatch& request,hand& out)
    {const void* caller=_ReturnAddress();return MaterialsAt(cache,token,request,out,caller);}
    static Building* SelectorHook(void* self)
    {
        SelectorFrame* previous=selectorFrame;FinderFrame* parent=finderFrame;
        if(previous){previous->round.bad=true;selectorFrame=0;Building* native=0;
            __try{native=originalSelector(self);}__finally{selectorFrame=previous;}return native;}
        SelectorFrame frame={};
        if(!BeginSelector(&frame,parent,self))return originalSelector(self);
        selectorFrame=&frame;Building* native=0;
        __try{native=originalSelector(self);}__finally{selectorFrame=previous;}
        if(frame.round.bad||parent->bad||!SameEpoch(parent->generation,parent->invalidation))return native;
        Candidate nativeInfo={};if(!Snapshot(native,&nativeInfo))return native;
        Key replacement={};bool replace=false;
        EnterCriticalSection(&metadataLock);
        if(registryInvalidation==parent->invalidation&&Invalid()==parent->invalidation)
            replace=registry.Select(frame.round,parent->generation,nativeInfo,replacement);
        LeaveCriticalSection(&metadataLock);
        if(!replace)return native;
        Building* chosen=Resolve(replacement);Candidate live={};
        if(!Snapshot(chosen,&live)||!AIFixedEngineerPriority::Equal(live.key,replacement)||
           !AIFixedEngineerPriority::Construction(live)||!SameEpoch(parent->generation,parent->invalidation))return native;
        if(frame.round.waiting)
        {
            if(!EnRoute(parent->ai,chosen)||!SameEpoch(parent->generation,parent->invalidation))return native;
            if(!frame.round.hasAlternative)return NULL;
            // Preserve another positive wall/repair candidate from this
            // native round when the next machine has to wait.
            Building* other=Resolve(frame.round.alternative);Candidate alternative={};
            if(!SameEpoch(parent->generation,parent->invalidation))return native;
            const bool valid=Snapshot(other,&alternative)&&AIFixedEngineerPriority::Equal(alternative.key,frame.round.alternative)&&
                AIFixedEngineerPriority::OtherConstruction(alternative);
            if(!SameEpoch(parent->generation,parent->invalidation))return native;
            return valid?other:NULL;
        }
        return chosen;
    }
    static float FinderHook(AI* ai,const hand& in,hand& out,bool justAsking)
    {
        FinderFrame* previous=finderFrame;
        if(previous){previous->bad=true;if(selectorFrame)selectorFrame->round.bad=true;finderFrame=0;float native=0;
            __try{native=originalFinder(ai,in,out,justAsking);}__finally{finderFrame=previous;}return native;}
        unsigned generation=0;const LONG local=Invalid();
        if(!Current(&generation)||!Allowed(ai)||!SameEpoch(generation,local))return originalFinder(ai,in,out,justAsking);
        FinderFrame frame={ai,generation,local,false};finderFrame=&frame;float native=0;
        __try{native=originalFinder(ai,in,out,justAsking);}__finally{finderFrame=previous;}
        return native;
    }
    static Building* FactoryAt(RootObjectFactory* self,GameData* data,Ogre::Vector3 position,TownBase* town,Faction* owner,
        Ogre::Quaternion rotation,FactoryCallbackInterface* callback,Layout* furnitureOf,Building* isDoorOf,GameSaveState* saveState,
        Building* indoors,bool invisible,bool completed,bool foliage,int floorNumber,bool outsideFurniture,const void* caller)
    {
        const bool placement=!factoryDepth&&(caller==image+0x4D6DE5||placementDepth==1)&&
            !saveState&&!completed&&!foliage&&!invisible;
        unsigned generation=0;const LONG local=placement?Invalid():0;
        const bool observe=placement&&Current(&generation);
        Building* native=0;++factoryDepth;
        __try {native=originalFactory(self,data,position,town,owner,rotation,callback,furnitureOf,isDoorOf,saveState,
            indoors,invisible,completed,foliage,floorNumber,outsideFurniture);}
        __finally {--factoryDepth;}
        if(!observe||!SameEpoch(generation,local))return native;
        Candidate candidate={};if(!Snapshot(native,&candidate)||!AIFixedEngineerPriority::Construction(candidate))return native;
        if(!SameEpoch(generation,local))return native;
        EnterCriticalSection(&metadataLock);
        if(Invalid()==local){Synchronise(generation,local);registry.Record(candidate.key);}
        LeaveCriticalSection(&metadataLock);return native;
    }
    static __declspec(noinline) Building* FactoryHook(RootObjectFactory* self,GameData* data,Ogre::Vector3 position,TownBase* town,Faction* owner,
        Ogre::Quaternion rotation,FactoryCallbackInterface* callback,Layout* furnitureOf,Building* isDoorOf,GameSaveState* saveState,
        Building* indoors,bool invisible,bool completed,bool foliage,int floorNumber,bool outsideFurniture)
    {
        const void* caller=_ReturnAddress();return FactoryAt(self,data,position,town,owner,rotation,callback,furnitureOf,isDoorOf,
            saveState,indoors,invisible,completed,foliage,floorNumber,outsideFurniture,caller);
    }
    static void BuilderHook(void* self)
    {
        // A preceding AddHook factory wrapper changes _ReturnAddress. Scope
        // the exact native placement batch instead; only outer factory calls
        // count, so recursive allocation callbacks cannot seed extra ordinals.
        ++placementDepth;
        __try {originalBuilder(self);} __finally {--placementDepth;}
    }
    static void Forget(const Key& key)
    {
        // Optional completion/cancel observer. Stale keys remain bounded even
        // without this hint, and full identities prevent address/index reuse.
        unsigned generation=0;const LONG local=Invalid();if(!Current(&generation))return;
        EnterCriticalSection(&metadataLock);
        if(Invalid()==local){Synchronise(generation,local);registry.Forget(key);}
        LeaveCriticalSection(&metadataLock);
    }
    // Host schedules this after the native world update, e.g. 8 keys/250ms.
    // At most 64 POD slots scanned under lock and 8 handles resolved per call.
    // Unavailable/deleted handles lose only our transient ordinal; if unloaded
    // objects return later their unknown age causes native fallback. Pausing
    // never retires an ordinal. This is metadata maintenance, not a world scan.
    static void Maintenance(unsigned requested=8)
    {
        unsigned generation=0;const LONG local=Invalid();if(!Current(&generation))return;
        if(requested>8)requested=8;if(!requested)return;
        Key keys[8];unsigned count=0;
        EnterCriticalSection(&metadataLock);
        if(Invalid()==local){Synchronise(generation,local);count=registry.Collect(keys,requested,maintenanceCursor);}
        LeaveCriticalSection(&metadataLock);
        for(unsigned i=0;i<count;++i)
        {
            if(!SameEpoch(generation,local))return;
            Building* b=Resolve(keys[i]);Candidate state={};
            const bool known=b&&Snapshot(b,&state)&&AIFixedEngineerPriority::Equal(state.key,keys[i]);
            const bool retire=!b||(known&&(state.complete||state.dismantled));
            if(!retire||!SameEpoch(generation,local))continue;
            bool removed=false;
            EnterCriticalSection(&metadataLock);
            if(registryGeneration==generation&&registryInvalidation==local&&Invalid()==local)removed=registry.Forget(keys[i]);
            LeaveCriticalSection(&metadataLock);
            if(removed&&known&&state.complete&&config.constructionCompleted&&SameEpoch(generation,local))
                config.constructionCompleted(keys[i],generation);
        }
    }
    static bool Configure(const Config& c,const unsigned char* module=0,size_t bytes=0)
    {
        if(configured||attempted||!c.current||!c.allowActor||!c.snapshot||!c.resolve||!c.addHook||!c.report||!c.materialsEnRoute)return false;
        if(!module){module=reinterpret_cast<const unsigned char*>(GetModuleHandleW(0));bytes=0x232C000;}
        config=c;image=module;imageSize=bytes;InitializeCriticalSection(&metadataLock);configured=true;return true;
    }
    static bool FileSteamIdentity(const wchar_t* path)
    {
        if(!path||!*path)return false;
        HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,0,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,0);
        if(file==INVALID_HANDLE_VALUE)return false;
        HCRYPTPROV provider=0;HCRYPTHASH hash=0;bool ok=false;
        __try
        {
            LARGE_INTEGER bytes;unsigned char block[16384],digest[32];DWORD count=0;
            static const unsigned char expected[32]={0x50,0x4b,0x36,0x2c,0xde,0x85,0x0d,0x56,0xaf,0xb1,0xce,0xa6,0xf5,0xb7,0xb0,0xee,0x01,0x4d,0x9d,0xd7,0xb4,0x7e,0x18,0x85,0x99,0xd9,0x1c,0x80,0x45,0x02,0xcd,0x3e};
            if(!GetFileSizeEx(file,&bytes)||bytes.QuadPart!=36713984||
               !CryptAcquireContextW(&provider,0,0,PROV_RSA_AES,CRYPT_VERIFYCONTEXT)||
               !CryptCreateHash(provider,CALG_SHA_256,0,0,&hash))__leave;
            for(;;){if(!ReadFile(file,block,sizeof(block),&count,0))__leave;if(!count)break;
                if(!CryptHashData(hash,block,count,0))__leave;}
            count=sizeof(digest);
            ok=CryptGetHashParam(hash,HP_HASHVAL,digest,&count,0)&&count==sizeof(digest)&&!std::memcmp(digest,expected,sizeof(digest));
        }
        __finally {if(hash)CryptDestroyHash(hash);if(provider)CryptReleaseContext(provider,0);CloseHandle(file);}
        return ok;
    }
    static bool DiskSteamIdentity()
    {
        wchar_t path[1024];const DWORD length=GetModuleFileNameW(0,path,1024);
        return length&&length<1024&&FileSteamIdentity(path);
    }
    static bool Install()
    {
        if(!configured||attempted)return Ready();attempted=true;
        if(!(config.diskIdentity?config.diskIdentity():DiskSteamIdentity()))
        {config.report("Engineer priority: main EXE on disk is not the verified Steam 1.0.65 binary; keeping native targets.");return false;}
        if(!AdmitImage(image,imageSize))
        {config.report("Engineer priority: unsupported or modified Steam native contract; keeping native targets.");return false;}
        bool ok=config.addHook(const_cast<unsigned char*>(image)+0x998190,reinterpret_cast<void*>(&FinderHook),reinterpret_cast<void**>(&originalFinder));
        if(ok)ok=config.addHook(const_cast<unsigned char*>(image)+0x9A5D40,reinterpret_cast<void*>(&SelectorHook),reinterpret_cast<void**>(&originalSelector));
        if(ok)ok=config.addHook(const_cast<unsigned char*>(image)+0x8D9600,reinterpret_cast<void*>(&ScoreSlotHook),reinterpret_cast<void**>(&originalScoreSlot));
        if(ok)ok=config.addHook(const_cast<unsigned char*>(image)+0x57C1E0,reinterpret_cast<void*>(&FactoryHook),reinterpret_cast<void**>(&originalFactory));
        if(ok)ok=config.addHook(const_cast<unsigned char*>(image)+0x4D6810,reinterpret_cast<void*>(&BuilderHook),reinterpret_cast<void**>(&originalBuilder));
        if(ok)ok=config.addHook(const_cast<unsigned char*>(image)+0x60E740,reinterpret_cast<void*>(&MaterialsHook),reinterpret_cast<void**>(&originalMaterials));
        ok=ok&&originalFinder&&originalSelector&&originalScoreSlot&&originalFactory&&originalBuilder&&originalMaterials;
        if(ok)InterlockedExchange(&ready,1);
        else config.report("Engineer priority: native observer installation failed; keeping native targets.");
        return ok;
    }
}
