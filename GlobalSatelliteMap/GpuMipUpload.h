#ifndef GLOBAL_SATELLITE_GPU_MIP_UPLOAD_H
#define GLOBAL_SATELLITE_GPU_MIP_UPLOAD_H
#include <d3d11.h>
#include <stddef.h>

// D3D11 COM only; compatible with VS2010 and Windows SDK 7.1.
// The caller owns the texture/view and serializes this call on its render thread.
// borrowedView is never AddRef'ed, released, stored, bound or replaced here.
// Acquired COM references are scoped, including every early-return path.
// No engine state/accounting calls belong in this helper: the caller completes
// its native resource notification/state exactly once only after success.
namespace SatelliteGpuMip
{
struct Layer {const void* data;size_t bytes;};

namespace Detail
{
template<class T> struct ScopedCom
{
    T* p;
    ScopedCom():p(NULL){}
    ~ScopedCom(){if(p)p->Release();}
private:
    ScopedCom(const ScopedCom&);
    ScopedCom& operator=(const ScopedCom&);
};
}

// Uploads tightly packed four-byte mip0 images, then generates the complete
// lower chain once for all four layers. Extra bytes in a source are ignored;
// every layer must contain at least width*height*4 readable bytes. The function
// validates ALL sources, resource/view descriptors and format support before
// the first write. Input ownership/readability remains the caller's contract.
//
// S_OK means commands were submitted and the device reported healthy, not a
// synchronous GPU/pixel proof. UpdateSubresource/GenerateMips return void. A
// device failure detected after submission can return failure after writes;
// the caller must retain its existing partial-operation quarantine policy.
// Source storage may be freed after return (UpdateSubresource snapshots it).
// No Flush/ClearState, pipeline binding, viewport mutation or persistent state.
inline HRESULT UploadFourLayerMipChain(ID3D11ShaderResourceView* borrowedView,
    UINT width,UINT height,UINT mipLevels,DXGI_FORMAT format,const Layer (&layers)[4])
{
    if(!borrowedView || !width || !height ||
        width>D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION ||
        height>D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION ||
        (format!=DXGI_FORMAT_R8G8B8A8_UNORM && format!=DXGI_FORMAT_B8G8R8A8_UNORM))
        return E_INVALIDARG;

    UINT chain=1;
    for(UINT side=width>height?width:height;side>1;side>>=1)++chain;
    if(mipLevels!=chain)return E_INVALIDARG;
    // The dimension cap also makes rowPitch fit UINT. Guard the byte product
    // explicitly for 32-bit size_t as well as the deployed 64-bit build.
    const size_t rowBytes=size_t(width)*4;
    if(size_t(height)>size_t(-1)/rowBytes)return E_INVALIDARG;
    const size_t layerBytes=rowBytes*height;
    for(UINT layer=0;layer<4;++layer)
        if(!layers[layer].data || layers[layer].bytes<layerBytes)return E_INVALIDARG;

    D3D11_SHADER_RESOURCE_VIEW_DESC view;
    borrowedView->GetDesc(&view);
    if(view.Format!=format || view.ViewDimension!=D3D11_SRV_DIMENSION_TEXTURE2DARRAY ||
        view.Texture2DArray.MostDetailedMip!=0 || view.Texture2DArray.MipLevels!=mipLevels ||
        view.Texture2DArray.FirstArraySlice!=0 || view.Texture2DArray.ArraySize!=4)
        return E_INVALIDARG;

    Detail::ScopedCom<ID3D11Resource> resource;
    borrowedView->GetResource(&resource.p); // GetResource AddRefs its output.
    if(!resource.p)return E_FAIL;
    Detail::ScopedCom<ID3D11Texture2D> texture;
    HRESULT hr=resource.p->QueryInterface(__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&texture.p));
    if(FAILED(hr))return hr;
    if(!texture.p)return E_NOINTERFACE;
    D3D11_TEXTURE2D_DESC actual;
    texture.p->GetDesc(&actual);
    if(actual.Width!=width || actual.Height!=height || actual.MipLevels!=mipLevels ||
        actual.ArraySize!=4 || actual.Format!=format || actual.SampleDesc.Count!=1 ||
        actual.SampleDesc.Quality!=0 || actual.Usage!=D3D11_USAGE_DEFAULT || actual.CPUAccessFlags!=0 ||
        actual.BindFlags!=(D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET) ||
        actual.MiscFlags!=D3D11_RESOURCE_MISC_GENERATE_MIPS)
        return E_INVALIDARG;

    Detail::ScopedCom<ID3D11Device> device;
    texture.p->GetDevice(&device.p); // GetDevice AddRefs its output.
    if(!device.p)return E_FAIL;
    hr=device.p->GetDeviceRemovedReason();
    if(FAILED(hr))return hr;
    UINT support=0;
    hr=device.p->CheckFormatSupport(format,&support);
    if(FAILED(hr))return hr;
    const UINT required=D3D11_FORMAT_SUPPORT_TEXTURE2D|D3D11_FORMAT_SUPPORT_SHADER_SAMPLE|
        D3D11_FORMAT_SUPPORT_RENDER_TARGET|D3D11_FORMAT_SUPPORT_MIP|D3D11_FORMAT_SUPPORT_MIP_AUTOGEN;
    if((support&required)!=required)return DXGI_ERROR_UNSUPPORTED;

    Detail::ScopedCom<ID3D11DeviceContext> context;
    device.p->GetImmediateContext(&context.p); // GetImmediateContext AddRefs its output.
    if(!context.p)return E_FAIL;
    for(UINT layer=0;layer<4;++layer)
        context.p->UpdateSubresource(texture.p,D3D11CalcSubresource(0,layer,mipLevels),
            NULL,layers[layer].data,static_cast<UINT>(rowBytes),0);
    context.p->GenerateMips(borrowedView);
    return device.p->GetDeviceRemovedReason();
}
}
#endif
