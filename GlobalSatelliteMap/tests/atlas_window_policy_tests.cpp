#include "../atlas-view/AtlasWindowPolicy.h"
#include <cstdio>
#include <cstdlib>
using namespace AtlasView;
unsigned checks=0;void Check(bool b,const char*s){++checks;if(!b){std::printf("FAIL %s\n",s);std::exit(1);}}
int main(){
    Check(WindowGpuBytes()<GpuCap,"all64 windows within existing96MiB GPU cap");
    WindowRect cells[16];for(unsigned t=0;t<16;++t)cells[t]=WindowRect((t%4)*8192.,(t/4)*8192.,8192,8192);
    WindowPlan p=PlanWindows(cells,WindowRect(800,900,1600,900));
    Check(p.level==0&&p.count>1&&p.count<=48,"native8K level visible window instead of full2048 cap");
    for(unsigned i=0;i<p.count;++i)Check(p.keys[i].Valid()&&p.Contains(p.keys[i]),"plan valid keys");
    WindowKey slots[64];unsigned ages[64]={0};slots[16]=WindowKey(0,0,0,0);ages[16]=1;
    Check(WindowVictim(slots,ages,p)==17,"zoom retains existing image while free slots exist");
    for(unsigned i=16;i<64;++i){slots[i]=WindowKey(15,0,i-16,0);ages[i]=i;} // invalid x also counts free
    for(unsigned i=16;i<64;++i)slots[i]=WindowKey(15,0,(i-16)%32,(i-16)/32);
    Check(WindowVictim(slots,ages,p)==16,"bounded LRU evicts oldest unneeded only");
    for(unsigned i=0;i<p.count;++i){slots[16+i]=p.keys[i];ages[16+i]=0;}
    Check(WindowVictim(slots,ages,p)>=int(16+p.count),"current visible detail protected");
    for(unsigned size=256;size<=32768;size*=2)for(unsigned x=0;x<4;++x)for(unsigned z=0;z<4;++z){
        for(unsigned t=0;t<16;++t)cells[t]=WindowRect((t%4)*double(size),(t/4)*double(size),size,size);
        WindowPlan q=PlanWindows(cells,WindowRect(x*double(size)+size*.48,z*double(size)+size*.48,3840,2160));
        Check(q.count<=48&&q.level<6,"4K viewport bounded across zoom and seams");
        for(unsigned i=0;i<q.count;++i)Check(q.keys[i].Valid()&&WindowVisible(SubWindow(cells[q.keys[i].tile],q.keys[i]),WindowRect(x*double(size)+size*.48,z*double(size)+size*.48,3840,2160)),"all plannedwindows intersectviewport");
    }
    Check(WindowRecord(WindowKey(0,0,0,0))==0&&WindowRecord(WindowKey(15,5,0,0))==21839,"index first and last");
    std::printf("window policy %u checks PASS\n",checks);return 0;
}
