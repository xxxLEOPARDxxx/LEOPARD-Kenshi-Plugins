#include "../../GsmFinePump.h"
#include <cstdio>
static unsigned checks=0;
static void C(bool yes,const char*why){++checks;if(!yes){std::printf("FAIL %s\n",why);throw 1;}}
int main(){try{
 GsmFinePump::Budget b(100000,1000000);
 C(b.CanStep(100000),"first ready transition admitted");
 for(unsigned i=0;i<8;++i){C(b.CanStep(100000+i),"at most eight ready state transitions");bool more=b.Finished(100000+i,true);C(more==(i<7),"step eight ends this frame");}
 C(b.Steps()==8&&!b.CanStep(100020),"step cap does not depend on fast clock");
 GsmFinePump::Budget wait(0,1000000);C(!wait.Finished(1,false)&&!wait.CanStep(2)&&wait.Steps()==1,"first unchanged snapshot stops immediately without polling");
 GsmFinePump::Budget time(20,1000000);C(time.CanStep(1519)&&!time.CanStep(1520),"inclusive start and strict1.5ms deadline");
 C(!time.Finished(1520,true)&&!time.CanStep(1521),"time expiry latches stop");
 GsmFinePump::Budget invalid(0,0);C(!invalid.CanStep(0),"zero frequency cannot fabricate elapsed time");
 GsmFinePump::Budget coarse(0,100);C(!coarse.CanStep(0),"insufficient clock resolution safely refuses batch");
 GsmFinePump::Budget backwards(100,1000000);C(!backwards.CanStep(99),"backward time refuses");
 GsmFinePump::Budget huge(0,~GsmFinePump::U64(0));C(huge.CanStep(1)&&!huge.CanStep(~GsmFinePump::U64(0)),"frequency multiplication and elapsed bounds cannot overflow");
 GsmFinePump::Budget close(~GsmFinePump::U64(0)-20,1000000);C(close.CanStep(~GsmFinePump::U64(0))&&!close.CanStep(0),"near counter limit uses difference without deadline overflow");
 std::printf("PASS %u bounded fine pump policy controls\n",checks);return 0;
 }catch(...){return 1;}}
