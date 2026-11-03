#define _CRT_SECURE_NO_WARNINGS
#include "StagedMaterial.h"
#include "_generated/StagedShaders.h"
#include "../compact-material/_generated/Shaders.h"
#include "../raw-rgb/_generated/RawShaders.h"
#include "../fixed-geometry/FixedTerrainGeometry.h"
#include "../../atlas-view/ContextFixture.h"
#include <cstdio>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
using AtlasView::Com;
static unsigned checks=0;
static void C(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}}
static void HR(HRESULT h,const char*s){C(SUCCEEDED(h),s);}
static std::vector<BYTE>Read(const std::string&p){std::ifstream f(p.c_str(),std::ios::binary|std::ios::ate);C(!!f,"real fixture open");size_t n=size_t(f.tellg());C(n&&n<=20*1024*1024,"bounded fixture bytes");std::vector<BYTE>b(n);f.seekg(0);f.read(reinterpret_cast<char*>(&b[0]),n);C(!!f,"fixture read");return b;}
static void Buffer(ID3D11Device*d,const void*p,unsigned n,unsigned bind,ID3D11Buffer**out){D3D11_BUFFER_DESC b={};b.ByteWidth=n;b.BindFlags=bind;D3D11_SUBRESOURCE_DATA s={};s.pSysMem=p;HR(d->CreateBuffer(&b,p?&s:NULL,out),"proof buffer");}
static ULONG Refs(IUnknown*p){ULONG n=p->AddRef();p->Release();return n;}
static std::vector<BYTE>Pixels(ID3D11Device*d,ID3D11DeviceContext*c,ID3D11Texture2D*t,unsigned bpp){CompactMaterial::SavedPredicate pred(c);D3D11_TEXTURE2D_DESC td;t->GetDesc(&td);td.Usage=D3D11_USAGE_STAGING;td.BindFlags=td.MiscFlags=0;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;Com<ID3D11Texture2D>s;HR(d->CreateTexture2D(&td,NULL,&s.p),"one bounded readback");c->CopyResource(s.p,t);D3D11_MAPPED_SUBRESOURCE m;HR(c->Map(s.p,0,D3D11_MAP_READ,0,&m),"readback map");std::vector<BYTE>v(256*256*bpp);for(unsigned y=0;y<256;++y)std::memcpy(&v[y*256*bpp],static_cast<BYTE*>(m.pData)+y*m.RowPitch,256*bpp);c->Unmap(s.p,0);return v;}
struct Proof{
 Com<ID3D11Device>d;Com<ID3D11DeviceContext>c;CompactMaterial::Inputs sources;StagedMaterial::Carry carry;
 Com<ID3D11VertexShader>vs;Com<ID3D11PixelShader>partial,tail,oracle,nativeRgb;Com<ID3D11InputLayout>layout;
 Com<ID3D11Buffer>vb,ib,vcb,pcb;Com<ID3D11SamplerState>sampler[3];Com<ID3D11RasterizerState>rs;Com<ID3D11DepthStencilState>ds;Com<ID3D11BlendState>blend;
 Com<ID3D11Texture2D>output[4];Com<ID3D11RenderTargetView>rt[3];Com<ID3D11ShaderResourceView>view[3];Com<ID3D11DepthStencilView>depth;
 static const unsigned Payload=4194320+4*169000+17144+448+3856+1835008;
 void Init(){
  HR(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&d.p,NULL,&c.p),"actual headless WARP");
  C(CompactMaterial::SourceBytes+Payload==90612732,"independent complete staged proof payload ledger");
  C(carry.Init(d.p,CompactMaterial::Limit-StagedMaterial::PrivateBytes+1)==E_OUTOFMEMORY&&!carry.OwnedGpuBytes(),"carry cap+1 before resources");
  HR(carry.Init(d.p,CompactMaterial::SourceBytes+Payload-StagedMaterial::PrivateBytes),"two complete carry sets admitted");C(carry.OwnedGpuBytes()==4194320,"4MiB ping-pong plus16B slot CB");C(FAILED(carry.Init(d.p,0)),"double carry Init refuses");
  HR(sources.Init(d.p,Payload),"complete source owner plus proof admitted under90MiB");
  for(unsigned i=0;i<12;++i){std::vector<BYTE>b=Read(std::string("D:/SteamLibrary/steamapps/common/Kenshi/")+CompactMaterial::DdsPins[i].path);HR(sources.UploadDds(i,&b[0],b.size()),"real pinned DDS serial upload");}
  for(unsigned i=0;i<3;++i){char p[180];std::sprintf(p,"../raw-rgb/_generated/fixture/image%u.rgba",i);std::vector<BYTE>b=Read(p);HR(sources.UploadRgba(i,&b[0],b.size()),"pinned real decoded painted pixels");}HR(sources.Complete(),"canonical fixed two-SID sources complete");
  HR(d.p->CreateVertexShader(CompactMaterial::VertexShader,sizeof(CompactMaterial::VertexShader),NULL,&vs.p),"unchanged native geography VS");
  HR(d.p->CreatePixelShader(StagedMaterial::PartialShader,sizeof(StagedMaterial::PartialShader),NULL,&partial.p),"ordered one-slot PS");HR(d.p->CreatePixelShader(StagedMaterial::TailShader,sizeof(StagedMaterial::TailShader),NULL,&tail.p),"once-only native tail PS");HR(d.p->CreatePixelShader(TerrainRaw::PrewriteShader,sizeof(TerrainRaw::PrewriteShader),NULL,&oracle.p),"independent native prewrite PS");HR(d.p->CreatePixelShader(TerrainRaw::RgbShader,sizeof(TerrainRaw::RgbShader),NULL,&nativeRgb.p),"native RGB shader for extra algebra controls");
  D3D11_INPUT_ELEMENT_DESC e[3]={{"POSITION",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,16,D3D11_INPUT_PER_VERTEX_DATA,0},{"BINORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,28,D3D11_INPUT_PER_VERTEX_DATA,0}};HR(d.p->CreateInputLayout(e,3,CompactMaterial::VertexShader,sizeof(CompactMaterial::VertexShader),&layout.p),"unchanged raw40B native vertex ABI");
  std::vector<BYTE>h=Read("../compact-material/_generated/height.u16");FixedTerrain::Samples source={7358,7358,133,133,133,reinterpret_cast<const unsigned short*>(&h[0]),133*133};std::vector<FixedTerrain::Vertex>v(4*FixedTerrain::Vertices);for(unsigned z=0;z<2;++z)for(unsigned x=0;x<2;++x)C(FixedTerrain::Build(source,7360+x*64,7360+z*64,&v[(z*2+x)*FixedTerrain::Vertices],FixedTerrain::Vertices),"literal four actual height patches");unsigned short index[FixedTerrain::IndexCount];C(FixedTerrain::Indices(index,FixedTerrain::IndexCount),"native strip topology");Buffer(d.p,&v[0],unsigned(v.size()*40),D3D11_BIND_VERTEX_BUFFER,&vb.p);Buffer(d.p,index,sizeof(index),D3D11_BIND_INDEX_BUFFER,&ib.p);Buffer(d.p,NULL,448,D3D11_BIND_CONSTANT_BUFFER,&vcb.p);Buffer(d.p,NULL,3856,D3D11_BIND_CONSTANT_BUFFER,&pcb.p);
  for(unsigned i=0;i<4;++i){D3D11_TEXTURE2D_DESC t={};t.Width=t.Height=256;t.ArraySize=t.MipLevels=t.SampleDesc.Count=1;t.Format=i==0?DXGI_FORMAT_R32G32B32A32_FLOAT:i==1?DXGI_FORMAT_R8G8B8A8_UNORM:i==2?DXGI_FORMAT_R32_FLOAT:DXGI_FORMAT_D32_FLOAT;t.BindFlags=i==3?D3D11_BIND_DEPTH_STENCIL:D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;HR(d.p->CreateTexture2D(&t,NULL,&output[i].p),"three proof planes plus depth");if(i<3){HR(d.p->CreateRenderTargetView(output[i].p,NULL,&rt[i].p),"proof RTV");HR(d.p->CreateShaderResourceView(output[i].p,NULL,&view[i].p),"proof read view");}}HR(d.p->CreateDepthStencilView(output[3].p,NULL,&depth.p),"proof depth view");
  for(unsigned i=0;i<3;++i){D3D11_SAMPLER_DESC s={};s.Filter=i==0?D3D11_FILTER_ANISOTROPIC:i==1?D3D11_FILTER_MIN_MAG_MIP_LINEAR:D3D11_FILTER_MIN_MAG_MIP_POINT;s.AddressU=s.AddressV=s.AddressW=i==2?D3D11_TEXTURE_ADDRESS_CLAMP:D3D11_TEXTURE_ADDRESS_WRAP;s.MaxAnisotropy=16;s.MaxLOD=D3D11_FLOAT32_MAX;HR(d.p->CreateSamplerState(&s,&sampler[i].p),"literal native sampler roles");}
  D3D11_RASTERIZER_DESC r={};r.FillMode=D3D11_FILL_SOLID;r.CullMode=D3D11_CULL_NONE;r.DepthClipEnable=TRUE;HR(d.p->CreateRasterizerState(&r,&rs.p),"owned raster");D3D11_DEPTH_STENCIL_DESC dd={};dd.DepthEnable=TRUE;dd.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;dd.DepthFunc=D3D11_COMPARISON_LESS_EQUAL;HR(d.p->CreateDepthStencilState(&dd,&ds.p),"owned depth");D3D11_BLEND_DESC b={};for(unsigned i=0;i<8;++i)b.RenderTarget[i].RenderTargetWriteMask=15;HR(d.p->CreateBlendState(&b,&blend.p),"NO additive blend");
 }
 void Draw(){UINT stride=40;for(unsigned i=0;i<4;++i){UINT offset=i*FixedTerrain::Vertices*40;c.p->IASetVertexBuffers(0,1,&vb.p,&stride,&offset);c.p->DrawIndexed(FixedTerrain::IndexCount,0,0);}}
 void Run(const std::vector<BYTE>&vc,const std::vector<BYTE>&pc,bool staged,ID3D11ShaderResourceView*selectorOverride=NULL,ID3D11ShaderResourceView*blendOverride=NULL,bool fullReference=false){
  C(vc.size()==448&&pc.size()==3856,"unaltered reference native CB spans");
  AtlasView::SavedOutputReads a(c.p,output[0].p),b(c.p,output[1].p),cc(c.p,output[2].p),r0(c.p,carry.Texture(0,0)),r1(c.p,carry.Texture(0,1)),r2(c.p,carry.Texture(1,0)),r3(c.p,carry.Texture(1,1));StagedMaterial::Bindings extra(c.p);FixedTerrainRaster::PixelState pixels(c.p);AtlasModels::GraphicsState saved(c.p);
  c.p->SetPredication(NULL,FALSE);ID3D11UnorderedAccessView*empty[8]={};c.p->OMSetRenderTargetsAndUnorderedAccessViews(0,NULL,NULL,0,8,empty,NULL);c.p->HSSetShader(NULL,NULL,0);c.p->DSSetShader(NULL,NULL,0);c.p->GSSetShader(NULL,NULL,0);c.p->VSSetShader(vs.p,NULL,0);c.p->VSSetConstantBuffers(0,1,&vcb.p);c.p->PSSetConstantBuffers(0,1,&pcb.p);ID3D11Buffer*slot=carry.Constants();c.p->PSSetConstantBuffers(1,1,&slot);ID3D11ShaderResourceView*inputs[8];for(unsigned i=0;i<8;++i)inputs[i]=sources.View(i);if(selectorOverride)inputs[6]=selectorOverride;if(blendOverride)inputs[5]=blendOverride;c.p->PSSetShaderResources(0,8,inputs);c.p->PSSetSamplers(0,1,&sampler[0].p);c.p->PSSetSamplers(2,1,&sampler[1].p);c.p->PSSetSamplers(4,1,&sampler[2].p);c.p->RSSetState(rs.p);D3D11_VIEWPORT vp={0,0,256,256,0,1};c.p->RSSetViewports(1,&vp);c.p->RSSetScissorRects(0,NULL);c.p->OMSetBlendState(blend.p,NULL,~0u);c.p->OMSetDepthStencilState(ds.p,0);c.p->IASetInputLayout(layout.p);c.p->IASetIndexBuffer(ib.p,DXGI_FORMAT_R16_UINT,0);c.p->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);c.p->UpdateSubresource(vcb.p,0,NULL,&vc[0],0,0);c.p->UpdateSubresource(pcb.p,0,NULL,&pc[0],0,0);
  ID3D11ShaderResourceView*none[2]={};const float zero[4]={};
  if(staged)for(unsigned slotIndex=0;slotIndex<4;++slotIndex){
   c.p->PSSetShaderResources(8,2,none);ID3D11RenderTargetView*write[2]={carry.Target(slotIndex&1,0),carry.Target(slotIndex&1,1)};c.p->OMSetRenderTargets(2,write,depth.p);for(unsigned i=0;i<2;++i)c.p->ClearRenderTargetView(write[i],zero);c.p->ClearDepthStencilView(depth.p,D3D11_CLEAR_DEPTH,1,0);
   ID3D11ShaderResourceView*previous[2]={slotIndex?carry.View((slotIndex-1)&1,0):NULL,slotIndex?carry.View((slotIndex-1)&1,1):NULL};c.p->PSSetShaderResources(8,2,previous);unsigned control[4]={slotIndex,0,0,0};c.p->UpdateSubresource(slot,0,NULL,control,0,0);c.p->PSSetShader(partial.p,NULL,0);Draw();
  }
  c.p->PSSetShaderResources(8,2,none);ID3D11RenderTargetView*final[3]={rt[0].p,rt[1].p,rt[2].p};c.p->OMSetRenderTargets(3,final,depth.p);for(unsigned i=0;i<3;++i)c.p->ClearRenderTargetView(final[i],zero);c.p->ClearDepthStencilView(depth.p,D3D11_CLEAR_DEPTH,1,0);ID3D11ShaderResourceView*last[2]={carry.View(1,0),carry.View(1,1)};if(staged)c.p->PSSetShaderResources(8,2,last);c.p->PSSetShader(staged?tail.p:fullReference?nativeRgb.p:oracle.p,NULL,0);Draw();HR(d.p->GetDeviceRemovedReason(),"complete healthy proof submission");
 }
};
int main(){try{
 Proof p;p.Init();const char*names[9]={"control3","control9","shift3","control1","wet3","underwater3","hdr3","hdr1","odd3"};float maxError=0;
 for(unsigned pass=0;pass<9;++pass){std::string base=std::string("../raw-rgb/_generated/")+names[pass];std::vector<BYTE>vc=Read(base+"-vertex-cb.bin"),pc=Read(base+"-pixel-cb.bin");p.Run(vc,pc,false);std::vector<BYTE>native=Pixels(p.d.p,p.c.p,p.output[0].p,16),expected=Read(base+"-prewrite.rgba32f");C(native==expected,"fresh original native prewrite equals frozen oracle bytes");
  {AtlasViewTest::ContextFixture before(p.c.p);p.Run(vc,pc,true);AtlasViewTest::ContextFixture after(p.c.p);C(before.Equal(after),"ordered pass sequence restores foreign graphics and extra slots");}
  std::vector<BYTE>actual=Pixels(p.d.p,p.c.p,p.output[0].p,16);for(unsigned i=0;i<256*256*4;++i){float a,b;std::memcpy(&a,&actual[i*4],4);std::memcpy(&b,&native[i*4],4);C(_finite(a)&&_finite(b),"canonical fixture finite prewrite and carry");float e=std::fabs(a-b);maxError=(std::max)(maxError,e);if(e>2e-6f)std::fprintf(stderr,"case%s component%u staged%.9g native%.9g\n",names[pass],i,a,b);C(e<=2e-6f,"ordered staged albedo/gloss matches original native prewrite");}
  C(actual==native,"all nine actual material controls bit-exact to original prewrite");C(Pixels(p.d.p,p.c.p,p.output[1].p,4)==Read(base+"-normal.rgba8"),"tail native normal bytes unchanged");C(Pixels(p.d.p,p.c.p,p.output[2].p,4)==Read(base+"-radial.r32f"),"tail native radial bytes unchanged");
 }
 // Explicit test-only selector/blend bindings stress four LIVE contributions.
 // They use the identical real two-SID textures/parameters and native oracle;
 // they are not evidence for a live four-distinct-SID cell or source winner.
 const BYTE codes[3][4]={{2,4,18,36},{2,0,4,0},{2,4,2,4}};
 const BYTE weights[3][4]={{39,67,95,0},{57,0,0,0},{217,0,0,0}};
 for(unsigned fixture=0;fixture<3;++fixture){
  Com<ID3D11Texture2D>select,blend;Com<ID3D11ShaderResourceView>selectView,blendView;BYTE cells[64];for(unsigned j=0;j<16;++j)std::memcpy(cells+j*4,codes[fixture],4);D3D11_TEXTURE2D_DESC t={};t.Width=t.Height=4;t.ArraySize=t.MipLevels=t.SampleDesc.Count=1;t.Format=DXGI_FORMAT_R8G8B8A8_UNORM;t.BindFlags=D3D11_BIND_SHADER_RESOURCE;t.Usage=D3D11_USAGE_IMMUTABLE;D3D11_SUBRESOURCE_DATA data={};data.pSysMem=cells;data.SysMemPitch=16;HR(p.d.p->CreateTexture2D(&t,&data,&select.p),"explicit synthetic selector control");HR(p.d.p->CreateShaderResourceView(select.p,NULL,&selectView.p),"synthetic selector view");t.Width=t.Height=1;data.pSysMem=weights[fixture];data.SysMemPitch=4;HR(p.d.p->CreateTexture2D(&t,&data,&blend.p),"explicit synthetic blend control");HR(p.d.p->CreateShaderResourceView(blend.p,NULL,&blendView.p),"synthetic blend view");
  std::vector<BYTE>vc=Read("../raw-rgb/_generated/control3-vertex-cb.bin"),pc=Read("../raw-rgb/_generated/wet3-pixel-cb.bin");p.Run(vc,pc,false,selectView.p,blendView.p,true);std::vector<BYTE>reference[3]={Pixels(p.d.p,p.c.p,p.output[0].p,16),Pixels(p.d.p,p.c.p,p.output[1].p,4),Pixels(p.d.p,p.c.p,p.output[2].p,4)};p.Run(vc,pc,true,selectView.p,blendView.p);
  for(unsigned plane=0;plane<3;++plane)C(Pixels(p.d.p,p.c.p,p.output[plane].p,plane?4:16)==reference[plane],"four weighted/repeated/empty-slot and negative residual native oracle byte parity");
 }
 // Deliberate foreign reads of both ping-pong sets at different stages and
 // unrelated extra CB/SRV bindings must be restored even under predication.
 p.Run(Read("../raw-rgb/_generated/control9-vertex-cb.bin"),Read("../raw-rgb/_generated/control9-pixel-cb.bin"),true);
 ID3D11ShaderResourceView*read=p.carry.View(0,0);p.c.p->CSSetShaderResources(71,1,&read);read=p.carry.View(1,1);p.c.p->VSSetShaderResources(91,1,&read);p.c.p->PSSetShaderResources(8,1,&read);ID3D11Buffer*cb=p.pcb.p;p.c.p->PSSetConstantBuffers(1,1,&cb);
 Com<ID3D11Predicate>pred;D3D11_QUERY_DESC q={D3D11_QUERY_OCCLUSION_PREDICATE,0};HR(p.d.p->CreatePredicate(&q,&pred.p),"foreign false predicate");p.c.p->Begin(pred.p);p.c.p->End(pred.p);p.c.p->Flush();BOOL result=TRUE;HRESULT hr=S_FALSE;for(unsigned i=0;i<10000&&hr==S_FALSE;++i)hr=p.c.p->GetData(pred.p,&result,sizeof(result),0);C(hr==S_OK&&!result,"actual predicate false");ULONG refs=Refs(p.d.p),carryRefs=Refs(read);
 for(unsigned mode=0;mode<2;++mode){p.c.p->SetPredication(pred.p,mode?TRUE:FALSE);AtlasViewTest::ContextFixture before(p.c.p);p.Run(Read(std::string("../raw-rgb/_generated/")+(mode?"control9":"control3")+"-vertex-cb.bin"),Read("../raw-rgb/_generated/control3-pixel-cb.bin"),true);AtlasViewTest::ContextFixture after(p.c.p);C(before.Equal(after),"complete sequence restores alias reads predicate and explicit extra ABI");C(Pixels(p.d.p,p.c.p,p.output[2].p,4)==Read(std::string("../raw-rgb/_generated/")+(mode?"control9":"control3")+"-radial.r32f"),"predicate cannot suppress carry/constants/draws or hide stale output");}
 C(Refs(p.d.p)==refs&&Refs(read)==carryRefs,"whole staged submission COM references balanced");p.c.p->ClearState();p.carry.Clear();C(!p.carry.OwnedGpuBytes()&&!p.carry.Texture(0,0)&&!p.carry.View(1,1)&&!p.carry.Constants(),"Clear resets both sets and slot CB");HR(p.carry.Init(p.d.p,CompactMaterial::SourceBytes),"carry reInit after Clear");std::printf("PASS %u ordered staged material WARP checks;9 actual cases;maxFloatError%.9g;carry4194304+16B\n",checks,maxError);return 0;
 }catch(...){return 1;}}
