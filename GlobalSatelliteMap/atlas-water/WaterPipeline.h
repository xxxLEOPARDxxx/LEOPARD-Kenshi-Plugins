#pragma once
#include "AtlasWater.h"
namespace AtlasWater {
const U64 HdrTerrainPrivateBytes=256ULL*256*8+AtlasView::UniformGpuBytes;
const U64 HdrWorkBytes=256ULL*256*8,TonePrivateBytes=256ULL*256*4+16;
const U64 PipelinePrivateBytes=HdrTerrainPrivateBytes+HdrWorkBytes+PrivateBytes+TonePrivateBytes;
inline bool PipelineFits(U64 callerOwnedBytes){return callerOwnedBytes<=AtlasView::GpuCap-PipelinePrivateBytes;}
class Pipeline {
 AtlasView::Relighter terrain_;
 Layer water_;
 AtlasView::Com<ID3D11Device>device_;
 AtlasView::Com<ID3D11Texture2D>work_,toneScratch_;
 AtlasView::Com<ID3D11ShaderResourceView>workView_;
 AtlasView::Com<ID3D11UnorderedAccessView>toneUav_;
 AtlasView::Com<ID3D11ComputeShader>toneShader_;
 AtlasView::Com<ID3D11Buffer>toneConstants_;
 Pipeline(const Pipeline&);Pipeline&operator=(const Pipeline&);
 HRESULT Tone(ID3D11DeviceContext*c,const AtlasView::Uniforms&values,ID3D11Texture2D*output){
  AtlasView::SavedOutputReads outputReads(c,output);SavedPredicate predicate(c);SavedCompute saved(c);
  const float exposure[4]={values.exposure[0],values.exposure[1],256,0};
  c->UpdateSubresource(toneConstants_.p,0,NULL,exposure,0,0);
  c->CSSetShader(toneShader_.p,NULL,0);c->CSSetConstantBuffers(0,1,&toneConstants_.p);c->CSSetShaderResources(0,1,&workView_.p);c->CSSetUnorderedAccessViews(0,1,&toneUav_.p,NULL);
  c->Dispatch(32,32,1);ID3D11UnorderedAccessView*none=NULL;c->CSSetUnorderedAccessViews(0,1,&none,NULL);
  c->CopySubresourceRegion(output,0,0,0,0,toneScratch_.p,0,NULL);
  return device_.p->GetDeviceRemovedReason();
 }
 bool InputSafe(ID3D11DeviceContext*c,ID3D11ShaderResourceView*v,ID3D11Texture2D*output)const{
  if(!AtlasView::SameDevice(v,device_.p))return false;
  AtlasView::Com<ID3D11Resource>r;v->GetResource(&r.p);
  return !AtlasView::Identity(r.p,output)&&!AtlasView::Identity(r.p,work_.p)&&!AtlasView::Bound(c,r.p,true);
 }
public:
 Pipeline(){}
 ~Pipeline(){Clear();}
 void Clear(){toneUav_.Reset();toneScratch_.Reset();toneConstants_.Reset();toneShader_.Reset();workView_.Reset();work_.Reset();water_.Clear();terrain_.Clear();device_.Reset();}
 // callerOwnedBytes includes ALL coexisting cache/display/default-renderer and
 // pending/retired owned payload, excluding only this pipeline's fixed private bytes.
 HRESULT Init(ID3D11Device*d,const void*terrainCode,SIZE_T terrainBytes,const void*waterCode,SIZE_T waterBytes,const void*toneCode,SIZE_T toneBytes,U64 callerOwnedBytes){
  if(device_.p||!d||!terrainCode||!terrainBytes||!waterCode||!waterBytes||!toneCode||!toneBytes||!PipelineFits(callerOwnedBytes))return E_INVALIDARG;
  HRESULT hr=terrain_.Init(d,terrainCode,terrainBytes,256,callerOwnedBytes+PipelinePrivateBytes-HdrTerrainPrivateBytes,DXGI_FORMAT_R16G16B16A16_FLOAT);
  if(SUCCEEDED(hr))hr=water_.Init(d,waterCode,waterBytes,callerOwnedBytes+PipelinePrivateBytes-PrivateBytes);
  D3D11_TEXTURE2D_DESC t={};t.Width=t.Height=256;t.MipLevels=t.ArraySize=1;t.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;t.SampleDesc.Count=1;t.Usage=D3D11_USAGE_DEFAULT;t.BindFlags=D3D11_BIND_SHADER_RESOURCE;
  if(SUCCEEDED(hr))hr=d->CreateTexture2D(&t,NULL,&work_.p);
  if(SUCCEEDED(hr))hr=d->CreateShaderResourceView(work_.p,NULL,&workView_.p);
  t.Format=DXGI_FORMAT_R8G8B8A8_UNORM;t.BindFlags=D3D11_BIND_UNORDERED_ACCESS;
  if(SUCCEEDED(hr))hr=d->CreateTexture2D(&t,NULL,&toneScratch_.p);
  if(SUCCEEDED(hr))hr=d->CreateUnorderedAccessView(toneScratch_.p,NULL,&toneUav_.p);
  if(SUCCEEDED(hr))hr=d->CreateComputeShader(toneCode,toneBytes,NULL,&toneShader_.p);
  D3D11_BUFFER_DESC b={};b.ByteWidth=16;b.Usage=D3D11_USAGE_DEFAULT;b.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
  if(SUCCEEDED(hr))hr=d->CreateBuffer(&b,NULL,&toneConstants_.p);
  if(FAILED(hr)){Clear();return hr;}device_.p=d;d->AddRef();return S_OK;
 }
 // OFF is an exact no-op, including before Init: caller retains original terrain
 // rendering. Enabled failure returns before final display copy where possible;
 // caller renders the original terrain path on any failed HRESULT.
 HRESULT Render(const AtlasView::Cell&cell,const AtlasView::Uniforms&terrain,ID3D11ShaderResourceView*const terrainLight[3],ID3D11SamplerState*const terrainSampler[3],const Uniforms&water,ID3D11ShaderResourceView*const waterInputs[5],ID3D11SamplerState*const waterSamplers[5],ID3D11Texture2D*output,U64 callerOwnedBytes){
  if(water.waterPlane[1]==0)return S_FALSE;
  if(!device_.p||!PipelineFits(callerOwnedBytes)||cell.side!=256||!terrain.Valid(256)||!water.Valid(256,256)||water.sourceWindow[0]!=256||water.sourceWindow[1]!=0||water.sourceWindow[2]!=0||std::memcmp(terrain.tileBounds,water.tileBounds,16)||!terrainLight||!terrainSampler||!waterInputs||!waterSamplers||!AtlasView::SameDevice(output,device_.p))return E_INVALIDARG;
  D3D11_TEXTURE2D_DESC d;output->GetDesc(&d);if(!AtlasView::OutputShape(d,256))return E_INVALIDARG;
  AtlasView::Com<ID3D11DeviceContext>c;device_.p->GetImmediateContext(&c.p);
  if(!c.p||c.p->GetType()!=D3D11_DEVICE_CONTEXT_IMMEDIATE||AtlasView::Bound(c.p,output,true))return DXGI_ERROR_INVALID_CALL;
  for(unsigned p=0;p<3;++p)if(!InputSafe(c.p,cell.view[p].p,output)||!InputSafe(c.p,terrainLight[p],output)||!AtlasView::SameDevice(terrainSampler[p],device_.p))return DXGI_ERROR_INVALID_CALL;
  for(unsigned p=0;p<5;++p)if(!InputSafe(c.p,waterInputs[p],output)||!AtlasView::SameDevice(waterSamplers[p],device_.p)||!AtlasView::LightView(waterInputs[p],device_.p,p>=3,1))return DXGI_ERROR_INVALID_CALL;
  HRESULT hr=terrain_.Render(cell,terrain,terrainLight,terrainSampler,work_.p);
  ID3D11ShaderResourceView*source[8]={workView_.p,cell.view[1].p,cell.view[2].p,waterInputs[0],waterInputs[1],waterInputs[2],waterInputs[3],waterInputs[4]};
  if(SUCCEEDED(hr))hr=water_.Render(water,source,waterSamplers,work_.p,callerOwnedBytes+PipelinePrivateBytes-PrivateBytes);
  if(SUCCEEDED(hr))hr=Tone(c.p,terrain,output);return hr;
 }
};
}
