#ifndef SURFACE_HEADER
#define SURFACE_HEADER "AtlasModelSurface.h"
#endif
#include SURFACE_HEADER
#include "../../atlas-view/ContextFixture.h"
#include <cstdio>
#include <cstdlib>
#include <new>
using AtlasView::Com;
static unsigned checks=0,allocations=0;static bool track=false;
void*operator new(size_t n){if(track)++allocations;void*p=std::malloc(n?n:1);if(!p)throw std::bad_alloc();return p;}void operator delete(void*p){std::free(p);}void*operator new[](size_t n){return ::operator new(n);}void operator delete[](void*p){::operator delete(p);}
static void Check(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}}
static void Hr(HRESULT h,const char*s){if(FAILED(h)){std::fprintf(stderr,"HRESULT %08x ",unsigned(h));Check(false,s);}}
struct Device {Com<ID3D11Device>d;Com<ID3D11DeviceContext>c;Device(){D3D_FEATURE_LEVEL l;Hr(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&d.p,&l,&c.p),"WARP");}};
static const DXGI_FORMAT Formats[3]={DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_R32_FLOAT};
static const unsigned Strides[3]={8,4,4};
static void Texture(Device&f,unsigned plane,D3D11_USAGE usage,unsigned side,unsigned mips,unsigned flags,unsigned misc,const void*data,Com<ID3D11Texture2D>&t,Com<ID3D11ShaderResourceView>&v){
 D3D11_TEXTURE2D_DESC d={};d.Width=d.Height=side;d.MipLevels=mips;d.ArraySize=1;d.Format=Formats[plane];d.SampleDesc.Count=1;d.Usage=usage;d.BindFlags=flags;d.MiscFlags=misc;D3D11_SUBRESOURCE_DATA s={};s.pSysMem=data;s.SysMemPitch=side*Strides[plane];Hr(f.d.p->CreateTexture2D(&d,data?&s:NULL,&t.p),"source texture");Hr(f.d.p->CreateShaderResourceView(t.p,NULL,&v.p),"source SRV");
}
struct Base {Com<ID3D11Texture2D>t[3];Com<ID3D11ShaderResourceView>v[3];ID3D11ShaderResourceView*views[3];std::vector<BYTE>bytes[3];Base(Device&f,bool immutable=true){
 for(unsigned p=0;p<3;++p){bytes[p].resize(65536*Strides[p]);for(unsigned i=0;i<bytes[p].size();++i)bytes[p][i]=BYTE((i*37+(i>>9)*13+p*71)&255);
 // Exact copying includes invalid-height NaN bits, signed zero, half HDR >1 and explicit coverage.
 if(p==0){unsigned short hdr[4]={0x4000,0x3c00,0x3800,0x3a00};std::memcpy(&bytes[p][0],hdr,8);}
 if(p==1)bytes[p][3]=0;
 if(p==2){unsigned nan=0x7fc12345;std::memcpy(&bytes[p][0],&nan,4);unsigned negZero=0x80000000;std::memcpy(&bytes[p][4],&negZero,4);}
 Texture(f,p,immutable?D3D11_USAGE_IMMUTABLE:D3D11_USAGE_DEFAULT,256,1,D3D11_BIND_SHADER_RESOURCE|(immutable?0:D3D11_BIND_RENDER_TARGET|D3D11_BIND_UNORDERED_ACCESS),0,&bytes[p][0],t[p],v[p]);views[p]=v[p].p;
 }}};
