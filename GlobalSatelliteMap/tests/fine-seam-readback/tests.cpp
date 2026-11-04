#ifdef TEST_FAULTS
#include <d3d11.h>
HRESULT TestQuery(ID3D11DeviceContext*,ID3D11Query*,void*,UINT,UINT);
HRESULT TestMap(ID3D11DeviceContext*,ID3D11Resource*,UINT,D3D11_MAP,UINT,D3D11_MAPPED_SUBRESOURCE*);
void TestUnmap(ID3D11DeviceContext*,ID3D11Resource*,UINT);
BYTE* TestAllocate();
HRESULT TestCreate(ID3D11Device*,const D3D11_TEXTURE2D_DESC*,ID3D11Texture2D**);
HRESULT TestCreateQuery(ID3D11Device*,const D3D11_QUERY_DESC*,ID3D11Query**);
#endif
#ifndef SEAM_READBACK_HEADER
#define SEAM_READBACK_HEADER "../../GsmFineSeamReadback.h"
#endif
#include SEAM_READBACK_HEADER
#include "../../atlas-view/ContextFixture.h"
#include <cstdio>
#include <vector>
#include <cstdlib>
using AtlasView::Com;
static unsigned checks=0;static void C(bool v,const char*s){++checks;if(!v){std::printf("FAIL %s\n",s);std::exit(1);}}
static void H(HRESULT hr,const char*s){if(FAILED(hr)){std::printf("HRESULT %08x %s\n",unsigned(hr),s);std::exit(1);}}
struct Device {Com<ID3D11Device>d;Com<ID3D11DeviceContext>c;Device(){D3D_FEATURE_LEVEL level;H(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&d.p,&level,&c.p),"WARP");}};
static void Image(Device&d,unsigned side,DXGI_FORMAT format,const BYTE*data,Com<ID3D11Texture2D>&out){D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=side;td.MipLevels=td.ArraySize=td.SampleDesc.Count=1;td.Format=format;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;D3D11_SUBRESOURCE_DATA sd={};sd.pSysMem=data;sd.SysMemPitch=side*4;H(d.d.p->CreateTexture2D(&td,data?&sd:NULL,&out.p),"source texture");}
static HRESULT Finish(Device&d,GsmFineSeamReadback::Request&r,GsmFineSeamReadback::Blob&b){d.c.p->Flush();HRESULT hr=S_FALSE;for(unsigned n=0;n<10000&&hr==S_FALSE;++n){hr=r.Poll(b,0);Sleep(0);}return hr;}
static ULONG Refs(IUnknown*p){p->AddRef();return p->Release();}
#ifdef TEST_FAULTS
static unsigned queryMode=0,mapMode=0,mapCalls=0,unmaps=0,createMode=0,createCalls=0;static bool allocFail=false;static ID3D11Texture2D*createdCapture=NULL;
HRESULT TestQuery(ID3D11DeviceContext*c,ID3D11Query*q,void*p,UINT n,UINT flags){if(flags!=D3D11_ASYNC_GETDATA_DONOTFLUSH)std::exit(90);if(queryMode==1)return S_FALSE;if(queryMode==2)return E_FAIL;return c->GetData(q,p,n,flags);}
HRESULT TestMap(ID3D11DeviceContext*c,ID3D11Resource*r,UINT sub,D3D11_MAP mode,UINT flags,D3D11_MAPPED_SUBRESOURCE*out){if(mode!=D3D11_MAP_READ||flags!=D3D11_MAP_FLAG_DO_NOT_WAIT)std::exit(91);++mapCalls;if(mapCalls==2&&mapMode)return mapMode==1?DXGI_ERROR_WAS_STILL_DRAWING:E_FAIL;return c->Map(r,sub,mode,flags,out);}
void TestUnmap(ID3D11DeviceContext*c,ID3D11Resource*r,UINT sub){++unmaps;c->Unmap(r,sub);}
BYTE* TestAllocate(){return allocFail?NULL:new(std::nothrow)BYTE[GsmFineSeamReadback::PayloadBytes];}
HRESULT TestCreate(ID3D11Device*d,const D3D11_TEXTURE2D_DESC*desc,ID3D11Texture2D**out){++createCalls;if(createMode==1&&createCalls==2)return E_OUTOFMEMORY;HRESULT hr=d->CreateTexture2D(desc,NULL,out);if(SUCCEEDED(hr)&&createMode&&createCalls==1){createdCapture=*out;createdCapture->AddRef();}return hr;}
HRESULT TestCreateQuery(ID3D11Device*d,const D3D11_QUERY_DESC*desc,ID3D11Query**out){return createMode==2?E_OUTOFMEMORY:d->CreateQuery(desc,out);}
#endif
int main(){Device d;std::vector<BYTE>a(262144),b(262144);for(unsigned z=0;z<256;++z)for(unsigned x=0;x<256;++x){unsigned p=(z*256+x)*4;for(unsigned plane=0;plane<2;++plane){std::vector<BYTE>&v=plane?b:a;unsigned globalZ=z+plane*256;v[p]=BYTE(globalZ/2);v[p+1]=BYTE(x);v[p+2]=BYTE((globalZ+x)/4);v[p+3]=255;}}
 Com<ID3D11Texture2D>source[2];Image(d,256,DXGI_FORMAT_R8G8B8A8_UNORM,&a[0],source[0]);Image(d,256,DXGI_FORMAT_R8G8B8A8_UNORM,&b[0],source[1]);GsmFineSeamReadback::Request request;GsmFineSeamReadback::Blob result;
 Com<ID3D11Predicate>predicate;D3D11_QUERY_DESC pd={D3D11_QUERY_OCCLUSION_PREDICATE,0};H(d.d.p->CreatePredicate(&pd,&predicate.p),"predicate");d.c.p->Begin(predicate.p);d.c.p->End(predicate.p);d.c.p->SetPredication(predicate.p,TRUE);
 D3D11_VIEWPORT vp={17,19,128,121,0.1f,0.8f};d.c.p->RSSetViewports(1,&vp);D3D11_RECT rect={3,5,77,88};d.c.p->RSSetScissorRects(1,&rect);d.c.p->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);Com<ID3D11ShaderResourceView>srv;H(d.d.p->CreateShaderResourceView(source[0].p,NULL,&srv.p),"SRV");d.c.p->PSSetShaderResources(7,1,&srv.p);
 AtlasViewTest::ContextFixture before(d.c.p);ULONG refs=Refs(source[1].p);
 C(request.Begin(d.d.p,source[0].p,source[1].p,0,0)==S_OK,"actual two-texture async readback begins");C(request.Active()&&request.GpuBytes()==524288,"two staging surfaces accounted");C(FAILED(request.Begin(d.d.p,source[0].p,source[1].p,0,0)),"one request at a time");
 C(Refs(source[1].p)==refs+1&&request.PinnedSourceBytes()==524288,"source reference and explicit pinned bytes retained until known event completion");{AtlasViewTest::ContextFixture after(d.c.p);C(before.Equal(after),"Begin preserves complete immediate pipeline including predication");}
 HRESULT hr=request.Poll(result,0);C(hr==S_FALSE||hr==S_OK,"first nonflushing poll is bounded");if(hr==S_FALSE){C(!result.OwnedBytes(),"pending publishes no CPU blob");d.c.p->Flush();for(unsigned n=0;n<10000&&hr==S_FALSE;++n){hr=request.Poll(result,0);Sleep(0);}}
 C(hr==S_OK&&result.Data(0)&&result.Data(1)&&!result.Data(2),"completed read publishes two independent planes");C(!std::memcmp(result.Data(0),&a[0],a.size())&&!std::memcmp(result.Data(1),&b[0],b.size()),"actual WARP bytes match distinct continuous source planes");
 C(result.Data(1)[0]==result.Data(0)[255*256*4]+1,"adjacent final RGBA8 edge is continuous");C(Refs(source[1].p)==refs&&!request.PinnedSourceBytes(),"known completion releases source pin and its debt");{AtlasViewTest::ContextFixture after(d.c.p);C(before.Equal(after),"Poll preserves complete immediate pipeline");}
 C(request.Poll(result,0)==E_INVALIDARG,"no repeat completion or overwrite");request.Clear();C(!request.GpuBytes()&&!request.Active()&&result.Data(0)[33]==a[33],"explicit Clear preserves independent completed blob");result.Clear();C(!result.OwnedBytes(),"blob clear releases payload");
 C(request.Poll(result,0)==E_INVALIDARG,"idle poll refused");C(request.Begin(d.d.p,NULL,source[1].p,0,0)==E_INVALIDARG,"null input refused");C(request.Begin(d.d.p,source[0].p,source[0].p,0,0)==E_INVALIDARG,"alias input refused");
 Com<ID3D11Texture2D>bad;Image(d,128,DXGI_FORMAT_R8G8B8A8_UNORM,NULL,bad);C(request.Begin(d.d.p,bad.p,source[1].p,0,0)==E_INVALIDARG,"wrong dimensions refused");bad.Reset();Image(d,256,DXGI_FORMAT_B8G8R8A8_UNORM,NULL,bad);C(request.Begin(d.d.p,bad.p,source[1].p,0,0)==E_INVALIDARG,"wrong format refused");Device other;Com<ID3D11Texture2D>foreign;Image(other,256,DXGI_FORMAT_R8G8B8A8_UNORM,NULL,foreign);C(request.Begin(d.d.p,foreign.p,source[1].p,0,0)==E_INVALIDARG,"foreign device refused");
 C(request.Begin(d.d.p,source[0].p,source[1].p,AtlasView::GpuCap-524288+1,0)==E_OUTOFMEMORY,"GPU aggregate exact plus one refused");C(request.Begin(d.d.p,source[0].p,source[1].p,0,AtlasView::CpuCap-524288-sizeof(request)+1)==E_OUTOFMEMORY,"CPU aggregate exact plus one refused");C(!request.Active()&&!request.GpuBytes(),"refusals allocate no retained surfaces");
 H(request.Begin(d.d.p,source[0].p,source[1].p,AtlasView::GpuCap-524288,AtlasView::CpuCap-524288-sizeof(request)),"inclusive boundary");d.c.p->Flush();hr=S_FALSE;for(unsigned n=0;n<10000&&hr==S_FALSE;++n){hr=request.Poll(result,AtlasView::CpuCap-request.CpuBytes()-sizeof(result)-524288+1);Sleep(0);}C(hr==E_OUTOFMEMORY&&request.Ready()&&!request.Failed()&&!result.OwnedBytes(),"completed query with no CPU room retains retryable staging");C(Finish(d,request,result)==S_OK,"CPU pressure retry completes same request");request.Clear();result.Clear();
