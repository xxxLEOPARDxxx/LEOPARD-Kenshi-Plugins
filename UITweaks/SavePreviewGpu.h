#pragma once
#include "SavePreviewCodec.h"
#include <d3d11.h>

// A one-save scene backup, never a history buffer or per-frame cache.
namespace SavePreviewGpu
{
    template<class T> struct Com
    {
        T* p;Com():p(NULL){}~Com(){if(p)p->Release();}
        T** Out(){return &p;}T* operator->()const{return p;}operator T*()const{return p;}
    private:Com(const Com&);Com& operator=(const Com&);
    };
    struct OutputScope
    {
        ID3D11DeviceContext* context;ID3D11RenderTargetView* views[8];ID3D11DepthStencilView* depth;UINT count;
        explicit OutputScope(ID3D11DeviceContext* c):context(c),depth(NULL),count(0)
        {
            memset(views,0,sizeof(views));context->OMGetRenderTargets(8,views,&depth);
            for(UINT i=0;i<8;++i)if(views[i])count=i+1;
            context->OMSetRenderTargets(0,NULL,NULL);
        }
        ~OutputScope()
        {
            context->OMSetRenderTargets(count,views,depth);
            for(UINT i=0;i<8;++i)if(views[i])views[i]->Release();
            if(depth)depth->Release();
        }
    private:OutputScope(const OutputScope&);OutputScope& operator=(const OutputScope&);
    };
    inline bool Allowed(const D3D11_TEXTURE2D_DESC& d)
    {
        const bool format=d.Format==DXGI_FORMAT_R8G8B8A8_UNORM||d.Format==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB||
            d.Format==DXGI_FORMAT_B8G8R8A8_UNORM||d.Format==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        return format&&d.ArraySize==1&&d.MipLevels==1&&d.SampleDesc.Count&&d.SampleDesc.Count<=8&&
            SavePreviewPolicy::AllowedReadback(d.Width,d.Height)&&
            static_cast<unsigned __int64>(d.Width)*d.Height*4*d.SampleDesc.Count<=64ULL*1024*1024;
    }
    class SceneBackup
    {
        Com<ID3D11Texture2D> scene,backup;Com<ID3D11Device> device;Com<ID3D11DeviceContext> context;bool saved;
    public:
        SceneBackup():saved(false){}
        ~SceneBackup(){Restore();}
        bool Begin(ID3D11Texture2D* borrowed)
        {
            if(!borrowed||saved||scene.p)return false;
            D3D11_TEXTURE2D_DESC d={};borrowed->GetDesc(&d);if(!Allowed(d))return false;
            borrowed->AddRef();scene.p=borrowed;scene->GetDevice(device.Out());if(!device.p)return false;
            device->GetImmediateContext(context.Out());if(!context.p)return false;
            // The supported main GUI pass has exactly the window RTV at slot0.
            ID3D11RenderTargetView* views[8]={};context->OMGetRenderTargets(8,views,NULL);
            bool match=views[0]!=NULL;
            for(unsigned i=1;i<8;++i)if(views[i])match=false;
            Com<ID3D11Resource> resource;Com<ID3D11Texture2D> current;
            if(views[0])views[0]->GetResource(resource.Out());
            for(unsigned i=0;i<8;++i)if(views[i])views[i]->Release();
            if(!resource.p||FAILED(resource->QueryInterface(__uuidof(ID3D11Texture2D),(void**)current.Out()))||current.p!=scene.p)match=false;
            if(!match)return false;
            d.Usage=D3D11_USAGE_DEFAULT;d.BindFlags=0;d.CPUAccessFlags=0;d.MiscFlags=0;
            if(FAILED(device->CreateTexture2D(&d,NULL,backup.Out())))return false;
            {OutputScope restore(context);context->CopyResource(backup,scene);}
            saved=SUCCEEDED(device->GetDeviceRemovedReason());return saved;
        }
        bool Restore()
        {
            if(!saved)return true;saved=false;
            {OutputScope restore(context);context->CopyResource(scene,backup);}
            return SUCCEEDED(device->GetDeviceRemovedReason());
        }
    private:SceneBackup(const SceneBackup&);SceneBackup& operator=(const SceneBackup&);
    };
}
