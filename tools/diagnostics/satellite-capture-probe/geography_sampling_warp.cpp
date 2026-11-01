#include <d3d11.h>
#include <vector>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdlib>
struct NativeCheck{unsigned rva;unsigned char code[24];};
#include "GeographyInputs.h"
template<class T>struct Com{T*p;Com():p(NULL){}~Com(){if(p)p->Release();}T*operator->(){return p;}private:Com(const Com&);Com&operator=(const Com&);};
void Check(bool b,const char*s){if(!b){std::fprintf(stderr,"FAIL %s\n",s);std::exit(1);}}
void HR(HRESULT h,const char*s){Check(SUCCEEDED(h),s);}
std::vector<char> File(const char*p){std::ifstream f(p,std::ios::binary|std::ios::ate);Check(!!f,"fixture exists");size_t n=size_t(f.tellg());Check(n&&n<8*1024*1024,"fixture bound");f.seekg(0);std::vector<char>b(n);f.read(&b[0],n);Check(!!f,"fixture read");return b;}
int main(int argc,char**argv)
{
    Check(argc==5,"expected shader/points/expected/RGBA files");std::vector<char> code=File(argv[1]),points=File(argv[2]),expected=File(argv[3]),rgba=File(argv[4]);
    Check(points.size()%8==0&&expected.size()==points.size()*4&&rgba.size()==1024*1024*4,"fixture layouts");unsigned count=unsigned(points.size()/8);
    Com<ID3D11Device>d;Com<ID3D11DeviceContext>c;HR(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&d.p,NULL,&c.p),"WARP");
    Com<ID3D11ComputeShader> shader;HR(d->CreateComputeShader(&code[0],code.size(),NULL,&shader.p),"actual shared HLSL compile");c->CSSetShader(shader.p,NULL,0);
    D3D11_BUFFER_DESC bd={};bd.ByteWidth=unsigned(points.size());bd.Usage=D3D11_USAGE_IMMUTABLE;bd.BindFlags=D3D11_BIND_SHADER_RESOURCE;bd.MiscFlags=D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;bd.StructureByteStride=8;
    D3D11_SUBRESOURCE_DATA sd={&points[0],0,0};Com<ID3D11Buffer>in;HR(d->CreateBuffer(&bd,&sd,&in.p),"point buffer");
    D3D11_SHADER_RESOURCE_VIEW_DESC vd={};vd.Format=DXGI_FORMAT_UNKNOWN;vd.ViewDimension=D3D11_SRV_DIMENSION_BUFFER;vd.Buffer.NumElements=count;
    Com<ID3D11ShaderResourceView>iv;HR(d->CreateShaderResourceView(in.p,&vd,&iv.p),"input view");c->CSSetShaderResources(0,1,&iv.p);
    bd.ByteWidth=count*32;bd.StructureByteStride=32;bd.Usage=D3D11_USAGE_DEFAULT;bd.BindFlags=D3D11_BIND_UNORDERED_ACCESS;Com<ID3D11Buffer>out;HR(d->CreateBuffer(&bd,NULL,&out.p),"output buffer");
    Com<ID3D11UnorderedAccessView>uav;HR(d->CreateUnorderedAccessView(out.p,NULL,&uav.p),"output UAV");c->CSSetUnorderedAccessViews(0,1,&uav.p,NULL);
    D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=1024;td.MipLevels=td.ArraySize=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.SampleDesc.Count=1;td.Usage=D3D11_USAGE_IMMUTABLE;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA texData={&rgba[0],4096,0};Com<ID3D11Texture2D>blend,selector;Com<ID3D11ShaderResourceView>bv,sv;
    HR(d->CreateTexture2D(&td,&texData,&blend.p),"native blend pixels");HR(d->CreateShaderResourceView(blend.p,NULL,&bv.p),"blend view");
    td.Width=td.Height=8;texData.pSysMem=GeoSelector;texData.SysMemPitch=32;HR(d->CreateTexture2D(&td,&texData,&selector.p),"selector pixels");HR(d->CreateShaderResourceView(selector.p,NULL,&sv.p),"selector view");
    c->CSSetShaderResources(5,1,&bv.p);c->CSSetShaderResources(6,1,&sv.p);D3D11_SAMPLER_DESC samp={};samp.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;samp.AddressU=samp.AddressV=samp.AddressW=D3D11_TEXTURE_ADDRESS_WRAP;samp.MaxLOD=D3D11_FLOAT32_MAX;
    Com<ID3D11SamplerState>sampler;HR(d->CreateSamplerState(&samp,&sampler.p),"native linear wrap sampler");c->CSSetSamplers(2,1,&sampler.p);c->Dispatch((count+63)/64,1,1);
    bd.Usage=D3D11_USAGE_STAGING;bd.BindFlags=bd.MiscFlags=bd.StructureByteStride=0;bd.CPUAccessFlags=D3D11_CPU_ACCESS_READ;Com<ID3D11Buffer>stage;HR(d->CreateBuffer(&bd,NULL,&stage.p),"readback");c->CopyResource(stage.p,out.p);
    D3D11_MAPPED_SUBRESOURCE mapped={};HR(c->Map(stage.p,0,D3D11_MAP_READ,0,&mapped),"map results");
    for(unsigned i=0;i<count;++i)
    {
        const char* got=static_cast<char*>(mapped.pData)+size_t(i)*32;const char* want=&expected[size_t(i)*32];
        Check(!std::memcmp(got,want,16),"actual geographic world SID/channel selection");
        for(unsigned k=0;k<4;++k){float a,b;std::memcpy(&a,got+16+k*4,4);std::memcpy(&b,want+16+k*4,4);
            if(std::fabs(a-b)>0.000002f){std::fprintf(stderr,"case%u weight%u actual%.9g expected%.9g\n",i,k,a,b);Check(false,"native blend residual/channel weights");}}
    }
    c->Unmap(stage.p,0);c->ClearState();std::printf("PASS %u real WARP geographic world SID/channel/residual-weight cases from production HLSL\n",count);
}
