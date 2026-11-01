#include <d3d11.h>
#include <vector>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
struct NativeCheck{unsigned rva;unsigned char code[24];};
#include "GeographyInputs.h"
template<class T>struct Com{T*p;Com():p(NULL){}~Com(){if(p)p->Release();}T*operator->(){return p;}private:Com(const Com&);Com&operator=(const Com&);};
void Check(bool ok,const char*s){if(!ok){std::fprintf(stderr,"FAIL %s\n",s);std::exit(1);}}
void HR(HRESULT hr,const char*s){Check(SUCCEEDED(hr),s);}
void Colour(unsigned index,bool normal,BYTE (&out)[4]){out[0]=BYTE(index+7);out[1]=BYTE(index*3+9);out[2]=normal?171:81;out[3]=BYTE(249-index);}
void Texture(ID3D11Device*d,unsigned count,unsigned first,bool normal,ID3D11ShaderResourceView**view)
{
    std::vector<BYTE>data(count*4);std::vector<D3D11_SUBRESOURCE_DATA>source(count);
    for(unsigned i=0;i<count;++i){BYTE value[4];Colour(first+i,normal,value);for(unsigned c=0;c<4;++c)data[i*4+c]=value[c];source[i].pSysMem=&data[i*4];source[i].SysMemPitch=4;source[i].SysMemSlicePitch=0;}
    D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=td.MipLevels=1;td.ArraySize=count;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.SampleDesc.Count=1;td.Usage=D3D11_USAGE_IMMUTABLE;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    Com<ID3D11Texture2D> texture;HR(d->CreateTexture2D(&td,&source[0],&texture.p),"fixture array");HR(d->CreateShaderResourceView(texture.p,NULL,view),"fixture view");
}
int main(int argc,char**argv)
{
    Check(argc==2,"expected compiled shared bank shader");std::ifstream f(argv[1],std::ios::binary|std::ios::ate);Check(!!f,"shader exists");size_t bytes=size_t(f.tellg());f.seekg(0);std::vector<char>code(bytes);f.read(&code[0],bytes);
    Com<ID3D11Device>d;Com<ID3D11DeviceContext>c;HR(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&d.p,NULL,&c.p),"WARP");
    Com<ID3D11ComputeShader>shader;HR(d->CreateComputeShader(&code[0],code.size(),NULL,&shader.p),"shared production bank HLSL");c->CSSetShader(shader.p,NULL,0);
    unsigned indexes[90][2];for(unsigned b=0;b<15;++b)for(unsigned role=0;role<6;++role){indexes[b*6+role][0]=GeoBiomes[b].diffuse[role];indexes[b*6+role][1]=GeoBiomes[b].normal[role];}
    D3D11_BUFFER_DESC bd={};bd.ByteWidth=sizeof(indexes);bd.Usage=D3D11_USAGE_IMMUTABLE;bd.BindFlags=D3D11_BIND_SHADER_RESOURCE;bd.MiscFlags=D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;bd.StructureByteStride=8;
    D3D11_SUBRESOURCE_DATA data={indexes,0,0};Com<ID3D11Buffer>input;HR(d->CreateBuffer(&bd,&data,&input.p),"role-index buffer");
    D3D11_SHADER_RESOURCE_VIEW_DESC vd={};vd.ViewDimension=D3D11_SRV_DIMENSION_BUFFER;vd.Buffer.NumElements=90;Com<ID3D11ShaderResourceView>iv;HR(d->CreateShaderResourceView(input.p,&vd,&iv.p),"role-index view");c->CSSetShaderResources(2,1,&iv.p);
    Com<ID3D11ShaderResourceView>dv0,dv1,nv;Texture(d.p,24,0,false,&dv0.p);Texture(d.p,22,24,false,&dv1.p);Texture(d.p,46,0,true,&nv.p);
    c->CSSetShaderResources(0,1,&dv0.p);c->CSSetShaderResources(1,1,&nv.p);c->CSSetShaderResources(7,1,&dv1.p);
    D3D11_SAMPLER_DESC samplerDesc={};samplerDesc.Filter=D3D11_FILTER_ANISOTROPIC;samplerDesc.MaxAnisotropy=16;samplerDesc.AddressU=samplerDesc.AddressV=samplerDesc.AddressW=D3D11_TEXTURE_ADDRESS_WRAP;samplerDesc.MaxLOD=D3D11_FLOAT32_MAX;
    Com<ID3D11SamplerState>sampler;HR(d->CreateSamplerState(&samplerDesc,&sampler.p),"native anisotropic sampler");c->CSSetSamplers(0,1,&sampler.p);
    bd.ByteWidth=90*32;bd.StructureByteStride=32;bd.Usage=D3D11_USAGE_DEFAULT;bd.BindFlags=D3D11_BIND_UNORDERED_ACCESS;Com<ID3D11Buffer>output;HR(d->CreateBuffer(&bd,NULL,&output.p),"result buffer");Com<ID3D11UnorderedAccessView>uav;HR(d->CreateUnorderedAccessView(output.p,NULL,&uav.p),"result view");c->CSSetUnorderedAccessViews(0,1,&uav.p,NULL);c->Dispatch(2,1,1);
    bd.Usage=D3D11_USAGE_STAGING;bd.BindFlags=bd.MiscFlags=bd.StructureByteStride=0;bd.CPUAccessFlags=D3D11_CPU_ACCESS_READ;Com<ID3D11Buffer>stage;HR(d->CreateBuffer(&bd,NULL,&stage.p),"stage");c->CopyResource(stage.p,output.p);D3D11_MAPPED_SUBRESOURCE mapped={};HR(c->Map(stage.p,0,D3D11_MAP_READ,0,&mapped),"readback");
    const float*got=static_cast<const float*>(mapped.pData);
    for(unsigned i=0;i<90;++i)for(unsigned kind=0;kind<2;++kind){BYTE want[4];Colour(indexes[i][kind],kind!=0,want);for(unsigned channel=0;channel<4;++channel)Check(std::fabs(got[i*8+kind*4+channel]*255-want[channel])<.0001,"correct independent diffuse/normal role indexes across both banks");}
    c->Unmap(stage.p,0);c->ClearState();std::puts("PASS90 real WARP native role-pair sampling cases:15biomes/all6roles/independent46+46indexes/bank23-24 boundary");
}
