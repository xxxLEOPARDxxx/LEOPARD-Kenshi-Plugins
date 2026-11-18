#include <Windows.h>
#include <cstdio>
#include <EngineerSurplusPolicy.h>
static int fail=0,checks=0;static void Check(bool x,const char*s){++checks;if(!x){++fail;printf("FAIL %s\n",s);}}
struct Tasker{};struct Character{};struct AITaskSytem{Character*character;};struct AI{AITaskSytem*taskSystemAI;};struct TaskMatch{};struct Episode{};
static LONG currentEpoch=3;static bool ready=true,permit=true,credit=true,resetScore=false;static unsigned depth=0;static AITaskSytem*normalJobScope=NULL;static int slot=2,calls=0,prepares=0;static float nativeScore=0;
struct DepthScope{DepthScope(){++depth;}~DepthScope(){--depth;}};static LONG Epoch(){return currentEpoch;}static bool Live(LONG e){return ready&&e==currentEpoch;}static bool Eligible(Character*,LONG){return permit;}static int BuilderSlot(AITaskSytem*,Tasker*){return slot;}static void RestorePending(Character*){}static bool Prepare(AITaskSytem*,Tasker*,int,Episode&){++prepares;return credit;}
static float Native(Tasker*,AI*){++calls;if(resetScore)++currentEpoch;return nativeScore;}static float KeoForward(Tasker*t,AI*a){return Native(t,a);}static float(*originalScore)(Tasker*,AI*)=Native;
// @SCORE@
static Tasker task;static AI ai;static float score=0;static bool keoOuter=false;
static float KeoOuter(Tasker*t,AI*a){return ScoreHook(t,a);}static void NativeChoose(AITaskSytem*,void*,TaskMatch&,bool,bool){score=keoOuter?KeoOuter(&task,&ai):ScoreHook(&task,&ai);}static void(*originalChoosePerma)(AITaskSytem*,void*,TaskMatch&,bool,bool)=NativeChoose;
// @CHOOSE@
static void Reset(){ready=permit=credit=true;resetScore=false;depth=0;normalJobScope=NULL;currentEpoch=3;slot=2;calls=prepares=0;nativeScore=0;originalScore=Native;keoOuter=false;}
int main(){Character c;AITaskSytem tasks;tasks.character=&c;ai.taskSystemAI=&tasks;TaskMatch previous;
Reset();ChoosePermaHook(&tasks,NULL,previous,false,true);Check(score==1&&calls==1&&prepares==1&&!normalJobScope,"normal exact builder gains admission and TLS restored");
Reset();ChoosePermaHook(&tasks,NULL,previous,true,true);Check(score==0&&calls==1&&prepares==0,"urgent native pass never admits cleanup");
Reset();ChoosePermaHook(&tasks,NULL,previous,false,false);Check(score==0&&calls==1&&prepares==0,"disabled jobs never admitted");
Reset();originalScore=KeoForward;ChoosePermaHook(&tasks,NULL,previous,false,true);Check(score==1&&calls==1,"KEO inner score hook preserves exact native job scope");
Reset();keoOuter=true;ChoosePermaHook(&tasks,NULL,previous,false,true);Check(score==1&&calls==1,"KEO outer score hook preserves exact native job scope");
Reset();Check(ScoreHook(&task,&ai)==0&&calls==1&&prepares==0,"foreign direct score caller untouched");
Reset();nativeScore=9;ChoosePermaHook(&tasks,NULL,previous,false,true);Check(score==9&&calls==1&&prepares==0,"positive construction score unchanged without storage query");
Reset();credit=false;ChoosePermaHook(&tasks,NULL,previous,false,true);Check(score==0&&calls==1,"no owned credit native");
Reset();slot=-1;ChoosePermaHook(&tasks,NULL,previous,false,true);Check(score==0&&prepares==0,"nonpermanent builder task native");
Reset();permit=false;ChoosePermaHook(&tasks,NULL,previous,false,true);Check(score==0&&prepares==0,"combat/rest/input preemption native");
Reset();resetScore=true;ChoosePermaHook(&tasks,NULL,previous,false,true);Check(score==0&&prepares==0,"epoch changed inside original score stops old actor access");
printf("%d checks, %d failures\n",checks,fail);return fail?1:0;}
