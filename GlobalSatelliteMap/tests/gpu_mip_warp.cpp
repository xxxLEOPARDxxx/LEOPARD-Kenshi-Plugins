// Real GPU regression: catches omitted mip generation, wrong array subresources,
// channel/alpha corruption, partial validation writes and retained COM ownership.
#include <d3d11.h>
#include <algorithm>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include GPU_MIP_HEADER

template<class T> struct Com
{
    T* p;
    Com():p(NULL){}
    ~Com(){if(p)p->Release();}
    T* operator->() const{return p;}
private:
    Com(const Com&); Com& operator=(const Com&);
};
static unsigned checks=0;
static void Check(bool ok,const char* message)
{
    ++checks;
    if(!ok){std::fprintf(stderr,"FAIL: %s (check %u)\n",message,checks);throw std::runtime_error(message);}
}
static void HR(HRESULT hr,const char* message){Check(SUCCEEDED(hr),message);}
static ULONG Refs(IUnknown* p){p->AddRef();return p->Release();}

// Real D3D child objects retain this private-data interface. Destruction of the
// interface proves the resource/view actually released their final ownership,
// rather than merely comparing an arbitrary implementation refcount value.
class DestructionWatch : public IUnknown
{
    LONG refs;
    unsigned* destroyed;
public:
    DestructionWatch(unsigned* count):refs(1),destroyed(count){}
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void** result)
    {
        if(!result)return E_POINTER;*result=NULL;
        if(id!=__uuidof(IUnknown))return E_NOINTERFACE;
        *result=static_cast<IUnknown*>(this);AddRef();return S_OK;
    }
    virtual ULONG STDMETHODCALLTYPE AddRef(){return ULONG(InterlockedIncrement(&refs));}
    virtual ULONG STDMETHODCALLTYPE Release()
    {
        LONG remaining=InterlockedDecrement(&refs);
        if(!remaining){++*destroyed;delete this;}return ULONG(remaining);
    }
};
static void Watch(ID3D11DeviceChild* object,unsigned* count)
{
    const GUID id={0x9bc1abc3,0x271e,0x4ed0,{0x92,0x76,0x7c,0xe1,0x24,0x5a,0xf1,0x0d}};
    DestructionWatch* watch=new DestructionWatch(count);
    HRESULT hr=object->SetPrivateDataInterface(id,watch);watch->Release();HR(hr,"attach destruction witness");
}

