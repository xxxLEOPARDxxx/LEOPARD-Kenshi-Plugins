#pragma once
// Owned staged D3D11.0/C++03 primitive. No engine/file/thread/worker callback.
#include "../world-bank/WorldSourceBank.h"
#include "../fixed-geometry/FixedTerrainGeometry.h"
#include "../staged-material/StagedMaterial.h"
namespace WorldRaster {
typedef AtlasView::U64 U64;
typedef WorldBank::Ledger Ledger;
typedef FixedTerrainRaster::Bytecode Bytecode;
struct Shaders{Bytecode vertex,partial,tail;};
struct Grid{unsigned x,z,columns,rows;U64 sourceEpoch;};
struct Frame{WorldBank::SourceEpoch source;U64 materialGeneration,generation;unsigned region,cell,side;float x,z,spacing,cameraY,farClip,waterHeight,wetness;};
struct Material{WorldBank::SourceEpoch source;U64 generation;unsigned region;float parameters[900],distortion[60];};
inline U64 CarryBytes(unsigned side){return side==256||side==512?U64(side)*side*64:0;}
inline U64 TargetBytes(unsigned side){return side==256||side==512?U64(side)*side*20:0;}
inline U64 MeshBytes(unsigned patches){return patches&&patches<=16?U64(patches)*169000+17144+448+3856+16:0;}
inline U64 RequiredBytes(unsigned side,unsigned patches){return CarryBytes(side)&&MeshBytes(patches)?CarryBytes(side)+TargetBytes(side)+MeshBytes(patches):0;}
inline bool Fits(unsigned side,unsigned patches,const Ledger&l){return RequiredBytes(side,patches)&&l.carry>=CarryBytes(side)&&l.outputs>=TargetBytes(side)&&l.raster>=MeshBytes(patches)&&WorldBank::Fits(l);}
inline bool ValidGrid(const Grid&g){return g.sourceEpoch&&g.columns&&g.rows&&g.columns<=16&&g.rows<=16&&g.columns*g.rows<=16&&!(g.x&63)&&!(g.z&63)&&g.x<=16384-g.columns*64&&g.z<=16384-g.rows*64;}
inline bool ValidShaders(const Shaders&s){
 const unsigned char v[32]={219,209,56,56,69,15,166,131,66,107,243,223,146,114,143,192,195,219,223,43,121,140,49,198,193,0,77,10,62,62,210,56};
 const unsigned char p[32]={120,40,82,232,56,238,248,235,166,135,128,58,127,70,182,105,140,189,137,22,161,185,203,171,44,198,89,99,135,118,231,69};
 const unsigned char t[32]={193,204,60,65,198,192,254,144,228,210,253,115,66,151,106,13,186,37,210,152,80,210,230,187,246,31,80,120,105,16,156,159};
 return FixedTerrainRaster::Code(s.vertex,4440,v)&&FixedTerrainRaster::Code(s.partial,13208,p)&&FixedTerrainRaster::Code(s.tail,4272,t);
}
inline bool Bits(float a,float b){return !std::memcmp(&a,&b,4);}
inline bool SameFrame(const Frame&a,const Frame&b){return a.generation&&WorldBank::SameEpoch(a.source,b.source)&&a.materialGeneration==b.materialGeneration&&a.generation==b.generation&&a.region==b.region&&a.cell==b.cell&&a.side==b.side&&Bits(a.x,b.x)&&Bits(a.z,b.z)&&Bits(a.spacing,b.spacing)&&Bits(a.cameraY,b.cameraY)&&Bits(a.farClip,b.farClip)&&Bits(a.waterHeight,b.waterHeight)&&Bits(a.wetness,b.wetness);}
// Single scan for read aliases of all seven owned color resources. Reverse scope
// restoration follows GraphicsState/extra pixel restoration, as in saved guards.
class Reads{
 ID3D11DeviceContext*c_;ID3D11ShaderResourceView*v_[6][128];Reads(const Reads&);Reads&operator=(const Reads&);
 void Set(unsigned s,unsigned i,ID3D11ShaderResourceView*v){switch(s){case 0:c_->VSSetShaderResources(i,1,&v);break;case 1:c_->HSSetShaderResources(i,1,&v);break;case 2:c_->DSSetShaderResources(i,1,&v);break;case 3:c_->GSSetShaderResources(i,1,&v);break;case 4:c_->PSSetShaderResources(i,1,&v);break;default:c_->CSSetShaderResources(i,1,&v);}}
public:
 Reads(ID3D11DeviceContext*c,ID3D11Resource*const*owned):c_(c){c_->VSGetShaderResources(0,128,v_[0]);c_->HSGetShaderResources(0,128,v_[1]);c_->DSGetShaderResources(0,128,v_[2]);c_->GSGetShaderResources(0,128,v_[3]);c_->PSGetShaderResources(0,128,v_[4]);c_->CSGetShaderResources(0,128,v_[5]);for(unsigned s=0;s<6;++s)for(unsigned i=0;i<128;++i)if(v_[s][i]){AtlasView::Com<ID3D11Resource>r;v_[s][i]->GetResource(&r.p);bool alias=false;for(unsigned k=0;k<8;++k)alias=AtlasView::Identity(r.p,owned[k])||alias;if(alias)Set(s,i,NULL);else{v_[s][i]->Release();v_[s][i]=NULL;}}}
 ~Reads(){for(unsigned s=0;s<6;++s)for(unsigned i=0;i<128;++i)if(v_[s][i]){Set(s,i,v_[s][i]);v_[s][i]->Release();}}
};
class Raster{
 AtlasView::Com<ID3D11Device>device_;AtlasView::Com<ID3D11DeviceContext>context_;
 AtlasView::Com<ID3D11VertexShader>vs_;AtlasView::Com<ID3D11PixelShader>partial_,tail_;AtlasView::Com<ID3D11InputLayout>layout_;
 AtlasView::Com<ID3D11Buffer>vb_[16],ib_,vcb_,pcb_,slotcb_;AtlasView::Com<ID3D11Texture2D>tex_[8];AtlasView::Com<ID3D11ShaderResourceView>view_[8];AtlasView::Com<ID3D11RenderTargetView>rt_[8];AtlasView::Com<ID3D11DepthStencilView>depth_;
 AtlasView::Com<ID3D11RasterizerState>raster_;AtlasView::Com<ID3D11DepthStencilState>ds_;AtlasView::Com<ID3D11BlendState>blend_;AtlasView::Com<ID3D11SamplerState>sampler_[3];
 Grid grid_;unsigned side_,next_;float highest_;bool active_,ready_;const void*bankIdentity_;U64 bankToken_;Frame frame_;Material material_;WorldMaterial::Planner planner_;WorldMaterial::Plan plan_;WorldBank::Bindings lease_;
 Raster(const Raster&);Raster&operator=(const Raster&);
 static HRESULT Buffer(ID3D11Device*d,const void*data,unsigned bytes,unsigned bind,ID3D11Buffer**out){D3D11_BUFFER_DESC b={};b.ByteWidth=bytes;b.BindFlags=bind;b.Usage=data?D3D11_USAGE_IMMUTABLE:D3D11_USAGE_DEFAULT;D3D11_SUBRESOURCE_DATA s={};s.pSysMem=data;return d->CreateBuffer(&b,data?&s:NULL,out);}
 static void Put(unsigned char*b,unsigned at,const float*p,unsigned count){std::memcpy(b+at,p,count*4);}
 unsigned Patches()const{return grid_.columns*grid_.rows;}
 bool ValidFrame(const Frame&f,const Material&m)const{
  if(!device_.p||f.side!=side_||!WorldBank::ValidEpoch(f.source)||f.source.value!=grid_.sourceEpoch||!f.generation||!f.materialGeneration||f.materialGeneration!=m.generation||!WorldBank::SameEpoch(f.source,m.source)||f.region!=m.region||f.region>=64||f.cell>=16||!_finite(f.x)||!_finite(f.z)||!_finite(f.spacing)||!_finite(f.cameraY)||!_finite(f.farClip)||!_finite(f.waterHeight)||!_finite(f.wetness)||f.spacing<1||f.spacing>9||f.cameraY<=highest_+1||f.cameraY>20000||f.farClip<=0||f.farClip>1000000||f.waterHeight< -10000||f.waterHeight>10000||f.wetness<0||f.wetness>1)return false;
  for(unsigned i=0;i<900;++i)if(!_finite(m.parameters[i]))return false;for(unsigned i=0;i<60;++i)if(!_finite(m.distortion[i]))return false;
  const double half=double(side_)*f.spacing*.5,x0=-147456+int(grid_.x)*18,z0=-147456+int(grid_.z)*18,x1=x0+grid_.columns*1152,z1=z0+grid_.rows*1152;const SatelliteRegion::Region&r=SatelliteRegion::Data.regions[f.region];const int cx=r.worldX+int(f.cell%4)*9216,cz=r.worldZ+int(f.cell/4)*9216;
  return f.x-half>=x0&&f.x+half<=x1&&f.z-half>=z0&&f.z+half<=z1&&f.x-half>=cx&&f.x+half<=cx+9216&&f.z-half>=cz&&f.z+half<=cz+9216&&double(f.farClip)*f.farClip>double(f.cameraY)*f.cameraY+2*half*half;
 }
 bool NoHazards()const{if(!AtlasModels::NoStreamOutput(context_.p))return false;for(unsigned i=0;i<8;++i)if(AtlasView::Bound(context_.p,tex_[i].p,true))return false;return true;}
 HRESULT Acquire(WorldBank::Sources&bank,unsigned slot,const Ledger&l,WorldBank::Bindings&b){
  if(!device_.p||!Fits(side_,Patches(),l))return E_OUTOFMEMORY;if(!bank.SameDevice(device_.p)||bank.OwnedGpuBytes()!=WorldBank::SourceBytes||&bank!=bankIdentity_||bank.ResourceToken()!=bankToken_)return E_INVALIDARG;
  HRESULT hr=bank.AcquireBase(frame_.source,slot,l,b);if(FAILED(hr))return hr;if(b.region!=frame_.region||b.cell!=frame_.cell||b.row!=plan_.stage[slot].shaderRow||!bank.LeaseCurrent(b)||!NoHazards())return E_INVALIDARG;
  for(unsigned i=0;i<8;++i){if(!AtlasView::SameDevice(b.view[i],device_.p))return E_INVALIDARG;AtlasView::Com<ID3D11Resource>r;b.view[i]->GetResource(&r.p);for(unsigned k=0;k<8;++k)if(AtlasView::Identity(r.p,tex_[k].p))return E_INVALIDARG;}return S_OK;
 }
 void Pack(unsigned char(&vc)[448],unsigned char(&pc)[3856])const{
  const Frame&f=frame_;std::memset(vc,0,sizeof(vc));std::memset(pc,0,sizeof(pc));const float span=f.spacing*side_,matrix[16]={2/span,0,0,0,0,0,-1/f.farClip,0,0,-2/span,0,0,-2*f.x/span,2*f.z/span,f.cameraY/f.farClip,1},identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};const SatelliteRegion::Region&r=SatelliteRegion::Data.regions[f.region];const float bounds[4]={-147456,-147456,147456,147456},paint[4]={float(r.worldX),float(r.worldZ),0,0},viewport[4]={float(side_),float(side_),1.f/side_,1.f/side_},camera[3]={f.x,f.cameraY,f.z},selector[4]={float(r.worldX),float(r.worldZ),4,4};
  Put(vc,0,matrix,16);Put(vc,64,identity,16);Put(vc,144,material_.distortion,60);Put(vc,384,bounds,4);Put(vc,400,bounds,4);Put(pc,0,paint,4);Put(pc,16,viewport,4);Put(pc,32,&f.farClip,1);Put(pc,36,camera,3);Put(pc,48,&f.waterHeight,1);Put(pc,52,&f.wetness,1);Put(pc,240,material_.parameters,900);Put(pc,3840,selector,4);
 }
 HRESULT Pass(bool finish,unsigned slot,WorldBank::Sources&bank,const Ledger&l,WorldBank::Bindings&b){
  unsigned char vc[448],pc[3856];Pack(vc,pc);if(!finish&&slot<plan_.sidCount){HRESULT hr=bank.PatchRoles(frame_.source,slot,l,pc,sizeof(pc));if(FAILED(hr))return hr;}
  ID3D11Resource*owned[8];for(unsigned i=0;i<8;++i)owned[i]=tex_[i].p;Reads reads(context_.p,owned);StagedMaterial::Bindings extra(context_.p);FixedTerrainRaster::PixelState pixels(context_.p);AtlasModels::GraphicsState saved(context_.p);ID3D11DeviceContext*c=context_.p;c->SetPredication(NULL,FALSE);
  ID3D11UnorderedAccessView*empty[8]={};c->OMSetRenderTargetsAndUnorderedAccessViews(0,NULL,NULL,0,8,empty,NULL);ID3D11ShaderResourceView*none[2]={};c->PSSetShaderResources(8,2,none);ID3D11RenderTargetView*targets[3]={finish?rt_[0].p:rt_[4+(slot&1)*2].p,finish?rt_[1].p:rt_[5+(slot&1)*2].p,finish?rt_[2].p:NULL};c->OMSetRenderTargets(finish?3:2,targets,depth_.p);const float zero[4]={};for(unsigned i=0;i<(finish?3u:2u);++i)c->ClearRenderTargetView(targets[i],zero);c->ClearDepthStencilView(depth_.p,D3D11_CLEAR_DEPTH,1,0);
  ID3D11ShaderResourceView*previous[2]={finish?view_[6].p:slot?view_[4+((slot-1)&1)*2].p:NULL,finish?view_[7].p:slot?view_[5+((slot-1)&1)*2].p:NULL};c->PSSetShaderResources(8,2,previous);c->HSSetShader(NULL,NULL,0);c->DSSetShader(NULL,NULL,0);c->GSSetShader(NULL,NULL,0);c->VSSetShader(vs_.p,NULL,0);c->PSSetShader(finish?tail_.p:partial_.p,NULL,0);c->VSSetConstantBuffers(0,1,&vcb_.p);c->PSSetConstantBuffers(0,1,&pcb_.p);c->PSSetConstantBuffers(1,1,&slotcb_.p);c->PSSetShaderResources(0,8,b.view);for(unsigned i=0;i<3;++i){const unsigned at=i==0?0:i==1?2:4;c->PSSetSamplers(at,1,&sampler_[i].p);}c->RSSetState(raster_.p);D3D11_VIEWPORT viewport={0,0,float(side_),float(side_),0,1};c->RSSetViewports(1,&viewport);c->RSSetScissorRects(0,NULL);c->OMSetBlendState(blend_.p,NULL,~0u);c->OMSetDepthStencilState(ds_.p,0);c->IASetInputLayout(layout_.p);c->IASetIndexBuffer(ib_.p,DXGI_FORMAT_R16_UINT,0);c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
  const unsigned control[4]={slot,0,0,0};c->UpdateSubresource(vcb_.p,0,NULL,vc,0,0);c->UpdateSubresource(pcb_.p,0,NULL,pc,0,0);c->UpdateSubresource(slotcb_.p,0,NULL,control,0,0);const UINT stride=40,offset=0;for(unsigned i=0;i<Patches();++i){c->IASetVertexBuffers(0,1,&vb_[i].p,&stride,&offset);c->DrawIndexed(FixedTerrain::IndexCount,0,0);}HRESULT hr=device_.p->GetDeviceRemovedReason();return SUCCEEDED(hr)&&!bank.LeaseCurrent(b)?E_ABORT:hr;
 }
public:
 Raster():side_(0),next_(0),highest_(0),active_(false),ready_(false),bankIdentity_(NULL),bankToken_(0){std::memset(&grid_,0,sizeof(grid_));std::memset(&frame_,0,sizeof(frame_));std::memset(&material_,0,sizeof(material_));std::memset(&lease_,0,sizeof(lease_));}
 void Cancel(){active_=ready_=false;next_=0;bankIdentity_=NULL;bankToken_=0;std::memset(&lease_,0,sizeof(lease_));}
 void Clear(){Cancel();for(unsigned i=0;i<16;++i)vb_[i].Reset();ib_.Reset();vcb_.Reset();pcb_.Reset();slotcb_.Reset();for(unsigned i=0;i<8;++i){view_[i].Reset();rt_[i].Reset();tex_[i].Reset();}depth_.Reset();for(unsigned i=0;i<3;++i)sampler_[i].Reset();layout_.Reset();vs_.Reset();partial_.Reset();tail_.Reset();raster_.Reset();ds_.Reset();blend_.Reset();context_.Reset();device_.Reset();planner_.Clear();std::memset(&grid_,0,sizeof(grid_));side_=0;highest_=0;}
 U64 OwnedGpuBytes()const{return device_.p?RequiredBytes(side_,Patches()):0;}
 HRESULT Init(ID3D11Device*d,unsigned side,const Shaders&s,const FixedTerrain::Samples&source,const Grid&g,const Ledger&l){
  if(!d||device_.p||!ValidGrid(g)||!CarryBytes(side)||!ValidShaders(s))return E_INVALIDARG;if(!Fits(side,g.columns*g.rows,l))return E_OUTOFMEMORY;if(!FixedTerrain::Supports(source,int(g.x)-2,int(g.z)-2,g.x+g.columns*64+2,g.z+g.rows*64+2))return E_INVALIDARG;
  HRESULT hr=S_OK;try{
   hr=d->CreateVertexShader(s.vertex.data,s.vertex.size,NULL,&vs_.p);if(SUCCEEDED(hr))hr=d->CreatePixelShader(s.partial.data,s.partial.size,NULL,&partial_.p);if(SUCCEEDED(hr))hr=d->CreatePixelShader(s.tail.data,s.tail.size,NULL,&tail_.p);D3D11_INPUT_ELEMENT_DESC e[3]={{"POSITION",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,16,D3D11_INPUT_PER_VERTEX_DATA,0},{"BINORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,28,D3D11_INPUT_PER_VERTEX_DATA,0}};if(SUCCEEDED(hr))hr=d->CreateInputLayout(e,3,s.vertex.data,s.vertex.size,&layout_.p);
   FixedTerrain::Vertex vertices[FixedTerrain::Vertices];float highest=0;for(unsigned z=0;z<g.rows&&SUCCEEDED(hr);++z)for(unsigned x=0;x<g.columns&&SUCCEEDED(hr);++x){if(!FixedTerrain::Build(source,g.x+x*64,g.z+z*64,vertices,FixedTerrain::Vertices)){hr=E_INVALIDARG;break;}for(unsigned i=0;i<FixedTerrain::Vertices;++i)if(vertices[i].value[1]>highest)highest=vertices[i].value[1];hr=Buffer(d,vertices,sizeof(vertices),D3D11_BIND_VERTEX_BUFFER,&vb_[z*g.columns+x].p);}unsigned short indices[FixedTerrain::IndexCount];FixedTerrain::Indices(indices,FixedTerrain::IndexCount);if(SUCCEEDED(hr))hr=Buffer(d,indices,sizeof(indices),D3D11_BIND_INDEX_BUFFER,&ib_.p);if(SUCCEEDED(hr))hr=Buffer(d,NULL,448,D3D11_BIND_CONSTANT_BUFFER,&vcb_.p);if(SUCCEEDED(hr))hr=Buffer(d,NULL,3856,D3D11_BIND_CONSTANT_BUFFER,&pcb_.p);if(SUCCEEDED(hr))hr=Buffer(d,NULL,16,D3D11_BIND_CONSTANT_BUFFER,&slotcb_.p);
   for(unsigned i=0;i<8&&SUCCEEDED(hr);++i){D3D11_TEXTURE2D_DESC t={};t.Width=t.Height=side;t.MipLevels=t.ArraySize=t.SampleDesc.Count=1;t.Format=i==0?DXGI_FORMAT_R16G16B16A16_FLOAT:i==1?DXGI_FORMAT_R8G8B8A8_UNORM:i==2?DXGI_FORMAT_R32_FLOAT:i==3?DXGI_FORMAT_D32_FLOAT:DXGI_FORMAT_R32G32B32A32_FLOAT;t.BindFlags=i==3?D3D11_BIND_DEPTH_STENCIL:D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;hr=d->CreateTexture2D(&t,NULL,&tex_[i].p);if(i!=3&&SUCCEEDED(hr))hr=d->CreateShaderResourceView(tex_[i].p,NULL,&view_[i].p);if(i!=3&&SUCCEEDED(hr))hr=d->CreateRenderTargetView(tex_[i].p,NULL,&rt_[i].p);}if(SUCCEEDED(hr))hr=d->CreateDepthStencilView(tex_[3].p,NULL,&depth_.p);
   D3D11_RASTERIZER_DESC rd={};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_NONE;rd.DepthClipEnable=TRUE;if(SUCCEEDED(hr))hr=d->CreateRasterizerState(&rd,&raster_.p);D3D11_DEPTH_STENCIL_DESC dd={};dd.DepthEnable=TRUE;dd.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;dd.DepthFunc=D3D11_COMPARISON_LESS_EQUAL;if(SUCCEEDED(hr))hr=d->CreateDepthStencilState(&dd,&ds_.p);D3D11_BLEND_DESC bd={};for(unsigned i=0;i<8;++i)bd.RenderTarget[i].RenderTargetWriteMask=15;if(SUCCEEDED(hr))hr=d->CreateBlendState(&bd,&blend_.p);
   for(unsigned i=0;i<3&&SUCCEEDED(hr);++i){D3D11_SAMPLER_DESC sd={};sd.Filter=i==0?D3D11_FILTER_ANISOTROPIC:i==1?D3D11_FILTER_MIN_MAG_MIP_LINEAR:D3D11_FILTER_MIN_MAG_MIP_POINT;sd.AddressU=sd.AddressV=sd.AddressW=i==2?D3D11_TEXTURE_ADDRESS_CLAMP:D3D11_TEXTURE_ADDRESS_WRAP;sd.MaxAnisotropy=16;sd.ComparisonFunc=D3D11_COMPARISON_NEVER;sd.MaxLOD=D3D11_FLOAT32_MAX;hr=d->CreateSamplerState(&sd,&sampler_[i].p);}if(SUCCEEDED(hr))hr=d->GetDeviceRemovedReason();if(SUCCEEDED(hr)&&!planner_.Init(g.sourceEpoch))hr=E_INVALIDARG;if(SUCCEEDED(hr)){d->GetImmediateContext(&context_.p);if(!context_.p)hr=E_FAIL;}if(SUCCEEDED(hr)){device_.p=d;d->AddRef();grid_=g;side_=side;highest_=highest;}
  }catch(...){hr=E_FAIL;}if(FAILED(hr))Clear();return hr;
 }
 HRESULT BeginFrame(bool enabled,const Frame&f,const WorldMaterial::Plan&p,const Material&m,WorldBank::Sources&bank,const Ledger&l){
  if(!enabled)return S_FALSE;Cancel();try{if(!ValidFrame(f,m))return E_INVALIDARG;if(!Fits(side_,Patches(),l))return E_OUTOFMEMORY;WorldMaterial::Plan expected;if(!planner_.Build(f.region,f.cell,f.source.value,0,expected)||!WorldBank::EqualPlan(p,expected)||!bank.SameDevice(device_.p)||!bank.ResourceToken())return E_INVALIDARG;frame_=f;material_=m;plan_=expected;bankIdentity_=&bank;bankToken_=bank.ResourceToken();WorldBank::Bindings b={};HRESULT hr=Acquire(bank,0,l,b);if(FAILED(hr)){Cancel();return hr;}lease_=b;active_=true;return S_OK;}catch(...){Cancel();return E_FAIL;}
 }
 HRESULT RenderStage(unsigned slot,WorldBank::Sources&bank,const Ledger&l){
  if(!active_||slot!=next_||slot>=4){Cancel();return E_INVALIDARG;}try{const unsigned actual=slot<plan_.sidCount?slot:plan_.sidCount-1;WorldBank::Bindings b={};HRESULT hr=Acquire(bank,actual,l,b);if(SUCCEEDED(hr)&&((slot==0||slot>=plan_.sidCount)?b.revision!=lease_.revision:b.revision<=lease_.revision))hr=E_ABORT;if(SUCCEEDED(hr))hr=Pass(false,slot,bank,l,b);if(FAILED(hr)){Cancel();return hr;}lease_=b;++next_;return S_OK;}catch(...){Cancel();return E_FAIL;}
 }
 HRESULT Finish(WorldBank::Sources&bank,const Ledger&l){
  if(!active_||next_!=4){Cancel();return E_INVALIDARG;}try{WorldBank::Bindings b={};HRESULT hr=Acquire(bank,plan_.sidCount-1,l,b);if(SUCCEEDED(hr)&&b.revision!=lease_.revision)hr=E_ABORT;if(SUCCEEDED(hr))hr=Pass(true,4,bank,l,b);if(FAILED(hr)){Cancel();return hr;}active_=false;ready_=true;return S_OK;}catch(...){Cancel();return E_FAIL;}
 }
 bool Ready(const Frame&current)const{return ready_&&SameFrame(frame_,current)&&device_.p&&SUCCEEDED(device_.p->GetDeviceRemovedReason());}
 ID3D11ShaderResourceView*Output(const Frame&current,unsigned plane)const{return plane<3&&Ready(current)?view_[plane].p:NULL;}
 // Allocation-only diagnostics; carry is never a completed image publication.
 ID3D11Texture2D*CarryTexture(unsigned set,unsigned plane)const{return device_.p&&set<2&&plane<2?tex_[4+set*2+plane].p:NULL;}
 ID3D11ShaderResourceView*CarryView(unsigned set,unsigned plane)const{return device_.p&&set<2&&plane<2?view_[4+set*2+plane].p:NULL;}
 ID3D11Texture2D*Texture(unsigned plane)const{return device_.p&&plane<3?tex_[plane].p:NULL;}
};
}
