#pragma once
#include "InstancePolicy.h"
#include "../GraphicsState.h"
namespace LoadedInstances {
struct Bytecode {const void*data;SIZE_T size;Bytecode(const void*p=NULL,SIZE_T n=0):data(p),size(n){}};
struct ShaderSet {Bytecode vertex[2],pixel[2];bool Valid()const{return vertex[0].data&&vertex[0].size&&vertex[1].data&&vertex[1].size&&pixel[0].data&&pixel[0].size&&pixel[1].data&&pixel[1].size;}};
struct RawDraw {
 Shape shape;Surface surface;float rawWorld[16],localBounds[6];ID3D11Buffer*vertex,*index;ID3D11ShaderResourceView*texture[7];ID3D11SamplerState*sampler;
 Kind kind;unsigned family,batch,cullMode;bool verifiedBlack;
 RawDraw():vertex(NULL),index(NULL),sampler(NULL),kind(Dual43),family(0),batch(0),cullMode(2),verifiedBlack(false){std::memset(rawWorld,0,64);std::memset(localBounds,0,24);std::memset(texture,0,sizeof(texture));}
};
inline bool TextureShape(ID3D11ShaderResourceView*v,ID3D11Device*d,D3D11_TEXTURE2D_DESC&td){
 if(!v||!AtlasView::SameDevice(v,d))return false;D3D11_SHADER_RESOURCE_VIEW_DESC sd;v->GetDesc(&sd);AtlasView::Com<ID3D11Resource>r;v->GetResource(&r.p);AtlasView::Com<ID3D11Texture2D>t;
 if(!r.p||FAILED(r.p->QueryInterface(__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&t.p))))return false;t.p->GetDesc(&td);
 unsigned maxMips=1;for(unsigned side=(std::max)(td.Width,td.Height);side>1;side/=2)++maxMips;
 return td.Width&&td.Height&&td.Width<=8192&&td.Height<=8192&&td.MipLevels&&td.MipLevels<=maxMips&&td.ArraySize==1&&td.SampleDesc.Count==1&&!td.SampleDesc.Quality&&(td.Usage==D3D11_USAGE_DEFAULT||td.Usage==D3D11_USAGE_IMMUTABLE)&&!td.CPUAccessFlags&&td.BindFlags&D3D11_BIND_SHADER_RESOURCE&&!(td.MiscFlags&~D3D11_RESOURCE_MISC_GENERATE_MIPS)&&sd.ViewDimension==D3D11_SRV_DIMENSION_TEXTURE2D&&sd.Format==td.Format&&!sd.Texture2D.MostDetailedMip&&sd.Texture2D.MipLevels==td.MipLevels&&TextureBytes(td)!=0;
}
class Packet {
 struct Draw {Shape shape;Surface surface;float rawWorld[16],localBounds[6];AtlasView::Com<ID3D11Buffer>vertex,index;AtlasView::Com<ID3D11ShaderResourceView>texture[7];AtlasView::Com<ID3D11SamplerState>sampler;Kind kind;unsigned cullMode;};
 typedef char PacketCpuBound[sizeof(Draw)*MaxRecords<=256*1024?1:-1];
 Draw*draw_;unsigned count_,generation_;float origin_[3];U64 buffers_,textures_;Packet(const Packet&);Packet&operator=(const Packet&);friend class Raster;
public:Packet():draw_(NULL),count_(0),generation_(0),buffers_(0),textures_(0){std::memset(origin_,0,12);}~Packet(){Clear();}
 void Clear(){delete[]draw_;draw_=NULL;count_=generation_=0;buffers_=textures_=0;std::memset(origin_,0,12);}
 unsigned Count()const{return count_;}unsigned Generation()const{return generation_;}const float*SourceSceneOrigin()const{return origin_;}U64 BorrowedBuffers()const{return buffers_;}U64 BorrowedTextures()const{return textures_;}
 const float*RawSceneWorld(unsigned i)const{return i<count_?draw_[i].rawWorld:NULL;}const float*LocalBounds(unsigned i)const{return i<count_?draw_[i].localBounds:NULL;}
 bool SameContents(const Packet&other)const{
  if(count_!=other.count_||generation_!=other.generation_||std::memcmp(origin_,other.origin_,12))return false;
  for(unsigned i=0;i<count_;++i){const Draw&a=draw_[i],&b=other.draw_[i];const Shape&x=a.shape,&y=b.shape;
   if(a.kind!=b.kind||a.cullMode!=b.cullMode||a.vertex.p!=b.vertex.p||a.index.p!=b.index.p||x.layout!=y.layout||x.vertexStart!=y.vertexStart||x.vertexCount!=y.vertexCount||x.vertexBytes!=y.vertexBytes||x.vertexOffset!=y.vertexOffset||x.indexStart!=y.indexStart||x.indexCount!=y.indexCount||x.indexBytes!=y.indexBytes||x.indexFormat!=y.indexFormat)return false;
   if(std::memcmp(&a.surface,&b.surface,sizeof(Surface))||std::memcmp(a.rawWorld,b.rawWorld,64)||std::memcmp(a.localBounds,b.localBounds,24))return false;
   for(unsigned k=0;k<TextureCount(a.kind);++k)if(a.texture[k].p!=b.texture[k].p)return false;
   D3D11_SAMPLER_DESC sa={},sb={};a.sampler.p->GetDesc(&sa);b.sampler.p->GetDesc(&sb);if(std::memcmp(&sa,&sb,sizeof(sa)))return false;
  }return true;
 }
 bool RequiresNativeLighting(unsigned i)const{return i<count_&&LoadedInstances::RequiresNativeLighting(draw_[i].kind);}bool CanDecode()const{if(!count_)return false;for(unsigned i=0;i<count_;++i)if(RequiresNativeLighting(i))return false;return true;}
 HRESULT Adopt(ID3D11Device*d,const RawDraw*raw,unsigned count,const float*origin,unsigned generation,U64 otherBuffers=0,U64 otherTextures=0,const BorrowLimits&limits=BorrowLimits(),const char**reason=NULL){
  if(reason)*reason="instance COM borrowed-resource validation";
  if(!d||!raw||!count||count>MaxRecords||draw_||!origin||!generation||!AtlasModels::Finite(origin,3)||!limits.Valid()||otherBuffers>limits.totalBuffers||otherTextures>limits.totalTextures)return E_INVALIDARG;
  ID3D11Buffer*unique[MaxBatches*2]={0};ID3D11Resource*images[MaxMaterials*7]={0};unsigned ub=0,ut=0,materialMask=0;U64 batchMask=0,bbytes=0,tbytes=0,vertices=0,indices=0;
  // Scoped resource refs make failure paths and canonical resource keys safe.
  AtlasView::Com<ID3D11Resource>held[MaxMaterials*7];
  for(unsigned n=0;n<count;++n){const RawDraw&r=raw[n];if(!ValidShape(r.shape,r.kind)||!Basis(r.rawWorld)||!r.verifiedBlack||r.family>=5||r.batch>=MaxBatches||r.kind!=(r.family<2?Dual43:Alpha48)||!r.vertex||!r.index||!r.sampler||r.cullMode<1||r.cullMode>3||!AtlasModels::Finite(reinterpret_cast<const float*>(&r.surface),13)||(r.kind==Alpha48&&r.surface.threshold>=1))return E_INVALIDARG;
   if(!AtlasModels::Finite(r.localBounds,6))return E_INVALIDARG;for(unsigned k=0;k<3;++k)if(r.localBounds[k]>r.localBounds[k+3])return E_INVALIDARG;
   materialMask|=1u<<r.family;batchMask|=U64(1)<<r.batch;vertices+=r.shape.vertexCount;indices+=r.shape.indexCount;if(vertices>MaxTotalVertices||indices>MaxTotalIndices)return E_INVALIDARG;
   ID3D11Buffer*buf[2]={r.vertex,r.index};for(unsigned i=0;i<2;++i){if(!AtlasView::SameDevice(buf[i],d))return E_INVALIDARG;D3D11_BUFFER_DESC bd;buf[i]->GetDesc(&bd);if(bd.ByteWidth!=(i?r.shape.indexBytes:r.shape.vertexBytes)||bd.Usage==D3D11_USAGE_STAGING||!(bd.BindFlags&(i?D3D11_BIND_INDEX_BUFFER:D3D11_BIND_VERTEX_BUFFER)))return E_INVALIDARG;bool seen=false;for(unsigned j=0;j<ub;++j)if(unique[j]==buf[i])seen=true;if(!seen){if(ub==MaxBatches*2)return E_INVALIDARG;unique[ub++]=buf[i];bbytes+=bd.ByteWidth;const char*why=BorrowRefusal(bbytes,otherBuffers,limits.freshBuffers,limits.totalBuffers,false);if(why){if(reason)*reason=why;return E_OUTOFMEMORY;}}}
   if(!AtlasView::SameDevice(r.sampler,d))return E_INVALIDARG;D3D11_SAMPLER_DESC sampler;r.sampler->GetDesc(&sampler);if(sampler.ComparisonFunc!=D3D11_COMPARISON_NEVER||!_finite(sampler.MipLODBias)||!_finite(sampler.MinLOD)||!_finite(sampler.MaxLOD)||!ZeroMetalSampler(sampler))return E_INVALIDARG;
   for(unsigned i=0;i<7;++i){if(i>=TextureCount(r.kind)){if(r.texture[i])return E_INVALIDARG;continue;}D3D11_TEXTURE2D_DESC td;if(!TextureShape(r.texture[i],d,td))return E_INVALIDARG;AtlasView::Com<ID3D11Resource>resource;r.texture[i]->GetResource(&resource.p);bool seen=false;for(unsigned j=0;j<ut;++j)if(AtlasView::Identity(images[j],resource.p))seen=true;if(!seen){if(ut==MaxMaterials*7)return E_INVALIDARG;images[ut]=resource.p;held[ut++].p=resource.Detach();tbytes+=TextureBytes(td);const char*why=BorrowRefusal(tbytes,otherTextures,limits.freshTextures,limits.totalTextures,true);if(why){if(reason)*reason=why;return E_OUTOFMEMORY;}}}
  }
  Draw*out=NULL;try{out=new Draw[count];}catch(...){return E_OUTOFMEMORY;}
  for(unsigned n=0;n<count;++n){const RawDraw&r=raw[n];Draw&o=out[n];o.shape=r.shape;o.surface=r.surface;std::memcpy(o.rawWorld,r.rawWorld,64);std::memcpy(o.localBounds,r.localBounds,24);o.kind=r.kind;o.cullMode=r.cullMode;o.vertex.p=r.vertex;r.vertex->AddRef();o.index.p=r.index;r.index->AddRef();for(unsigned k=0;k<TextureCount(r.kind);++k){o.texture[k].p=r.texture[k];r.texture[k]->AddRef();}o.sampler.p=r.sampler;r.sampler->AddRef();}
  draw_=out;count_=count;generation_=generation;buffers_=bbytes;textures_=tbytes;std::memcpy(origin_,origin,12);return S_OK;
 }
};
class PixelReads {ID3D11DeviceContext*c_;ID3D11ShaderResourceView*old_[6];PixelReads(const PixelReads&);PixelReads&operator=(const PixelReads&);public:explicit PixelReads(ID3D11DeviceContext*c):c_(c){c_->PSGetShaderResources(1,6,old_);}~PixelReads(){c_->PSSetShaderResources(1,6,old_);for(unsigned i=0;i<6;++i)if(old_[i])old_[i]->Release();}};
class Raster {
 AtlasView::Com<ID3D11Device>device_;AtlasView::Com<ID3D11VertexShader>vs_[2];AtlasView::Com<ID3D11PixelShader>ps_[2];AtlasView::Com<ID3D11InputLayout>layout_[3];AtlasView::Com<ID3D11RasterizerState>raster_[3];AtlasView::Com<ID3D11Buffer>vcb_,pcb_,instance_;AtlasView::Com<ID3D11Texture2D>tex_[3],depth_;AtlasView::Com<ID3D11ShaderResourceView>view_[3];AtlasView::Com<ID3D11RenderTargetView>rt_[3];AtlasView::Com<ID3D11DepthStencilView>dsv_;AtlasView::Com<ID3D11DepthStencilState>ds_;AtlasView::Com<ID3D11BlendState>blend_;
 bool rawRgb_;Raster(const Raster&);Raster&operator=(const Raster&);
public:Raster():rawRgb_(false){}void Clear(){blend_.Reset();ds_.Reset();dsv_.Reset();depth_.Reset();for(unsigned i=0;i<3;++i){rt_[i].Reset();view_[i].Reset();tex_[i].Reset();layout_[i].Reset();raster_[i].Reset();}for(unsigned i=0;i<2;++i){vs_[i].Reset();ps_[i].Reset();}instance_.Reset();pcb_.Reset();vcb_.Reset();device_.Reset();rawRgb_=false;}
 HRESULT Init(ID3D11Device*d,const ShaderSet&s,U64 caller){return InitMode(d,s,caller,false);}
 HRESULT InitRawRgb(ID3D11Device*d,const ShaderSet&s,U64 caller){return InitMode(d,s,caller,true);}
 bool IsRawRgb()const{return device_.p&&rawRgb_;}
 U64 OwnedGpuBytes()const{return device_.p?(rawRgb_?RawRgbPrivateBytes:PrivateBytes):0;}
private:
 HRESULT InitMode(ID3D11Device*d,const ShaderSet&s,U64 caller,bool rgb){
  if(!d||device_.p||!s.Valid())return E_INVALIDARG;if(!(rgb?FitsRawRgb(caller):Fits(caller)))return E_OUTOFMEMORY;HRESULT hr=S_OK;
  for(unsigned i=0;i<2&&SUCCEEDED(hr);++i){hr=d->CreateVertexShader(s.vertex[i].data,s.vertex[i].size,NULL,&vs_[i].p);if(SUCCEEDED(hr))hr=d->CreatePixelShader(s.pixel[i].data,s.pixel[i].size,NULL,&ps_[i].p);}
  for(unsigned layout=0;layout<3&&SUCCEEDED(hr);++layout){bool coloured=layout==Coloured64;D3D11_INPUT_ELEMENT_DESC e[8]={};const char*names[]={"POSITION","NORMAL","TEXCOORD","TANGENT","COLOR","TEXCOORD","TEXCOORD","TEXCOORD"};unsigned offset[]={0,12,coloured?28:24,coloured?48:44,24,0,16,32};for(unsigned i=0;i<8;++i){e[i].SemanticName=names[i];e[i].AlignedByteOffset=offset[i];e[i].Format=i<2?DXGI_FORMAT_R32G32B32_FLOAT:i==2?DXGI_FORMAT_R32G32_FLOAT:i==4?DXGI_FORMAT_R8G8B8A8_UNORM:i==3&&layout==Plain56?DXGI_FORMAT_R32G32B32_FLOAT:DXGI_FORMAT_R32G32B32A32_FLOAT;if(i>=5){e[i].InputSlot=1;e[i].SemanticIndex=i-4;e[i].InputSlotClass=D3D11_INPUT_PER_INSTANCE_DATA;e[i].InstanceDataStepRate=1;}}
   if(!coloured){e[4]=e[5];e[5]=e[6];e[6]=e[7];}const Bytecode&code=s.vertex[coloured?Dual43:Alpha48];hr=d->CreateInputLayout(e,coloured?8:7,code.data,code.size,&layout_[layout].p);
  }
  for(unsigned i=0;i<3&&SUCCEEDED(hr);++i){D3D11_RASTERIZER_DESC rs={};rs.FillMode=D3D11_FILL_SOLID;rs.CullMode=i==0?D3D11_CULL_NONE:i==1?D3D11_CULL_BACK:D3D11_CULL_FRONT;rs.FrontCounterClockwise=TRUE;rs.DepthClipEnable=rs.MultisampleEnable=TRUE;hr=d->CreateRasterizerState(&rs,&raster_[i].p);}
  for(unsigned i=0;i<4&&SUCCEEDED(hr);++i){D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=256;td.ArraySize=td.MipLevels=1;td.SampleDesc.Count=1;td.Format=i==3?DXGI_FORMAT_D32_FLOAT:i==2?DXGI_FORMAT_R32_FLOAT:i==0&&rgb?DXGI_FORMAT_R16G16B16A16_FLOAT:DXGI_FORMAT_R8G8B8A8_UNORM;td.BindFlags=i==3?D3D11_BIND_DEPTH_STENCIL:D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;hr=d->CreateTexture2D(&td,NULL,i==3?&depth_.p:&tex_[i].p);if(i<3&&SUCCEEDED(hr))hr=d->CreateShaderResourceView(tex_[i].p,NULL,&view_[i].p);if(i<3&&SUCCEEDED(hr))hr=d->CreateRenderTargetView(tex_[i].p,NULL,&rt_[i].p);}
  if(SUCCEEDED(hr))hr=d->CreateDepthStencilView(depth_.p,NULL,&dsv_.p);D3D11_BUFFER_DESC b={};b.ByteWidth=160;b.BindFlags=D3D11_BIND_CONSTANT_BUFFER;if(SUCCEEDED(hr))hr=d->CreateBuffer(&b,NULL,&vcb_.p);b.ByteWidth=64;if(SUCCEEDED(hr))hr=d->CreateBuffer(&b,NULL,&pcb_.p);b.ByteWidth=48;b.BindFlags=D3D11_BIND_VERTEX_BUFFER;if(SUCCEEDED(hr))hr=d->CreateBuffer(&b,NULL,&instance_.p);
  D3D11_DEPTH_STENCIL_DESC ds={};ds.DepthEnable=TRUE;ds.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;ds.DepthFunc=D3D11_COMPARISON_LESS_EQUAL;if(SUCCEEDED(hr))hr=d->CreateDepthStencilState(&ds,&ds_.p);D3D11_BLEND_DESC blend={};for(unsigned i=0;i<8;++i)blend.RenderTarget[i].RenderTargetWriteMask=15;if(SUCCEEDED(hr))hr=d->CreateBlendState(&blend,&blend_.p);
  if(FAILED(hr)){Clear();return hr;}device_.p=d;d->AddRef();rawRgb_=rgb;return S_OK;
 }
public:
 HRESULT Render(bool enabled,const Packet&p,const View&v,U64 caller){
  if(!enabled)return S_FALSE;if(!device_.p||!p.count_||v.generation!=p.generation_||!AtlasModels::Finite(v.viewProjection,16)||!AtlasModels::Finite(v.cameraScene,3)||!_finite(v.farClip)||v.farClip<=0)return E_INVALIDARG;if(!(rawRgb_?FitsRawRgb(caller):Fits(caller)))return E_OUTOFMEMORY;
  AtlasView::Com<ID3D11DeviceContext>c;device_.p->GetImmediateContext(&c.p);if(!c.p||!AtlasModels::NoStreamOutput(c.p))return E_INVALIDARG;for(unsigned i=0;i<3;++i)if(AtlasView::Bound(c.p,tex_[i].p,true))return E_INVALIDARG;if(AtlasView::Bound(c.p,depth_.p,true))return E_INVALIDARG;
  VertexConstants vc;PixelConstants pc;for(unsigned n=0;n<p.count_;++n){const Packet::Draw&d=p.draw_[n];if(!AtlasView::SameDevice(d.index.p,device_.p)||!Pack(d.rawWorld,v,d.surface,d.kind,vc,pc)||AtlasView::Bound(c.p,d.index.p,true)||AtlasView::Bound(c.p,d.vertex.p,true))return E_INVALIDARG;for(unsigned k=0;k<TextureCount(d.kind);++k){AtlasView::Com<ID3D11Resource>r;d.texture[k].p->GetResource(&r.p);for(unsigned j=0;j<3;++j)if(AtlasView::Identity(r.p,tex_[j].p))return E_INVALIDARG;if(AtlasView::Identity(r.p,depth_.p)||AtlasView::Bound(c.p,r.p,true))return E_INVALIDARG;}}
  AtlasView::SavedOutputReads a(c.p,tex_[0].p),b(c.p,tex_[1].p),q(c.p,tex_[2].p);PixelReads extra(c.p);AtlasModels::GraphicsState saved(c.p);c.p->SetPredication(NULL,FALSE);
  ID3D11UnorderedAccessView*empty[8]={};c.p->OMSetRenderTargetsAndUnorderedAccessViews(0,NULL,NULL,0,8,empty,NULL);ID3D11RenderTargetView*targets[]={rt_[0].p,rt_[1].p,rt_[2].p};c.p->OMSetRenderTargets(3,targets,dsv_.p);const float zero[4]={};for(unsigned i=0;i<3;++i)c.p->ClearRenderTargetView(targets[i],zero);c.p->ClearDepthStencilView(dsv_.p,D3D11_CLEAR_DEPTH,1,0);
  c.p->HSSetShader(NULL,NULL,0);c.p->DSSetShader(NULL,NULL,0);c.p->GSSetShader(NULL,NULL,0);c.p->VSSetConstantBuffers(0,1,&vcb_.p);c.p->PSSetConstantBuffers(0,1,&pcb_.p);c.p->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);D3D11_VIEWPORT vp={0,0,256,256,0,1};c.p->RSSetViewports(1,&vp);c.p->RSSetScissorRects(0,NULL);c.p->OMSetDepthStencilState(ds_.p,0);c.p->OMSetBlendState(blend_.p,NULL,~0u);
  for(unsigned n=0;n<p.count_;++n){const Packet::Draw&d=p.draw_[n];Pack(d.rawWorld,v,d.surface,d.kind,vc,pc);c.p->UpdateSubresource(vcb_.p,0,NULL,&vc,0,0);c.p->UpdateSubresource(pcb_.p,0,NULL,&pc,0,0);c.p->UpdateSubresource(instance_.p,0,NULL,d.rawWorld,0,0);ID3D11Buffer*vb[]={d.vertex.p,instance_.p};const UINT stride[]={d.shape.Stride(),48},offset[]={d.shape.vertexOffset,0};c.p->IASetVertexBuffers(0,2,vb,stride,offset);c.p->IASetIndexBuffer(d.index.p,d.shape.indexFormat,0);c.p->IASetInputLayout(layout_[d.shape.layout].p);c.p->RSSetState(raster_[d.cullMode-1].p);c.p->VSSetShader(vs_[d.kind].p,NULL,0);c.p->PSSetShader(ps_[d.kind].p,NULL,0);ID3D11ShaderResourceView*views[7]={};for(unsigned k=0;k<TextureCount(d.kind);++k)views[k]=d.texture[k].p;c.p->PSSetShaderResources(0,7,views);c.p->PSSetSamplers(0,1,&d.sampler.p);c.p->DrawIndexedInstanced(d.shape.indexCount,1,d.shape.indexStart,INT(d.shape.vertexStart),0);}
  return S_OK;
 }
 ID3D11Texture2D*Texture(unsigned i)const{return i<3?tex_[i].p:NULL;}ID3D11ShaderResourceView*Output(unsigned i)const{return i<3?view_[i].p:NULL;}
};
}