struct Fixture
{
    Com<ID3D11Device> device;
    Com<ID3D11DeviceContext> context;
    Fixture()
    {
        D3D_FEATURE_LEVEL actual;
        HR(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,
            D3D11_SDK_VERSION,&device.p,&actual,&context.p),"create WARP device");
        Check(actual>=D3D_FEATURE_LEVEL_10_0,"WARP feature level");
    }
};
static D3D11_TEXTURE2D_DESC Desc(DXGI_FORMAT format)
{
    D3D11_TEXTURE2D_DESC d={};d.Width=8;d.Height=8;d.MipLevels=4;
    d.ArraySize=4;d.Format=format;d.SampleDesc.Count=1;
    d.Usage=D3D11_USAGE_DEFAULT;d.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;
    d.MiscFlags=D3D11_RESOURCE_MISC_GENERATE_MIPS;return d;
}
static D3D11_SHADER_RESOURCE_VIEW_DESC ViewDesc(const D3D11_TEXTURE2D_DESC& d)
{
    D3D11_SHADER_RESOURCE_VIEW_DESC v={};v.Format=d.Format;v.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    v.Texture2DArray.MipLevels=d.MipLevels;v.Texture2DArray.ArraySize=d.ArraySize;return v;
}
struct Array
{
    Com<ID3D11Texture2D> texture;
    Com<ID3D11ShaderResourceView> view;
    Array(Fixture& f,const D3D11_TEXTURE2D_DESC& d,const D3D11_SHADER_RESOURCE_VIEW_DESC* v=NULL)
    {
        HR(f.device->CreateTexture2D(&d,NULL,&texture.p),"create test array");
        D3D11_SHADER_RESOURCE_VIEW_DESC own=ViewDesc(d);
        HR(f.device->CreateShaderResourceView(texture.p,v?v:&own,&view.p),"create test view");
    }
};
struct Inputs
{
    std::vector<unsigned char> data[4];
    SatelliteGpuMip::Layer layers[4];
    Inputs(unsigned iteration=0)
    {
        for(unsigned layer=0;layer<4;++layer)
        {
            data[layer].resize(8*8*4);
            for(unsigned y=0;y<8;++y)for(unsigned x=0;x<8;++x)for(unsigned c=0;c<4;++c)
                data[layer][(y*8+x)*4+c]=static_cast<unsigned char>(
                    8+layer*40+c*5+iteration*3+((x*(c+1)+y*(4-c))%8)*4);
            layers[layer].data=&data[layer][0];layers[layer].bytes=data[layer].size();
        }
    }
};
static std::vector<unsigned char> Read(Fixture& f,ID3D11Texture2D* source)
{
    D3D11_TEXTURE2D_DESC d;source->GetDesc(&d);
    d.Usage=D3D11_USAGE_STAGING;d.BindFlags=0;d.MiscFlags=0;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    Com<ID3D11Texture2D> staging;HR(f.device->CreateTexture2D(&d,NULL,&staging.p),"create staging");
    f.context->CopyResource(staging.p,source);
    std::vector<unsigned char> all;
    for(unsigned layer=0;layer<d.ArraySize;++layer)for(unsigned mip=0;mip<d.MipLevels;++mip)
    {
        unsigned w=(std::max)(1u,d.Width>>mip),h=(std::max)(1u,d.Height>>mip);
        D3D11_MAPPED_SUBRESOURCE mapped={};
        HR(f.context->Map(staging.p,layer*d.MipLevels+mip,D3D11_MAP_READ,0,&mapped),"map mip readback");
        for(unsigned y=0;y<h;++y)
        {
            const unsigned char* row=static_cast<const unsigned char*>(mapped.pData)+mapped.RowPitch*y;
            all.insert(all.end(),row,row+w*4);
        }
        f.context->Unmap(staging.p,layer*d.MipLevels+mip);
    }
    return all;
}
static void VerifyPixels(Fixture& f,Array& a,const Inputs& in)
{
    const std::vector<unsigned char> actual=Read(f,a.texture.p);size_t at=0;
    for(unsigned layer=0;layer<4;++layer)for(unsigned mip=0;mip<4;++mip)
    {
        unsigned side=8>>mip,block=1<<mip;
        for(unsigned y=0;y<side;++y)for(unsigned x=0;x<side;++x)for(unsigned c=0;c<4;++c)
        {
            // Independent reference: exact area mean from original bytes, no helper filtering.
            unsigned sum=0;
            for(unsigned by=0;by<block;++by)for(unsigned bx=0;bx<block;++bx)
                sum+=in.data[layer][((y*block+by)*8+x*block+bx)*4+c];
            int want=int((sum+block*block/2)/(block*block));
            int delta=int(actual[at++])-want;
            if(delta<0)delta=-delta;
            if(delta>int(mip?1:0))
            {
                std::fprintf(stderr,"layer=%u mip=%u x=%u y=%u channel=%u actual=%u want=%d\n",
                    layer,mip,x,y,c,unsigned(actual[at-1]),want);
                Check(false,"GPU mip pixel/alpha mismatch");
            }
            ++checks;
        }
    }
    Check(at==actual.size(),"all four layers/all mips checked");
}
static void Seed(Fixture& f,Array& a)
{
    D3D11_TEXTURE2D_DESC d;a.texture->GetDesc(&d);
    std::vector<unsigned char> sentinel(d.Width*d.Height*4,211);
    for(unsigned layer=0;layer<d.ArraySize;++layer)for(unsigned mip=0;mip<d.MipLevels;++mip)
        f.context->UpdateSubresource(a.texture.p,layer*d.MipLevels+mip,NULL,&sentinel[0],
            (std::max)(1u,d.Width>>mip)*4,0);
}
static HRESULT Upload(Array& a,Inputs& in,unsigned w=8,unsigned h=8,unsigned mips=4,
    DXGI_FORMAT format=DXGI_FORMAT_R8G8B8A8_UNORM)
{
    return SatelliteGpuMip::UploadFourLayerMipChain(a.view.p,w,h,mips,format,in.layers);
}
static void RejectedUnchanged(Fixture& f,Array& a,Inputs& in,const char* name,
    unsigned w=8,unsigned h=8,unsigned mips=4,DXGI_FORMAT format=DXGI_FORMAT_R8G8B8A8_UNORM)
{
    Seed(f,a);const std::vector<unsigned char> before=Read(f,a.texture.p);
    ULONG tr=Refs(a.texture.p),vr=Refs(a.view.p),dr=Refs(f.device.p),cr=Refs(f.context.p);
    Check(FAILED(Upload(a,in,w,h,mips,format)),name);
    Check(before==Read(f,a.texture.p),"rejected call must not write any subresource");
    Check(tr==Refs(a.texture.p)&&vr==Refs(a.view.p)&&dr==Refs(f.device.p)&&cr==Refs(f.context.p),
        "failure releases acquired COM refs and keeps borrowed view");
}
static void BadInputs(Fixture& f)
{
    Inputs in;Array normal(f,Desc(DXGI_FORMAT_R8G8B8A8_UNORM));
    Check(FAILED(SatelliteGpuMip::UploadFourLayerMipChain(NULL,8,8,4,DXGI_FORMAT_R8G8B8A8_UNORM,in.layers)),"null borrowed view");
    RejectedUnchanged(f,normal,in,"wrong width",7);
    RejectedUnchanged(f,normal,in,"wrong height",8,7);
    RejectedUnchanged(f,normal,in,"zero dimension",0);
    RejectedUnchanged(f,normal,in,"huge dimension",~0u);
    RejectedUnchanged(f,normal,in,"not a full chain",8,8,3);
    RejectedUnchanged(f,normal,in,"zero mip count",8,8,0);
    RejectedUnchanged(f,normal,in,"huge mip count",8,8,~0u);
    RejectedUnchanged(f,normal,in,"wrong accepted format",8,8,4,DXGI_FORMAT_B8G8R8A8_UNORM);
    RejectedUnchanged(f,normal,in,"unsupported/sRGB format",8,8,4,DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
    RejectedUnchanged(f,normal,in,"unknown format",8,8,4,DXGI_FORMAT_UNKNOWN);
    for(unsigned layer=0;layer<4;++layer)
    {
        in.layers[layer].bytes=255;RejectedUnchanged(f,normal,in,"each layer bounds checked before writes");
        in.layers[layer].bytes=256;in.layers[layer].data=NULL;
        RejectedUnchanged(f,normal,in,"each layer null checked before writes");
        in.layers[layer].data=&in.data[layer][0];
    }
    D3D11_TEXTURE2D_DESC d=Desc(DXGI_FORMAT_R8G8B8A8_UNORM);
    d.MipLevels=3;Array shortChain(f,d);RejectedUnchanged(f,shortChain,in,"short resource mip chain");
    d=Desc(DXGI_FORMAT_R8G8B8A8_UNORM);d.ArraySize=3;
    Array shortArray(f,d);RejectedUnchanged(f,shortArray,in,"resource must have four layers");
    d=Desc(DXGI_FORMAT_R8G8B8A8_UNORM);d.MiscFlags=0;
    Array noGenerateFlag(f,d);RejectedUnchanged(f,noGenerateFlag,in,"missing generate flag");
    d.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    Array noRT(f,d);RejectedUnchanged(f,noRT,in,"missing render-target bind");
    // D3D11 refuses dynamic texture arrays at creation, so no fabricated COM
    // object is used to pretend this impossible descriptor is a real resource.
    d=Desc(DXGI_FORMAT_R8G8B8A8_UNORM);d.BindFlags|=D3D11_BIND_UNORDERED_ACCESS;
    Array extraBind(f,d);RejectedUnchanged(f,extraBind,in,"unexpected extra bind flags");
    d=Desc(DXGI_FORMAT_R8G8B8A8_UNORM);D3D11_SHADER_RESOURCE_VIEW_DESC v=ViewDesc(d);
    v.Texture2DArray.MipLevels=3;Array partialMip(f,d,&v);RejectedUnchanged(f,partialMip,in,"partial mip view");
    v=ViewDesc(d);v.Texture2DArray.MostDetailedMip=1;v.Texture2DArray.MipLevels=3;
    Array offsetMip(f,d,&v);RejectedUnchanged(f,offsetMip,in,"offset mip view");
    v=ViewDesc(d);v.Texture2DArray.ArraySize=3;Array partialLayer(f,d,&v);RejectedUnchanged(f,partialLayer,in,"partial layer view");
    v.Texture2DArray.FirstArraySlice=1;Array offsetLayer(f,d,&v);RejectedUnchanged(f,offsetLayer,in,"offset layer view");
    d=Desc(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);Array srgb(f,d);RejectedUnchanged(f,srgb,in,"sRGB resource rejected");
    d=Desc(DXGI_FORMAT_R8G8B8A8_UNORM);d.MiscFlags=0;d.ArraySize=1;
    v=ViewDesc(d);v.ViewDimension=D3D11_SRV_DIMENSION_TEXTURE2D;v.Texture2D.MostDetailedMip=0;v.Texture2D.MipLevels=4;
    Array flat(f,d,&v);RejectedUnchanged(f,flat,in,"non-array view");
    std::puts("PASS: invalid inputs/descriptors leave every subresource and COM ownership unchanged");
}
static std::vector<char> File(const char* path)
{
    FILE* file=NULL;fopen_s(&file,path,"rb");Check(file!=NULL,"open shader");
    fseek(file,0,SEEK_END);long n=ftell(file);rewind(file);Check(n>0,"shader length");
    std::vector<char> code(n);size_t got=fread(&code[0],1,n,file);fclose(file);Check(got==size_t(n),"read shader");return code;
}
static void StatePreserved(Fixture& f,const char* vsPath,const char* psPath,const char* csPath)
{
    Com<ID3D11VertexShader> vs;Com<ID3D11PixelShader> ps;Com<ID3D11ComputeShader> cs;
    std::vector<char> code=File(vsPath);HR(f.device->CreateVertexShader(&code[0],code.size(),NULL,&vs.p),"create VS");
    code=File(psPath);HR(f.device->CreatePixelShader(&code[0],code.size(),NULL,&ps.p),"create PS");
    code=File(csPath);HR(f.device->CreateComputeShader(&code[0],code.size(),NULL,&cs.p),"create CS");
    f.context->VSSetShader(vs.p,NULL,0);f.context->PSSetShader(ps.p,NULL,0);f.context->CSSetShader(cs.p,NULL,0);
    D3D11_TEXTURE2D_DESC td=Desc(DXGI_FORMAT_R8G8B8A8_UNORM);td.ArraySize=1;td.MipLevels=1;td.MiscFlags=0;
    Com<ID3D11Texture2D> target;Com<ID3D11RenderTargetView> rtv;
    HR(f.device->CreateTexture2D(&td,NULL,&target.p),"create sentinel target");
    HR(f.device->CreateRenderTargetView(target.p,NULL,&rtv.p),"create sentinel RTV");
    f.context->OMSetRenderTargets(1,&rtv.p,NULL);
    D3D11_BLEND_DESC bd={};bd.RenderTarget[0].BlendEnable=TRUE;
    bd.RenderTarget[0].SrcBlend=D3D11_BLEND_BLEND_FACTOR;bd.RenderTarget[0].DestBlend=D3D11_BLEND_INV_BLEND_FACTOR;
    bd.RenderTarget[0].BlendOp=D3D11_BLEND_OP_ADD;bd.RenderTarget[0].SrcBlendAlpha=D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha=D3D11_BLEND_ZERO;bd.RenderTarget[0].BlendOpAlpha=D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].RenderTargetWriteMask=D3D11_COLOR_WRITE_ENABLE_ALL;
    Com<ID3D11BlendState> blend;HR(f.device->CreateBlendState(&bd,&blend.p),"create sentinel blend state");
    const FLOAT blendFactor[4]={.1f,.3f,.6f,.9f};f.context->OMSetBlendState(blend.p,blendFactor,0x35a7);
    D3D11_DEPTH_STENCIL_DESC dd={};dd.DepthEnable=TRUE;dd.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO;
    dd.DepthFunc=D3D11_COMPARISON_GREATER;
    Com<ID3D11DepthStencilState> depth;HR(f.device->CreateDepthStencilState(&dd,&depth.p),"create sentinel depth state");
    f.context->OMSetDepthStencilState(depth.p,73);
    D3D11_RASTERIZER_DESC rd={};rd.FillMode=D3D11_FILL_WIREFRAME;rd.CullMode=D3D11_CULL_FRONT;
    rd.DepthClipEnable=TRUE;rd.ScissorEnable=TRUE;
    Com<ID3D11RasterizerState> raster;HR(f.device->CreateRasterizerState(&rd,&raster.p),"create sentinel raster state");
    f.context->RSSetState(raster.p);
    D3D11_VIEWPORT viewport={3,5,7,11,.2f,.8f};f.context->RSSetViewports(1,&viewport);
    D3D11_RECT scissor={1,2,6,7};f.context->RSSetScissorRects(1,&scissor);
    f.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
    Array sentinel(f,Desc(DXGI_FORMAT_R8G8B8A8_UNORM));
    f.context->PSSetShaderResources(3,1,&sentinel.view.p);f.context->VSSetShaderResources(7,1,&sentinel.view.p);
    f.context->CSSetShaderResources(11,1,&sentinel.view.p);
    Array destination(f,Desc(DXGI_FORMAT_R8G8B8A8_UNORM));Inputs in;
    for(unsigned repeat=0;repeat<16;++repeat)HR(Upload(destination,in),"upload with live pipeline state");
    Com<ID3D11VertexShader> gotVS;Com<ID3D11PixelShader> gotPS;Com<ID3D11ComputeShader> gotCS;
    f.context->VSGetShader(&gotVS.p,NULL,NULL);f.context->PSGetShader(&gotPS.p,NULL,NULL);f.context->CSGetShader(&gotCS.p,NULL,NULL);
    Check(gotVS.p==vs.p&&gotPS.p==ps.p&&gotCS.p==cs.p,"shader bindings preserved");
    Com<ID3D11RenderTargetView> gotRT;Com<ID3D11DepthStencilView> gotDS;
    f.context->OMGetRenderTargets(1,&gotRT.p,&gotDS.p);Check(gotRT.p==rtv.p&&!gotDS.p,"OM targets preserved");
    Com<ID3D11BlendState> gotBlend;FLOAT gotFactor[4]={};UINT gotMask=0;
    f.context->OMGetBlendState(&gotBlend.p,gotFactor,&gotMask);
    Check(gotBlend.p==blend.p&&gotMask==0x35a7&&std::memcmp(blendFactor,gotFactor,sizeof(gotFactor))==0,"OM blend state preserved");
    Com<ID3D11DepthStencilState> gotDepth;UINT stencil=0;
    f.context->OMGetDepthStencilState(&gotDepth.p,&stencil);Check(gotDepth.p==depth.p&&stencil==73,"OM depth state preserved");
    Com<ID3D11RasterizerState> gotRaster;f.context->RSGetState(&gotRaster.p);Check(gotRaster.p==raster.p,"raster state preserved");
    D3D11_VIEWPORT gotVP={};UINT count=1;f.context->RSGetViewports(&count,&gotVP);
    Check(count==1&&std::memcmp(&gotVP,&viewport,sizeof(viewport))==0,"viewport preserved");
    D3D11_RECT gotSC={};count=1;f.context->RSGetScissorRects(&count,&gotSC);
    Check(count==1&&std::memcmp(&gotSC,&scissor,sizeof(scissor))==0,"scissor preserved");
    D3D11_PRIMITIVE_TOPOLOGY topology;f.context->IAGetPrimitiveTopology(&topology);
    Check(topology==D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,"IA topology preserved");
    Com<ID3D11ShaderResourceView> gotPSR,gotVSR,gotCSR;
    f.context->PSGetShaderResources(3,1,&gotPSR.p);f.context->VSGetShaderResources(7,1,&gotVSR.p);f.context->CSGetShaderResources(11,1,&gotCSR.p);
    Check(gotPSR.p==sentinel.view.p&&gotVSR.p==sentinel.view.p&&gotCSR.p==sentinel.view.p,"unrelated SRV bindings preserved");
    VerifyPixels(f,destination,in);f.context->ClearState();
    std::puts("PASS: OM/shader/viewport/scissor/IA/SRV state preserved across 16 uploads");
}
static void DestructionLifetime(Fixture& f)
{
    for(unsigned iteration=0;iteration<32;++iteration)
    {
        unsigned destroyed=0;
        {
            Array a(f,Desc(DXGI_FORMAT_R8G8B8A8_UNORM));Inputs in;
            Watch(a.texture.p,&destroyed);Watch(a.view.p,&destroyed);
            HR(Upload(a,in),"lifetime success path");VerifyPixels(f,a,in);
            in.layers[3].bytes=0;Check(FAILED(Upload(a,in)),"lifetime early source refusal");
            in.layers[3].bytes=256;
            Check(FAILED(Upload(a,in,8,8,4,DXGI_FORMAT_B8G8R8A8_UNORM)),"lifetime view refusal");
            Check(destroyed==0,"helper does not release caller's borrowed ownership");
        }
        Check(destroyed==2,"both real GPU objects destroyed after caller releases them");
    }
    std::puts("PASS: 32 create/upload/refuse/destroy cycles released both real GPU objects");
}
int main(int argc,char** argv)
{
    try
    {
        Check(argc==4,"expected VS PS CS shader paths");Fixture f;
        const DXGI_FORMAT formats[]={DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_B8G8R8A8_UNORM};
        for(unsigned format=0;format<2;++format)
        {
            Array a(f,Desc(formats[format]));Seed(f,a);
            ULONG vr=Refs(a.view.p),tr=Refs(a.texture.p),dr=Refs(f.device.p),cr=Refs(f.context.p);
            for(unsigned repeat=0;repeat<32;++repeat)
            {
                Inputs in(repeat%4);HR(Upload(a,in,8,8,4,formats[format]),"valid four-layer upload");
                VerifyPixels(f,a,in);
                Check(vr==Refs(a.view.p)&&tr==Refs(a.texture.p)&&dr==Refs(f.device.p)&&cr==Refs(f.context.p),
                    "success releases all temporary COM refs; borrowed SRV remains owned by caller");
            }
            std::printf("PASS: DXGI %u, 32 rewrites, 4 distinct nonconstant RGBA layers, every pixel of every mip\n",unsigned(formats[format]));
        }
        BadInputs(f);StatePreserved(f,argv[1],argv[2],argv[3]);DestructionLifetime(f);
        HR(f.device->GetDeviceRemovedReason(),"WARP device remains healthy");
        std::printf("PASS: %u checks\n",checks);return 0;
    }
    catch(const std::exception& e){std::fprintf(stderr,"gpu_mip_warp: %s\n",e.what());return 1;}
}
