#include <d3d11.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <stdexcept>
#include MIP_COMPARISON_HEADER
#include "../../../GlobalSatelliteMap/GpuMipUpload.h"
template<class T> struct Com
{
    T* p;Com():p(NULL){}~Com(){if(p)p->Release();}
    T* operator->() const{return p;}
private:Com(const Com&);Com& operator=(const Com&);
};
static unsigned checks=0;
static void Check(bool ok,const char* name)
{++checks;if(!ok){std::fprintf(stderr,"FAIL: %s\n",name);throw std::runtime_error(name);}}
static void HR(HRESULT hr,const char* name){Check(SUCCEEDED(hr),name);}
static ULONG Refs(IUnknown* p){p->AddRef();return p->Release();}
class Watch : public IUnknown
{
    LONG refs;unsigned* count;
public:
    Watch(unsigned* value):refs(1),count(value){}
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** out)
    {if(!out)return E_POINTER;*out=NULL;if(id!=__uuidof(IUnknown))return E_NOINTERFACE;*out=this;AddRef();return S_OK;}
    virtual ULONG STDMETHODCALLTYPE AddRef(){return ULONG(InterlockedIncrement(&refs));}
    virtual ULONG STDMETHODCALLTYPE Release(){LONG value=InterlockedDecrement(&refs);if(!value){++*count;delete this;}return ULONG(value);}
};
static void Observe(ID3D11DeviceChild* child,unsigned* count)
{
    const GUID key={0x7f2a146d,0xc3a1,0x49a3,{0x84,0x29,0x46,0xb7,0x7a,0x94,0x80,0xea}};
    Watch* watch=new Watch(count);HRESULT hr=child->SetPrivateDataInterface(key,watch);watch->Release();HR(hr,"attach destruction witness");
}
struct Fixture
{
    Com<ID3D11Device> device;Com<ID3D11DeviceContext> context;
    Fixture(){HR(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&device.p,NULL,&context.p),"create WARP");}
};
struct Texture
{
    Com<ID3D11Texture2D> texture;Com<ID3D11ShaderResourceView> view;
    Texture(Fixture& f,unsigned side,unsigned levels,unsigned slices,DXGI_FORMAT format)
    {
        D3D11_TEXTURE2D_DESC d={};d.Width=d.Height=side;d.MipLevels=levels;d.ArraySize=slices;d.Format=format;
        d.SampleDesc.Count=1;d.Usage=D3D11_USAGE_DEFAULT;d.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;
        d.MiscFlags=D3D11_RESOURCE_MISC_GENERATE_MIPS;
        HR(f.device->CreateTexture2D(&d,NULL,&texture.p),"create source texture");
        HR(f.device->CreateShaderResourceView(texture.p,NULL,&view.p),"create full source view");
    }
};
static std::vector<unsigned char> Pixels(unsigned side,unsigned layer)
{
    std::vector<unsigned char> bytes(size_t(side)*side*4);
    for(unsigned y=0;y<side;++y)for(unsigned x=0;x<side;++x)for(unsigned c=0;c<4;++c)
        bytes[(size_t(y)*side+x)*4+c]=static_cast<unsigned char>(7+layer*47+c*9+((x*(c+1)+y*(4-c))%16)*3);
    return bytes;
}
static void Native(Fixture& f,Texture& native,const std::vector<unsigned char>& bytes,unsigned side)
{
    f.context->UpdateSubresource(native.texture.p,0,NULL,&bytes[0],side*4,0);f.context->GenerateMips(native.view.p);
}
static void WaitGPU(Fixture& f)
{
    D3D11_QUERY_DESC d={D3D11_QUERY_EVENT,0};Com<ID3D11Query> query;
    HR(f.device->CreateQuery(&d,&query.p),"create test-only drain query");f.context->End(query.p);f.context->Flush();
    DWORD started=GetTickCount();HRESULT hr;
    while((hr=f.context->GetData(query.p,NULL,0,0))==S_FALSE&&GetTickCount()-started<5000)Sleep(1);
    Check(hr==S_OK,"test-only GPU drain completes");
}
static ProbeMipComparison::State Run(Fixture& f,ProbeMipComparison::Comparison& comparison,unsigned long start)
{
    for(unsigned tick=0;tick<120000;++tick)
    {
        unsigned attempts=comparison.MapAttempts();
        ProbeMipComparison::State state=comparison.Tick(start+tick);
        Check(comparison.MapAttempts()-attempts<=1,"at most one nonblocking Map attempt per tick");
        if(state==ProbeMipComparison::Equal||state==ProbeMipComparison::Different||
           state==ProbeMipComparison::Failed||state==ProbeMipComparison::Cancelled)return state;
        // Test harness has no normal Present loop. Submission is external to
        // production comparison; the helper itself must never Flush/Present.
        f.context->Flush();Sleep(1);
    }
    throw std::runtime_error("headless WARP comparison did not terminate");
}
static void Pair(Fixture& f,unsigned side,unsigned levels,DXGI_FORMAT format)
{
    using namespace ProbeMipComparison;
    Texture array(f,side,levels,4,format),native(f,side,levels,1,format);
    std::vector<unsigned char> bytes[4];SatelliteGpuMip::Layer input[4];
    for(unsigned layer=0;layer<4;++layer){bytes[layer]=Pixels(side,layer);input[layer].data=&bytes[layer][0];input[layer].bytes=bytes[layer].size();}
    HR(SatelliteGpuMip::UploadFourLayerMipChain(array.view.p,side,side,levels,format,input),"populate array using reviewed production helper");
    for(unsigned layer=0;layer<4;++layer)
    {
        Native(f,native,bytes[layer],side);Comparison job;
        ULONG ar=Refs(array.texture.p),nr=Refs(native.texture.p),dr=Refs(f.device.p),cr=Refs(f.context.p);
        HR(job.Begin(native.view.p,array.view.p,side,levels,layer,format,100,120000),"begin matching source pair");
        Check(Run(f,job,100)==Equal,"all real GPU mip pixels/channels must match");
        Check(job.Count()==levels&&job.Readbacks()==levels*2,"every mip has both readbacks");
        Budget budget;Check(Plan(side,levels,0,budget)&&job.ComparedBytes()==budget.nativeBytes,"all source bytes compared");
        for(unsigned mip=0;mip<levels;++mip)
        {
            const LevelResult& r=job.Result(mip);
            Check(std::memcmp(r.nativeHash,r.arrayHash,32)==0,"per-mip SHA256 equality");
            for(unsigned c=0;c<4;++c)Check(!r.mismatches[c]&&!r.maxError[c]&&!r.sumError[c],"RGBA exact equality metrics");
        }
        if(side==8&&layer==0)
        {
            const unsigned char golden[32]={0x50,0xa5,0x8c,0x38,0x2d,0xf5,0xde,0x04,
                0xcc,0x7b,0x90,0x03,0x8e,0xeb,0xa7,0x7b,0xf8,0x4a,0x0c,0x25,0x0b,0xf0,0xf1,0xd1,
                0x41,0xcb,0xc1,0xaa,0x51,0x43,0xe5,0x08}; // Independent hashlib SHA of the raw fixture.
            Check(!std::memcmp(job.Result(0).nativeHash,golden,32),"known raw CPU fixture SHA256");
        }
        Check(ar==Refs(array.texture.p)&&nr==Refs(native.texture.p)&&dr==Refs(f.device.p)&&cr==Refs(f.context.p),"terminal success releases all COM refs");
    }
    std::printf("PASS: side%u DXGI%u, four layers and all%u mips\n",side,unsigned(format),levels);
}
static void MutatedInput(Fixture& f)
{
    using namespace ProbeMipComparison;const unsigned side=8,levels=4;
    Texture array(f,side,levels,4,DXGI_FORMAT_R8G8B8A8_UNORM),native(f,side,levels,1,DXGI_FORMAT_R8G8B8A8_UNORM);
    std::vector<unsigned char> bytes[4];SatelliteGpuMip::Layer input[4];
    for(unsigned l=0;l<4;++l){bytes[l]=Pixels(side,l);input[l].data=&bytes[l][0];input[l].bytes=bytes[l].size();}
    HR(SatelliteGpuMip::UploadFourLayerMipChain(array.view.p,side,side,levels,DXGI_FORMAT_R8G8B8A8_UNORM,input),"populate negative test array");
    for(unsigned channel=0;channel<4;++channel)
    {
        std::vector<unsigned char> altered=bytes[0];altered[4*11+channel]^=0x7f;
        Native(f,native,altered,side);Comparison job;
        HR(job.Begin(native.view.p,array.view.p,side,levels,0,DXGI_FORMAT_R8G8B8A8_UNORM,100,120000),"begin corrupted channel pair");
        Check(Run(f,job,100)==Different,"one-pixel RGB/alpha corruption must be detected");
        Check(job.Result(0).mismatches[channel]==1,"exact changed base-pixel/channel count");
        for(unsigned c=0;c<4;++c)if(c!=channel)Check(!job.Result(0).mismatches[c],"other base channels remain exact");
    }
    Native(f,native,bytes[0],side);Comparison wrongSlice;
    HR(wrongSlice.Begin(native.view.p,array.view.p,side,levels,1,DXGI_FORMAT_R8G8B8A8_UNORM,100,120000),"begin wrong slice");
    Check(Run(f,wrongSlice,100)==Different,"wrong array slice cannot falsely match");
    std::puts("PASS: wrong slice and individual RGB/alpha corruption detected");
}
static void ValidationAndLifetime(Fixture& f)
{
    using namespace ProbeMipComparison;
    Texture native(f,8,4,1,DXGI_FORMAT_R8G8B8A8_UNORM),array(f,8,4,4,DXGI_FORMAT_R8G8B8A8_UNORM);
    Comparison job;const ULONG nr=Refs(native.texture.p),ar=Refs(array.texture.p),dr=Refs(f.device.p),cr=Refs(f.context.p);
    Check(FAILED(job.Begin(NULL,array.view.p,8,4,0,DXGI_FORMAT_R8G8B8A8_UNORM,0,100)),"null source refused");
    Check(FAILED(job.Begin(native.view.p,array.view.p,8,4,4,DXGI_FORMAT_R8G8B8A8_UNORM,0,100)),"layer four refused");
    Check(FAILED(job.Begin(native.view.p,array.view.p,8,3,0,DXGI_FORMAT_R8G8B8A8_UNORM,0,100)),"partial chain refused");
    Check(FAILED(job.Begin(native.view.p,array.view.p,8,4,0,DXGI_FORMAT_B8G8R8A8_UNORM,0,100)),"source format mismatch refused");
    Check(FAILED(job.Begin(native.view.p,array.view.p,8,4,0,DXGI_FORMAT_R8G8B8A8_UNORM,0,0)),"zero deadline refused");
    Check(FAILED(job.Begin(native.view.p,array.view.p,8,4,0,DXGI_FORMAT_R8G8B8A8_UNORM,0,120001)),"unbounded deadline refused");
    Check(FAILED(job.Begin(native.view.p,array.view.p,8,4,0,DXGI_FORMAT_R8G8B8A8_UNORM,0,100,size_t(-1))),"retained budget overflow refused");
    Fixture other;Texture otherArray(other,8,4,4,DXGI_FORMAT_R8G8B8A8_UNORM);
    Check(FAILED(job.Begin(native.view.p,otherArray.view.p,8,4,0,DXGI_FORMAT_R8G8B8A8_UNORM,0,100)),"cross-device copies refused");
    Check(nr==Refs(native.texture.p)&&ar==Refs(array.texture.p)&&dr==Refs(f.device.p)&&cr==Refs(f.context.p),"all early rejection paths release source/device refs");
    D3D11_SHADER_RESOURCE_VIEW_DESC v={};v.Format=DXGI_FORMAT_R8G8B8A8_UNORM;v.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    v.Texture2DArray.MipLevels=3;v.Texture2DArray.ArraySize=4;Com<ID3D11ShaderResourceView> partial;
    HR(f.device->CreateShaderResourceView(array.texture.p,&v,&partial.p),"create partial view");
    const ULONG partialAr=Refs(array.texture.p),partialDr=Refs(f.device.p);
    Check(FAILED(job.Begin(native.view.p,partial.p,8,4,0,DXGI_FORMAT_R8G8B8A8_UNORM,0,100)),"partial view refused before copies");
    Check(nr==Refs(native.texture.p)&&partialAr==Refs(array.texture.p)&&partialDr==Refs(f.device.p)&&cr==Refs(f.context.p),"rejected begins retain no new COM references");
    for(unsigned stage=0;stage<4;++stage)
    {
        HR(job.Begin(native.view.p,array.view.p,8,4,0,DXGI_FORMAT_R8G8B8A8_UNORM,0xfffffff0UL,100),"start cancel/deadline case");
        Check(FAILED(job.Begin(native.view.p,array.view.p,8,4,0,DXGI_FORMAT_R8G8B8A8_UNORM,0,100)),"active job cannot be overwritten");
        for(unsigned step=0;step<stage;++step){job.Tick(0xfffffff1UL+step);f.context->Flush();Sleep(1);}
        job.Cancel();job.Cancel();Check(job.Current()==Cancelled&&job.Tick(1)==Cancelled,"cancel idempotent at every copy/map state");
        Check(nr==Refs(native.texture.p)&&partialAr==Refs(array.texture.p)&&partialDr==Refs(f.device.p)&&cr==Refs(f.context.p),"cancel releases source/device/context refs");
    }
    HR(job.Begin(native.view.p,array.view.p,8,4,0,DXGI_FORMAT_R8G8B8A8_UNORM,0xfffffff0UL,32),"start wrapped deadline");
    Check(job.Tick(0x10UL)==Failed&&job.Error()==HRESULT_FROM_WIN32(ERROR_TIMEOUT),"deadline remains bounded across DWORD wrap");
    Check(!job.Count()&&!job.Result(99).width,"timeout/invalid result cannot publish success");
    for(unsigned iteration=0;iteration<16;++iteration)
    {
        unsigned destroyed=0;
        {
            Texture n(f,8,4,1,DXGI_FORMAT_R8G8B8A8_UNORM),a(f,8,4,4,DXGI_FORMAT_R8G8B8A8_UNORM);
            Observe(n.texture.p,&destroyed);Observe(n.view.p,&destroyed);Observe(a.texture.p,&destroyed);Observe(a.view.p,&destroyed);
            Comparison pending;HR(pending.Begin(n.view.p,a.view.p,8,4,0,DXGI_FORMAT_R8G8B8A8_UNORM,0,100),"start destruction cycle");
            if(iteration&1)pending.Tick(1); // Destruction cancels with a queued copy too.
        }
        WaitGPU(f);Check(destroyed==4,"destructor releases every real source/view even with pending copy");
    }
    std::puts("PASS: validation, cross-device refusal, cancellation, DWORD-wrap deadline and real COM destruction");
}
static void Pipeline(Fixture& f)
{
    using namespace ProbeMipComparison;
    Texture native(f,8,4,1,DXGI_FORMAT_R8G8B8A8_UNORM),array(f,8,4,4,DXGI_FORMAT_R8G8B8A8_UNORM),sentinel(f,8,4,1,DXGI_FORMAT_R8G8B8A8_UNORM);
    Com<ID3D11RenderTargetView> target;HR(f.device->CreateRenderTargetView(sentinel.texture.p,NULL,&target.p),"create bound sentinel RTV");
    f.context->OMSetRenderTargets(1,&target.p,NULL);f.context->PSSetShaderResources(3,1,&array.view.p);
    D3D11_VIEWPORT viewport={1,3,7,5,.2f,.8f};f.context->RSSetViewports(1,&viewport);
    f.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
    std::vector<unsigned char> source=Pixels(8,0);Native(f,native,source,8);
    SatelliteGpuMip::Layer layers[4];for(unsigned l=0;l<4;++l){layers[l].data=&source[0];layers[l].bytes=source.size();}
    HR(SatelliteGpuMip::UploadFourLayerMipChain(array.view.p,8,8,4,DXGI_FORMAT_R8G8B8A8_UNORM,layers),"populate bound sentinel source");
    Comparison job;HR(job.Begin(native.view.p,array.view.p,8,4,0,DXGI_FORMAT_R8G8B8A8_UNORM,0,120000),"begin with live pipeline");
    Check(Run(f,job,0)==Equal,"comparison works with unrelated live pipeline");
    Com<ID3D11RenderTargetView> gotTarget;f.context->OMGetRenderTargets(1,&gotTarget.p,NULL);Check(gotTarget.p==target.p,"OM targets preserved");
    Com<ID3D11ShaderResourceView> gotView;f.context->PSGetShaderResources(3,1,&gotView.p);Check(gotView.p==array.view.p,"SRV slots preserved");
    D3D11_VIEWPORT got={};UINT count=1;f.context->RSGetViewports(&count,&got);
    Check(count==1&&!std::memcmp(&viewport,&got,sizeof(got)),"viewport preserved");
    D3D11_PRIMITIVE_TOPOLOGY topology;f.context->IAGetPrimitiveTopology(&topology);Check(topology==D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,"IA topology preserved");
    f.context->ClearState();std::puts("PASS: comparison preserves OM/SRV/viewport/IA state");
}
static void Policy()
{
    using namespace ProbeMipComparison;Budget b;
    Check(Plan(1024,11,0,b)&&b.totalBytes==32156324,"overlay inclusive native/array/staging budget");
    Check(Plan(2048,12,0,b)&&b.totalBytes==128625316,"colour inclusive native/array/staging budget");
    Check(!Plan(2048,12,22369616,b),"second resident array exceeds inclusive cap");
    Check(!Plan(2048,11,0,b)&&!Plan(4096,13,0,b)&&!Plan(0,0,0,b),"invalid shape/full-chain plans refused");
    Check(!Plan(2048,12,size_t(-1),b),"overflowing retained bytes refused");
    Schedule s;Check(!s.Begin(false,false,true),"default-off diagnostic cannot start");
    Check(!s.Begin(true,true,true),"normal capture exclusion");
    Check(!s.Begin(true,false,false),"unsafe lifecycle exclusion");
    Check(s.Begin(true,false,true),"explicit isolated safe job starts");
    for(unsigned kind=0;kind<2;++kind)for(unsigned layer=0;layer<4;++layer)
    {
        Check(s.kind==kind&&s.layer==layer,"bounded eight-pair order");
        Check(!s.PairComplete(1)&&s.active,"partial mip chain cannot advance pair");
        Check(s.PairComplete(kind?12:11),"one complete pair advances");
    }
    Check(!s.active&&s.complete&&s.comparisons==92,"all eight pairs yield exactly92 comparisons");
    Check(!s.PairComplete(12),"completed schedule cannot advance again");
    Check(s.Begin(true,false,true),"explicit next job allowed after completion");s.Cancel();
    Check(!s.active&&!s.complete&&!s.PairComplete(11),"cancelled schedule cannot publish success");
}
int main()
{
    try
    {
        Fixture f;Pair(f,8,4,DXGI_FORMAT_R8G8B8A8_UNORM);Pair(f,8,4,DXGI_FORMAT_B8G8R8A8_UNORM);
        MutatedInput(f);Policy();ValidationAndLifetime(f);Pipeline(f);
        Pair(f,1024,11,DXGI_FORMAT_R8G8B8A8_UNORM);Pair(f,2048,12,DXGI_FORMAT_R8G8B8A8_UNORM);
        Pair(f,1024,11,DXGI_FORMAT_B8G8R8A8_UNORM);Pair(f,2048,12,DXGI_FORMAT_B8G8R8A8_UNORM);
        std::printf("PASS: %u bounded real WARP comparison checks\n",checks);return 0;
    }
    catch(const std::exception& e){std::fprintf(stderr,"mip_comparison_warp: %s\n",e.what());return 1;}
}