static std::vector<BYTE>Read(Device&f,ID3D11Texture2D*t,unsigned stride){D3D11_TEXTURE2D_DESC d;t->GetDesc(&d);d.BindFlags=d.MiscFlags=0;d.Usage=D3D11_USAGE_STAGING;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;Com<ID3D11Texture2D>s;Hr(f.d.p->CreateTexture2D(&d,NULL,&s.p),"staging");f.c.p->CopyResource(s.p,t);D3D11_MAPPED_SUBRESOURCE m;Hr(f.c.p->Map(s.p,0,D3D11_MAP_READ,0,&m),"map");std::vector<BYTE>b(65536*stride);for(unsigned y=0;y<256;++y)std::memcpy(&b[y*256*stride],static_cast<const BYTE*>(m.pData)+y*m.RowPitch,256*stride);f.c.p->Unmap(s.p,0);return b;}
static void Exact(Device&f,AtlasModelSurface::Layer&s,Base&b){for(unsigned p=0;p<3;++p){std::vector<BYTE>got=Read(f,s.Texture(p),Strides[p]);Check(got==b.bytes[p],"all bytes exact, including HDR/NaN/coverage/height");Check(Read(f,b.t[p].p,Strides[p])==b.bytes[p],"pristine base never mutated");}}
static void RefusedUnchanged(Device&f,AtlasModelSurface::Layer&s,ID3D11ShaderResourceView*const views[3],AtlasView::U64 bytes,const char*label){
 std::vector<BYTE>old[3];for(unsigned p=0;p<3;++p)old[p]=Read(f,s.Texture(p),Strides[p]);{AtlasViewTest::ContextFixture before(f.c.p);Check(FAILED(s.Copy(true,views,bytes)),label);AtlasViewTest::ContextFixture after(f.c.p);Check(before.Equal(after),"refusal preserves complete context");}for(unsigned p=0;p<3;++p)Check(Read(f,s.Texture(p),Strides[p])==old[p],"refusal publishes no partial plane copy");
}
static void Append(Device&f,Com<ID3D11Buffer>&b,Com<ID3D11UnorderedAccessView>&v){D3D11_BUFFER_DESC d={};d.ByteWidth=64;d.BindFlags=D3D11_BIND_UNORDERED_ACCESS;d.MiscFlags=D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;d.StructureByteStride=4;Hr(f.d.p->CreateBuffer(&d,NULL,&b.p),"append buffer");D3D11_UNORDERED_ACCESS_VIEW_DESC u={};u.ViewDimension=D3D11_UAV_DIMENSION_BUFFER;u.Buffer.NumElements=16;u.Buffer.Flags=D3D11_BUFFER_UAV_FLAG_APPEND;Hr(f.d.p->CreateUnorderedAccessView(b.p,&u,&v.p),"append UAV");}
static unsigned Counter(Device&f,ID3D11UnorderedAccessView*v){Com<ID3D11Buffer>b;D3D11_BUFFER_DESC d={};d.ByteWidth=4;d.Usage=D3D11_USAGE_STAGING;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;Hr(f.d.p->CreateBuffer(&d,NULL,&b.p),"counter staging");f.c.p->CopyStructureCount(b.p,0,v);D3D11_MAPPED_SUBRESOURCE m;Hr(f.c.p->Map(b.p,0,D3D11_MAP_READ,0,&m),"counter read");unsigned n=*static_cast<unsigned*>(m.pData);f.c.p->Unmap(b.p,0);return n;}
int main(){try{
 Device f;Base base(f);AtlasModelSurface::Layer s;
 Check(s.Copy(false,reinterpret_cast<ID3D11ShaderResourceView*const*>(1),~AtlasModelSurface::U64(0))==S_FALSE,"OFF Copy touches no pointers");Check(s.Init(false,reinterpret_cast<ID3D11Device*>(1),~AtlasModelSurface::U64(0))==S_FALSE,"OFF Init touches no pointers");
 Hr(s.Init(true,f.d.p,0),"surface Init");Hr(s.Copy(true,base.views,0),"copy pristine input");Exact(f,s,base);
 Check(AtlasModelSurface::PrivateBytes==1048576&&AtlasModelSurface::Fits(AtlasView::GpuCap-1048576)&&!AtlasModelSurface::Fits(AtlasView::GpuCap-1048575)&&!AtlasModelSurface::Fits(~AtlasModelSurface::U64(0)),"inclusive shared192MiB budget");
 Check(AtlasModelSurface::Fits(189006000ULL+3408256+526416+1048720+1048640)&&189006000ULL+3408256+526416+1048720+1048640+AtlasModelSurface::PrivateBytes==196086608ULL,"combined terrain water local raster decoder surface ledger");
 {D3D11_TEXTURE2D_DESC good;base.t[0].p->GetDesc(&good);Check(AtlasModelSurface::SourceShape(good,0),"actual immutable descriptor admitted");for(unsigned field=0;field<11;++field){D3D11_TEXTURE2D_DESC bad=good;switch(field){case 0:bad.Width=255;break;case 1:bad.Height=512;break;case 2:bad.MipLevels=2;break;case 3:bad.ArraySize=2;break;case 4:bad.Format=DXGI_FORMAT_R16G16B16A16_UNORM;break;case 5:bad.SampleDesc.Count=2;break;case 6:bad.SampleDesc.Quality=1;break;case 7:bad.Usage=D3D11_USAGE_DYNAMIC;break;case 8:bad.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;break;case 9:bad.MiscFlags=D3D11_RESOURCE_MISC_GENERATE_MIPS;break;default:bad.BindFlags=D3D11_BIND_VERTEX_BUFFER;break;}Check(!AtlasModelSurface::SourceShape(bad,0),"each wrong actual descriptor field refused");}Check(!AtlasModelSurface::SourceShape(good,3),"invalid plane index refused");}
 Check(!s.Texture(3)&&!s.Output(3),"invalid output index null");Check(FAILED(s.Init(true,f.d.p,0)),"double Init refused");
 for(unsigned p=0;p<3;++p){D3D11_TEXTURE2D_DESC d;s.Texture(p)->GetDesc(&d);D3D11_SHADER_RESOURCE_VIEW_DESC v;s.Output(p)->GetDesc(&v);Check(d.Width==256&&d.Height==256&&d.MipLevels==1&&d.ArraySize==1&&d.Format==Formats[p]&&d.SampleDesc.Count==1&&!d.SampleDesc.Quality&&d.Usage==D3D11_USAGE_DEFAULT&&d.BindFlags==D3D11_BIND_SHADER_RESOURCE&&!d.CPUAccessFlags&&!d.MiscFlags,"fixed exact output descriptor");Check(v.Format==Formats[p]&&v.ViewDimension==D3D11_SRV_DIMENSION_TEXTURE2D&&v.Texture2D.MostDetailedMip==0&&v.Texture2D.MipLevels==1,"exact SRV descriptor");Check(!AtlasView::Identity(s.Texture(p),base.t[p].p)&&AtlasView::SameResource(s.Output(p),s.Texture(p)),"owned surface distinct from borrowed immutable base");}
 // Replay starts from pristine input, removing an old model rather than retaining its roof.
 std::vector<BYTE>roof(65536*8,0x5a);f.c.p->UpdateSubresource(s.Texture(0),0,NULL,&roof[0],2048,0);Check(Read(f,s.Texture(0),8)!=base.bytes[0],"old model fixture differs");Hr(s.Copy(true,base.views,0),"next current-model replay resets old roof");Exact(f,s,base);
 ID3D11Texture2D*owned[3]={s.Texture(0),s.Texture(1),s.Texture(2)};for(unsigned i=0;i<4;++i)Hr(s.Copy(true,base.views,0),"reusable copy");for(unsigned i=0;i<3;++i)Check(owned[i]==s.Texture(i),"copy creates no new owned resources");
 RefusedUnchanged(f,s,base.views,AtlasView::GpuCap-1048575,"copy plus1 budget refused");RefusedUnchanged(f,s,NULL,0,"null array refused");
 ID3D11ShaderResourceView*bad[3]={base.views[0],base.views[1],NULL};RefusedUnchanged(f,s,bad,0,"missing LAST plane validated before first plane write");bad[2]=base.views[2];bad[0]=s.Output(0);RefusedUnchanged(f,s,bad,0,"own output cannot become pristine base");bad[0]=base.views[0];bad[1]=base.views[0];RefusedUnchanged(f,s,bad,0,"duplicate source resource refused");
 Device other;Base remote(other);RefusedUnchanged(f,s,remote.views,0,"other device source refused");
 Com<ID3D11Texture2D>smallTex;Com<ID3D11ShaderResourceView>smallView;Texture(f,2,D3D11_USAGE_DEFAULT,128,1,D3D11_BIND_SHADER_RESOURCE,0,NULL,smallTex,smallView);bad[1]=base.views[1];bad[2]=smallView.p;RefusedUnchanged(f,s,bad,0,"wrong actual dimensions refused");
 Com<ID3D11Texture2D>multi;Com<ID3D11ShaderResourceView>multiView;Texture(f,2,D3D11_USAGE_DEFAULT,256,2,D3D11_BIND_SHADER_RESOURCE,0,NULL,multi,multiView);bad[2]=multiView.p;RefusedUnchanged(f,s,bad,0,"extra mip chain refused");
 // A superficially exact one-mip SRV must not conceal a larger texture allocation.
 Com<ID3D11ShaderResourceView>oneMip;D3D11_SHADER_RESOURCE_VIEW_DESC only={};only.Format=Formats[2];only.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2D;only.Texture2D.MipLevels=1;Hr(f.d.p->CreateShaderResourceView(multi.p,&only,&oneMip.p),"one-mip view over multi-mip resource");bad[2]=oneMip.p;RefusedUnchanged(f,s,bad,0,"actual extra mips refused despite exact view");
 // An active false predicate must neither suppress refresh nor leak changed predication.
 Com<ID3D11Predicate>predicate;D3D11_QUERY_DESC q={D3D11_QUERY_OCCLUSION_PREDICATE,0};Hr(f.d.p->CreatePredicate(&q,&predicate.p),"predicate");f.c.p->Begin(predicate.p);f.c.p->End(predicate.p);f.c.p->Flush();BOOL result=TRUE;HRESULT ready=S_FALSE;for(unsigned retry=0;retry<10000&&ready==S_FALSE;++retry)ready=f.c.p->GetData(predicate.p,&result,sizeof(result),0);Check(ready==S_OK&&!result,"completed false predicate");
 f.c.p->UpdateSubresource(s.Texture(0),0,NULL,&roof[0],2048,0);
 f.c.p->VSSetShaderResources(127,1,&base.views[0]);ID3D11ShaderResourceView*read[3]={s.Output(0),s.Output(1),s.Output(2)};f.c.p->VSSetShaderResources(17,1,&read[0]);f.c.p->HSSetShaderResources(31,1,&read[1]);f.c.p->DSSetShaderResources(63,1,&read[2]);f.c.p->GSSetShaderResources(127,1,&read[0]);f.c.p->PSSetShaderResources(109,1,&read[1]);f.c.p->CSSetShaderResources(121,1,&read[2]);
 Com<ID3D11Buffer>csBuffer,omBuffer;Com<ID3D11UnorderedAccessView>csAppend,omAppend;Append(f,csBuffer,csAppend);Append(f,omBuffer,omAppend);UINT seven=7,eleven=11;f.c.p->CSSetUnorderedAccessViews(7,1,&csAppend.p,&seven);f.c.p->OMSetRenderTargetsAndUnorderedAccessViews(0,NULL,NULL,6,1,&omAppend.p,&eleven);f.c.p->SetPredication(predicate.p,FALSE);
 {AtlasViewTest::ContextFixture before(f.c.p);Hr(s.Copy(true,base.views,0),"stateful false-predicate copy");AtlasViewTest::ContextFixture after(f.c.p);Check(before.Equal(after),"all six stage reads, OM/UAV/predicate and all untouched state restored");}
 f.c.p->SetPredication(NULL,FALSE);Check(Counter(f,csAppend.p)==7&&Counter(f,omAppend.p)==11,"native CS/OM hidden append counters unchanged");f.c.p->ClearState();Exact(f,s,base);
 // Writable bindings are refused before copy; never unbind native UAV counters.
 Base writable(f,false);Com<ID3D11RenderTargetView>rt;Hr(f.d.p->CreateRenderTargetView(writable.t[0].p,NULL,&rt.p),"RT alias");f.c.p->OMSetRenderTargets(1,&rt.p,NULL);RefusedUnchanged(f,s,writable.views,0,"source RT hazard refused");f.c.p->ClearState();
 Com<ID3D11UnorderedAccessView>uav;Hr(f.d.p->CreateUnorderedAccessView(writable.t[1].p,NULL,&uav.p),"source UAV");f.c.p->CSSetUnorderedAccessViews(7,1,&uav.p,NULL);RefusedUnchanged(f,s,writable.views,0,"source CS UAV hazard refused");f.c.p->ClearState();f.c.p->OMSetRenderTargetsAndUnorderedAccessViews(0,NULL,NULL,7,1,&uav.p,NULL);RefusedUnchanged(f,s,writable.views,0,"source OM UAV hazard refused");f.c.p->ClearState();Hr(s.Copy(true,writable.views,0),"unbound DEFAULT source supported");Exact(f,s,writable);
 allocations=0;track=true;HRESULT warm=s.Copy(true,base.views,0);track=false;Hr(warm,"warm copy");Check(!allocations,"zero C++ allocations per Copy");Exact(f,s,base);
 // Source ownership is borrowed only during Copy. Clear releases owned views/device exactly once.
 ULONG before=base.v[0].p->AddRef();base.v[0].p->Release();Hr(s.Copy(true,base.views,0),"source borrow");ULONG after=base.v[0].p->AddRef();base.v[0].p->Release();Check(before==after,"no retained source SRV references");
 s.Clear();Check(!s.Texture(0)&&!s.Output(0),"Clear empties outputs");Check(FAILED(s.Copy(true,base.views,0)),"uninitialized Copy refused");s.Clear();
 before=f.d.p->AddRef();f.d.p->Release();for(unsigned i=0;i<4;++i){AtlasModelSurface::Layer repeat;Hr(repeat.Init(true,f.d.p,AtlasView::GpuCap-1048576),"exact budget Init");repeat.Clear();Check(FAILED(repeat.Init(true,f.d.p,AtlasView::GpuCap-1048575)),"Init plus1 refused");Check(!repeat.Texture(0),"failed Init leaves no outputs");}after=f.d.p->AddRef();f.d.p->Release();Check(before==after,"repeated lifecycle COM balance");
 std::printf("model surface WARP %u checks PASS\n",checks);return 0;
}catch(...){return 1;}}

