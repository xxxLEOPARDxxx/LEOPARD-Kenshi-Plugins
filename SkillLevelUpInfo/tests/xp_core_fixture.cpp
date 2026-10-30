#include "XpInfoCore.h"
#include <cstdio>
int count=0,failed=0;
void check(bool v,const char* s){++count;if(!v){++failed;printf("FAIL %s\n",s);}}
int main(){
 using namespace XpInfoCore;
 Skill s;s.Observe(3.2f,1000);check(s.Take(1000,1,1000)==0,"first baseline silent");
 s.Observe(3.205f,1100);s.Observe(3.21f,1200);
 check(s.Take(2099,.9f,1000)==0,"pending waits for interval from first gain");
 check(s.Take(2100,.9f,1000)==1,"small gains accumulate without integer truncation");
 check(s.Take(2101,.9f,1000)==0,"emitted gain consumed once");
 check(s.Recent(21200,20000),"recent skill visible at exact lifetime boundary");
 check(!s.Recent(21201,20000),"recent skill expires");
 s.Observe(4.15f,22000);check(s.Take(22000,100,60000)==2,"level crossing emits immediately regardless of threshold");
 check(s.emittedLevels==1,"level count preserves crossing");
 check(s.Take(22001,0,0)==0,"level event has no duplicate fractional event");
 s.Observe(3.0f,23000);check(s.Take(23000,0,0)==0,"decrease not XP");
 s.Observe(3.001f,24000);check(s.Take(144001,100,1)==0&&s.pending==0,"stale small pending expires after120s");
 s.Observe(3.1f,25000);float prev=s.value;
 union{unsigned u;float f;}n;n.u=0x7fc00001;s.Observe(n.f,26000);
 check(s.value==prev,"invalid native values rejected");
 Skill wrap;wrap.Observe(1,0xffffff00UL);wrap.Observe(1.03f,0xffffff10UL);
 check(wrap.Take(0x310,1,1000)==1,"tick overflow preserves elapsed interval");
 Skill skills[3];for(int i=0;i<3;++i){skills[i].Observe(1,100);skills[i].Observe(1.1f,1000+i*100);}
 Slots slots;slots.Select(skills,3,1300,120000,2);
 check(slots.count==2&&slots.ids[0]==2&&slots.ids[1]==1,"panel starts with newest active skills");
 skills[0].Observe(1.2f,2000);slots.Select(skills,3,2000,120000,2);
 check(slots.ids[0]==2&&slots.ids[1]==1,"panel slots stay stable during five-second hold");
 slots.Select(skills,3,7000,120000,2);
 check(slots.ids[0]==2&&slots.ids[1]==0,"new activity replaces oldest eligible slot");
 slots.Select(skills,3,122001,120000,5);
 check(slots.count==0,"panel removes all expired skills");
 slots.Select(skills,3,2000,120000,0);check(slots.count==0,"no panel capacity renders no rows");
 printf("XP core: %d checks, %d failed\n",count,failed);return failed?1:0;
}
