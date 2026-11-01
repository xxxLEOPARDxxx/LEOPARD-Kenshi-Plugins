#include <d3d11.h>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <fstream>
#include <stdexcept>
#ifndef GEOGRAPHY_UPLOAD_HEADER
#define GEOGRAPHY_UPLOAD_HEADER "GpuCompressedUpload.h"
#endif
#include GEOGRAPHY_UPLOAD_HEADER
template<class T>struct Com{T*p;Com():p(NULL){}~Com(){if(p)p->Release();}T*operator->(){return p;}private:Com(const Com&);Com&operator=(const Com&);};
static unsigned checks=0;void Check(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);std::exit(1);}}
void HR(HRESULT h,const char*s){Check(SUCCEEDED(h),s);}
int main(int argc,char**argv)
{
    using namespace SatelliteGeography;
    Check(argc==2,"expected actual DDS fixture");std::ifstream file(argv[1],std::ios::binary|std::ios::ate);
    Check(!!file,"DDS exists");size_t n=size_t(file.tellg());file.seekg(0);std::vector<unsigned char> bytes(n);file.read(reinterpret_cast<char*>(&bytes[0]),n);
    Mip mips[9];Check(Dds(&bytes[0],n,77,mips),"strict actual BC3 DDS parsed");
    Check(mips[0].bytes==4194304&&mips[8].bytes==64,"nine native upload mip byte bounds");
    Check(!Dds(&bytes[0],n-1,77,mips),"truncated tail refused before allocation");
    Check(!Dds(&bytes[0],n,71,mips),"wrong BC format refused");
    Check(Combined(0,0)&&!Combined(GpuLimit,0)&&!Combined(0,~0ULL),"combined unchanged1GiB bound/overflow");
    Com<ID3D11Device>d;Com<ID3D11DeviceContext>c;HR(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&d.p,NULL,&c.p),"WARP");
    {
        // Reproduce request1509's native BC shape: requested additional8 became
        // actual7 levels. The strict9-level upload must refuse without writes.
        D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=2048;td.MipLevels=7;td.ArraySize=24;td.Format=DXGI_FORMAT_BC3_UNORM;td.SampleDesc.Count=1;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
        Com<ID3D11Texture2D> texture;Com<ID3D11ShaderResourceView> view;HR(d->CreateTexture2D(&td,NULL,&texture.p),"native7-level reproduction create");HR(d->CreateShaderResourceView(texture.p,NULL,&view.p),"native7-level reproduction view");
        std::vector<unsigned char> poison(4194304,193);c->UpdateSubresource(texture.p,0,NULL,&poison[0],8192,0);
        UploadDescription detail;HR(DescribeUpload(view.p,detail),"native7-level descriptor snapshot");
        Check(detail.hasView&&detail.hasTexture&&detail.view.Texture2DArray.MipLevels==7&&detail.texture.MipLevels==7&&detail.texture.ArraySize==24,"failure descriptor preserves actual7 levels and24 layers");
        Check(UploadCompressedLayer(view.p,24,0,77,mips)==E_INVALIDARG,"native7-level texture refused by unchanged9-level guard");
        D3D11_TEXTURE2D_DESC sd=td;sd.MipLevels=sd.ArraySize=1;sd.Usage=D3D11_USAGE_STAGING;sd.BindFlags=0;sd.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
        Com<ID3D11Texture2D> staging;HR(d->CreateTexture2D(&sd,NULL,&staging.p),"native7-level unchanged staging");c->CopySubresourceRegion(staging.p,0,0,0,0,texture.p,0,NULL);
        D3D11_MAPPED_SUBRESOURCE mapped={};HR(c->Map(staging.p,0,D3D11_MAP_READ,0,&mapped),"native7-level unchanged readback");
        Check(*static_cast<unsigned char*>(mapped.pData)==193,"native7-level refusal performs no first-layer writes");c->Unmap(staging.p,0);
        Check(DescribeUpload(NULL,detail)==E_INVALIDARG&&!detail.hasView&&!detail.hasTexture,"null descriptor snapshot clears previous evidence");
    }
    const unsigned layers[3]={24,22,46};const unsigned formats[3]={77,77,71};
    for(unsigned bank=0;bank<3;++bank)
    {
        D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=2048;td.MipLevels=9;td.ArraySize=layers[bank];td.Format=DXGI_FORMAT(formats[bank]);td.SampleDesc.Count=1;
        td.Usage=D3D11_USAGE_DEFAULT;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
        Com<ID3D11Texture2D> texture;Com<ID3D11ShaderResourceView> view;HR(d->CreateTexture2D(&td,NULL,&texture.p),"bank create");HR(d->CreateShaderResourceView(texture.p,NULL,&view.p),"bank view");
        for(unsigned layer=0;layer<layers[bank];++layer)
        {
            std::vector<unsigned char> payload[9];
            for(unsigned mip=0;mip<9;++mip)
            {
                unsigned side=2048>>mip;size_t size=size_t(side/4)*(side/4)*(bank==2?8:16);payload[mip].resize(size);
                for(size_t b=0;b<size;++b)payload[mip][b]=static_cast<unsigned char>((b*13+layer*47+mip*11)%251);
                mips[mip].data=&payload[mip][0];mips[mip].bytes=size;
            }
            HR(UploadCompressedLayer(view.p,layers[bank],layer,formats[bank],mips),"stream one complete compressed layer");
            for(unsigned mip=0;mip<9;++mip)
            {
                D3D11_TEXTURE2D_DESC sd=td;sd.Width=sd.Height=2048>>mip;sd.MipLevels=sd.ArraySize=1;sd.Usage=D3D11_USAGE_STAGING;sd.BindFlags=0;sd.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
                Com<ID3D11Texture2D> staging;HR(d->CreateTexture2D(&sd,NULL,&staging.p),"one mip staging");
                c->CopySubresourceRegion(staging.p,0,0,0,0,texture.p,mip+9*layer,NULL);D3D11_MAPPED_SUBRESOURCE mapped={};HR(c->Map(staging.p,0,D3D11_MAP_READ,0,&mapped),"read raw BC mip");
                size_t row=size_t(sd.Width/4)*(bank==2?8:16);
                for(unsigned y=0;y<sd.Height/4;++y)Check(!std::memcmp(static_cast<unsigned char*>(mapped.pData)+size_t(y)*mapped.RowPitch,&payload[mip][size_t(y)*row],row),"exact source BC block bytes/layer/mip");
                c->Unmap(staging.p,0);
            }
            payload[0][0]^=255;--mips[8].bytes;Check(FAILED(UploadCompressedLayer(view.p,layers[bank],layer,formats[bank],mips)),"last mip invalid refuses whole layer");
            D3D11_TEXTURE2D_DESC sd=td;sd.MipLevels=sd.ArraySize=1;sd.Usage=D3D11_USAGE_STAGING;sd.BindFlags=0;sd.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
            Com<ID3D11Texture2D> unchanged;HR(d->CreateTexture2D(&sd,NULL,&unchanged.p),"failed input no-write staging");c->CopySubresourceRegion(unchanged.p,0,0,0,0,texture.p,9*layer,NULL);
            D3D11_MAPPED_SUBRESOURCE mapped={};HR(c->Map(unchanged.p,0,D3D11_MAP_READ,0,&mapped),"failed-input readback");
            Check(*static_cast<unsigned char*>(mapped.pData)==static_cast<unsigned char>((layer*47)%251),"invalid last mip performs no earlier writes");c->Unmap(unchanged.p,0);
        }
        std::printf("PASS bank%u %u layers all9mips byte-preserved\n",bank,layers[bank]);
    }
    for(unsigned test=0;test<4;++test)
    {
        unsigned side=test<2?1024:8,levels=test<2?11:1,format=test%2?87:28;
        D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=side;td.MipLevels=levels;td.ArraySize=1;td.Format=DXGI_FORMAT(format);td.SampleDesc.Count=1;
        td.Usage=D3D11_USAGE_DEFAULT;td.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;td.MiscFlags=D3D11_RESOURCE_MISC_GENERATE_MIPS;
        Com<ID3D11Texture2D> texture;Com<ID3D11ShaderResourceView> view;HR(d->CreateTexture2D(&td,NULL,&texture.p),"RGBA2D create");HR(d->CreateShaderResourceView(texture.p,NULL,&view.p),"RGBA2D view");
        std::vector<unsigned char> data(side*side*4,211);
        for(unsigned mip=0;mip<levels;++mip)c->UpdateSubresource(texture.p,mip,NULL,&data[0],(side>>mip)*4,0);
        const BYTE value[4]={53,91,121,181};for(size_t at=0;at<data.size();++at)data[at]=value[at%4];
        HR(UploadRgba2D(view.p,side,levels,format,&data[0],data.size()),"RGBA2D full-chain upload");
        Check(FAILED(UploadRgba2D(view.p,side,levels,format,&data[0],data.size()-1)),"RGBA2D truncated source refused");
        td.Usage=D3D11_USAGE_STAGING;td.BindFlags=td.MiscFlags=0;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;Com<ID3D11Texture2D> stage;HR(d->CreateTexture2D(&td,NULL,&stage.p),"RGBA2D staging");c->CopyResource(stage.p,texture.p);
        for(unsigned mip=0;mip<levels;++mip)
        {D3D11_MAPPED_SUBRESOURCE mapped={};HR(c->Map(stage.p,mip,D3D11_MAP_READ,0,&mapped),"RGBA2D readback");
            for(unsigned y=0;y<(side>>mip);++y)for(unsigned x=0;x<(side>>mip);++x)
                Check(!std::memcmp(static_cast<BYTE*>(mapped.pData)+size_t(y)*mapped.RowPitch+x*4,value,4),"RGBA2D all mips/alpha generated");c->Unmap(stage.p,mip);}
    }
    std::printf("%u geography DDS/budget/real WARP upload checks passed\n",checks);
}
