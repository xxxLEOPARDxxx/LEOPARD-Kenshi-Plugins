#include <Windows.h>
#include <cstdio>
#include <EngineerMaterialsPolicy.h>
static unsigned checks=0,failures=0;
static void Check(bool b,const char*s){++checks;if(!b){++failures;printf("FAIL %s\n",s);}}
typedef AIFixedEngineer::Identity Identity;typedef AIFixedEngineer::Owned Owned;
static Identity Id(unsigned n){Identity a;a.type=1;a.index=n;a.serial=4;a.container=2;a.containerSerial=9;return a;}
static LONG generation=3;static DWORD clockNow=100;static unsigned resolves=0,adds=0;static bool feature=false,hasBuilderJob=false,combat=true,resetInAdd=false;
static LONG Epoch(){return generation;}static LONG(*epoch)()=Epoch;static bool ready=true;static DWORD Clock(){return clockNow;}
#define GetTickCount Clock
static SRWLOCK metadata=SRWLOCK_INIT;enum{Capacity=128};
struct Tasker{};struct hand{Identity id;hand(const Identity& n=Identity()):id(n){}struct Character*getCharacter();struct Item*getItem();};static Identity Of(const hand&h){return h.id;}
struct Item{hand handle;int quantity;bool isInInventory;Item():quantity(1),isInInventory(false){}};static Item pieces[128];
struct Inventory{hand handle;bool fail,contains;Inventory():fail(false),contains(false){}const hand& getHandle(){return handle;}bool hasItem(Item*){return contains;}bool addItem(Item*p,int n,bool drop,bool destroy){++adds;Check(n==1&&!drop&&!destroy,"maintenance cannot drop or destroy owned piece");if(resetInAdd)++generation;if(fail)return false;p->isInInventory=true;contains=true;return true;}};static Inventory body,pack;
struct Character{hand handle;bool player;Character():player(true){}bool isPlayerCharacter(){return player;}Inventory*getInventory(){return &body;}};static Character actor;
Character* hand::getCharacter(){++resolves;return &actor;}Item*hand::getItem(){++resolves;return id.index<128?&pieces[id.index]:NULL;}
static Inventory* Backpack(Character*){return &pack;}
namespace EngineerMaterialsRuntime{static hand Handle(const Identity&i){return hand(i);}}
// @PRODUCTION@
static void Reset(){for(unsigned i=0;i<Capacity;++i){episodes[i]=Episode();pieces[i]=Item();pieces[i].handle=hand(Id(i));}generation=3;clockNow=100;resolves=adds=0;feature=hasBuilderJob=false;combat=true;resetInAdd=false;body=Inventory();pack=Inventory();body.handle=hand(Id(150));pack.handle=hand(Id(151));actor=Character();actor.handle=hand(Id(200));maintenanceGeneration=-1;maintenanceWindow=0;maintenanceCursor=0;maintenanceStarted=false;}
static void Make(unsigned i){Episode&e=episodes[i];e.used=e.pending=true;e.retired=e.deleted=true;e.generation=3;e.token=i+1;e.actor=Id(200);e.sourceInventory=Id(150);e.piece=Id(i);}
int main(){Episode out;
Reset();Make(0);Maintenance(false);Check(adds==0&&resolves==0&&episodes[0].pending,"unsafe world does not resolve any receipt");Maintenance(true);Check(adds==1&&!ReadToken(1,out),"OFF, removed builder job and combat still restore already-owned transaction");
Reset();Make(0);body.fail=true;Maintenance(true);Check(adds==1&&ReadToken(1,out)&&out.pending&&!out.transaction,"full destination and failed original rollback retain receipt on retry failure");body.fail=false;maintenanceCursor=0;clockNow+=250;Maintenance(true);Check(adds==1&&ReadToken(1,out),"per-piece retry backoff suppresses native add");maintenanceCursor=0;clockNow+=1250;Maintenance(true);Check(adds==2&&!ReadToken(1,out),"later same-world inventory room restores without AI or feature eligibility");
Reset();for(unsigned i=0;i<Capacity;++i)Make(i);body.fail=true;Maintenance(true);Check(adds==4&&maintenanceCursor==4,"at most eight metadata rows and four native retries per batch");for(unsigned i=0;i<1000;++i)Maintenance(true);Check(adds==4&&maintenanceCursor==4,"paused same timestamp cannot multiply native retries");clockNow+=250;Maintenance(true);Check(adds==8&&maintenanceCursor==8&&episodes[4].retryAt!=0,"next batch advances round robin without starving rows behind persistent failures");
Reset();Make(0);++body.handle.id.serial;Maintenance(true);Check(adds==0&&episodes[0].pending,"replacement source inventory serial never receives detached item");
Reset();Make(0);++actor.handle.id.serial;Maintenance(true);Check(adds==0&&episodes[0].pending,"replacement actor serial never receives detached item");
Reset();Make(0);++pieces[0].handle.id.serial;Maintenance(true);Check(adds==0&&episodes[0].pending,"replacement piece serial never consumed");
Reset();Make(0);generation=4;Maintenance(true);Check(adds==0&&resolves==0&&!ReadToken(1,out),"old world receipt retired without resolving old native object");
Reset();Make(0);pieces[0].isInInventory=true;Maintenance(true);Check(adds==0&&episodes[0].pending,"piece already in foreign inventory is never taken back");
Reset();Make(0);pieces[0].isInInventory=true;body.contains=true;Maintenance(true);Check(adds==0&&!ReadToken(1,out),"same source already owns piece: receipt completion without duplicate add");
Reset();Make(0);body.fail=resetInAdd=true;Maintenance(true);Check(adds==1&&!ReadToken(1,out),"world change inside native rollback clears receipt without later old pointer access");
Reset();Make(0);clockNow=0xf0000000u;Maintenance(true);Check(adds==1,"first retry remains eligible after long machine uptime");
printf("%u checks, %u failures\n",checks,failures);return failures?1:0;}
