#include <d3d11.h>
#include <vector>
#include <fstream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <float.h>
#ifndef GEOGRAPHY_FADE_HEADER
#define GEOGRAPHY_FADE_HEADER "GeographyPolicy.h"
#endif
#include GEOGRAPHY_FADE_HEADER
struct NativeCheck{unsigned rva;unsigned char code[24];};
#include "GeographyInputs.h"
#include "GeographyFadeInputs.h"
template<class T>struct Com{T*p;Com():p(NULL){}~Com(){if(p)p->Release();}T*operator->(){return p;}private:Com(const Com&);Com&operator=(const Com&);};
static unsigned checks=0;void Check(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);std::exit(1);}}
void HR(HRESULT h,const char*s){Check(SUCCEEDED(h),s);}
std::vector<char>File(const char*p){std::ifstream f(p,std::ios::binary|std::ios::ate);Check(!!f,"fixture exists");size_t n=size_t(f.tellg());Check(n&&n<1024*1024,"fixture bounded");f.seekg(0);std::vector<char>b(n);f.read(&b[0],n);Check(!!f,"fixture read");return b;}
void Texels(unsigned index,bool normal,float(&out)[8*8*4])
{for(unsigned y=0;y<8;++y)for(unsigned x=0;x<8;++x){float*p=out+(y*8+x)*4;if(normal){p[0]=.25f+.005f*(index%20)+.004f*x;p[1]=.4f+.002f*index+.005f*y;p[2]=.6f+.004f*(index%20)+.003f*(x+y);p[3]=.2f+.008f*(index%23)+.002f*(x+y);}else{p[0]=.07f+.004f*index+.008f*x;p[1]=.11f+.003f*index+.005f*y;p[2]=.21f+.002f*index+.003f*(x+y);p[3]=.15f+.01f*(index%20)+.005f*(x+y);}}}
void Array(ID3D11Device*d,ID3D11DeviceContext*c,unsigned count,unsigned first,bool normal,ID3D11Texture2D**texture,ID3D11ShaderResourceView**view)
{D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=8;td.MipLevels=1;td.ArraySize=count;td.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;td.SampleDesc.Count=1;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;HR(d->CreateTexture2D(&td,NULL,texture),"tiny bounded role array");HR(d->CreateShaderResourceView(*texture,NULL,view),"role array view");float pixels[256];for(unsigned i=0;i<count;++i){Texels(first+i,normal,pixels);c->UpdateSubresource(*texture,i,NULL,pixels,128,0);}}
struct Constants{float p[15][4];};
int main(int argc,char**argv)
{
    Check(argc==4,"expected fullscreenVS/nativePS/ownedPS");std::vector<char>v=File(argv[1]),reference=File(argv[2]),owned=File(argv[3]);
    Com<ID3D11Device>d;Com<ID3D11DeviceContext>c;HR(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&d.p,NULL,&c.p),"WARP");
    Com<ID3D11VertexShader>vs;Com<ID3D11PixelShader>ps[2];HR(d->CreateVertexShader(&v[0],v.size(),NULL,&vs.p),"fullscreen VS");HR(d->CreatePixelShader(&reference[0],reference.size(),NULL,&ps[0].p),"pinned native computeBiome");HR(d->CreatePixelShader(&owned[0],owned.size(),NULL,&ps[1].p),"production computeGeoBiome");c->VSSetShader(vs.p,NULL,0);c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    Com<ID3D11Texture2D>pool[3],native[2];Com<ID3D11ShaderResourceView>poolView[3],nativeView[2];Array(d.p,c.p,24,0,false,&pool[0].p,&poolView[0].p);Array(d.p,c.p,22,24,false,&pool[1].p,&poolView[1].p);Array(d.p,c.p,46,0,true,&pool[2].p,&poolView[2].p);for(unsigned k=0;k<2;++k)Array(d.p,c.p,6,0,k!=0,&native[k].p,&nativeView[k].p);
    D3D11_SAMPLER_DESC sd={};sd.Filter=D3D11_FILTER_ANISOTROPIC;sd.MaxAnisotropy=16;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_WRAP;sd.MaxLOD=D3D11_FLOAT32_MAX;Com<ID3D11SamplerState>sampler;HR(d->CreateSamplerState(&sd,&sampler.p),"native anisotropic sampler");c->PSSetSamplers(0,1,&sampler.p);
    D3D11_RASTERIZER_DESC rsd={};rsd.FillMode=D3D11_FILL_SOLID;rsd.CullMode=D3D11_CULL_NONE;rsd.DepthClipEnable=TRUE;Com<ID3D11RasterizerState>raster;HR(d->CreateRasterizerState(&rsd,&raster.p),"raster");c->RSSetState(raster.p);D3D11_VIEWPORT vp={0,0,16,16,0,1};c->RSSetViewports(1,&vp);
    D3D11_BUFFER_DESC bd={};bd.ByteWidth=sizeof(Constants);bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;Com<ID3D11Buffer>cb;HR(d->CreateBuffer(&bd,NULL,&cb.p),"fade constants");c->PSSetConstantBuffers(0,1,&cb.p);
    Com<ID3D11Texture2D>target[3],stage[3];Com<ID3D11RenderTargetView>views[3];ID3D11RenderTargetView*bound[3];
    for(unsigned t=0;t<3;++t){D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=16;td.MipLevels=td.ArraySize=1;td.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;td.SampleDesc.Count=1;td.BindFlags=D3D11_BIND_RENDER_TARGET;HR(d->CreateTexture2D(&td,NULL,&target[t].p),"float result");HR(d->CreateRenderTargetView(target[t].p,NULL,&views[t].p),"float result view");bound[t]=views[t].p;td.Usage=D3D11_USAGE_STAGING;td.BindFlags=0;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;HR(d->CreateTexture2D(&td,NULL,&stage[t].p),"float readback");}
    std::vector<float>result[2];result[0].resize(3*16*16*4);result[1].resize(result[0].size());float maxDelta=0;unsigned cases=0;
    for(unsigned sid=0;sid<15;++sid)
    {
        for(unsigned k=0;k<2;++k)for(unsigned role=0;role<6;++role){float pixels[256];Texels(k?GeoBiomes[sid].normal[role]:GeoBiomes[sid].diffuse[role],k!=0,pixels);c->UpdateSubresource(native[k].p,role,NULL,pixels,128,0);}
        for(unsigned mode=0;mode<7;++mode)for(unsigned mix=0;mix<4;++mix)
        {
            Constants values={};const float scales[3][4]={{.7f,.9f,.6f,1.2f},{.8f,1.1f,.7f,1.3f},{.9f,1.4f,.5f,1.2f}};std::memcpy(values.p,scales,sizeof(scales));
            const float slope[4]={.1f,.6f,1,0},cliff[4]={0,.35f,1,0},road[4]={0,.6f,1,1},ratios[7]={.8f,0,.3f,.55f,.8f,1.3f,2};
            values.p[3][0]=slope[mix];values.p[3][1]=cliff[mix];values.p[4][0]=.3f;values.p[4][2]=.35f;values.p[4][3]=road[mix];
            const float colour[4]={.8f,1.1f,.7f,.83f},overlay[4]={.45f,.65f,.3f,.55f},absorb[8]={.12f,.2f,.35f,.18f,.27f,.4f,0,0};std::memcpy(values.p[5],colour,16);std::memcpy(values.p[6],overlay,16);std::memcpy(values.p[7],absorb,32);
            values.p[10][0]=FadeFixtures[sid].distance*ratios[mode];values.p[10][1]=FadeFixtures[sid].brightness;
            for(unsigned role=0;role<6;++role){values.p[11+role/4][role%4]=float(GeoBiomes[sid].diffuse[role]);values.p[13+role/4][role%4]=float(GeoBiomes[sid].normal[role]);}
            for(unsigned variant=0;variant<2;++variant)
            {
                std::memset(values.p[9],0,16);if(mode){if(variant==0)std::memcpy(values.p[9],FadeFixtures[sid].expected,16);else{float row[4];Check(SatelliteGeography::NativeFadeParameters(FadeFixtures[sid].argb,FadeFixtures[sid].distance,row),"production fade row");std::memcpy(values.p[9],row,16);}}
                c->UpdateSubresource(cb.p,0,NULL,&values,0,0);ID3D11ShaderResourceView*inputs[2]={variant?poolView[0].p:nativeView[0].p,variant?poolView[2].p:nativeView[1].p};c->PSSetShaderResources(0,2,inputs);c->PSSetShaderResources(7,1,&poolView[1].p);
                const float clear[4]={-12345,-12345,-12345,-12345};for(unsigned t=0;t<3;++t)c->ClearRenderTargetView(views[t].p,clear);c->OMSetRenderTargets(3,bound,NULL);c->PSSetShader(ps[variant].p,NULL,0);c->Draw(3,0);c->OMSetRenderTargets(0,NULL,NULL);
                for(unsigned t=0;t<3;++t){c->CopyResource(stage[t].p,target[t].p);D3D11_MAPPED_SUBRESOURCE m={};HR(c->Map(stage[t].p,0,D3D11_MAP_READ,0,&m),"float material readback");for(unsigned y=0;y<16;++y)std::memcpy(&result[variant][(t*256+y*16)*4],static_cast<char*>(m.pData)+y*m.RowPitch,256);c->Unmap(stage[t].p,0);}
            }
            for(unsigned i=0;i<result[0].size();++i){float a=result[0][i],b=result[1][i];Check(_finite(a)&&_finite(b)&&a!=-12345&&b!=-12345,"finite complete RGBA/normal/absorbance");float delta=std::fabs(a-b);maxDelta=maxDelta>delta?maxDelta:delta;if(delta>.000002f){std::fprintf(stderr,"sid%u mode%u mix%u target%u component%u native%.9g owned%.9g\n",sid,mode,mix,i/(256*4),i%4,a,b);Check(false,"native fade float RGBA/normal-alpha/absorbance/order parity");}}
            ++cases;
        }
    }
    c->ClearState();std::printf("PASS %u actual native computeBiome/production computeGeoBiome fade cases;%uchecks;15ARGBrows/off+0/onset/partial/full/road+cliff/alpha/normalRGBA/absorbance/brightness;maxAbs=%.9g tolerance=2e-6\n",cases,checks,maxDelta);
}
