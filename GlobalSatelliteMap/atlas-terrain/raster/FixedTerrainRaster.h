#pragma once
// C++03 / D3D11.0. Owned pass primitive; no native/Ogre/file calls.
#include "../fixed-geometry/FixedTerrainGeometry.h"
#include "../compact-material/CompactMaterial.h"
#include "../../atlas-models/GraphicsState.h"
namespace FixedTerrainRaster {
typedef AtlasView::U64 U64;
enum{Side=256,MaxPatches=16,VertexBytes=448,PixelBytes=3856};
struct Bytecode{const void*data;size_t size;};
struct Shaders{Bytecode vertex,pixel;};
struct Grid{unsigned x,z,columns,rows,epoch;};
struct Frame{unsigned epoch;float x,z,spacing,cameraY,farClip,waterHeight,wetness;};
inline bool Code(const Bytecode&b,unsigned bytes,const unsigned char(&pin)[32]){unsigned char h[32];return b.data&&b.size==bytes&&AtlasView::Digest(b.data,bytes,h)&&!std::memcmp(h,pin,32);}
inline bool ValidShaders(const Shaders&s){
 const unsigned char v[32]={219,209,56,56,69,15,166,131,66,107,243,223,146,114,143,192,195,219,223,43,121,140,49,198,193,0,77,10,62,62,210,56};
 const unsigned char p[32]={75,59,113,204,185,199,91,121,121,27,156,243,190,245,248,88,186,156,83,21,26,156,252,13,101,205,46,125,17,63,222,190};
 return Code(s.vertex,4440,v)&&Code(s.pixel,37520,p);
}
inline bool ValidGrid(const Grid&g){return g.epoch&&g.columns&&g.rows&&g.columns<=MaxPatches&&g.rows<=MaxPatches&&g.columns*g.rows<=MaxPatches&&!(g.x&63)&&!(g.z&63)&&g.x<=16384-g.columns*64&&g.z<=16384-g.rows*64;}
// GraphicsState saves PS t0/s0. These are precisely the additional slots written.
class PixelState{
 ID3D11DeviceContext*c_;ID3D11ShaderResourceView*views_[7];ID3D11SamplerState*samplers_[2];PixelState(const PixelState&);PixelState&operator=(const PixelState&);
public:explicit PixelState(ID3D11DeviceContext*c):c_(c){c_->PSGetShaderResources(1,7,views_);c_->PSGetSamplers(2,1,&samplers_[0]);c_->PSGetSamplers(4,1,&samplers_[1]);}
 ~PixelState(){c_->PSSetShaderResources(1,7,views_);c_->PSSetSamplers(2,1,&samplers_[0]);c_->PSSetSamplers(4,1,&samplers_[1]);for(unsigned i=0;i<7;++i)if(views_[i])views_[i]->Release();for(unsigned i=0;i<2;++i)if(samplers_[i])samplers_[i]->Release();}
};
class Raster{
 AtlasView::Com<ID3D11Device>device_;AtlasView::Com<ID3D11VertexShader>vs_;AtlasView::Com<ID3D11PixelShader>ps_;AtlasView::Com<ID3D11InputLayout>layout_;
 AtlasView::Com<ID3D11Buffer>vb_[MaxPatches],ib_,vcb_,pcb_;AtlasView::Com<ID3D11Texture2D>tex_[3],depth_;AtlasView::Com<ID3D11ShaderResourceView>views_[3];AtlasView::Com<ID3D11RenderTargetView>rt_[3];AtlasView::Com<ID3D11DepthStencilView>dsv_;
 AtlasView::Com<ID3D11RasterizerState>raster_;AtlasView::Com<ID3D11DepthStencilState>ds_;AtlasView::Com<ID3D11BlendState>blend_;AtlasView::Com<ID3D11SamplerState>sampler_[3];Grid grid_;float maxHeight_;
 Raster(const Raster&);Raster&operator=(const Raster&);
 static HRESULT Buffer(ID3D11Device*d,const void*data,unsigned bytes,unsigned bind,ID3D11Buffer**out){D3D11_BUFFER_DESC b={};b.ByteWidth=bytes;b.BindFlags=bind;b.Usage=data?D3D11_USAGE_IMMUTABLE:D3D11_USAGE_DEFAULT;D3D11_SUBRESOURCE_DATA s={};s.pSysMem=data;return d->CreateBuffer(&b,data?&s:NULL,out);}
 static void Put(unsigned char*b,unsigned at,const float*p,unsigned count){std::memcpy(b+at,p,count*sizeof(float));}
 bool ValidFrame(const Frame&f)const{
  if(!device_.p||!f.epoch||f.epoch!=grid_.epoch||!_finite(f.x)||!_finite(f.z)||!_finite(f.spacing)||!_finite(f.cameraY)||!_finite(f.farClip)||!_finite(f.waterHeight)||!_finite(f.wetness)||f.spacing<1||f.spacing>9||f.cameraY<=maxHeight_+1||f.cameraY>20000||f.farClip<=0||f.farClip>1000000||f.wetness<0||f.wetness>1||f.waterHeight< -10000||f.waterHeight>9800)return false;
  const double half=double(f.spacing)*128,x0=-147456+int(grid_.x)*18,z0=-147456+int(grid_.z)*18,x1=x0+grid_.columns*1152,z1=z0+grid_.rows*1152;
  // Fixed compact selector cell10 is [-18432,-9216] on both axes.
  if(double(f.x)-half<x0||double(f.x)+half>x1||double(f.z)-half<z0||double(f.z)+half>z1||double(f.x)-half< -18432||double(f.x)+half> -9216||double(f.z)-half< -18432||double(f.z)+half> -9216)return false;
  // Native radial depth must remain strictly below one even at lowest source0.
  return double(f.farClip)*f.farClip>double(f.cameraY)*f.cameraY+2*half*half;
 }
 static void Pack(const Frame&f,unsigned char(&vc)[VertexBytes],unsigned char(&pc)[PixelBytes]){
  std::memset(vc,0,sizeof(vc));std::memset(pc,0,sizeof(pc));float span=f.spacing*Side;
  const float matrix[16]={2/span,0,0,0,0,0,-1/f.farClip,0,0,-2/span,0,0,-2*f.x/span,2*f.z/span,f.cameraY/f.farClip,1},identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
  const float bound[4]={-147456,-147456,147456,147456},paint[4]={-36864,-36864,0,0},viewport[4]={256,256,1.f/256,1.f/256},camera[3]={f.x,f.cameraY,f.z},selector[4]={-36864,-36864,4,4};
  Put(vc,0,matrix,16);Put(vc,64,identity,16);Put(vc,144,CompactMaterial::PinnedDistortion,60);Put(vc,384,bound,4);Put(vc,400,bound,4);
  Put(pc,0,paint,4);Put(pc,16,viewport,4);Put(pc,32,&f.farClip,1);Put(pc,36,camera,3);Put(pc,48,&f.waterHeight,1);Put(pc,52,&f.wetness,1);Put(pc,240,CompactMaterial::PinnedParameters,900);Put(pc,3840,selector,4);
 }
public:
 Raster():maxHeight_(0){std::memset(&grid_,0,sizeof(grid_));}
 void Clear(){for(unsigned i=0;i<MaxPatches;++i)vb_[i].Reset();ib_.Reset();vcb_.Reset();pcb_.Reset();for(unsigned i=0;i<3;++i){views_[i].Reset();rt_[i].Reset();tex_[i].Reset();sampler_[i].Reset();}dsv_.Reset();depth_.Reset();layout_.Reset();vs_.Reset();ps_.Reset();raster_.Reset();ds_.Reset();blend_.Reset();device_.Reset();std::memset(&grid_,0,sizeof(grid_));maxHeight_=0;}
 static U64 RequiredBytes(unsigned patches){return patches&&patches<=MaxPatches?U64(patches)*FixedTerrain::Vertices*40+FixedTerrain::IndexCount*2+1048576+VertexBytes+PixelBytes:0;}
 static bool Fits(U64 other,unsigned patches){const U64 own=RequiredBytes(patches);return own&&other>=CompactMaterial::SourceBytes&&own<=CompactMaterial::Limit&&other<=CompactMaterial::Limit-own&&other<=AtlasView::GpuCap-own;}
 U64 OwnedGpuBytes()const{return device_.p?RequiredBytes(grid_.columns*grid_.rows):0;}
 HRESULT Init(ID3D11Device*d,const Shaders&s,const FixedTerrain::Samples&source,const Grid&g,U64 other){
  if(!d||device_.p||!ValidGrid(g)||!ValidShaders(s))return E_INVALIDARG;if(!Fits(other,g.columns*g.rows))return E_OUTOFMEMORY;
  // Entire rectangle plus normal halo is admitted before creating any owner.
  if(!FixedTerrain::Supports(source,int(g.x)-2,int(g.z)-2,g.x+g.columns*64+2,g.z+g.rows*64+2))return E_INVALIDARG;
  HRESULT hr=d->CreateVertexShader(s.vertex.data,s.vertex.size,NULL,&vs_.p);if(SUCCEEDED(hr))hr=d->CreatePixelShader(s.pixel.data,s.pixel.size,NULL,&ps_.p);
  D3D11_INPUT_ELEMENT_DESC e[3]={{"POSITION",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,16,D3D11_INPUT_PER_VERTEX_DATA,0},{"BINORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,28,D3D11_INPUT_PER_VERTEX_DATA,0}};
  if(SUCCEEDED(hr))hr=d->CreateInputLayout(e,3,s.vertex.data,s.vertex.size,&layout_.p);
  FixedTerrain::Vertex vertices[FixedTerrain::Vertices];float highest=0;
  for(unsigned z=0;z<g.rows&&SUCCEEDED(hr);++z)for(unsigned x=0;x<g.columns&&SUCCEEDED(hr);++x){if(!FixedTerrain::Build(source,g.x+x*64,g.z+z*64,vertices,FixedTerrain::Vertices)){hr=E_INVALIDARG;break;}for(unsigned i=0;i<FixedTerrain::Vertices;++i)if(vertices[i].value[1]>highest)highest=vertices[i].value[1];hr=Buffer(d,vertices,sizeof(vertices),D3D11_BIND_VERTEX_BUFFER,&vb_[z*g.columns+x].p);}
  unsigned short indices[FixedTerrain::IndexCount];FixedTerrain::Indices(indices,FixedTerrain::IndexCount);if(SUCCEEDED(hr))hr=Buffer(d,indices,sizeof(indices),D3D11_BIND_INDEX_BUFFER,&ib_.p);if(SUCCEEDED(hr))hr=Buffer(d,NULL,VertexBytes,D3D11_BIND_CONSTANT_BUFFER,&vcb_.p);if(SUCCEEDED(hr))hr=Buffer(d,NULL,PixelBytes,D3D11_BIND_CONSTANT_BUFFER,&pcb_.p);
  for(unsigned i=0;i<4&&SUCCEEDED(hr);++i){D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=Side;td.MipLevels=td.ArraySize=td.SampleDesc.Count=1;td.Format=i==3?DXGI_FORMAT_D32_FLOAT:i==2?DXGI_FORMAT_R32_FLOAT:DXGI_FORMAT_R8G8B8A8_UNORM;td.BindFlags=i==3?D3D11_BIND_DEPTH_STENCIL:D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;hr=d->CreateTexture2D(&td,NULL,i==3?&depth_.p:&tex_[i].p);if(i<3&&SUCCEEDED(hr))hr=d->CreateRenderTargetView(tex_[i].p,NULL,&rt_[i].p);if(i<3&&SUCCEEDED(hr))hr=d->CreateShaderResourceView(tex_[i].p,NULL,&views_[i].p);}
  if(SUCCEEDED(hr))hr=d->CreateDepthStencilView(depth_.p,NULL,&dsv_.p);D3D11_RASTERIZER_DESC rd={};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_NONE;rd.DepthClipEnable=TRUE;if(SUCCEEDED(hr))hr=d->CreateRasterizerState(&rd,&raster_.p);
  D3D11_DEPTH_STENCIL_DESC dd={};dd.DepthEnable=TRUE;dd.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;dd.DepthFunc=D3D11_COMPARISON_LESS_EQUAL;if(SUCCEEDED(hr))hr=d->CreateDepthStencilState(&dd,&ds_.p);D3D11_BLEND_DESC bd={};for(unsigned i=0;i<8;++i)bd.RenderTarget[i].RenderTargetWriteMask=15;if(SUCCEEDED(hr))hr=d->CreateBlendState(&bd,&blend_.p);
  for(unsigned i=0;i<3&&SUCCEEDED(hr);++i){D3D11_SAMPLER_DESC sd={};sd.Filter=i==0?D3D11_FILTER_ANISOTROPIC:i==1?D3D11_FILTER_MIN_MAG_MIP_LINEAR:D3D11_FILTER_MIN_MAG_MIP_POINT;sd.AddressU=sd.AddressV=sd.AddressW=i==2?D3D11_TEXTURE_ADDRESS_CLAMP:D3D11_TEXTURE_ADDRESS_WRAP;sd.MaxAnisotropy=16;sd.ComparisonFunc=D3D11_COMPARISON_NEVER;sd.MaxLOD=D3D11_FLOAT32_MAX;hr=d->CreateSamplerState(&sd,&sampler_[i].p);}
  if(SUCCEEDED(hr))hr=d->GetDeviceRemovedReason();if(FAILED(hr)){Clear();return hr;}grid_=g;maxHeight_=highest;device_.p=d;d->AddRef();return S_OK;
 }
 HRESULT Render(bool enabled,const CompactMaterial::Inputs&sources,const Frame&f,U64 other){
  if(!enabled)return S_FALSE;if(!ValidFrame(f)||!sources.Ready()||sources.OwnedGpuBytes()!=CompactMaterial::SourceBytes)return E_INVALIDARG;if(!Fits(other,grid_.columns*grid_.rows))return E_OUTOFMEMORY;
  AtlasView::Com<ID3D11DeviceContext>c;device_.p->GetImmediateContext(&c.p);if(!c.p||!AtlasModels::NoStreamOutput(c.p))return E_INVALIDARG;
  for(unsigned i=0;i<3;++i)if(AtlasView::Bound(c.p,tex_[i].p,true))return E_INVALIDARG;if(AtlasView::Bound(c.p,depth_.p,true))return E_INVALIDARG;
  ID3D11ShaderResourceView*input[8];for(unsigned i=0;i<8;++i){input[i]=sources.View(i);if(!AtlasView::SameDevice(input[i],device_.p))return E_INVALIDARG;AtlasView::Com<ID3D11Resource>r;input[i]->GetResource(&r.p);for(unsigned k=0;k<3;++k)if(AtlasView::Identity(r.p,tex_[k].p))return E_INVALIDARG;if(AtlasView::Identity(r.p,depth_.p)||AtlasView::Bound(c.p,r.p,true))return E_INVALIDARG;}
  unsigned char vc[VertexBytes],pc[PixelBytes];Pack(f,vc,pc);
  AtlasView::SavedOutputReads reads0(c.p,tex_[0].p),reads1(c.p,tex_[1].p),reads2(c.p,tex_[2].p);PixelState extra(c.p);AtlasModels::GraphicsState saved(c.p);c.p->SetPredication(NULL,FALSE);
  ID3D11UnorderedAccessView*empty[8]={};c.p->OMSetRenderTargetsAndUnorderedAccessViews(0,NULL,NULL,0,8,empty,NULL);ID3D11RenderTargetView*targets[3]={rt_[0].p,rt_[1].p,rt_[2].p};c.p->OMSetRenderTargets(3,targets,dsv_.p);const float zero[4]={};for(unsigned i=0;i<3;++i)c.p->ClearRenderTargetView(targets[i],zero);c.p->ClearDepthStencilView(dsv_.p,D3D11_CLEAR_DEPTH,1,0);
  c.p->HSSetShader(NULL,NULL,0);c.p->DSSetShader(NULL,NULL,0);c.p->GSSetShader(NULL,NULL,0);c.p->VSSetShader(vs_.p,NULL,0);c.p->PSSetShader(ps_.p,NULL,0);c.p->VSSetConstantBuffers(0,1,&vcb_.p);c.p->PSSetConstantBuffers(0,1,&pcb_.p);c.p->PSSetShaderResources(0,8,input);c.p->PSSetSamplers(0,1,&sampler_[0].p);c.p->PSSetSamplers(2,1,&sampler_[1].p);c.p->PSSetSamplers(4,1,&sampler_[2].p);
  c.p->RSSetState(raster_.p);D3D11_VIEWPORT viewport={0,0,256,256,0,1};c.p->RSSetViewports(1,&viewport);c.p->RSSetScissorRects(0,NULL);c.p->OMSetBlendState(blend_.p,NULL,~0u);c.p->OMSetDepthStencilState(ds_.p,0);c.p->IASetInputLayout(layout_.p);c.p->IASetIndexBuffer(ib_.p,DXGI_FORMAT_R16_UINT,0);c.p->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
  c.p->UpdateSubresource(vcb_.p,0,NULL,vc,0,0);c.p->UpdateSubresource(pcb_.p,0,NULL,pc,0,0);const UINT stride=40,offset=0;for(unsigned i=0;i<grid_.columns*grid_.rows;++i){c.p->IASetVertexBuffers(0,1,&vb_[i].p,&stride,&offset);c.p->DrawIndexed(FixedTerrain::IndexCount,0,0);}
  return device_.p->GetDeviceRemovedReason();
 }
 ID3D11Texture2D*Texture(unsigned i)const{return i<3?tex_[i].p:NULL;}
 ID3D11ShaderResourceView*Output(unsigned i)const{return i<3?views_[i].p:NULL;}
};
}
