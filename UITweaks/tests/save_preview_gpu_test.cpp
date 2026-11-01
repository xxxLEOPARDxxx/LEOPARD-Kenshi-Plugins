#include "../SavePreviewGpu.h"
#include <cstdio>
#include <stdexcept>
#pragma comment(lib,"d3d11.lib")
static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::printf("FAIL line %u: %s\n",__LINE__,#x);return 1;}}while(0)
static bool Red(ID3D11Device* d,ID3D11DeviceContext* c,ID3D11Texture2D* source)
{
    D3D11_TEXTURE2D_DESC desc={};source->GetDesc(&desc);desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    SavePreviewGpu::Com<ID3D11Texture2D> read;if(FAILED(d->CreateTexture2D(&desc,NULL,read.Out())))return false;
    c->CopyResource(read,source);D3D11_MAPPED_SUBRESOURCE map={};if(FAILED(c->Map(read,0,D3D11_MAP_READ,0,&map)))return false;
    bool ok=true;for(unsigned y=0;y<desc.Height;++y)for(unsigned x=0;x<desc.Width;++x)
    {unsigned char* p=(unsigned char*)map.pData+size_t(y)*map.RowPitch+x*4;if(p[0]!=255||p[1]||p[2]||p[3]!=255)ok=false;}
    c->Unmap(read,0);return ok;
}
int main()
{
    using namespace SavePreviewGpu;
    Com<ID3D11Device> device;Com<ID3D11DeviceContext> context;D3D_FEATURE_LEVEL level;
    CHECK(SUCCEEDED(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,device.Out(),&level,context.Out())));
    D3D11_TEXTURE2D_DESC d={};d.Width=640;d.Height=360;d.MipLevels=1;d.ArraySize=1;d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    d.SampleDesc.Count=1;d.Usage=D3D11_USAGE_DEFAULT;d.BindFlags=D3D11_BIND_RENDER_TARGET;CHECK(Allowed(d));
    Com<ID3D11Texture2D> scene;Com<ID3D11RenderTargetView> view;
    CHECK(SUCCEEDED(device->CreateTexture2D(&d,NULL,scene.Out())));CHECK(SUCCEEDED(device->CreateRenderTargetView(scene,NULL,view.Out())));
    ID3D11RenderTargetView* ptr=view.p;context->OMSetRenderTargets(1,&ptr,NULL);float red[]={1,0,0,1},blue[]={0,0,1,1};context->ClearRenderTargetView(view,red);
    {SceneBackup backup;CHECK(backup.Begin(scene));context->ClearRenderTargetView(view,blue);CHECK(backup.Restore());}
    CHECK(Red(device,context,scene));
    ID3D11RenderTargetView* rebound=NULL;context->OMGetRenderTargets(1,&rebound,NULL);CHECK(rebound==view.p);if(rebound)rebound->Release();
    try{SceneBackup backup;CHECK(backup.Begin(scene));context->ClearRenderTargetView(view,blue);throw 7;}catch(int){}
    CHECK(Red(device,context,scene));
    context->OMSetRenderTargets(0,NULL,NULL);{SceneBackup backup;CHECK(!backup.Begin(scene));}context->OMSetRenderTargets(1,&ptr,NULL);
    D3D11_TEXTURE2D_DESC depthDesc=d;depthDesc.Format=DXGI_FORMAT_D24_UNORM_S8_UINT;depthDesc.BindFlags=D3D11_BIND_DEPTH_STENCIL;
    Com<ID3D11Texture2D> depthTexture;Com<ID3D11DepthStencilView> depthView;
    CHECK(SUCCEEDED(device->CreateTexture2D(&depthDesc,NULL,depthTexture.Out())));
    CHECK(SUCCEEDED(device->CreateDepthStencilView(depthTexture,NULL,depthView.Out())));
    context->OMSetRenderTargets(1,&ptr,depthView);context->ClearDepthStencilView(depthView,D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL,.37f,3);
    {SceneBackup backup;CHECK(backup.Begin(scene));context->ClearRenderTargetView(view,blue);CHECK(backup.Restore());}
    ID3D11DepthStencilView* reboundDepth=NULL;rebound=NULL;context->OMGetRenderTargets(1,&rebound,&reboundDepth);
    CHECK(rebound==view.p&&reboundDepth==depthView.p);if(rebound)rebound->Release();if(reboundDepth)reboundDepth->Release();
    CHECK(Red(device,context,scene));
    d.Width=7680;d.Height=4320;CHECK(!Allowed(d));d.Width=3840;d.Height=2160;d.SampleDesc.Count=4;CHECK(!Allowed(d));
    d.SampleDesc.Count=1;CHECK(Allowed(d));d.ArraySize=2;CHECK(!Allowed(d));
    std::printf("PASS actual one-shot GPU backup/restore: %u checks; WARP pixels and OM bindings exact\n",checks);return 0;
}
