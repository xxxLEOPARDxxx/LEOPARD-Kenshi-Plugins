#pragma once
#include <d3d11.h>
#include "GeographyPolicy.h"
namespace SatelliteGeography
{
namespace CompressedDetail
{
template<class T>struct Owned{T*p;Owned():p(NULL){}~Owned(){if(p)p->Release();}private:Owned(const Owned&);Owned&operator=(const Owned&);};
}
// Read-only snapshot for the existing probe's failure log. No retained COM.
struct UploadDescription
{
    bool hasView,hasTexture;D3D11_SHADER_RESOURCE_VIEW_DESC view;D3D11_TEXTURE2D_DESC texture;
    UploadDescription():hasView(false),hasTexture(false){std::memset(&view,0,sizeof(view));std::memset(&texture,0,sizeof(texture));}
};
inline HRESULT DescribeUpload(ID3D11ShaderResourceView* view,UploadDescription& out)
{
    out=UploadDescription();if(!view)return E_INVALIDARG;
    view->GetDesc(&out.view);out.hasView=true;
    CompressedDetail::Owned<ID3D11Resource> r;view->GetResource(&r.p);if(!r.p)return E_FAIL;
    CompressedDetail::Owned<ID3D11Texture2D> t;HRESULT hr=r.p->QueryInterface(__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&t.p));if(FAILED(hr))return hr;
    t.p->GetDesc(&out.texture);out.hasTexture=true;return S_OK;
}
// Borrowed SRV, one validated source layer at a time. All nine bounds are
// checked before any UpdateSubresource. No autogen, retained COM or engine calls.
inline HRESULT UploadCompressedLayer(ID3D11ShaderResourceView* view,unsigned layers,unsigned layer,unsigned format,const Mip (&source)[9])
{
    if(!view||layer>=layers||!((format==77&&(layers==24||layers==22))||(format==71&&layers==46)))return E_INVALIDARG;
    unsigned block=format==77?16:8;
    for(unsigned mip=0;mip<9;++mip)
    {unsigned blocks=(2048>>mip)/4;if(!source[mip].data||source[mip].bytes!=size_t(blocks)*blocks*block)return E_INVALIDARG;}
    D3D11_SHADER_RESOURCE_VIEW_DESC v;view->GetDesc(&v);
    if(v.Format!=DXGI_FORMAT(format)||v.ViewDimension!=D3D11_SRV_DIMENSION_TEXTURE2DARRAY||v.Texture2DArray.MostDetailedMip||
       v.Texture2DArray.FirstArraySlice||v.Texture2DArray.MipLevels!=9||v.Texture2DArray.ArraySize!=layers)return E_INVALIDARG;
    CompressedDetail::Owned<ID3D11Resource> r;view->GetResource(&r.p);if(!r.p)return E_FAIL;
    CompressedDetail::Owned<ID3D11Texture2D> t;HRESULT hr=r.p->QueryInterface(__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&t.p));if(FAILED(hr))return hr;
    D3D11_TEXTURE2D_DESC d;t.p->GetDesc(&d);
    if(d.Width!=2048||d.Height!=2048||d.MipLevels!=9||d.ArraySize!=layers||d.Format!=DXGI_FORMAT(format)||
       d.SampleDesc.Count!=1||d.SampleDesc.Quality||d.Usage!=D3D11_USAGE_DEFAULT||d.CPUAccessFlags||
       d.BindFlags!=D3D11_BIND_SHADER_RESOURCE||d.MiscFlags)return E_INVALIDARG;
    CompressedDetail::Owned<ID3D11Device> device;t.p->GetDevice(&device.p);if(!device.p)return E_FAIL;
    hr=device.p->GetDeviceRemovedReason();if(FAILED(hr))return hr;
    UINT support=0;hr=device.p->CheckFormatSupport(DXGI_FORMAT(format),&support);if(FAILED(hr))return hr;
    const UINT needed=D3D11_FORMAT_SUPPORT_TEXTURE2D|D3D11_FORMAT_SUPPORT_SHADER_SAMPLE|D3D11_FORMAT_SUPPORT_MIP;
    if((support&needed)!=needed)return DXGI_ERROR_UNSUPPORTED;
    CompressedDetail::Owned<ID3D11DeviceContext> context;device.p->GetImmediateContext(&context.p);if(!context.p)return E_FAIL;
    for(unsigned mip=0;mip<9;++mip)
        context.p->UpdateSubresource(t.p,D3D11CalcSubresource(mip,layer,9),NULL,source[mip].data,((2048>>mip)/4)*block,0);
    return device.p->GetDeviceRemovedReason();
}
inline HRESULT UploadRgba2D(ID3D11ShaderResourceView* view,unsigned side,unsigned levels,unsigned format,const void* pixels,size_t bytes)
{
    if(!view||!pixels||!((side==1024&&levels==11)||(side==8&&levels==1))||(format!=28&&format!=87)||bytes!=size_t(side)*side*4)return E_INVALIDARG;
    D3D11_SHADER_RESOURCE_VIEW_DESC v;view->GetDesc(&v);
    if(v.Format!=DXGI_FORMAT(format)||v.ViewDimension!=D3D11_SRV_DIMENSION_TEXTURE2D||v.Texture2D.MostDetailedMip||v.Texture2D.MipLevels!=levels)return E_INVALIDARG;
    CompressedDetail::Owned<ID3D11Resource> r;view->GetResource(&r.p);if(!r.p)return E_FAIL;
    CompressedDetail::Owned<ID3D11Texture2D> t;HRESULT hr=r.p->QueryInterface(__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&t.p));if(FAILED(hr))return hr;
    D3D11_TEXTURE2D_DESC d;t.p->GetDesc(&d);
    if(d.Width!=side||d.Height!=side||d.MipLevels!=levels||d.ArraySize!=1||d.Format!=DXGI_FORMAT(format)||d.Usage!=D3D11_USAGE_DEFAULT||d.CPUAccessFlags||
       d.SampleDesc.Count!=1||d.SampleDesc.Quality||d.BindFlags!=(D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET)||d.MiscFlags!=D3D11_RESOURCE_MISC_GENERATE_MIPS)return E_INVALIDARG;
    CompressedDetail::Owned<ID3D11Device> device;t.p->GetDevice(&device.p);if(!device.p)return E_FAIL;
    UINT support=0;hr=device.p->CheckFormatSupport(DXGI_FORMAT(format),&support);if(FAILED(hr))return hr;
    if(!(support&D3D11_FORMAT_SUPPORT_MIP_AUTOGEN))return DXGI_ERROR_UNSUPPORTED;
    hr=device.p->GetDeviceRemovedReason();if(FAILED(hr))return hr;
    CompressedDetail::Owned<ID3D11DeviceContext> context;device.p->GetImmediateContext(&context.p);if(!context.p)return E_FAIL;
    context.p->UpdateSubresource(t.p,0,NULL,pixels,side*4,0);context.p->GenerateMips(view);
    return device.p->GetDeviceRemovedReason();
}
}
