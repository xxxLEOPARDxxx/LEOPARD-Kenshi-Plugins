#include "AtlasWater.h"
#include "../atlas-view/ContextFixture.h"
#include <cstdio>
#include <vector>
#include <limits>
using AtlasView::Com;
static unsigned checks=0;static float maximumDelta=0;
static void Check(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}}
static void Hr(HRESULT h,const char*s){if(FAILED(h)){std::fprintf(stderr,"HRESULT%08x ",unsigned(h));Check(false,s);}}
static std::vector<unsigned char> File(const char*p){FILE*f=NULL;fopen_s(&f,p,"rb");Check(f!=NULL,"shader open");fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);std::vector<unsigned char>v(n);Check(fread(&v[0],1,n,f)==size_t(n),"shader read");fclose(f);return v;}
static float Half(unsigned short h){int e=(h>>10)&31,m=h&1023;float f=e==0?std::ldexp(float(m),-24):e==31?(m?std::numeric_limits<float>::quiet_NaN():std::numeric_limits<float>::infinity()):std::ldexp(float(1024+m),e-25);return h&0x8000?-f:f;}
struct Fixture {
 Com<ID3D11Device>d;Com<ID3D11DeviceContext>c;
 Fixture(){D3D_FEATURE_LEVEL got;Hr(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&d.p,&got,&c.p),"WARP");Check(got>=D3D_FEATURE_LEVEL_11_0,"feature11");}
};
static void Texture(Fixture&f,unsigned w,unsigned h,DXGI_FORMAT fmt,unsigned bind,Com<ID3D11Texture2D>&t,const void*data=NULL,unsigned pitch=0){D3D11_TEXTURE2D_DESC d={};d.Width=w;d.Height=h;d.MipLevels=d.ArraySize=1;d.Format=fmt;d.SampleDesc.Count=1;d.Usage=D3D11_USAGE_DEFAULT;d.BindFlags=bind;D3D11_SUBRESOURCE_DATA s={};s.pSysMem=data;s.SysMemPitch=pitch;Hr(f.d.p->CreateTexture2D(&d,data?&s:NULL,&t.p),"texture");}
static std::vector<unsigned short> Readback(Fixture&f,ID3D11Texture2D*t){D3D11_TEXTURE2D_DESC d;t->GetDesc(&d);d.BindFlags=d.MiscFlags=0;d.Usage=D3D11_USAGE_STAGING;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;Com<ID3D11Texture2D>s;Hr(f.d.p->CreateTexture2D(&d,NULL,&s.p),"staging");f.c.p->CopyResource(s.p,t);D3D11_MAPPED_SUBRESOURCE m;Hr(f.c.p->Map(s.p,0,D3D11_MAP_READ,0,&m),"map");std::vector<unsigned short>v(d.Width*d.Height*4);for(unsigned y=0;y<d.Height;++y)std::memcpy(&v[y*d.Width*4],static_cast<unsigned char*>(m.pData)+y*m.RowPitch,d.Width*8);f.c.p->Unmap(s.p,0);return v;}
struct Inputs {
 Com<ID3D11Texture2D>texture[8],dummy;Com<ID3D11ShaderResourceView>view[8],dummyView;ID3D11ShaderResourceView*views[8];
 Com<ID3D11SamplerState>state[5];ID3D11SamplerState*samplers[5];unsigned side,block;std::vector<unsigned short>base;std::vector<float>heights;
 Inputs(Fixture&f,unsigned s=512,unsigned b=256):side(s),block(b){
  base.resize(block*block*4);for(unsigned i=0;i<block*block;++i){base[i*4]=0x3e00;base[i*4+1]=0x3800;base[i*4+2]=0x3400;base[i*4+3]=0x3c00;}
  std::vector<unsigned> nc(side*side,0xff80ff80u);heights.resize(side*side,90);
  Texture(f,block,block,DXGI_FORMAT_R16G16B16A16_FLOAT,D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET|D3D11_BIND_UNORDERED_ACCESS,texture[0],&base[0],block*8);
  Texture(f,side,side,DXGI_FORMAT_R8G8B8A8_UNORM,D3D11_BIND_SHADER_RESOURCE,texture[1],&nc[0],side*4);
  Texture(f,side,side,DXGI_FORMAT_R32_FLOAT,D3D11_BIND_SHADER_RESOURCE,texture[2],&heights[0],side*4);
  for(unsigned p=3;p<8;++p){D3D11_TEXTURE2D_DESC d={};d.Width=d.Height=128;d.MipLevels=8;d.ArraySize=p>=6?6:1;d.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;d.SampleDesc.Count=1;d.Usage=D3D11_USAGE_DEFAULT;d.BindFlags=D3D11_BIND_SHADER_RESOURCE|(p==3?D3D11_BIND_RENDER_TARGET:0);d.MiscFlags=p>=6?D3D11_RESOURCE_MISC_TEXTURECUBE:0;Hr(f.d.p->CreateTexture2D(&d,NULL,&texture[p].p),"native material texture");
   for(unsigned face=0;face<d.ArraySize;++face)for(unsigned mip=0;mip<8;++mip){unsigned n=128>>mip;std::vector<float>v(n*n*4);
    for(unsigned z=0;z<n;++z)for(unsigned x=0;x<n;++x){unsigned i=(z*n+x)*4;float a=float((x^z)&1),xx=float(x)/n,zz=float(z)/n;
     if(p==3){v[i]=.04f+.01f*mip+.015f*a;v[i+1]=.1f+.02f*xx;v[i+2]=.16f+.02f*zz;}
     if(p==4){v[i]=.65f+.03f*xx;v[i+1]=.55f+.02f*zz;v[i+2]=.2f;}
     if(p==5){v[i]=.5f+.08f*(a*2-1)/(mip+1);v[i+1]=.5f+.06f*xx/(mip+1);v[i+2]=.99f;}
     if(p>=6){float energy=1.0f;v[i]=energy*(.04f+.01f*mip+.002f*face);v[i+1]=energy*(.07f+.005f*mip);v[i+2]=energy*(.09f+.003f*face);}v[i+3]=1;
    }f.c.p->UpdateSubresource(texture[p].p,D3D11CalcSubresource(mip,face,8),NULL,&v[0],n*16,0);
   }
  }
  for(unsigned p=0;p<8;++p){Hr(f.d.p->CreateShaderResourceView(texture[p].p,NULL,&view[p].p),"view");views[p]=view[p].p;}
  float black[4]={0,0,0,0};Texture(f,1,1,DXGI_FORMAT_R32G32B32A32_FLOAT,D3D11_BIND_SHADER_RESOURCE,dummy,black,16);Hr(f.d.p->CreateShaderResourceView(dummy.p,NULL,&dummyView.p),"reference NO_REFLECTION dummy");
  for(unsigned i=0;i<5;++i){D3D11_SAMPLER_DESC d={};d.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;d.AddressU=d.AddressV=d.AddressW=i==2?D3D11_TEXTURE_ADDRESS_WRAP:D3D11_TEXTURE_ADDRESS_CLAMP;d.MaxLOD=D3D11_FLOAT32_MAX;Hr(f.d.p->CreateSamplerState(&d,&state[i].p),"native sampler");samplers[i]=state[i].p;}
 }
 void ResetHdr(Fixture&f){f.c.p->UpdateSubresource(texture[0].p,0,NULL,&base[0],block*8,0);}
 void Height(Fixture&f,bool shoreline){for(unsigned z=0;z<side;++z)for(unsigned x=0;x<side;++x)heights[z*side+x]=shoreline?90.0f+float(x%32):90.0f;f.c.p->UpdateSubresource(texture[2].p,0,NULL,&heights[0],side*4,0);}
 void Flow(Fixture&f,float r,float g){for(unsigned mip=0;mip<8;++mip){unsigned n=128>>mip;std::vector<float>v(n*n*4,.2f);for(unsigned i=0;i<n*n;++i){v[i*4]=r;v[i*4+1]=g;v[i*4+3]=1;}f.c.p->UpdateSubresource(texture[4].p,mip,NULL,&v[0],n*16,0);}}
};
// VC10 /O2 miscompiled an inlined large-struct-return fixture and discarded
// colour-row tail stores. Avoid that return path and assert the rows below.
__declspec(noinline) static void Values(AtlasWater::Uniforms&u,unsigned side){std::memset(&u,0,sizeof(u));u.tileBounds[0]=-73728;u.tileBounds[1]=-73728;u.tileBounds[2]=u.tileBounds[3]=73728;u.sourceWindow[0]=float(side);u.waterPlane[0]=100;u.waterPlane[1]=1;u.nativeMapBounds[0]=u.nativeMapBounds[1]=-147456;u.nativeMapBounds[2]=u.nativeMapBounds[3]=147456;u.normalScale[0]=u.normalScale[1]=.004f;u.waterSunDirection[0]=.2f;u.waterSunDirection[1]=.9f;u.waterSunDirection[2]=.1f;u.waterSunColour[0]=1.1f;u.waterSunColour[1]=1;u.waterSunColour[2]=.9f;u.waterSunColour[3]=1;u.waterEnvColour[0]=.8f;u.waterEnvColour[1]=.9f;u.waterEnvColour[2]=1;u.waterEnvColour[3]=.7f;u.materialParams[0]=1;u.materialParams[1]=120;u.materialParams[2]=.9f;u.waterTime[0]=1.2345f;u.parallelView[1]=1;}
static void NativeDraw(Fixture&f,Inputs&i,const AtlasWater::Uniforms&u,const std::vector<unsigned char>&psCode,const std::vector<unsigned char>&vsCode,const std::vector<unsigned char>&terrainCode,ID3D11Texture2D*target,bool depthTest){
 f.c.p->ClearState();Com<ID3D11VertexShader>vs;Com<ID3D11PixelShader>ps;Hr(f.d.p->CreateVertexShader(&vsCode[0],vsCode.size(),NULL,&vs.p),"reference VS");Hr(f.d.p->CreatePixelShader(&psCode[0],psCode.size(),NULL,&ps.p),"ACTUAL native waterFP PS");
 AtlasWater::Uniforms values=u;values.dispatchWindow[0]=values.dispatchWindow[1]=float(i.block);Com<ID3D11Buffer>vcb;D3D11_BUFFER_DESC b={};b.ByteWidth=sizeof(values);b.BindFlags=D3D11_BIND_CONSTANT_BUFFER;b.Usage=D3D11_USAGE_IMMUTABLE;D3D11_SUBRESOURCE_DATA init={};init.pSysMem=&values;Hr(f.d.p->CreateBuffer(&b,&init,&vcb.p),"reference vertex constants");
 Com<ID3D11Texture2D>depth;Texture(f,i.block,i.block,DXGI_FORMAT_D32_FLOAT,D3D11_BIND_DEPTH_STENCIL,depth);Com<ID3D11DepthStencilView>dsv;Hr(f.d.p->CreateDepthStencilView(depth.p,NULL,&dsv.p),"native plane depth");Com<ID3D11DepthStencilState>ds;D3D11_DEPTH_STENCIL_DESC dd={};dd.DepthEnable=depthTest;dd.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;dd.DepthFunc=D3D11_COMPARISON_LESS_EQUAL;Hr(f.d.p->CreateDepthStencilState(&dd,&ds.p),"native depth state");f.c.p->OMSetDepthStencilState(ds.p,0);
 Com<ID3D11RenderTargetView>rt;Hr(f.d.p->CreateRenderTargetView(target,NULL,&rt.p),"referenceRTV");f.c.p->OMSetRenderTargets(1,&rt.p,dsv.p);f.c.p->ClearDepthStencilView(dsv.p,D3D11_CLEAR_DEPTH,1,0);float clear[4]={0,0,0,0};f.c.p->ClearRenderTargetView(rt.p,clear);
 Com<ID3D11RasterizerState>rs;D3D11_RASTERIZER_DESC rd={};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_NONE;rd.DepthClipEnable=TRUE;Hr(f.d.p->CreateRasterizerState(&rd,&rs.p),"reference raster");f.c.p->RSSetState(rs.p);D3D11_VIEWPORT vp={0,0,float(i.block),float(i.block),0,1};f.c.p->RSSetViewports(1,&vp);f.c.p->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);f.c.p->VSSetShader(vs.p,NULL,0);f.c.p->VSSetConstantBuffers(0,1,&vcb.p);
 if(depthTest){Com<ID3D11PixelShader>terrain;Hr(f.d.p->CreatePixelShader(&terrainCode[0],terrainCode.size(),NULL,&terrain.p),"terrain depth reference");f.c.p->PSSetShader(terrain.p,NULL,0);f.c.p->PSSetConstantBuffers(0,1,&vcb.p);f.c.p->PSSetShaderResources(0,3,i.views);f.c.p->Draw(3,0);}
 // Exact freshly compiled native $Params offsets/slots are checked by
 // verify_shaders.py before this executable runs; no guessed game ABI.
 std::vector<unsigned char>bytes(144,0);const unsigned offsets[]={0,64,100,112,120,124,128};
 const float*data[]={u.waterSunColour,u.waterEnvColour,u.materialParams,u.materialParams+1,u.materialParams+2,u.materialParams+3,u.waterTime};const unsigned lengths[]={16,16,4,4,4,4,4};
 for(unsigned k=0;k<7;++k){Check(offsets[k]+lengths[k]<=bytes.size(),"native constant range");std::memcpy(&bytes[offsets[k]],data[k],lengths[k]);}
 Com<ID3D11Buffer>pcb;b.ByteWidth=144;init.pSysMem=&bytes[0];Hr(f.d.p->CreateBuffer(&b,&init,&pcb.p),"native pixel constants");f.c.p->PSSetConstantBuffers(0,1,&pcb.p);f.c.p->PSSetShader(ps.p,NULL,0);
 ID3D11ShaderResourceView*source[6]={i.views[3],i.views[4],i.views[5],i.dummyView.p,i.views[6],i.views[7]};ID3D11SamplerState*samplers[6]={i.samplers[0],i.samplers[1],i.samplers[2],i.samplers[0],i.samplers[3],i.samplers[4]};f.c.p->PSSetShaderResources(0,6,source);f.c.p->PSSetSamplers(0,6,samplers);f.c.p->Draw(3,0);f.c.p->ClearState();
}
static void Compare(const std::vector<unsigned short>&a,const std::vector<unsigned short>&b,const char*why,float tolerance=.001f){Check(a.size()==b.size(),"readbacksize");float delta=0;for(size_t k=0;k<a.size();++k){float x=Half(a[k]),y=Half(b[k]);if(!_finite(x)||!_finite(y))Check(false,"finite actual native/material HDR");float d=std::fabs(x-y);if(d>delta)delta=d;}if(delta>maximumDelta)maximumDelta=delta;if(delta>tolerance)std::fprintf(stderr,"maxDelta=%.9g ",delta);Check(delta<=tolerance,why);}
static void BorderTest(Fixture&f,AtlasWater::Layer&layer,const std::vector<unsigned char>&native,const std::vector<unsigned char>&vertex,const std::vector<unsigned char>&terrain){
 Inputs i(f,2048,256);Com<ID3D11Texture2D>oracle;Texture(f,i.block,i.block,DXGI_FORMAT_R16G16B16A16_FLOAT,D3D11_BIND_RENDER_TARGET,oracle);
 // Last block of one primary cell and first block of its right neighbour.
 // Both use the same absolute native UV field, not restarted local UVs.
 for(unsigned t=0;t<2;++t){AtlasWater::Uniforms u;Values(u,i.side);u.tileBounds[0]=t?0.0f:-73728.0f;u.sourceWindow[1]=t?0.0f:1792.0f;u.sourceWindow[2]=1024.0f;u.waterTime[0]=7.89f;
  i.ResetHdr(f);NativeDraw(f,i,u,native,vertex,terrain,oracle.p,false);Hr(layer.Render(u,i.views,i.samplers,i.texture[0].p,80ULL*1024*1024),"primary border water");Compare(Readback(f,i.texture[0].p),Readback(f,oracle.p),"absolute primary-cell border native parity");
 }
 std::vector<unsigned>nc(i.side*i.side,0x4080ff80u);for(unsigned y=0;y<i.side;++y)for(unsigned x=0;x<i.side;++x)if(x%2==0)nc[y*i.side+x]=0x0080ff80u;
 f.c.p->UpdateSubresource(i.texture[1].p,0,NULL,&nc[0],i.side*4,0);AtlasWater::Uniforms u;Values(u,i.side);i.ResetHdr(f);Hr(layer.Render(u,i.views,i.samplers,i.texture[0].p,80ULL*1024*1024),"explicit coverage composition");std::vector<unsigned short>v=Readback(f,i.texture[0].p);bool changed=false;
 for(unsigned y=0;y<i.block;++y)for(unsigned x=0;x<i.block;++x){size_t p=(y*i.block+x)*4;if(x%2==0){for(unsigned c=0;c<4;++c)if(v[p+c]!=i.base[p+c])Check(false,"invalid coverage must retain complete HDR");}else {if(v[p]!=i.base[p])changed=true;if(v[p+3]!=i.base[p+3])Check(false,"valid fractional coverage retains base alpha");}}
 Check(changed,"fractional coverage is validity, not water opacity");
}
static void StateTest(Fixture&f,Inputs&i,AtlasWater::Layer&layer,const AtlasWater::Uniforms&u,const std::vector<unsigned char>&code,const std::vector<unsigned char>&vertex,const std::vector<unsigned char>&native){
 f.c.p->ClearState();Com<ID3D11ComputeShader>oldCs;Com<ID3D11VertexShader>vs;Com<ID3D11PixelShader>ps;
 Check(FAILED(layer.Render(u,i.views,i.samplers,i.texture[0].p,AtlasWater::Cap)),"updated caller payload budget enforced on every enabled Render");
 Hr(f.d.p->CreateComputeShader(&code[0],code.size(),NULL,&oldCs.p),"sentinel CS");Hr(f.d.p->CreateVertexShader(&vertex[0],vertex.size(),NULL,&vs.p),"sentinel VS");Hr(f.d.p->CreatePixelShader(&native[0],native.size(),NULL,&ps.p),"sentinel PS");f.c.p->VSSetShader(vs.p,NULL,0);f.c.p->PSSetShader(ps.p,NULL,0);f.c.p->CSSetShader(oldCs.p,NULL,0);
 Com<ID3D11Buffer>oldCb;D3D11_BUFFER_DESC b={};b.ByteWidth=192;b.BindFlags=D3D11_BIND_CONSTANT_BUFFER;Hr(f.d.p->CreateBuffer(&b,NULL,&oldCb.p),"sentinel CB");f.c.p->CSSetConstantBuffers(0,1,&oldCb.p);f.c.p->CSSetShaderResources(0,8,i.views);f.c.p->CSSetSamplers(0,5,i.samplers);
 Com<ID3D11Texture2D>target;Texture(f,16,16,DXGI_FORMAT_R16G16B16A16_FLOAT,D3D11_BIND_RENDER_TARGET,target);Com<ID3D11RenderTargetView>rt;Hr(f.d.p->CreateRenderTargetView(target.p,NULL,&rt.p),"sentinel RTV");f.c.p->OMSetRenderTargets(1,&rt.p,NULL);
 Com<ID3D11RasterizerState>rs;D3D11_RASTERIZER_DESC rd={};rd.FillMode=D3D11_FILL_WIREFRAME;rd.CullMode=D3D11_CULL_FRONT;rd.ScissorEnable=TRUE;rd.DepthClipEnable=TRUE;Hr(f.d.p->CreateRasterizerState(&rd,&rs.p),"sentinel raster");f.c.p->RSSetState(rs.p);D3D11_VIEWPORT vp={3,4,31,27,.1f,.9f};f.c.p->RSSetViewports(1,&vp);D3D11_RECT rect={1,2,15,16};f.c.p->RSSetScissorRects(1,&rect);f.c.p->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
 Com<ID3D11Buffer>append;D3D11_BUFFER_DESC ad={};ad.ByteWidth=64;ad.BindFlags=D3D11_BIND_UNORDERED_ACCESS;ad.MiscFlags=D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;ad.StructureByteStride=4;Hr(f.d.p->CreateBuffer(&ad,NULL,&append.p),"append");Com<ID3D11UnorderedAccessView>ua;D3D11_UNORDERED_ACCESS_VIEW_DESC ud={};ud.ViewDimension=D3D11_UAV_DIMENSION_BUFFER;ud.Buffer.NumElements=16;ud.Buffer.Flags=D3D11_BUFFER_UAV_FLAG_APPEND;Hr(f.d.p->CreateUnorderedAccessView(append.p,&ud,&ua.p),"append UAV");UINT count=7;f.c.p->CSSetUnorderedAccessViews(0,1,&ua.p,&count);
 // The verified-owned in-place output is read-bound in every native stage.
 f.c.p->VSSetShaderResources(127,1,&i.views[0]);f.c.p->HSSetShaderResources(127,1,&i.views[0]);f.c.p->DSSetShaderResources(127,1,&i.views[0]);f.c.p->GSSetShaderResources(127,1,&i.views[0]);f.c.p->PSSetShaderResources(7,1,&i.views[0]);f.c.p->CSSetShaderResources(127,1,&i.views[0]);
 Com<ID3D11Predicate>pred;D3D11_QUERY_DESC q={D3D11_QUERY_OCCLUSION_PREDICATE,0};Hr(f.d.p->CreatePredicate(&q,&pred.p),"predicate");f.c.p->SetPredication(pred.p,TRUE);
 {AtlasViewTest::ContextFixture before(f.c.p);Hr(layer.Render(u,i.views,i.samplers,i.texture[0].p,80ULL*1024*1024),"stateful in-place water");AtlasViewTest::ContextFixture after(f.c.p);Check(before.Equal(after),"complete D3D11.0 state/output SRVs restored");}
 f.c.p->SetPredication(NULL,FALSE);Com<ID3D11Buffer>counter,stage;D3D11_BUFFER_DESC cd={};cd.ByteWidth=4;Hr(f.d.p->CreateBuffer(&cd,NULL,&counter.p),"counter copy");f.c.p->CopyStructureCount(counter.p,0,ua.p);cd.Usage=D3D11_USAGE_STAGING;cd.CPUAccessFlags=D3D11_CPU_ACCESS_READ;Hr(f.d.p->CreateBuffer(&cd,NULL,&stage.p),"counter readback");f.c.p->CopyResource(stage.p,counter.p);D3D11_MAPPED_SUBRESOURCE mapped;Hr(f.c.p->Map(stage.p,0,D3D11_MAP_READ,0,&mapped),"counter map");Check(*static_cast<unsigned*>(mapped.pData)==7,"native append counter7 preserved");f.c.p->Unmap(stage.p,0);f.c.p->ClearState();
 Com<ID3D11UnorderedAccessView>outUav;Hr(f.d.p->CreateUnorderedAccessView(i.texture[0].p,NULL,&outUav.p),"HDR UAV");f.c.p->CSSetUnorderedAccessViews(0,1,&outUav.p,NULL);
 {AtlasViewTest::ContextFixture before(f.c.p);Check(FAILED(layer.Render(u,i.views,i.samplers,i.texture[0].p,80ULL*1024*1024)),"write-bound HDR refused");AtlasViewTest::ContextFixture after(f.c.p);Check(before.Equal(after),"refusal preserves native context");}f.c.p->ClearState();
 f.c.p->OMSetRenderTargetsAndUnorderedAccessViews(0,NULL,NULL,0,1,&outUav.p,NULL);Check(FAILED(layer.Render(u,i.views,i.samplers,i.texture[0].p,80ULL*1024*1024)),"OM UAV HDR refused");f.c.p->ClearState();
 Com<ID3D11RenderTargetView>hdrRtv;Hr(f.d.p->CreateRenderTargetView(i.texture[0].p,NULL,&hdrRtv.p),"HDR RTV");f.c.p->OMSetRenderTargets(1,&hdrRtv.p,NULL);Check(FAILED(layer.Render(u,i.views,i.samplers,i.texture[0].p,80ULL*1024*1024)),"RTV HDR refused");f.c.p->ClearState();
 Com<ID3D11RenderTargetView>colourRtv;Hr(f.d.p->CreateRenderTargetView(i.texture[3].p,NULL,&colourRtv.p),"native source RTV");f.c.p->OMSetRenderTargets(1,&colourRtv.p,NULL);{AtlasViewTest::ContextFixture before(f.c.p);Check(FAILED(layer.Render(u,i.views,i.samplers,i.texture[0].p,80ULL*1024*1024)),"writable native water source refused");AtlasViewTest::ContextFixture after(f.c.p);Check(before.Equal(after),"native source refusal has no automatic unbind");}f.c.p->ClearState();
 AtlasWater::Uniforms invalid=u;invalid.sourceWindow[1]=.5f;Check(FAILED(layer.Render(invalid,i.views,i.samplers,i.texture[0].p,80ULL*1024*1024)),"fractional source origin refused");invalid=u;invalid.waterPlane[0]=std::numeric_limits<float>::quiet_NaN();Check(FAILED(layer.Render(invalid,i.views,i.samplers,i.texture[0].p,80ULL*1024*1024)),"NaN water snapshot refused");
 Com<ID3D11Texture2D>wrong;Texture(f,i.block,i.block,DXGI_FORMAT_R8G8B8A8_UNORM,D3D11_BIND_SHADER_RESOURCE,wrong);Check(FAILED(layer.Render(u,i.views,i.samplers,wrong.p,80ULL*1024*1024)),"exposed UNORM output refused");
}
int main(int argc,char**argv){try{
 Check(argc==5,"fourshaderpaths");std::vector<unsigned char>code=File(argv[1]),native=File(argv[2]),vertex=File(argv[3]),terrain=File(argv[4]);Fixture f;Inputs i(f);AtlasWater::Layer layer;Hr(layer.Init(f.d.p,&code[0],code.size(),80ULL*1024*1024),"bounded layerinit");Com<ID3D11Texture2D>oracle;Texture(f,i.block,i.block,DXGI_FORMAT_R16G16B16A16_FLOAT,D3D11_BIND_RENDER_TARGET,oracle);
 for(unsigned test=0;test<8;++test){AtlasWater::Uniforms u;Values(u,i.side);if(test&1){u.waterSunDirection[1]=-.9f;u.waterSunColour[0]=u.waterSunColour[1]=u.waterSunColour[2]=0;}if(test&2)u.tileBounds[2]=u.tileBounds[3]=8192;if(test&4){u.sourceWindow[1]=256;u.sourceWindow[2]=256;u.waterTime[0]=13.456f;}
  i.ResetHdr(f);NativeDraw(f,i,u,native,vertex,terrain,oracle.p,false);Hr(layer.Render(u,i.views,i.samplers,i.texture[0].p,80ULL*1024*1024),"actual water compute");Compare(Readback(f,i.texture[0].p),Readback(f,oracle.p),"actual installed waterFP parity day/night/zoom/offset");
 }
 AtlasWater::Uniforms lit;Values(lit,i.side);Check(lit.waterSunColour[1]==1&&lit.waterSunColour[2]==.9f&&lit.waterEnvColour[1]==.9f&&lit.waterEnvColour[2]==1&&lit.waterEnvColour[3]==.7f,"fixture complete native sun/environment rows");lit.waterSunColour[0]=lit.waterSunColour[1]=lit.waterSunColour[2]=0;i.ResetHdr(f);Hr(layer.Render(lit,i.views,i.samplers,i.texture[0].p,80ULL*1024*1024),"native IBL only");std::vector<unsigned short>ibl=Readback(f,i.texture[0].p);lit.waterEnvColour[3]=0;i.ResetHdr(f);Hr(layer.Render(lit,i.views,i.samplers,i.texture[0].p,80ULL*1024*1024),"zero IBL only");std::vector<unsigned short>dark=Readback(f,i.texture[0].p);float iblEnergy=0;for(size_t k=0;k<ibl.size();++k)if(k%4!=3)iblEnergy+=std::fabs(Half(ibl[k])-Half(dark[k]));Check(iblEnergy>1,"IBL fixture actually contributes sampled native energy");
 AtlasWater::Uniforms u;Values(u,i.side);i.Height(f,true);
 for(unsigned t=0;t<3;++t){u.waterPlane[0]=t==0?100.0f:t==1?110.0f:-100000000.0f;i.ResetHdr(f);NativeDraw(f,i,u,native,vertex,terrain,oracle.p,true);Hr(layer.Render(u,i.views,i.samplers,i.texture[0].p,80ULL*1024*1024),"native waterY mask");Compare(Readback(f,i.texture[0].p),Readback(f,oracle.p),"native depth plane shoreline/currentY+10/sentinel");}
 i.Height(f,false);Values(u,i.side);i.Flow(f,.5f,.5f);i.ResetHdr(f);Hr(layer.Render(u,i.views,i.samplers,i.texture[0].p,80ULL*1024*1024),"stationary guard");std::vector<unsigned short>stationary=Readback(f,i.texture[0].p);i.Flow(f,.500001f,.500001f);NativeDraw(f,i,u,native,vertex,terrain,oracle.p,false);Compare(stationary,Readback(f,oracle.p),"stationary defined native small-velocity limit");
 StateTest(f,i,layer,u,code,vertex,native);
 BorderTest(f,layer,native,vertex,terrain);
 i.ResetHdr(f);u.waterPlane[1]=0;AtlasViewTest::ContextFixture offBefore(f.c.p);Check(layer.Render(u,NULL,NULL,NULL,0)==S_FALSE,"OFF requires no native inputs or context");AtlasViewTest::ContextFixture offAfter(f.c.p);Check(offBefore.Equal(offAfter)&&Readback(f,i.texture[0].p)==i.base,"OFF unchanged HDR and state");
 Values(u,i.side);u.materialParams[3]=8;i.ResetHdr(f);Hr(layer.Render(u,i.views,i.samplers,i.texture[0].p,80ULL*1024*1024),"native HDR glow");NativeDraw(f,i,u,native,vertex,terrain,oracle.p,false);std::vector<unsigned short>hdr=Readback(f,i.texture[0].p);Compare(hdr,Readback(f,oracle.p),"native HDR before exposure");bool over=false;for(size_t k=0;k<hdr.size();++k)if(k%4!=3&&Half(hdr[k])>1)over=true;Check(over,"HDR notclamped or toned");
 std::printf("Water WARP PASS %u checks; installed waterFP8material+3shoreline+stationary/HDR/OFF cases; maximum absolute HDR delta %.9g\n",checks,maximumDelta);return 0;
 }catch(...){return 1;}}
