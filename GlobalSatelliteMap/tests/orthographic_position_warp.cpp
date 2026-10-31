#include <d3d11.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <cmath>
#include <limits>
#include <stdexcept>
template<class T> struct Com
{
    T *p;
    Com():p(NULL){}
    ~Com(){if(p)p->Release();}
    T *operator->(){return p;}
private:
    Com(const Com&); Com &operator=(const Com&);
};
void HR(HRESULT hr){if(FAILED(hr))throw std::runtime_error("D3D11 operation failed");}
struct Four {float v[4];};
struct Case {Four input;double x,y,z;bool valid;};
void Point(std::vector<Case>&c,double x,double y,double forward,float farClip)
{
    Case a={};a.input.v[0]=float(std::sqrt(x*x+y*y+forward*forward)/farClip);
    a.input.v[1]=farClip;a.input.v[2]=float(x);a.input.v[3]=float(y);
    a.x=x;a.y=y;a.z=-forward;a.valid=true;c.push_back(a);
}
void Invalid(std::vector<Case>&c,float depth,float farClip,float x,float y)
{
    Case a={};a.input.v[0]=depth;a.input.v[1]=farClip;a.input.v[2]=x;a.input.v[3]=y;c.push_back(a);
}
int main(int argc,char**argv)
{
    try
    {
        if(argc!=2)throw std::runtime_error("expected compiled shader path");
        std::ifstream file(argv[1],std::ios::binary|std::ios::ate);
        if(!file)throw std::runtime_error("shader unavailable");
        std::streamoff bytes=file.tellg();
        if(bytes<=0||bytes>1024*1024)throw std::runtime_error("shader size invalid");
        file.seekg(0);std::vector<char> code(size_t(bytes),0);file.read(&code[0],bytes);
        if(!file)throw std::runtime_error("shader truncated");
        std::vector<Case> cases;
        for(int x=-1;x<=1;++x)for(int y=-1;y<=1;++y)
        {
            Point(cases,x*512.0,y*512.0,4000,10000);
            Point(cases,x*25600.0,y*25600.0,4000,120000);
        }
        Point(cases,0,0,1,10000);
        Point(cases,512,0,1,10000);
        Point(cases,512,0,0,1024);
        Invalid(cases,0,10000,0,0);
        Invalid(cases,-1,10000,0,0);
        Invalid(cases,.01f,10000,512,0);
        Invalid(cases,std::numeric_limits<float>::quiet_NaN(),10000,0,0);
        Invalid(cases,std::numeric_limits<float>::infinity(),10000,0,0);
        Invalid(cases,1,10000,std::numeric_limits<float>::quiet_NaN(),0);
        Invalid(cases,1,10000,0,std::numeric_limits<float>::quiet_NaN());
        Invalid(cases,1,10000,0,std::numeric_limits<float>::infinity());
        Invalid(cases,1,1e38f,0,0);
        Invalid(cases,1,10000,1e38f,0);
        Case grazing={};grazing.x=24000;grazing.y=12000;grazing.z=0;grazing.valid=true;
        grazing.input.v[0]=float(std::sqrt(720000000.0-180.0)/120000.0);
        grazing.input.v[1]=120000;grazing.input.v[2]=24000;grazing.input.v[3]=12000;
        cases.push_back(grazing);
        Invalid(cases,float(std::sqrt(720000000.0-1400.0)/120000.0),120000,24000,12000);
        std::vector<Four> input(cases.size());
        for(size_t i=0;i<cases.size();++i)input[i]=cases[i].input;
        Com<ID3D11Device> device;Com<ID3D11DeviceContext> context;
        D3D_FEATURE_LEVEL requested=D3D_FEATURE_LEVEL_11_0,got;
        HR(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,&requested,1,D3D11_SDK_VERSION,&device.p,&got,&context.p));
        Com<ID3D11ComputeShader> shader;HR(device->CreateComputeShader(&code[0],code.size(),NULL,&shader.p));
        D3D11_BUFFER_DESC desc={};desc.ByteWidth=UINT(input.size()*sizeof(Four));
        desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
        desc.MiscFlags=D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;desc.StructureByteStride=sizeof(Four);
        D3D11_SUBRESOURCE_DATA data={};data.pSysMem=&input[0];
        Com<ID3D11Buffer> in,out,staging;HR(device->CreateBuffer(&desc,&data,&in.p));
        desc.BindFlags=D3D11_BIND_UNORDERED_ACCESS;HR(device->CreateBuffer(&desc,NULL,&out.p));
        desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
        desc.MiscFlags=0;desc.StructureByteStride=0;HR(device->CreateBuffer(&desc,NULL,&staging.p));
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc={};srvDesc.Format=DXGI_FORMAT_UNKNOWN;
        srvDesc.ViewDimension=D3D11_SRV_DIMENSION_BUFFER;srvDesc.Buffer.NumElements=UINT(cases.size());
        Com<ID3D11ShaderResourceView> srv;HR(device->CreateShaderResourceView(in.p,&srvDesc,&srv.p));
        D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc={};uavDesc.Format=DXGI_FORMAT_UNKNOWN;
        uavDesc.ViewDimension=D3D11_UAV_DIMENSION_BUFFER;uavDesc.Buffer.NumElements=UINT(cases.size());
        Com<ID3D11UnorderedAccessView> uav;HR(device->CreateUnorderedAccessView(out.p,&uavDesc,&uav.p));
        context->CSSetShader(shader.p,NULL,0);context->CSSetShaderResources(0,1,&srv.p);
        context->CSSetUnorderedAccessViews(0,1,&uav.p,NULL);context->Dispatch(UINT(cases.size()),1,1);
        context->CopyResource(staging.p,out.p);
        D3D11_MAPPED_SUBRESOURCE mapped={};HR(context->Map(staging.p,0,D3D11_MAP_READ,0,&mapped));
        const Four*result=static_cast<const Four*>(mapped.pData);unsigned failures=0;
        for(size_t i=0;i<cases.size();++i)
        {
            const Case&a=cases[i];const Four&r=result[i];
            bool ok=(r.v[3]>.5f)==a.valid;
            if(a.valid)ok=ok&&std::fabs(r.v[0]-a.x)<.003&&std::fabs(r.v[1]-a.y)<.003&&std::fabs(r.v[2]-a.z)<.1;
            else ok=ok&&r.v[0]==0&&r.v[1]==0&&r.v[2]==0;
            if(!ok){++failures;std::cerr<<"FAIL case"<<i<<" got "<<r.v[0]<<','<<r.v[1]<<','<<r.v[2]<<','<<r.v[3]<<'\n';}
        }
        context->Unmap(staging.p,0);context->ClearState();
        if(failures){std::cerr<<failures<<" shader failures\n";return 1;}
        std::cout<<cases.size()<<" actual HLSL orthographic-position cases passed on isolated D3D11 WARP\n";
        return 0;
    }
    catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 2;}
}