#ifdef TEST_FAULTS
 for(unsigned failure=1;failure<=2;++failure){createMode=failure;createCalls=0;C(request.Begin(d.d.p,source[0].p,source[1].p,0,0)==E_OUTOFMEMORY,"partial staging or query construction fault");C(!request.GpuBytes()&&!request.Active()&&createdCapture&&Refs(createdCapture)==1,"partial initialization releases every staging reference");createdCapture->Release();createdCapture=NULL;}createMode=0;
 queryMode=1;H(request.Begin(d.d.p,source[0].p,source[1].p,0,0),"pending begin");C(request.Poll(result,0)==S_FALSE&&!result.Data(0)&&request.Active(),"forced pending makes no map or publication");queryMode=2;C(request.Poll(result,0)==E_FAIL&&request.Failed()&&request.GpuBytes()==524288,"unknown query failure retains charged staging");C(Refs(source[1].p)==refs+1&&request.PinnedSourceBytes()==524288&&request.Poll(result,0)==E_FAIL,"unknown fence retains source pin, explicit debt and error latch");request.Clear();C(Refs(source[1].p)==refs&&!request.PinnedSourceBytes(),"explicit final teardown drops pin and debt");queryMode=0;
 for(unsigned fault=1;fault<=2;++fault){mapMode=fault;mapCalls=unmaps=0;H(request.Begin(d.d.p,source[0].p,source[1].p,0,0),"partial map begin");hr=Finish(d,request,result);C(hr==(fault==1?S_OK:E_FAIL),"partial map fault follows transient or terminal path");if(fault==1){C(hr==S_OK,"transient second-map retry completes");C(unmaps==mapCalls-1,"transient map leaves no first-plane mapping");}else C(unmaps==1&&request.Failed()&&!result.Data(0),"second-map failure unmaps first plane and publishes nothing");request.Clear();result.Clear();}mapMode=0;
 allocFail=true;mapCalls=unmaps=0;H(request.Begin(d.d.p,source[0].p,source[1].p,0,0),"OOM begin");C(Finish(d,request,result)==E_OUTOFMEMORY&&request.Ready()&&!request.Failed()&&unmaps==2,"payload OOM unmaps both and stays retryable");allocFail=false;C(Finish(d,request,result)==S_OK,"payload OOM retry preserves source pixels");request.Clear();result.Clear();
#endif
 d.c.p->ClearState();std::printf("PASS %u async seam-readback controls\n",checks);}
