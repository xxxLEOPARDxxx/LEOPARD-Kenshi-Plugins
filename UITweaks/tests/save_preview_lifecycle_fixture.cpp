#include "../../SavePreviewCodec.h"
#include <cstdio>
struct GameWorld{};struct Renderer{};
struct RenderOwner{Renderer*render;void*titleScreen;};
struct SaveFileSystem{enum State{NORMAL,SAVING,COMPLETE};State state;std::string currentSave,failedToCopyError;};
bool enabled=true,mainAllowed=true;DWORD mainThread=GetCurrentThreadId(),started=0;unsigned serial=0;
GameWorld game,otherGame,*ou=&game,*world=NULL;Renderer render,*renderer=NULL;RenderOwner owner={&render,NULL},*au=&owner;
SavePreviewPolicy::Pending pending;std::vector<unsigned char> thumbnail;
unsigned submits=0,cancels=0,nativeSaves=0,nativeSyncs=0,nativeLoads=0,nativeNews=0,hides=0,errors=0;
bool saveAccepted=true;
bool OnMain(){return mainAllowed;}
void Cancel(){++cancels;pending.Reset();thumbnail.clear();world=NULL;renderer=NULL;}
void Submit(){if(pending.Ready()){++submits;pending.Reset();thumbnail.clear();}}
void Hide(){++hides;}
void ErrorLog(const std::string&){++errors;}
bool NativeSave(SaveFileSystem*s,const std::string&p){++nativeSaves;s->currentSave=p;s->state=SaveFileSystem::SAVING;return saveAccepted;}
void NativeSync(SaveFileSystem*s){++nativeSyncs;if(s->state==SaveFileSystem::COMPLETE){s->state=SaveFileSystem::NORMAL;s->failedToCopyError.clear();}}
void NativeLoad(SaveFileSystem*,const std::string&){++nativeLoads;}
void NativeNew(SaveFileSystem*){++nativeNews;}
bool (*originalSave)(SaveFileSystem*,const std::string&)=NativeSave;
void (*originalSync)(SaveFileSystem*)=NativeSync;
void (*originalLoad)(SaveFileSystem*,const std::string&)=NativeLoad;
void (*originalNew)(SaveFileSystem*)=NativeNew;
// EXTRACT_LIFECYCLE
static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::printf("FAIL line %u: %s\n",__LINE__,#x);return 1;}}while(0)
int main()
{
    SaveFileSystem s;s.state=SaveFileSystem::NORMAL;
    CHECK(Save(&s,"C:/root-one/same-name"));CHECK(nativeSaves==1&&pending.active&&world==ou&&renderer==owner.render);
    CHECK(pending.path==FullPath("C:/root-one/same-name"));
    s.state=SaveFileSystem::COMPLETE;Sync(&s);CHECK(pending.active&&pending.complete&&submits==0&&nativeSyncs==1);
    pending.Captured();Submit();CHECK(submits==1&&!pending.active);
    CHECK(Save(&s,"D:/root-two/same-name"));CHECK(pending.path==FullPath("D:/root-two/same-name"));
    pending.Captured();s.state=SaveFileSystem::COMPLETE;s.failedToCopyError="disk full";Sync(&s);
    CHECK(submits==1&&!pending.active&&s.failedToCopyError.empty()); // failure inspected BEFORE native clears it
    CHECK(Save(&s,"C:/root-one/overwrite"));pending.Captured();s.currentSave="C:/root-one/different";s.state=SaveFileSystem::COMPLETE;Sync(&s);
    CHECK(submits==1&&!pending.active);
    CHECK(Save(&s,"C:/root-one/previous"));pending.Captured();saveAccepted=false;
    CHECK(!Save(&s,"C:/root-one/refused"));CHECK(!pending.active&&submits==1);saveAccepted=true;
    enabled=false;CHECK(Save(&s,"C:/root-one/disabled"));CHECK(!pending.active);enabled=true;
    mainAllowed=false;CHECK(Save(&s,"C:/root-one/wrong-thread"));CHECK(!pending.active);mainAllowed=true;
    owner.titleScreen=(void*)1;CHECK(Save(&s,"C:/root-one/menu"));CHECK(!pending.active);owner.titleScreen=NULL;
    CHECK(Save(&s,"C:/root-one/load-cancel"));pending.Captured();Load(&s,"other");CHECK(!pending.active&&nativeLoads==1&&hides==1);
    CHECK(Save(&s,"C:/root-one/new-cancel"));New(&s);CHECK(!pending.active&&nativeNews==1&&hides==2);
    CHECK(Save(&s,"C:/root-one/quicksave"));pending.Captured();s.state=SaveFileSystem::COMPLETE;Sync(&s);CHECK(submits==2);
    CHECK(Save(&s,"C:/root-one/autosave0"));pending.Captured();s.state=SaveFileSystem::SAVING;Sync(&s);CHECK(submits==2&&pending.active&&!pending.complete);
    s.state=SaveFileSystem::COMPLETE;Sync(&s);CHECK(submits==3);
    CHECK(FullPath("").empty());CHECK(FullPath(std::string(400,'a')).empty());
    std::printf("PASS actual save preview lifecycle: %u checks\n",checks);return 0;
}
