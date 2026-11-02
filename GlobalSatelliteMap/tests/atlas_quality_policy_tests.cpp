#include "../atlas-view/AtlasWindowPolicy.h"
#include "../atlas-water/WaterPipeline.h"
#include <cstdio>
#include <cstdlib>
static unsigned checks=0;
static void Check(bool x,const char*s){++checks;if(!x){std::printf("FAIL %s\n",s);std::exit(1);}}
static void Fixture(const AtlasView::WindowRect cells[16],const AtlasView::WindowRect&clip){
    AtlasView::WindowPlan p=AtlasView::PlanWindows(cells,clip);
    Check(p.densityLevel==1&&p.level==p.densityLevel,"measured4096 primary2160x1044 viewport retains densityLevel1");
    Check(p.count<=AtlasView::DetailSlots&&p.visibleCount<=p.count,"visible plus apron stays within bounded detail pool");
    AtlasView::WindowKey actual[AtlasView::DetailSlots];unsigned n=AtlasView::EnumerateWindows(cells,clip,p.level,actual,AtlasView::DetailSlots);
    Check(n==p.visibleCount,"every actual visible window is planned");
    for(unsigned i=0;i<n;++i)Check(actual[i].Valid()&&p.Contains(actual[i]),"actual visible windows are valid and not omitted");
}
int main(){
    AtlasView::WindowRect cells[16];for(unsigned z=0;z<4;++z)for(unsigned x=0;x<4;++x)cells[z*4+x]=AtlasView::WindowRect(x*4096.,z*4096.,4096,4096);
    AtlasView::WindowPlan measured=AtlasView::PlanWindows(cells,AtlasView::WindowRect(500,500,2160,1044));
    std::printf("Measured dimensions; synthetic worst-phase witness: primary4096x4096 viewport2160x1044 source%u density%u visible%u planned%u baseBytes%llu\n",measured.level,measured.densityLevel,measured.visibleCount,measured.count,AtlasView::WindowGpuBytes());
    Fixture(cells,AtlasView::WindowRect(500,500,2160,1044));
    // All subwindow pixel phases on each axis and diagonal, plus primary seams.
    for(unsigned i=0;i<256;++i){Fixture(cells,AtlasView::WindowRect(4096+i,500,2160,1044));Fixture(cells,AtlasView::WindowRect(500,4096+i,2160,1044));Fixture(cells,AtlasView::WindowRect(4096+i,4096+i,2160,1044));}
    for(int dz=-2;dz<=2;++dz)for(int dx=-2;dx<=2;++dx)Fixture(cells,AtlasView::WindowRect(8192+dx,8192+dz,2160,1044));
    // Native projection independently rounds adjacent primary boundaries.
    for(unsigned z=0;z<4;++z)for(unsigned x=0;x<4;++x){double left=std::floor(x*4095.5),right=std::floor((x+1)*4095.5),top=std::floor(z*4095.5),bottom=std::floor((z+1)*4095.5);cells[z*4+x]=AtlasView::WindowRect(left,top,right-left,bottom-top);}
    for(unsigned i=0;i<256;++i)Fixture(cells,AtlasView::WindowRect(4096+i,4096+i,2160,1044));
    // Above the measured4096 threshold the density policy requests source0.
    // The fixed128 pool may coarsen it, but panning must remain stable/bounded.
    for(unsigned z=0;z<4;++z)for(unsigned x=0;x<4;++x){double left=std::floor(x*4096.5),right=std::floor((x+1)*4096.5);cells[z*4+x]=AtlasView::WindowRect(left,z*4096.,right-left,4096);}
    for(unsigned i=0;i<256;++i){AtlasView::WindowPlan p=AtlasView::PlanWindows(cells,AtlasView::WindowRect(4096+i,4096+i,2160,1044));Check(p.level==1&&p.densityLevel==0&&p.count<=AtlasView::DetailSlots,"beyond measured density band remains pan-stable and bounded");}
    Check(AtlasView::GpuCap==192ULL*1024*1024&&AtlasWater::Cap==AtlasView::GpuCap,"one authoritative192MiB cap shared by water and terrain");
    Check(AtlasView::CpuCap==64ULL*1024*1024&&AtlasView::WindowSide==256,"CPU read cap and256 windows unchanged");
    Check(AtlasView::FallbackSlots==16&&AtlasView::DetailSlots==128&&AtlasView::WindowSlots==144,"fixed16fallback plus128detail slots");
    Check(AtlasView::WindowGpuBytes()==189006000ULL,"144window base includes default scratch and constants");
    Check(AtlasWater::PipelinePrivateBytes==3408256ULL&&AtlasView::WindowGpuBytes()+AtlasWater::PipelinePrivateBytes==192414256ULL,"inclusive terrain plus water payload exact");
    Check(AtlasView::WindowGpuBytes()+AtlasWater::PipelinePrivateBytes<=AtlasView::GpuCap,"current complete pool fits explicit bound");
    Check(AtlasWater::PipelineFits(AtlasView::GpuCap-AtlasWater::PipelinePrivateBytes)&&!AtlasWater::PipelineFits(AtlasView::GpuCap-AtlasWater::PipelinePrivateBytes+1),"pipeline plus-one boundary remains strict");
    Check(AtlasWater::Fits(AtlasView::GpuCap-AtlasWater::PrivateBytes)&&!AtlasWater::Fits(AtlasView::GpuCap-AtlasWater::PrivateBytes+1),"water layer uses same inclusive plus-one bound");
    const AtlasView::U64 rgba=256ULL*256*4+AtlasView::UniformGpuBytes;
    Check(AtlasView::Fits(AtlasView::GpuCap-rgba,256)&&!AtlasView::Fits(AtlasView::GpuCap-rgba+1,256),"default terrain plus-one boundary remains strict");
    const AtlasView::U64 hdr=256ULL*256*8+AtlasView::UniformGpuBytes;
    Check(AtlasView::RelighterFits(AtlasView::GpuCap-hdr,256,DXGI_FORMAT_R16G16B16A16_FLOAT)&&!AtlasView::RelighterFits(AtlasView::GpuCap-hdr+1,256,DXGI_FORMAT_R16G16B16A16_FLOAT),"HDR layer plus-one boundary remains strict");
    std::printf("Quality policy PASS %u checks\n",checks);return 0;
}
