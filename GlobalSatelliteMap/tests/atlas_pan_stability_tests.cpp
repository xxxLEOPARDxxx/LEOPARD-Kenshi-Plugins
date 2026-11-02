#include "D:/DEV/Kenshi/ours/GlobalSatelliteMap/atlas-view/AtlasWindowPolicy.h"
#include <cstdio>
int main(){
 AtlasView::WindowRect cells[16];for(unsigned z=0;z<4;++z)for(unsigned x=0;x<4;++x)cells[z*4+x]=AtlasView::WindowRect(x*4096.0,z*4096.0,4096,4096);
 AtlasView::WindowRect a(204,50,2100,1000),b(205,50,2100,1000);
 AtlasView::WindowPlan p=AtlasView::PlanWindows(cells,a),q=AtlasView::PlanWindows(cells,b);
 unsigned na=AtlasView::EnumerateWindows(cells,a,1,NULL,1000),nb=AtlasView::EnumerateWindows(cells,b,1,NULL,1000);
 std::printf("sameScale primary4096 viewport2100x1000 onePixelPan X204->205: rawL1 %u->%u selectedLevel %u->%u planned %u->%u\n",na,nb,p.level,q.level,p.count,q.count);
 for(unsigned z=0;z<4;++z)for(unsigned x=0;x<4;++x){double l=std::floor(x*4096.5),r=std::floor((x+1)*4096.5);cells[z*4+x]=AtlasView::WindowRect(l,z*4096.0,r-l,4096);}
 AtlasView::WindowPlan u=AtlasView::PlanWindows(cells,AtlasView::WindowRect(3096,50,1000,500)),v=AtlasView::PlanWindows(cells,AtlasView::WindowRect(3097,50,1000,500));
 std::printf("sameScale native4096/4097px rounded primaries onePixelPan X3096->3097: selectedLevel %u->%u planned %u->%u\n",u.level,v.level,u.count,v.count);
 if(p.level!=q.level||u.level!=v.level){std::puts("RED: same-scale one-pixel pan changes source LOD");return 1;}
 std::puts("GREEN stable source LOD");return 0;
}
