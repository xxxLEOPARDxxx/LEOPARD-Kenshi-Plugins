#define main upstream_bank_main
#include "../../GlobalSatelliteMap/atlas-terrain/world-bank/warp_tests.cpp"
#undef main
int main(){try{
 Com<ID3D11Device>d;Com<ID3D11DeviceContext>c;HR(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&d.p,NULL,&c.p),"review WARP");
 Ledger l={};U64 fields[8]={};
 for(unsigned i=0;i<8;++i){std::memset(fields,0,sizeof(fields));fields[i]=AtlasView::GpuCap-SourceBytes;std::memcpy(&l,fields,sizeof(l));U64 total=0;C(Fits(l,&total)&&total==AtlasView::GpuCap,"every named ledger owner accepts exact cap");fields[(i+1)%8]=1;std::memcpy(&l,fields,sizeof(l));C(!Fits(l),"every named ledger owner includes other live owner");fields[i]=~U64(0);std::memcpy(&l,fields,sizeof(l));C(!Fits(l),"every named ledger owner rejects overflow");}
 std::memset(&l,0,sizeof(l));Sources s;SourceEpoch e={0xf123456700000007ULL,PinnedBaseSnapshot};HR(s.Init(d.p,e,l),"full64 review source");WorldMaterial::Planner p;C(p.Init(e.value),"review planner");WorldMaterial::Plan plan;C(p.Build(27,10,e.value,0,plan),"review plan");HR(s.BeginStage(plan,e,0,l),"review begin");
 for(unsigned k=0;k<3;++k){char path[80];std::sprintf(path,"_generated/rgba-%u.bin",k==2?128:k*64+27);std::vector<BYTE>b=Read(path);HR(s.UploadRgba(e,0,k,&b[0],b.size(),l),"review RGBA");}
 for(unsigned k=0;k<2;++k)for(unsigned n=0;n<6;++n){const SatelliteRegion::Asset*a=s.ExpectedDds(e,0,k,n);std::vector<BYTE>b=Read(std::string("D:/SteamLibrary/steamapps/common/Kenshi/")+a->path);HR(s.UploadDds(e,0,k,n,&b[0],b.size(),l),"review DDS");}
 HR(s.Complete(e,0,l),"review complete");Bindings lease={};HR(s.AcquireBase(e,0,l,lease),"review acquire");
 for(unsigned i=0;i<9;++i){Bindings bad=lease;switch(i){case 0:bad.epoch.value^=1ULL<<63;break;case 1:bad.epoch.profile++;break;case 2:bad.revision++;break;case 3:bad.resourceToken++;break;case 4:bad.region++;break;case 5:bad.cell++;break;case 6:bad.slot++;break;case 7:bad.row++;break;default:bad.view[7]=NULL;}C(!s.LeaseCurrent(bad),"every lease identity field independently rejects stale/forged acquisition");C(s.LeaseCurrent(lease),"bad lease check preserves legitimate owner");}
 const U64 allocation=s.ResourceToken();s.Clear();C(!s.LeaseCurrent(lease)&&!s.OwnedGpuBytes(),"Clear revokes every outstanding borrowed lease");HR(s.Init(d.p,e,l),"same-epoch reallocation");C(s.ResourceToken()>allocation&&!s.LeaseCurrent(lease),"same-epoch replacement never resurrects borrowed lease");s.Clear();
 std::printf("PASS %u independent bank checks\n",checks);return 0;
 }catch(...){return 1;}}
