// Reuse the real native-water fixture and oracle, not a parallel water model.
#define main ExistingWaterOracleMain
#include "warp_tests.cpp"
#undef main
#include "WaterPipeline.h"
#include "../atlas-view/AtlasWindowPolicy.h"
static unsigned maximumPipelineUnorm=0;

__declspec(noinline) static void TerrainValues(AtlasView::Uniforms&u){
 std::memset(&u,0,sizeof(u));u.tileBounds[0]=u.tileBounds[1]=-73728;
 u.tileBounds[2]=u.tileBounds[3]=73728;u.referenceSide[3]=256;
 u.sunDirection[0]=.2f;u.sunDirection[1]=.95f;u.sunDirection[2]=.1f;
 u.sunColour[0]=1.1f;u.sunColour[1]=1;u.sunColour[2]=.9f;u.sunColour[3]=1;
 u.envColour[3]=.7f;u.ambientParams[3]=1;
 u.worldSize[0]=u.worldSize[1]=1.0f/294912;u.worldSize[2]=u.worldSize[3]=.5f;
 u.exposure[0]=.25f;u.exposure[1]=.8f;u.viewDirection[1]=1;
 u.dispatchWindow[2]=u.dispatchWindow[3]=256;
}
static void FillTerrain(AtlasView::ReadRequest&r,bool shore=false){
 r.Clear();r.level=5;for(unsigned p=0;p<3;++p)r.plane[p].resize(static_cast<size_t>(AtlasView::PlaneBytes(5,p)));
 for(unsigned z=0;z<256;++z)for(unsigned x=0;x<256;++x){unsigned i=z*256+x;
  // The real SunProbe oracle decodes the clamped four-neighbour YCoCg field.
  // Retain its edge samples too, rather than comparing mismatched test inputs.
  const bool same=(x&1)==(z&1);const float g=same?.5f:1.0f;
  const unsigned xl=x?x-1:0,xr=x<255?x+1:255,zt=z?z-1:0,zb=z<255?z+1:255;
  const float other=(((xl&1)==(z&1)?.5f:1.0f)+((xr&1)==(z&1)?.5f:1.0f)+((x&1)==(zt&1)?.5f:1.0f)+((x&1)==(zb&1)?.5f:1.0f))*.25f;
  const float co=(same?other:g)-.5f,cg=(same?g:other)-.5f;
  const unsigned short halves[11]={0,0x3000,0x3400,0x3600,0x3800,0x3900,0x3a00,0x3b00,0x3c00,0x3c80,0x3d00};
  const unsigned short colour[4]={halves[unsigned((.75f+co-cg)*8+.5f)],halves[unsigned((.75f+cg)*8+.5f)],halves[unsigned((.75f-co-cg)*8+.5f)],0x3800};
  std::memcpy(&r.plane[0][i*8],colour,8);unsigned char*n=&r.plane[1][i*4];n[0]=n[2]=128;n[1]=n[3]=255;
  float h=shore?90.0f+float(x%32):90.0f;std::memcpy(&r.plane[2][i*4],&h,4);
 }
}
static void ActualTerrainOracle(Fixture&f,const std::vector<unsigned char>&code,Inputs&i,const AtlasView::Uniforms&u,ID3D11Texture2D*target,ID3D11ShaderResourceView*const overrideLights[3]=NULL){
 std::vector<float>a(256*256*4),depth(256*256,1);std::vector<unsigned>normal(256*256,0x0080ff80u);
 for(unsigned y=0;y<256;++y)for(unsigned x=0;x<256;++x){unsigned at=(y*256+x)*4;a[at]=.75f;a[at+1]=((x&1)==(y&1))?.5f:1.0f;a[at+2]=0;a[at+3]=.5f;}
 Com<ID3D11Texture2D>t[3];Com<ID3D11ShaderResourceView>v[3];
 Texture(f,256,256,DXGI_FORMAT_R32G32B32A32_FLOAT,D3D11_BIND_SHADER_RESOURCE,t[0],&a[0],256*16);
 Texture(f,256,256,DXGI_FORMAT_R8G8B8A8_UNORM,D3D11_BIND_SHADER_RESOURCE,t[1],&normal[0],256*4);
 Texture(f,256,256,DXGI_FORMAT_R32_FLOAT,D3D11_BIND_SHADER_RESOURCE,t[2],&depth[0],256*4);
 for(unsigned p=0;p<3;++p)Hr(f.d.p->CreateShaderResourceView(t[p].p,NULL,&v[p].p),"native terrain oracle view");
 float cb[12][4]={};cb[0][0]=u.tileBounds[2];cb[0][1]=u.tileBounds[3];cb[0][2]=120000;cb[0][3]=256;
 cb[1][0]=1;cb[2][2]=-1;cb[3][1]=1;cb[4][0]=u.tileBounds[0]+u.tileBounds[2]*.5f-u.referenceSide[0];cb[4][1]=16000-u.referenceSide[1];cb[4][2]=u.tileBounds[1]+u.tileBounds[3]*.5f-u.referenceSide[2];
 std::memcpy(cb[5],u.sunDirection,16);std::memcpy(cb[6],u.worldOffset,16);std::memcpy(cb[7],u.sunColour,16);std::memcpy(cb[8],u.envColour,16);std::memcpy(cb[9],u.ambientParams,16);std::memcpy(cb[10],u.worldSize,16);
 cb[11][0]=u.exposure[0];cb[11][1]=0; // ACTUAL NativeExposure raw branch, no gamma/tone.
 Com<ID3D11Buffer>b;D3D11_BUFFER_DESC bd={};bd.ByteWidth=sizeof(cb);bd.Usage=D3D11_USAGE_IMMUTABLE;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;D3D11_SUBRESOURCE_DATA init={};init.pSysMem=cb;Hr(f.d.p->CreateBuffer(&bd,&init,&b.p),"native terrain constants");
 Com<ID3D11ComputeShader>cs;Hr(f.d.p->CreateComputeShader(&code[0],code.size(),NULL,&cs.p),"actual SunProbe HDR oracle");Com<ID3D11UnorderedAccessView>out;Hr(f.d.p->CreateUnorderedAccessView(target,NULL,&out.p),"native terrain HDR UAV");
 ID3D11ShaderResourceView*views[6]={v[0].p,v[1].p,v[2].p,overrideLights?overrideLights[0]:i.views[3],overrideLights?overrideLights[1]:i.views[6],overrideLights?overrideLights[2]:i.views[7]};ID3D11SamplerState*samplers[3]={i.samplers[0],i.samplers[3],i.samplers[4]};
 f.c.p->CSSetShader(cs.p,NULL,0);f.c.p->CSSetConstantBuffers(0,1,&b.p);f.c.p->CSSetShaderResources(0,6,views);f.c.p->CSSetSamplers(3,3,samplers);f.c.p->CSSetUnorderedAccessViews(0,1,&out.p,NULL);f.c.p->Dispatch(32,32,1);f.c.p->ClearState();
}
static void DirectHdrShader(Fixture&f,const std::vector<unsigned char>&code,AtlasView::Cell&cell,Inputs&i,const AtlasView::Uniforms&u,ID3D11Texture2D*target){
 Com<ID3D11ComputeShader>cs;Hr(f.d.p->CreateComputeShader(&code[0],code.size(),NULL,&cs.p),"production HDR terrain shader");Com<ID3D11Buffer>b;D3D11_BUFFER_DESC bd={};bd.ByteWidth=sizeof(u);bd.Usage=D3D11_USAGE_IMMUTABLE;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;D3D11_SUBRESOURCE_DATA data={};data.pSysMem=&u;Hr(f.d.p->CreateBuffer(&bd,&data,&b.p),"production HDR terrain constants");Com<ID3D11UnorderedAccessView>out;Hr(f.d.p->CreateUnorderedAccessView(target,NULL,&out.p),"production HDR target");
 ID3D11ShaderResourceView*views[6]={cell.view[0].p,cell.view[1].p,cell.view[2].p,i.views[3],i.views[6],i.views[7]};ID3D11SamplerState*samplers[3]={i.samplers[0],i.samplers[3],i.samplers[4]};
 f.c.p->CSSetShader(cs.p,NULL,0);f.c.p->CSSetConstantBuffers(0,1,&b.p);f.c.p->CSSetShaderResources(0,6,views);f.c.p->CSSetSamplers(0,3,samplers);f.c.p->CSSetUnorderedAccessViews(0,1,&out.p,NULL);f.c.p->Dispatch(32,32,1);f.c.p->ClearState();
}
static void NativeOneMipCube(Fixture&f,Inputs&i,unsigned slot){
 Com<ID3D11Texture2D>t;D3D11_TEXTURE2D_DESC d={};d.Width=d.Height=128;d.MipLevels=1;d.ArraySize=6;d.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;d.SampleDesc.Count=1;d.Usage=D3D11_USAGE_DEFAULT;d.BindFlags=D3D11_BIND_SHADER_RESOURCE;d.MiscFlags=D3D11_RESOURCE_MISC_TEXTURECUBE;
 Hr(f.d.p->CreateTexture2D(&d,NULL,&t.p),"actual one-mip water cube");
 for(unsigned face=0;face<6;++face){std::vector<float>v(128*128*4);for(unsigned p=0;p<128*128;++p){v[p*4]=.04f+.002f*face;v[p*4+1]=.07f;v[p*4+2]=.09f;v[p*4+3]=1;}f.c.p->UpdateSubresource(t.p,face,NULL,&v[0],128*16,0);}
 i.view[slot].Reset();i.texture[slot].Reset();i.texture[slot].p=t.Detach();Hr(f.d.p->CreateShaderResourceView(i.texture[slot].p,NULL,&i.view[slot].p),"one-mip water cube view");i.views[slot]=i.view[slot].p;
}
static void OneMipOracle(Fixture&f,const std::vector<unsigned char>&water,const std::vector<unsigned char>&native,const std::vector<unsigned char>&vertex,const std::vector<unsigned char>&terrain){
 Inputs i(f,256,256);NativeOneMipCube(f,i,6);NativeOneMipCube(f,i,7);AtlasWater::Uniforms u;Values(u,256);
 Com<ID3D11Texture2D>expected;Texture(f,256,256,DXGI_FORMAT_R16G16B16A16_FLOAT,D3D11_BIND_RENDER_TARGET,expected);
 NativeDraw(f,i,u,native,vertex,terrain,expected.p,false); // unmodified native SampleLevel clamps3/7 to actual mip0.
 AtlasWater::Layer layer;Hr(layer.Init(f.d.p,&water[0],water.size(),80ULL*1024*1024),"one-mip water layer");
 Hr(layer.Render(u,i.views,i.samplers,i.texture[0].p,80ULL*1024*1024),"actual native one-mip water cube inputs");
 Compare(Readback(f,i.texture[0].p),Readback(f,expected.p),"native water one-mip hardware LOD clamp",.001f);
}
static void NativeOutput8(Fixture&f,Com<ID3D11Texture2D>&out){
 D3D11_TEXTURE2D_DESC d={};d.Width=d.Height=256;d.MipLevels=d.ArraySize=1;d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;d.SampleDesc.Count=1;d.Usage=D3D11_USAGE_DEFAULT;d.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;d.MiscFlags=D3D11_RESOURCE_MISC_GENERATE_MIPS;Hr(f.d.p->CreateTexture2D(&d,NULL,&out.p),"native Ogre shaped pipeline display");
}
static std::vector<unsigned char> Read8(Fixture&f,ID3D11Texture2D*t){
 D3D11_TEXTURE2D_DESC d;t->GetDesc(&d);d.BindFlags=d.MiscFlags=0;d.Usage=D3D11_USAGE_STAGING;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;Com<ID3D11Texture2D>s;Hr(f.d.p->CreateTexture2D(&d,NULL,&s.p),"RGBA8 staging");f.c.p->CopyResource(s.p,t);D3D11_MAPPED_SUBRESOURCE m;Hr(f.c.p->Map(s.p,0,D3D11_MAP_READ,0,&m),"RGBA8 map");std::vector<unsigned char>v(d.Width*d.Height*4);for(unsigned y=0;y<d.Height;++y)std::memcpy(&v[y*d.Width*4],static_cast<unsigned char*>(m.pData)+y*m.RowPitch,d.Width*4);f.c.p->Unmap(s.p,0);return v;
}
static unsigned Compare8(const std::vector<unsigned char>&a,const std::vector<unsigned char>&b,const char*why){Check(a.size()==b.size(),"RGBA8 comparison length");unsigned maximum=0;for(size_t i=0;i<a.size();++i){unsigned d=unsigned(std::abs(int(a[i])-int(b[i])));if(d>maximum)maximum=d;}if(maximum>maximumPipelineUnorm)maximumPipelineUnorm=maximum;if(maximum>1)std::fprintf(stderr,"maxUNORMdelta=%u ",maximum);Check(maximum<=1,why);return maximum;}
static void ToneOracle(Fixture&f,const std::vector<unsigned char>&code,ID3D11Texture2D*raw,const AtlasView::Uniforms&u,ID3D11Texture2D*target){
 Com<ID3D11ShaderResourceView>view;Hr(f.d.p->CreateShaderResourceView(raw,NULL,&view.p),"native tone HDR input");Com<ID3D11UnorderedAccessView>out;Hr(f.d.p->CreateUnorderedAccessView(target,NULL,&out.p),"native tone output");float values[4]={u.exposure[0],u.exposure[1],256,0};Com<ID3D11Buffer>b;D3D11_BUFFER_DESC bd={};bd.ByteWidth=16;bd.Usage=D3D11_USAGE_IMMUTABLE;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;D3D11_SUBRESOURCE_DATA init={};init.pSysMem=values;Hr(f.d.p->CreateBuffer(&bd,&init,&b.p),"native tone constants");Com<ID3D11ComputeShader>cs;Hr(f.d.p->CreateComputeShader(&code[0],code.size(),NULL,&cs.p),"actual native exposure oracle");f.c.p->CSSetShader(cs.p,NULL,0);f.c.p->CSSetConstantBuffers(0,1,&b.p);f.c.p->CSSetShaderResources(0,1,&view.p);f.c.p->CSSetUnorderedAccessViews(0,1,&out.p,NULL);f.c.p->Dispatch(32,32,1);f.c.p->ClearState();
}
static ULONG DeviceReferences(ID3D11Device*d){ULONG n=d->AddRef();d->Release();return n;}
static const AtlasWater::U64 ExternalStageBytes=526416ULL;
static HRESULT StageOrOriginal(AtlasWater::Pipeline&pipeline,AtlasView::Cell&cell,const AtlasView::Uniforms&u,ID3D11ShaderResourceView*const lights[3],ID3D11SamplerState*const samplers[3],const AtlasWater::Uniforms&w,ID3D11ShaderResourceView*const water[5],ID3D11SamplerState*const waterSamplers[5],ID3D11Texture2D*out,AtlasWater::U64 caller,const AtlasWater::HdrStage*stage){return stage?pipeline.RenderWithStage(cell,u,lights,samplers,w,water,waterSamplers,out,caller,stage):pipeline.Render(cell,u,lights,samplers,w,water,waterSamplers,out,caller);}
static void PipelineState(Fixture&f,AtlasWater::Pipeline&pipeline,AtlasView::Cell&cell,const AtlasView::Uniforms&u,const AtlasWater::Uniforms&w,ID3D11ShaderResourceView*const lights[3],ID3D11SamplerState*const lightSamplers[3],ID3D11ShaderResourceView*const waterViews[5],Inputs&i,ID3D11Texture2D*output,const std::vector<unsigned char>&csCode,const std::vector<unsigned char>&vsCode,const std::vector<unsigned char>&psCode,const AtlasWater::HdrStage*stage=NULL){
 const AtlasWater::U64 caller=AtlasView::WindowGpuBytes()+(stage?ExternalStageBytes:0);
 f.c.p->ClearState();Hr(StageOrOriginal(pipeline,cell,u,lights,lightSamplers,w,waterViews,i.samplers,output,caller,stage),"state reference pixels");std::vector<unsigned char>expected=Read8(f,output),zero(256*256*4,0);f.c.p->UpdateSubresource(output,0,NULL,&zero[0],256*4,0);
 Com<ID3D11ComputeShader>oldCs;Com<ID3D11VertexShader>vs;Com<ID3D11PixelShader>ps;
 Hr(f.d.p->CreateComputeShader(&csCode[0],csCode.size(),NULL,&oldCs.p),"pipeline sentinel CS");Hr(f.d.p->CreateVertexShader(&vsCode[0],vsCode.size(),NULL,&vs.p),"pipeline sentinel VS");Hr(f.d.p->CreatePixelShader(&psCode[0],psCode.size(),NULL,&ps.p),"pipeline sentinel PS");f.c.p->CSSetShader(oldCs.p,NULL,0);f.c.p->VSSetShader(vs.p,NULL,0);f.c.p->PSSetShader(ps.p,NULL,0);
 Com<ID3D11Buffer>oldCb;D3D11_BUFFER_DESC bd={};bd.ByteWidth=192;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;Hr(f.d.p->CreateBuffer(&bd,NULL,&oldCb.p),"pipeline sentinel CB");f.c.p->CSSetConstantBuffers(0,1,&oldCb.p);f.c.p->CSSetShaderResources(0,8,i.views);f.c.p->CSSetSamplers(0,5,i.samplers);
 Com<ID3D11Texture2D>target;Texture(f,16,16,DXGI_FORMAT_R8G8B8A8_UNORM,D3D11_BIND_RENDER_TARGET,target);Com<ID3D11RenderTargetView>rt;Hr(f.d.p->CreateRenderTargetView(target.p,NULL,&rt.p),"pipeline sentinel RTV");f.c.p->OMSetRenderTargets(1,&rt.p,NULL);D3D11_VIEWPORT vp={3,4,31,27,.1f,.9f};f.c.p->RSSetViewports(1,&vp);D3D11_RECT rect={1,2,15,16};f.c.p->RSSetScissorRects(1,&rect);f.c.p->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
 Com<ID3D11RasterizerState>rs;D3D11_RASTERIZER_DESC rd={};rd.FillMode=D3D11_FILL_WIREFRAME;rd.CullMode=D3D11_CULL_FRONT;rd.ScissorEnable=TRUE;rd.DepthClipEnable=TRUE;Hr(f.d.p->CreateRasterizerState(&rd,&rs.p),"pipeline sentinel raster");f.c.p->RSSetState(rs.p);
 Com<ID3D11Buffer>append;bd.ByteWidth=64;bd.BindFlags=D3D11_BIND_UNORDERED_ACCESS;bd.MiscFlags=D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;bd.StructureByteStride=4;Hr(f.d.p->CreateBuffer(&bd,NULL,&append.p),"pipeline append");Com<ID3D11UnorderedAccessView>appendView;D3D11_UNORDERED_ACCESS_VIEW_DESC ad={};ad.ViewDimension=D3D11_UAV_DIMENSION_BUFFER;ad.Buffer.NumElements=16;ad.Buffer.Flags=D3D11_BUFFER_UAV_FLAG_APPEND;Hr(f.d.p->CreateUnorderedAccessView(append.p,&ad,&appendView.p),"pipeline append view");UINT count=7;f.c.p->CSSetUnorderedAccessViews(0,1,&appendView.p,&count);
 Com<ID3D11ShaderResourceView>display;Hr(f.d.p->CreateShaderResourceView(output,NULL,&display.p),"pipeline display SRV");f.c.p->VSSetShaderResources(127,1,&display.p);f.c.p->HSSetShaderResources(127,1,&display.p);f.c.p->DSSetShaderResources(127,1,&display.p);f.c.p->GSSetShaderResources(127,1,&display.p);f.c.p->PSSetShaderResources(7,1,&display.p);f.c.p->CSSetShaderResources(127,1,&display.p);
 Com<ID3D11Predicate>pred;D3D11_QUERY_DESC q={D3D11_QUERY_OCCLUSION_PREDICATE,0};Hr(f.d.p->CreatePredicate(&q,&pred.p),"completed false pipeline predicate");f.c.p->Begin(pred.p);f.c.p->End(pred.p);f.c.p->Flush();BOOL value=TRUE;HRESULT available=S_FALSE;DWORD started=GetTickCount();while(available==S_FALSE&&GetTickCount()-started<2000){available=f.c.p->GetData(pred.p,&value,sizeof(value),0);if(available==S_FALSE)Sleep(1);}Check(available==S_OK&&!value,"predicate actually false without geometry");f.c.p->SetPredication(pred.p,FALSE); // Matching result suppresses commands unless the private scope disables it.
 {AtlasViewTest::ContextFixture before(f.c.p);Hr(StageOrOriginal(pipeline,cell,u,lights,lightSamplers,w,waterViews,i.samplers,output,caller,stage),"stateful full pipeline");AtlasViewTest::ContextFixture after(f.c.p);Check(before.Equal(after),"pipeline restores complete D3D11.0 context/output reads/predicate");}
 f.c.p->SetPredication(NULL,FALSE);Com<ID3D11Buffer>counter,read;D3D11_BUFFER_DESC c={};c.ByteWidth=4;Hr(f.d.p->CreateBuffer(&c,NULL,&counter.p),"pipeline counter copy");f.c.p->CopyStructureCount(counter.p,0,appendView.p);c.Usage=D3D11_USAGE_STAGING;c.CPUAccessFlags=D3D11_CPU_ACCESS_READ;Hr(f.d.p->CreateBuffer(&c,NULL,&read.p),"pipeline counter stage");f.c.p->CopyResource(read.p,counter.p);D3D11_MAPPED_SUBRESOURCE mapped;Hr(f.c.p->Map(read.p,0,D3D11_MAP_READ,0,&mapped),"pipeline counter map");Check(*static_cast<unsigned*>(mapped.pData)==7,"pipeline preserves native append counter7");f.c.p->Unmap(read.p,0);f.c.p->ClearState();Check(Read8(f,output)==expected,"false native predicate never suppresses HDR/water/tone/copy");
 // Every enabled refusal is before the display copy and preserves native state.
 AtlasWater::Uniforms invalid=w;invalid.tileBounds[0]+=1;
 {AtlasViewTest::ContextFixture before(f.c.p);Check(FAILED(pipeline.Render(cell,u,lights,lightSamplers,invalid,waterViews,i.samplers,output,caller)),"mismatched terrain/water world mapping refused");AtlasViewTest::ContextFixture after(f.c.p);Check(before.Equal(after)&&Read8(f,output)==expected,"invalid mapping leaves terrain display/state intact");}
 invalid=w;invalid.sourceWindow[1]=2;Check(FAILED(pipeline.Render(cell,u,lights,lightSamplers,invalid,waterViews,i.samplers,output,caller)),"full-cell origin cannot be silently applied to window inputs");
 Check(FAILED(pipeline.Render(cell,u,lights,lightSamplers,w,waterViews,i.samplers,output,AtlasView::GpuCap-AtlasWater::PipelinePrivateBytes+1)),"render current inclusive payload budget rejects plus1");
 ID3D11ShaderResourceView*alias[5]={display.p,waterViews[1],waterViews[2],waterViews[3],waterViews[4]};Check(FAILED(pipeline.Render(cell,u,lights,lightSamplers,w,alias,i.samplers,output,caller)),"read-only native input aliasing display refused");
 Com<ID3D11RenderTargetView>displayRtv;Hr(f.d.p->CreateRenderTargetView(output,NULL,&displayRtv.p),"bound display RTV");f.c.p->OMSetRenderTargets(1,&displayRtv.p,NULL);{AtlasViewTest::ContextFixture before(f.c.p);Check(FAILED(pipeline.Render(cell,u,lights,lightSamplers,w,waterViews,i.samplers,output,caller)),"write-bound display refused");AtlasViewTest::ContextFixture after(f.c.p);Check(before.Equal(after),"output RTV refusal leaves complete context unchanged");}f.c.p->ClearState();
 Com<ID3D11RenderTargetView>nativeRtv;Hr(f.d.p->CreateRenderTargetView(i.texture[3].p,NULL,&nativeRtv.p),"native source RTV");f.c.p->OMSetRenderTargets(1,&nativeRtv.p,NULL);{AtlasViewTest::ContextFixture before(f.c.p);Check(FAILED(pipeline.Render(cell,u,lights,lightSamplers,w,waterViews,i.samplers,output,caller)),"write-bound native material refused");AtlasViewTest::ContextFixture after(f.c.p);Check(before.Equal(after),"native write alias refusal does not unbind engine state");}f.c.p->ClearState();Check(Read8(f,output)==expected,"all validation/alias refusals preserve existing display");
}
// Real compute fixture, deliberately leaves its fixed CS bindings dirty. The
// production pipeline scope must restore them and disable native predication.
struct AdditiveStage {
 Fixture&f;Com<ID3D11Texture2D>scratch;Com<ID3D11UnorderedAccessView>uav;Com<ID3D11Buffer>cb;Com<ID3D11ComputeShader>shader;unsigned calls;bool failBefore,failAfter;
 AdditiveStage(Fixture&fixture,const std::vector<unsigned char>&code):f(fixture),calls(0),failBefore(false),failAfter(false){Texture(f,256,256,DXGI_FORMAT_R16G16B16A16_FLOAT,D3D11_BIND_UNORDERED_ACCESS,scratch);Hr(f.d.p->CreateUnorderedAccessView(scratch.p,NULL,&uav.p),"real stage scratch UAV");D3D11_BUFFER_DESC d={};d.ByteWidth=16;d.BindFlags=D3D11_BIND_CONSTANT_BUFFER;Hr(f.d.p->CreateBuffer(&d,NULL,&cb.p),"stage constants");Hr(f.d.p->CreateComputeShader(&code[0],code.size(),NULL,&shader.p),"real additive stage shader");}
 static HRESULT Apply(void*context,const AtlasView::Cell&cell,const AtlasView::Uniforms&u,ID3D11ShaderResourceView*hdrView,ID3D11Texture2D*hdr){return static_cast<AdditiveStage*>(context)->Run(cell,u,hdrView,hdr);}
 HRESULT Run(const AtlasView::Cell&cell,const AtlasView::Uniforms&u,ID3D11ShaderResourceView*hdrView,ID3D11Texture2D*hdr){
  ++calls;if(failBefore)return E_FAIL;if(cell.side!=256||!u.Valid(256)||!AtlasView::SameResource(hdrView,hdr)||!AtlasView::HdrOutputShape(Description(hdr),256))return E_INVALIDARG;
  const float colour[4]={1.75f,.75f,.25f,0};f.c.p->UpdateSubresource(cb.p,0,NULL,colour,0,0);f.c.p->CSSetShader(shader.p,NULL,0);f.c.p->CSSetConstantBuffers(0,1,&cb.p);f.c.p->CSSetShaderResources(0,1,&hdrView);f.c.p->CSSetUnorderedAccessViews(0,1,&uav.p,NULL);f.c.p->Dispatch(32,32,1);ID3D11UnorderedAccessView*none=NULL;f.c.p->CSSetUnorderedAccessViews(0,1,&none,NULL);ID3D11ShaderResourceView*empty=NULL;f.c.p->CSSetShaderResources(0,1,&empty);f.c.p->CopyResource(hdr,scratch.p);return failAfter?E_FAIL:S_OK;
 }
 static D3D11_TEXTURE2D_DESC Description(ID3D11Texture2D*t){D3D11_TEXTURE2D_DESC d={};if(t)t->GetDesc(&d);return d;}
};
static void StageOracle(Fixture&f,char**argv){
 std::vector<unsigned char>hdrCode=File(argv[1]),nativeTerrain=File(argv[2]),waterCode=File(argv[3]),nativeWater=File(argv[4]),vertex=File(argv[5]),depth=File(argv[6]),toneCode=File(argv[7]),nativeTone=File(argv[8]);std::string path(argv[2]);path.resize(path.find_last_of("\\/")+1);path+="StageAdd.cso";AdditiveStage stage(f,File(path.c_str()));AtlasWater::HdrStage hook;hook.context=&stage;hook.apply=&AdditiveStage::Apply;
 const AtlasWater::U64 caller=AtlasView::WindowGpuBytes()+ExternalStageBytes;
 Inputs i(f,256,256);ID3D11ShaderResourceView*lights[3]={i.views[3],i.views[6],i.views[7]};ID3D11SamplerState*samplers[3]={i.samplers[0],i.samplers[3],i.samplers[4]};ID3D11ShaderResourceView*waterViews[5]={i.views[3],i.views[4],i.views[5],i.views[6],i.views[7]};AtlasView::ReadRequest r;FillTerrain(r,true);AtlasView::Cell cell;Hr(cell.Upload(f.d.p,r),"stage terrain cell");r.Clear();i.Height(f,true);AtlasView::Uniforms u;TerrainValues(u);AtlasWater::Uniforms on,off;Values(on,256);off=on;off.waterPlane[1]=0;
 Com<ID3D11Texture2D>raw,waterReference,expected,output;Texture(f,256,256,DXGI_FORMAT_R16G16B16A16_FLOAT,D3D11_BIND_SHADER_RESOURCE,raw);Texture(f,256,256,DXGI_FORMAT_R16G16B16A16_FLOAT,D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET,waterReference);Texture(f,256,256,DXGI_FORMAT_R8G8B8A8_UNORM,D3D11_BIND_UNORDERED_ACCESS,expected);NativeOutput8(f,output);Com<ID3D11ShaderResourceView>rawView;Hr(f.d.p->CreateShaderResourceView(raw.p,NULL,&rawView.p),"stage reference HDR view");
 AtlasWater::Pipeline pipeline;Hr(pipeline.Init(f.d.p,&hdrCode[0],hdrCode.size(),&waterCode[0],waterCode.size(),&toneCode[0],toneCode.size(),caller),"stage-inclusive pipeline init");
 // Independent native terrain shader into strict HDR, real add, native exposure.
 DirectHdrShader(f,hdrCode,cell,i,u,i.texture[0].p);f.c.p->CopyResource(raw.p,i.texture[0].p);Hr(stage.Run(cell,u,rawView.p,raw.p),"real stage reference apply");ToneOracle(f,nativeTone,raw.p,u,expected.p);
 stage.calls=0;Hr(pipeline.RenderTerrainWithStage(cell,u,lights,samplers,output.p,caller,&hook),"water OFF still renders local HDR stage");Check(stage.calls==1,"stage invoked once per terrain window");Compare8(Read8(f,output.p),Read8(f,expected.p),"real additive HDR then native exposure once");std::vector<unsigned char>terrainStage=Read8(f,output.p);
 // Independent actual native terrain->add->native opaque water->native tone.
 ActualTerrainOracle(f,nativeTerrain,i,u,i.texture[0].p);f.c.p->CopyResource(raw.p,i.texture[0].p);Hr(stage.Run(cell,u,rawView.p,raw.p),"native terrain stage reference");f.c.p->CopyResource(i.texture[0].p,raw.p);NativeDraw(f,i,on,nativeWater,vertex,depth,waterReference.p,true);ToneOracle(f,nativeTone,waterReference.p,u,expected.p);
 stage.calls=0;Hr(pipeline.RenderWithStage(cell,u,lights,samplers,on,waterViews,i.samplers,output.p,caller,&hook),"HDR stage precedes native water");Check(stage.calls==1,"water+stage invokes one stage");Compare8(Read8(f,output.p),Read8(f,expected.p),"native terrain/add/water/exposure composition order");
 std::vector<unsigned char>withStage=Read8(f,output.p);Hr(pipeline.Render(cell,u,lights,samplers,on,waterViews,i.samplers,output.p,caller),"old no-stage baseline");std::vector<unsigned char>withoutStage=Read8(f,output.p);Check(withStage!=withoutStage,"real stage visibly affects dry terrain");Hr(pipeline.RenderWithStage(cell,u,lights,samplers,on,waterViews,i.samplers,output.p,caller,NULL),"stage OFF old route");Check(Read8(f,output.p)==withoutStage,"null stage preserves exact old water pixels");
 unsigned wet=0,dry=0;for(unsigned y=0;y<256;++y)for(unsigned x=0;x<256;++x){unsigned at=(y*256+x)*4;if(90+x%32<=100){for(unsigned c=0;c<4;++c)Check(withStage[at+c]==withoutStage[at+c],"opaque water covers earlier stage");++wet;}else{for(unsigned c=0;c<4;++c)Check(withStage[at+c]==terrainStage[at+c],"dry terrain retains prewater stage");++dry;}}Check(wet&&dry,"both opaque water and dry terrain covered");
 stage.calls=0;Check(pipeline.RenderTerrainWithStage(cell,u,NULL,NULL,NULL,~0ULL,NULL)==S_FALSE&&stage.calls==0,"null stage and water OFF are no-query no-op");Check(Read8(f,output.p)==withoutStage,"OFF route leaves old display exact");AtlasWater::HdrStage invalid={};Check(FAILED(pipeline.RenderTerrainWithStage(cell,u,lights,samplers,output.p,caller,&invalid)),"nonnull stage without callback refused");
 stage.failAfter=true;{AtlasViewTest::ContextFixture before(f.c.p);Check(FAILED(pipeline.RenderTerrainWithStage(cell,u,lights,samplers,output.p,caller,&hook)),"stage error after private HDR write propagated");AtlasViewTest::ContextFixture after(f.c.p);Check(before.Equal(after),"failing stage compute bindings restored");}Check(Read8(f,output.p)==withoutStage,"stage failure never copies final display");stage.failAfter=false;
 stage.calls=0;Check(FAILED(pipeline.RenderTerrainWithStage(cell,u,lights,samplers,output.p,AtlasView::GpuCap-AtlasWater::PipelinePrivateBytes+1,&hook))&&stage.calls==0,"stage-inclusive plus1 refuses before callback");Check(Read8(f,output.p)==withoutStage,"stage budget error preserves display");
 AtlasWater::Uniforms dryOnly=on;dryOnly.waterPlane[0]=-1e8f;PipelineState(f,pipeline,cell,u,dryOnly,lights,samplers,waterViews,i,output.p,hdrCode,vertex,nativeWater,&hook);
}
static void PipelineOracle(Fixture&f,char**argv){
 std::vector<unsigned char>hdrCode=File(argv[1]),nativeTerrain=File(argv[2]),waterCode=File(argv[3]),nativeWater=File(argv[4]),vertex=File(argv[5]),depth=File(argv[6]),toneCode=File(argv[7]),nativeTone=File(argv[8]),defaultCode=File(argv[9]);
 Inputs i(f,256,256);Com<ID3D11ShaderResourceView>held[3];unsigned indexes[3]={3,6,7};ID3D11ShaderResourceView*terrainLights[3];ID3D11SamplerState*terrainSamplers[3]={i.samplers[0],i.samplers[3],i.samplers[4]};for(unsigned p=0;p<3;++p){held[p].p=i.views[indexes[p]];held[p].p->AddRef();terrainLights[p]=held[p].p;}
 NativeOneMipCube(f,i,6);NativeOneMipCube(f,i,7);ID3D11ShaderResourceView*waterViews[5]={i.views[3],i.views[4],i.views[5],i.views[6],i.views[7]};
 AtlasView::ReadRequest r;FillTerrain(r,true);AtlasView::Cell cell;Hr(cell.Upload(f.d.p,r),"pipeline cache cell");r.Clear();i.Height(f,true);
 Com<ID3D11Texture2D>raw,waterReference,expected,output;Texture(f,256,256,DXGI_FORMAT_R16G16B16A16_FLOAT,D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_UNORDERED_ACCESS,raw);Texture(f,256,256,DXGI_FORMAT_R16G16B16A16_FLOAT,D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET,waterReference);Texture(f,256,256,DXGI_FORMAT_R8G8B8A8_UNORM,D3D11_BIND_UNORDERED_ACCESS,expected);NativeOutput8(f,output);
 AtlasWater::Pipeline pipeline;const AtlasWater::U64 caller=AtlasView::WindowGpuBytes();Hr(pipeline.Init(f.d.p,&hdrCode[0],hdrCode.size(),&waterCode[0],waterCode.size(),&toneCode[0],toneCode.size(),caller),"bounded HDR/water/tone initialization");
 for(unsigned test=0;test<6;++test){AtlasView::Uniforms u;TerrainValues(u);if(test>=4)u.tileBounds[2]=u.tileBounds[3]=2304;AtlasWater::Uniforms w;Values(w,256);std::memcpy(w.tileBounds,u.tileBounds,16);w.waterPlane[0]=test==2?110:test==3?-1e8f:100;if(test==5)w.waterTime[0]=13.456f;
  if(test==1){u.sunColour[3]=0;u.sunDirection[1]=-.95f;u.exposure[0]=.03125f;u.exposure[1]=.05f;w.waterSunColour[0]=w.waterSunColour[1]=w.waterSunColour[2]=0;w.waterSunDirection[1]=-.9f;}
  ActualTerrainOracle(f,nativeTerrain,i,u,raw.p,terrainLights);f.c.p->CopyResource(i.texture[0].p,raw.p);NativeDraw(f,i,w,nativeWater,vertex,depth,waterReference.p,true);ToneOracle(f,nativeTone,waterReference.p,u,expected.p);
  Hr(pipeline.Render(cell,u,terrainLights,terrainSamplers,w,waterViews,i.samplers,output.p,caller),"pipeline actual native HDR water/exposure once");Compare8(Read8(f,output.p),Read8(f,expected.p),"actual native terrain + water + one exposure");
 }
 AtlasView::Uniforms u;TerrainValues(u);AtlasWater::Uniforms off;Values(off,256);off.waterPlane[1]=0;
 AtlasView::Relighter legacy;Hr(legacy.Init(f.d.p,&defaultCode[0],defaultCode.size(),256,caller),"default terrain path retained");Hr(legacy.Render(cell,u,terrainLights,terrainSamplers,output.p),"off original terrain before");std::vector<unsigned char>before=Read8(f,output.p);
 AtlasViewTest::ContextFixture stateBefore(f.c.p);AtlasWater::Pipeline neverInitialized;Check(neverInitialized.Render(cell,u,NULL,NULL,off,NULL,NULL,NULL,0)==S_FALSE,"water OFF bypasses all native inputs/device/init");AtlasViewTest::ContextFixture stateAfter(f.c.p);Check(stateBefore.Equal(stateAfter),"OFF preserves complete context");
 AtlasWater::Uniforms on;Values(on,256);std::memcpy(on.tileBounds,u.tileBounds,16);Hr(pipeline.Render(cell,u,terrainLights,terrainSamplers,on,waterViews,i.samplers,output.p,caller),"on between original terrain passes");Check(pipeline.Render(cell,u,NULL,NULL,off,NULL,NULL,NULL,0)==S_FALSE,"off after successful water avoids pipeline work");Hr(legacy.Render(cell,u,terrainLights,terrainSamplers,output.p),"off original terrain after");Check(Read8(f,output.p)==before,"off/on/off restores exact original exposed terrain pixels");
 Check(AtlasWater::PipelinePrivateBytes==3408256ULL&&AtlasWater::PipelineFits(AtlasView::GpuCap-AtlasWater::PipelinePrivateBytes)&&!AtlasWater::PipelineFits(AtlasView::GpuCap-AtlasWater::PipelinePrivateBytes+1),"inclusive pipeline private payload and plus1 boundary");
 Check(FAILED(pipeline.Init(f.d.p,&hdrCode[0],hdrCode.size(),&waterCode[0],waterCode.size(),&toneCode[0],toneCode.size(),caller)),"live pipeline cannot initialize again");
 PipelineState(f,pipeline,cell,u,on,terrainLights,terrainSamplers,waterViews,i,output.p,waterCode,vertex,nativeWater);
 f.c.p->ClearState();ULONG refs=DeviceReferences(f.d.p);for(unsigned n=0;n<3;++n){AtlasWater::Pipeline repeated;Hr(repeated.Init(f.d.p,&hdrCode[0],hdrCode.size(),&waterCode[0],waterCode.size(),&toneCode[0],toneCode.size(),AtlasView::GpuCap-AtlasWater::PipelinePrivateBytes),"exact boundary repeated pipeline init");Hr(repeated.Render(cell,u,terrainLights,terrainSamplers,on,waterViews,i.samplers,output.p,AtlasView::GpuCap-AtlasWater::PipelinePrivateBytes),"exact boundary repeated pipeline render");Read8(f,output.p);repeated.Clear();f.c.p->ClearState();f.c.p->Flush();}Check(DeviceReferences(f.d.p)==refs,"repeated pipeline init/render/clear balances observed WARP device references");
 {AtlasWater::Pipeline failed;const unsigned invalidCode=0;Check(FAILED(failed.Init(f.d.p,&hdrCode[0],hdrCode.size(),&waterCode[0],waterCode.size(),&invalidCode,sizeof(invalidCode),caller)),"bad final tone code fails after partial allocation");failed.Clear();}Check(DeviceReferences(f.d.p)==refs,"partial pipeline initialization releases private resources");
 cell.Clear();FillTerrain(r,true);r.plane[1][3]=0;r.plane[1][7]=64;const float nan=std::numeric_limits<float>::quiet_NaN();std::memcpy(&r.plane[2][8],&nan,4);Hr(cell.Upload(f.d.p,r),"pipeline coverage cell");r.Clear();Hr(pipeline.Render(cell,u,terrainLights,terrainSamplers,on,waterViews,i.samplers,output.p,caller),"coverage/NaN pipeline");std::vector<unsigned char>coverage=Read8(f,output.p);Check(coverage[3]==0&&coverage[11]==0&&coverage[0]==0&&coverage[8]==0,"invalid coverage or height stays transparent through HDR/water/tone");Check(coverage[7]==64,"fractional cache coverage survives water and single exposure");
}
int main(int argc,char**argv){try{
 Check(argc==10,"HDR/native terrain/water/native water/VS/depth/tone/native tone/default paths");Fixture f;Inputs i(f,256,256);AtlasView::ReadRequest r;FillTerrain(r);AtlasView::Cell cell;Hr(cell.Upload(f.d.p,r),"terrain cell");r.Clear();AtlasView::Uniforms u;TerrainValues(u);
 Com<ID3D11Texture2D>actual,expected;Texture(f,256,256,DXGI_FORMAT_R16G16B16A16_FLOAT,D3D11_BIND_UNORDERED_ACCESS|D3D11_BIND_SHADER_RESOURCE,actual);Texture(f,256,256,DXGI_FORMAT_R16G16B16A16_FLOAT,D3D11_BIND_UNORDERED_ACCESS|D3D11_BIND_SHADER_RESOURCE,expected);
 ActualTerrainOracle(f,File(argv[2]),i,u,expected.p);DirectHdrShader(f,File(argv[1]),cell,i,u,actual.p);
 Compare(Readback(f,actual.p),Readback(f,expected.p),"linear HDR cutpoint matches actual unexposed native terrain",.002f);
 Com<ID3D11Texture2D>work;Texture(f,256,256,DXGI_FORMAT_R16G16B16A16_FLOAT,D3D11_BIND_SHADER_RESOURCE,work);AtlasView::Relighter hdr;std::vector<unsigned char>hdrCode=File(argv[1]);Hr(hdr.Init(f.d.p,&hdrCode[0],hdrCode.size(),256,80ULL*1024*1024,DXGI_FORMAT_R16G16B16A16_FLOAT),"strict HDR terrain initialization");ID3D11ShaderResourceView*light[3]={i.views[3],i.views[6],i.views[7]};ID3D11SamplerState*samplers[3]={i.samplers[0],i.samplers[3],i.samplers[4]};Hr(hdr.Render(cell,u,light,samplers,work.p),"strict HDR production render");Compare(Readback(f,work.p),Readback(f,expected.p),"strict HDR owned output/native terrain",.002f);
 OneMipOracle(f,File(argv[3]),File(argv[4]),File(argv[5]),File(argv[6]));
 PipelineOracle(f,argv);
 StageOracle(f,argv);
 std::printf("Pipeline WARP PASS %u checks; native raw HDR maxAbs=%.9g; full native terrain/water/single-exposure maxUNORM=%u\n",checks,maximumDelta,maximumPipelineUnorm);return 0;
}catch(...){return 1;}}
