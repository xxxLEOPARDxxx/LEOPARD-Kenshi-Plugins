#define _CRT_SECURE_NO_WARNINGS
#include "FixedTerrainRaster.h"
#include "../compact-material/_generated/Shaders.h"
#include "../../atlas-view/ContextFixture.h"
#include <cstdio>
#include <fstream>
#include <vector>
#include <string>
static unsigned checks=0;
static void C(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}}
static void HR(HRESULT h,const char*s){C(SUCCEEDED(h),s);}
static std::vector<BYTE>Read(const std::string&path){std::ifstream f(path.c_str(),std::ios::binary|std::ios::ate);C(!!f,"fixture open");size_t n=size_t(f.tellg());C(n&&n<=20*1024*1024,"bounded fixture");std::vector<BYTE>b(n);f.seekg(0);f.read(reinterpret_cast<char*>(&b[0]),n);C(!!f,"fixture read");return b;}
static std::vector<BYTE>Pixels(ID3D11Device*d,ID3D11DeviceContext*c,ID3D11Texture2D*t){D3D11_TEXTURE2D_DESC td;t->GetDesc(&td);td.Usage=D3D11_USAGE_STAGING;td.BindFlags=td.MiscFlags=0;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;AtlasView::Com<ID3D11Texture2D>s;HR(d->CreateTexture2D(&td,NULL,&s.p),"readback allocate");c->CopyResource(s.p,t);D3D11_MAPPED_SUBRESOURCE m;HR(c->Map(s.p,0,D3D11_MAP_READ,0,&m),"readback map");std::vector<BYTE>v(256*256*4);for(unsigned y=0;y<256;++y)std::memcpy(&v[y*1024],static_cast<BYTE*>(m.pData)+y*m.RowPitch,1024);c->Unmap(s.p,0);return v;}
static ULONG Refs(IUnknown*p){ULONG n=p->AddRef();p->Release();return n-1;}
static void Buffer(ID3D11Device*d,unsigned bytes,unsigned flags,ID3D11Buffer**out){D3D11_BUFFER_DESC b={};b.ByteWidth=bytes;b.BindFlags=flags;HR(d->CreateBuffer(&b,NULL,out),"foreign buffer");}
int main(){try{
 AtlasView::Com<ID3D11Device>d;AtlasView::Com<ID3D11DeviceContext>c;HR(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&d.p,NULL,&c.p),"owned WARP");
 std::vector<BYTE>h=Read("../compact-material/_generated/height.u16");FixedTerrain::Samples source={7358,7358,133,133,133,reinterpret_cast<const unsigned short*>(&h[0]),133*133};
 FixedTerrainRaster::Shaders shaders={{CompactMaterial::VertexShader,sizeof(CompactMaterial::VertexShader)},{CompactMaterial::PixelShader,sizeof(CompactMaterial::PixelShader)}};FixedTerrainRaster::Grid grid={7360,7360,2,2,7};FixedTerrainRaster::Raster raster;
 using namespace FixedTerrainRaster;using AtlasView::Com;
 CompactMaterial::Inputs sources;
 Frame frame={7,-13824,-13824,3,10000,20000,-10000,0};
 C(raster.Render(false,sources,frame,~U64(0))==S_FALSE,"OFF returns before all admission/query");
 const U64 own=Raster::RequiredBytes(4),edge=CompactMaterial::Limit-own;C(own==1746024,"independent exact GPU payload ledger");
 C(!Raster::RequiredBytes(0)&&!Raster::RequiredBytes(17)&&Raster::RequiredBytes(16)>own,"bounded sixteen patch ledger");
 C(raster.Init(d.p,shaders,source,grid,edge+1)==E_OUTOFMEMORY&&raster.OwnedGpuBytes()==0,"Init cap+1 before owners");
 C(raster.Init(d.p,shaders,source,grid,CompactMaterial::SourceBytes-1)==E_OUTOFMEMORY,"caller ledger must include compact source owner");
 Grid bad=grid;bad.columns=17;C(FAILED(raster.Init(d.p,shaders,source,bad,edge)),"seventeenth patch refuses");bad=grid;bad.x++;C(FAILED(raster.Init(d.p,shaders,source,bad,edge)),"unaligned patch refuses");
 FixedTerrain::Samples shortSource=source;shortSource.sampleCount--;C(FAILED(raster.Init(d.p,shaders,shortSource,grid,edge)),"short source span refuses");
 Shaders wrong=shaders;wrong.pixel.size--;C(!ValidShaders(wrong),"shader identity predicate refuses altered span");C(FAILED(raster.Init(d.p,wrong,source,grid,edge)),"unpinned shader refuses");
 HR(raster.Init(d.p,shaders,source,grid,edge),"implemented owned raster Init");C(raster.OwnedGpuBytes()==own,"complete private ledger");C(FAILED(raster.Init(d.p,shaders,source,grid,edge)),"double Init refuses");
 C(FAILED(raster.Render(true,sources,frame,CompactMaterial::SourceBytes)),"incomplete source owner refuses");
 {std::vector<unsigned short>flat(261*261,1000);FixedTerrain::Samples large={7166,7166,261,261,261,&flat[0],flat.size()};Grid sixteen={7168,7168,4,4,9};Raster maximum;HR(maximum.Init(d.p,shaders,large,sixteen,CompactMaterial::Limit-Raster::RequiredBytes(16)),"sixteen immutable patches admitted");C(maximum.OwnedGpuBytes()==3774024,"sixteen-patch independent payload ledger");}
 HR(sources.Init(d.p,own),"source owner plus raster admitted");
 for(unsigned i=0;i<12;++i){std::vector<BYTE>b=Read(std::string("D:/SteamLibrary/steamapps/common/Kenshi/")+CompactMaterial::DdsPins[i].path);HR(sources.UploadDds(i,&b[0],b.size()),"staged real pinned DDS");}
 for(unsigned i=0;i<3;++i){char path[100];std::sprintf(path,"../compact-material/_generated/image%u.rgba",i);std::vector<BYTE>b=Read(path);HR(sources.UploadRgba(i,&b[0],b.size()),"staged real decoded PNG");}HR(sources.Complete(),"source set complete");
 // Four existing independent manual-native-material controls, unchanged bytes.
 const char*names[4]={"control3","control9","shift3","control1"};const float spacing[4]={3,9,3,1},center[4]={-13824,-13824,-13632,-13824};
 for(unsigned pass=0;pass<4;++pass){Frame f=frame;f.x=f.z=center[pass];f.spacing=spacing[pass];AtlasViewTest::ContextFixture before(c.p);HR(raster.Render(true,sources,f,edge),"actual packed terrain draw");AtlasViewTest::ContextFixture after(c.p);C(before.Equal(after),"complete foreign context restored");for(unsigned plane=0;plane<3;++plane){std::vector<BYTE>v=Pixels(d.p,c.p,raster.Texture(plane));char path[160];std::sprintf(path,"../compact-material/_generated/%s-gbuffer%u.bin",names[pass],plane);std::vector<BYTE>expected=Read(path);C(v.size()==expected.size(),"manual plane extent");for(unsigned i=0;i<v.size();++i)C(v[i]==expected[i],"exact independent manual native material GBuffer");}}
 // Foreign state has output reads at every stage, all modified pixel slots,
 // nondefault samplers, high IA slots, a false predicate and an append counter.
 Com<ID3D11SamplerState>sampler;D3D11_SAMPLER_DESC sd={};sd.Filter=D3D11_FILTER_MIN_MAG_MIP_POINT;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sd.MaxLOD=4;HR(d.p->CreateSamplerState(&sd,&sampler.p),"foreign sampler");for(unsigned slot=0;slot<16;++slot)c.p->PSSetSamplers(slot,1,&sampler.p);
 for(unsigned slot=0;slot<8;++slot){ID3D11ShaderResourceView*v=sources.View(slot);c.p->PSSetShaderResources(slot,1,&v);}ID3D11ShaderResourceView*read=raster.Output(0);c.p->VSSetShaderResources(17,1,&read);c.p->HSSetShaderResources(31,1,&read);c.p->DSSetShaderResources(63,1,&read);c.p->GSSetShaderResources(127,1,&read);c.p->PSSetShaderResources(109,1,&read);c.p->CSSetShaderResources(121,1,&read);
 Com<ID3D11Buffer>foreignVb;Buffer(d.p,64,D3D11_BIND_VERTEX_BUFFER,&foreignVb.p);UINT stride=16,offset=0;c.p->IASetVertexBuffers(15,1,&foreignVb.p,&stride,&offset);c.p->IASetVertexBuffers(31,1,&foreignVb.p,&stride,&offset);c.p->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);D3D11_VIEWPORT vp={3,5,17,19,.2f,.8f};c.p->RSSetViewports(1,&vp);D3D11_RECT rect={3,5,8,9};c.p->RSSetScissorRects(1,&rect);
 Com<ID3D11Predicate>pred;D3D11_QUERY_DESC qd={D3D11_QUERY_OCCLUSION_PREDICATE,0};HR(d.p->CreatePredicate(&qd,&pred.p),"false predicate");c.p->Begin(pred.p);c.p->End(pred.p);c.p->Flush();BOOL value=TRUE;HRESULT status=S_FALSE;for(unsigned i=0;i<10000&&status==S_FALSE;++i)status=c.p->GetData(pred.p,&value,sizeof(value),0);C(status==S_OK&&!value,"actual false predicate completed");c.p->SetPredication(pred.p,FALSE);
 Com<ID3D11Buffer>append,counter;D3D11_BUFFER_DESC ab={};ab.ByteWidth=64;ab.BindFlags=D3D11_BIND_UNORDERED_ACCESS;ab.MiscFlags=D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;ab.StructureByteStride=4;HR(d.p->CreateBuffer(&ab,NULL,&append.p),"foreign append buffer");Com<ID3D11UnorderedAccessView>uav;D3D11_UNORDERED_ACCESS_VIEW_DESC ud={};ud.ViewDimension=D3D11_UAV_DIMENSION_BUFFER;ud.Buffer.NumElements=16;ud.Buffer.Flags=D3D11_BUFFER_UAV_FLAG_APPEND;HR(d.p->CreateUnorderedAccessView(append.p,&ud,&uav.p),"foreign append UAV");UINT initial=11;c.p->OMSetRenderTargetsAndUnorderedAccessViews(0,NULL,NULL,3,1,&uav.p,&initial);
 ULONG deviceRefs=Refs(d.p),viewRefs=Refs(read),sourceRefs=Refs(sources.View(0));
 for(unsigned repeat=0;repeat<8;++repeat){AtlasViewTest::ContextFixture before(c.p);HR(raster.Render(true,sources,frame,edge),"draw under false foreign predicate");AtlasViewTest::ContextFixture after(c.p);C(before.Equal(after),"all stage reads samplers IA predicate and OM restored");}
 C(Refs(d.p)==deviceRefs&&Refs(read)==viewRefs&&Refs(sources.View(0))==sourceRefs,"repeated Render balanced COM query references");
 // Readback copy and counter observation must themselves disable predication.
 c.p->SetPredication(NULL,FALSE);ab.ByteWidth=4;ab.BindFlags=ab.MiscFlags=ab.StructureByteStride=0;ab.Usage=D3D11_USAGE_STAGING;ab.CPUAccessFlags=D3D11_CPU_ACCESS_READ;HR(d.p->CreateBuffer(&ab,NULL,&counter.p),"counter observer");c.p->CopyStructureCount(counter.p,0,uav.p);D3D11_MAPPED_SUBRESOURCE cm;HR(c.p->Map(counter.p,0,D3D11_MAP_READ,0,&cm),"counter map");C(*static_cast<unsigned*>(cm.pData)==11,"hidden OM append counter preserved");c.p->Unmap(counter.p,0);
 std::vector<BYTE>last=Pixels(d.p,c.p,raster.Texture(0)),expected=Read("../compact-material/_generated/control3-gbuffer0.bin");C(last==expected,"foreign false predicate did not suppress raster");
 for(unsigned badCase=0;badCase<11;++badCase){Frame b=frame;switch(badCase){case 0:b.epoch++;break;case 1:b.spacing=0;break;case 2:b.spacing=-1;break;case 3:b.x+=2000;break;case 4:b.farClip=10000;break;case 5:b.cameraY=0;break;case 6:b.wetness=-.7f;break;case 7:b.waterHeight=10000;break;case 8:{unsigned nan=0x7fc00000;std::memcpy(&b.z,&nan,4);break;}case 9:b.spacing=10;break;case 10:b.spacing=1e-38f;break;}AtlasViewTest::ContextFixture before(c.p);C(FAILED(raster.Render(true,sources,b,edge)),"invalid epoch/domain/frame refuses");AtlasViewTest::ContextFixture after(c.p);C(before.Equal(after),"refused frame preserves all foreign state");}
 {AtlasViewTest::ContextFixture before(c.p);C(raster.Render(true,sources,frame,edge+1)==E_OUTOFMEMORY,"Render budget rechecked");AtlasViewTest::ContextFixture after(c.p);C(before.Equal(after),"budget refusal no mutation");}
 // Writable output and source aliases refuse without touching their bindings.
 Com<ID3D11RenderTargetView>alias;HR(d.p->CreateRenderTargetView(raster.Texture(0),NULL,&alias.p),"output writable alias");c.p->OMSetRenderTargets(1,&alias.p,NULL);{AtlasViewTest::ContextFixture before(c.p);C(FAILED(raster.Render(true,sources,frame,edge)),"output writable alias refusal");AtlasViewTest::ContextFixture after(c.p);C(before.Equal(after),"output alias refusal preserves state");}c.p->OMSetRenderTargets(0,NULL,NULL);
 Com<ID3D11Resource>paint;sources.View(2)->GetResource(&paint.p);Com<ID3D11RenderTargetView>paintRt;HR(d.p->CreateRenderTargetView(paint.p,NULL,&paintRt.p),"source writable alias");c.p->OMSetRenderTargets(1,&paintRt.p,NULL);{AtlasViewTest::ContextFixture before(c.p);C(FAILED(raster.Render(true,sources,frame,edge)),"source writable alias refusal");AtlasViewTest::ContextFixture after(c.p);C(before.Equal(after),"source alias refusal preserves state");}c.p->OMSetRenderTargets(0,NULL,NULL);
 Com<ID3D11Buffer>so;Buffer(d.p,64,D3D11_BIND_STREAM_OUTPUT,&so.p);UINT soOffset=0;c.p->SOSetTargets(1,&so.p,&soOffset);{AtlasViewTest::ContextFixture before(c.p);C(FAILED(raster.Render(true,sources,frame,edge)),"foreign SO refuses");AtlasViewTest::ContextFixture after(c.p);C(before.Equal(after),"SO refusal preserves state and cursor");}c.p->SOSetTargets(0,NULL,NULL);
 {AtlasViewTest::ContextFixture before(c.p);C(raster.Render(false,sources,frame,~U64(0))==S_FALSE,"OFF initialized owner ignores impossible ledger");AtlasViewTest::ContextFixture after(c.p);C(before.Equal(after),"OFF preserves all state");}
 for(unsigned plane=0;plane<3;++plane){char path[100];std::sprintf(path,"../compact-material/_generated/control3-gbuffer%u.bin",plane);C(Pixels(d.p,c.p,raster.Texture(plane))==Read(path),"every refusal and OFF preserves previous output bytes");}
 // Caller-held output lifetime is explicit; Clear releases only this owner.
 read->AddRef();c.p->ClearState();raster.Clear();C(raster.OwnedGpuBytes()==0&&!raster.Texture(0)&&!raster.Output(0),"Clear resets resource ledger");Com<ID3D11Resource>survivor;read->GetResource(&survivor.p);C(!!survivor.p,"caller held output survives Clear");read->Release();raster.Clear();HR(raster.Init(d.p,shaders,source,grid,edge),"Clear supports reInit");HR(raster.Render(true,sources,frame,edge),"reInit render usable");
 std::printf("PASS %u actual raster checks\n",checks);return 0;
 }catch(...){return 1;}}
