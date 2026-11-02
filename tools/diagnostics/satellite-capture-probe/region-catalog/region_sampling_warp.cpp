#include <d3d11.h>
#include <fstream>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include "RegionCatalogData.h"
typedef unsigned char BYTE;
struct NativeCheck{unsigned rva;unsigned char code[24];};
#include "../build/GeographyInputs.h"
using namespace SatelliteRegion;
template<class T>struct Com {T* p;Com():p(NULL){}~Com(){if(p)p->Release();}T* operator->(){return p;}private:Com(const Com&);Com& operator=(const Com&);};
static unsigned checks=0;
static void Check(bool value,const char* what){++checks;if(!value){std::fprintf(stderr,"FAIL %s at%u\n",what,checks);std::exit(1);}}
static void HR(HRESULT hr,const char* what){Check(SUCCEEDED(hr),what);}
struct Point{float x,z,blend[4];};struct Result{unsigned codes[4];float weights[4];};
struct Constants{float region[4];unsigned count,padding[3];};
static std::vector<unsigned char> Read(const char* path){std::ifstream f(path,std::ios::binary);Check(f.good(),"fixture open");return std::vector<unsigned char>((std::istreambuf_iterator<char>(f)),std::istreambuf_iterator<char>());}
static Point Make(float x,float z){Point p={x,z,{1,1,1,1}};return p;}
int main(int argc,char** argv)
{
    Check(argc==3,"args shader/blend");Check(Validate(Data),"immutable catalog valid");
    std::vector<unsigned char> shader=Read(argv[1]),blend=Read(argv[2]);Check(blend.size()==1024*1024*4,"blend shape");
    Com<ID3D11Device> device;Com<ID3D11DeviceContext> context;D3D_FEATURE_LEVEL level;
    HR(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&device.p,&level,&context.p),"WARP device");
    Com<ID3D11ComputeShader> cs;HR(device->CreateComputeShader(&shader[0],shader.size(),NULL,&cs.p),"shared shader");
    const unsigned capacity=16448;
    D3D11_BUFFER_DESC bd={};bd.ByteWidth=capacity*sizeof(Point);bd.Usage=D3D11_USAGE_DEFAULT;bd.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    bd.MiscFlags=D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;bd.StructureByteStride=sizeof(Point);
    Com<ID3D11Buffer> input;HR(device->CreateBuffer(&bd,NULL,&input.p),"points");
    D3D11_SHADER_RESOURCE_VIEW_DESC sv={};sv.Format=DXGI_FORMAT_UNKNOWN;sv.ViewDimension=D3D11_SRV_DIMENSION_BUFFER;sv.Buffer.NumElements=capacity;
    Com<ID3D11ShaderResourceView> inputView;HR(device->CreateShaderResourceView(input.p,&sv,&inputView.p),"point SRV");
    bd.ByteWidth=capacity*sizeof(Result);bd.BindFlags=D3D11_BIND_UNORDERED_ACCESS;bd.StructureByteStride=sizeof(Result);
    Com<ID3D11Buffer> output;HR(device->CreateBuffer(&bd,NULL,&output.p),"results");
    D3D11_UNORDERED_ACCESS_VIEW_DESC uv={};uv.Format=DXGI_FORMAT_UNKNOWN;uv.ViewDimension=D3D11_UAV_DIMENSION_BUFFER;uv.Buffer.NumElements=capacity;
    Com<ID3D11UnorderedAccessView> outputView;HR(device->CreateUnorderedAccessView(output.p,&uv,&outputView.p),"result UAV");
    bd.Usage=D3D11_USAGE_STAGING;bd.BindFlags=0;bd.CPUAccessFlags=D3D11_CPU_ACCESS_READ;bd.MiscFlags=0;bd.StructureByteStride=0;
    Com<ID3D11Buffer> staging;HR(device->CreateBuffer(&bd,NULL,&staging.p),"bounded staging");
    bd.ByteWidth=sizeof(Constants);bd.Usage=D3D11_USAGE_DEFAULT;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;bd.CPUAccessFlags=0;
    Com<ID3D11Buffer> constants;HR(device->CreateBuffer(&bd,NULL,&constants.p),"region constants");
    D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=8;td.MipLevels=td.ArraySize=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count=1;td.Usage=D3D11_USAGE_DEFAULT;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    Com<ID3D11Texture2D> selector;HR(device->CreateTexture2D(&td,NULL,&selector.p),"selector8x8");
    Com<ID3D11ShaderResourceView> selectorView;HR(device->CreateShaderResourceView(selector.p,NULL,&selectorView.p),"selector SRV");
    context->CSSetShader(cs.p,NULL,0);context->CSSetShaderResources(0,1,&inputView.p);context->CSSetShaderResources(6,1,&selectorView.p);
    context->CSSetUnorderedAccessViews(0,1,&outputView.p,NULL);context->CSSetConstantBuffers(0,1,&constants.p);
    unsigned pointsChecked=0;
    for(unsigned index=0;index<64;++index)
    {
        const Region& r=Data.regions[index];std::vector<Point> points(capacity);unsigned count=0;
        for(unsigned z=0;z<128;++z)for(unsigned x=0;x<128;++x)
        {
            Point p=Make(float(r.worldX+(x+.5)*288),float(r.worldZ+(z+.5)*288));
            const unsigned gx=index%8*128+x,gz=index/8*128+z;
            for(unsigned ch=0;ch<4;++ch)p.blend[ch]=blend[(gz*1024+gx)*4+ch]*(1.0f/255.0f);
            points[count++]=p;
        }
        // Final9m pixel centers, exact edges, just outside, and hostile padding.
        const float offset[]={-4.5f,0.0f,4.5f,36859.5f,36864.0f,36868.5f};
        for(unsigned i=0;i<6;++i){points[count++]=Make(r.worldX+offset[i],r.worldZ+4.5f);points[count++]=Make(r.worldX+4.5f,r.worldZ+offset[i]);}
        // Exercise negative residual on every selected coarse layout too.
        for(unsigned z=0;z<4;++z)for(unsigned x=0;x<4;++x)points[count++]=Make(float(r.worldX+x*9216+4.5),float(r.worldZ+z*9216+4.5));
        unsigned char texels[256];std::memcpy(texels,r.selector,sizeof(texels));
        for(unsigned z=0;z<8;++z)for(unsigned x=0;x<8;++x)if(x>=4||z>=4)std::memcpy(texels+(z*8+x)*4,r.cells[0].selector,4);
        Constants cb={{float(r.worldX),float(r.worldZ),4,4},count,{0,0,0}};
        context->UpdateSubresource(input.p,0,NULL,&points[0],0,0);context->UpdateSubresource(selector.p,0,NULL,texels,32,0);
        context->UpdateSubresource(constants.p,0,NULL,&cb,0,0);context->Dispatch((count+63)/64,1,1);context->CopyResource(staging.p,output.p);
        D3D11_MAPPED_SUBRESOURCE mapped;HR(context->Map(staging.p,0,D3D11_MAP_READ,0,&mapped),"read bounded results");
        const Result* result=static_cast<const Result*>(mapped.pData);
        for(unsigned n=0;n<count;++n)
        {
            const Point& p=points[n];unsigned want[4]={0};float weights[4]={0};
            if(p.x>=r.worldX&&p.z>=r.worldZ&&p.x<r.worldX+36864&&p.z<r.worldZ+36864)
            {
                const unsigned x=unsigned((p.x-r.worldX)/9216),z=unsigned((p.z-r.worldZ)/9216);const Cell& c=r.cells[z*4+x];
                weights[0]=1;
                for(unsigned s=0;s<4;++s)want[s]=c.selector[s];
                for(unsigned s=1;s<4;++s)if(want[s]){weights[s]=p.blend[want[s]>>4];weights[0]-=weights[s];}
            }
            for(unsigned s=0;s<4;++s){Check(result[n].codes[s]==want[s],"regional source slot/local code");Check(std::fabs(result[n].weights[s]-weights[s])<=.000001f,"literal channel residual");}
            ++pointsChecked;
        }
        context->Unmap(staging.p,0);
    }
    // Actual production two-argument entry with historical primary1,1 table
    // and8x8 bounds. Expected native fixture bytes remain unchanged.
    {
        std::vector<Point> points(capacity);unsigned count=0;
        for(unsigned z=0;z<8;++z)for(unsigned x=0;x<8;++x)points[count++]=Make(float(-73728+(x+.5)*9216),float(-73728+(z+.5)*9216));
        Constants cb={{-73728,-73728,8,8},count,{0,0,0}};
        context->UpdateSubresource(input.p,0,NULL,&points[0],0,0);context->UpdateSubresource(selector.p,0,NULL,GeoSelector,32,0);
        context->UpdateSubresource(constants.p,0,NULL,&cb,0,0);context->Dispatch(1,1,1);context->CopyResource(staging.p,output.p);
        D3D11_MAPPED_SUBRESOURCE mapped;HR(context->Map(staging.p,0,D3D11_MAP_READ,0,&mapped),"legacy explicit bounds");
        const Result* result=static_cast<const Result*>(mapped.pData);
        for(unsigned n=0;n<count;++n){float weights[4]={1,0,0,0};for(unsigned s=1;s<4;++s)if(GeoSelector[n*4+s]){weights[s]=1;weights[0]-=1;}
            for(unsigned s=0;s<4;++s){Check(result[n].codes[s]==GeoSelector[n*4+s],"unchanged legacy8x8 codes");Check(result[n].weights[s]==weights[s],"unchanged legacy8x8 weights");}++pointsChecked;}
        context->Unmap(staging.p,0);
    }
    context->ClearState();context->Flush();
    std::printf("PASS real WARP64 regions+legacy8x8/%u points/%u checks; actual shared explicit-origin4x4 function, negative residual, poisoned unused selector texels\n",pointsChecked,checks);
    return 0;
}
