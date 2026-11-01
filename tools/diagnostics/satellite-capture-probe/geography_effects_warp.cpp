#include <d3d11.h>
#include <vector>
#include <fstream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <float.h>
template<class T>struct Com{T*p;Com():p(NULL){}~Com(){if(p)p->Release();}T*operator->(){return p;}private:Com(const Com&);Com&operator=(const Com&);};
static unsigned checks=0;void Check(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);std::exit(1);}}
void HR(HRESULT h,const char*s){Check(SUCCEEDED(h),s);}
std::vector<char>File(const std::string&p){std::ifstream f(p.c_str(),std::ios::binary|std::ios::ate);Check(!!f,"fixture exists");size_t n=size_t(f.tellg());Check(n&&n<1024*1024,"fixture bounded");f.seekg(0);std::vector<char>b(n);f.read(&b[0],n);Check(!!f,"fixture read");return b;}
struct Constants {float wvp[16],world[16],morph[4],distortion[60],options[4];};
struct Vertex {float position[4],normal[3],normal1[3];};
static const unsigned Side=32,Targets=5;
unsigned Ordered(float f){unsigned u;std::memcpy(&u,&f,4);return u&0x80000000u?~u:u|0x80000000u;}
int main(int argc,char**argv)
{
    Check(argc==6,"expected referenceVS/ownedVS/referencePS/ownedPS/distortion");
    std::vector<char>rvs=File(argv[1]),ovs=File(argv[2]),rps=File(argv[3]),ops=File(argv[4]),distortion=File(argv[5]);Check(distortion.size()==240,"15 actual signed parameters");
    Com<ID3D11Device>d;Com<ID3D11DeviceContext>c;HR(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&d.p,NULL,&c.p),"WARP");
    Com<ID3D11VertexShader>vs[2];Com<ID3D11PixelShader>ps[2];HR(d->CreateVertexShader(&rvs[0],rvs.size(),NULL,&vs[0].p),"native-source VS");HR(d->CreateVertexShader(&ovs[0],ovs.size(),NULL,&vs[1].p),"production packed VS");HR(d->CreatePixelShader(&rps[0],rps.size(),NULL,&ps[0].p),"independent native field derivatives");HR(d->CreatePixelShader(&ops[0],ops.size(),NULL,&ps[1].p),"production shared derivatives");
    D3D11_INPUT_ELEMENT_DESC elements[]={{"POSITION",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,16,D3D11_INPUT_PER_VERTEX_DATA,0},{"BINORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,28,D3D11_INPUT_PER_VERTEX_DATA,0}};
    Com<ID3D11InputLayout>layout;HR(d->CreateInputLayout(elements,3,&rvs[0],rvs.size(),&layout.p),"native packed input layout");c->IASetInputLayout(layout.p);c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    D3D11_BUFFER_DESC bd={};bd.ByteWidth=sizeof(Constants);bd.Usage=D3D11_USAGE_DEFAULT;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;Com<ID3D11Buffer>cb;HR(d->CreateBuffer(&bd,NULL,&cb.p),"bounded fixture constants");c->VSSetConstantBuffers(0,1,&cb.p);c->PSSetConstantBuffers(0,1,&cb.p);
    bd.ByteWidth=sizeof(Vertex)*6;bd.BindFlags=D3D11_BIND_VERTEX_BUFFER;Com<ID3D11Buffer>vb;HR(d->CreateBuffer(&bd,NULL,&vb.p),"native fixture mesh");UINT stride=sizeof(Vertex),offset=0;c->IASetVertexBuffers(0,1,&vb.p,&stride,&offset);
    D3D11_RASTERIZER_DESC raster={};raster.FillMode=D3D11_FILL_SOLID;raster.CullMode=D3D11_CULL_NONE;raster.DepthClipEnable=TRUE;Com<ID3D11RasterizerState>rs;HR(d->CreateRasterizerState(&raster,&rs.p),"fixture raster");c->RSSetState(rs.p);D3D11_VIEWPORT viewport={0,0,float(Side),float(Side),0,1};c->RSSetViewports(1,&viewport);
    Com<ID3D11Texture2D>textures[Targets],staging[Targets];Com<ID3D11RenderTargetView>views[Targets];ID3D11RenderTargetView*bound[Targets];
    for(unsigned t=0;t<Targets;++t){D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=Side;td.MipLevels=td.ArraySize=1;td.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;td.SampleDesc.Count=1;td.BindFlags=D3D11_BIND_RENDER_TARGET;HR(d->CreateTexture2D(&td,NULL,&textures[t].p),"float result");HR(d->CreateRenderTargetView(textures[t].p,NULL,&views[t].p),"result view");bound[t]=views[t].p;td.Usage=D3D11_USAGE_STAGING;td.BindFlags=0;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;HR(d->CreateTexture2D(&td,NULL,&staging[t].p),"bounded float readback");}
    std::vector<float>result[2];result[0].resize(Targets*Side*Side*4);result[1].resize(result[0].size());float maxHeight=0,maxDerivative=0;unsigned maxUlp=0,cases=0;
    for(unsigned phase=0;phase<3;++phase)for(unsigned morph=0;morph<3;++morph)for(unsigned sid=0;sid<16;++sid)
    {
        Constants values={};std::memcpy(values.distortion,&distortion[0],240);float cx=phase?65000.0f:-36864.0f,cz=phase?-90000.0f:-36864.0f,extent=36864.0f;
        values.wvp[0]=1/extent;values.wvp[3]=-cx/extent;values.wvp[6]=1/extent;values.wvp[7]=-cz/extent;values.wvp[9]=.00001f;values.wvp[11]=.5f;values.wvp[15]=1;
        values.world[0]=phase?.8f:1;values.world[2]=phase?.6f:0;values.world[3]=phase?17000.0f:0;values.world[5]=1;values.world[7]=phase?300.0f:0;values.world[8]=phase?-.6f:0;values.world[10]=phase?.8f:1;values.world[11]=phase?-13000.0f:0;values.world[15]=1;
        if(phase==2){values.distortion[14*4]=0;values.distortion[14*4+1]=-.07f;} // Nativezero-frequency nonzero amplitude must remain a signed constant offset.
        values.morph[0]=morph==0?0:morph==1?.37f:1;values.options[0]=float(sid);values.options[1]=phase?float((sid+morph)%4):0;c->UpdateSubresource(cb.p,0,NULL,&values,0,0);
        Vertex mesh[6];const int corners[6][2]={{-1,-1},{-1,1},{1,-1},{1,-1},{-1,1},{1,1}};
        for(unsigned v=0;v<6;++v){mesh[v].position[0]=cx+extent*corners[v][0];mesh[v].position[1]=600+200.0f*corners[v][0]+350.0f*corners[v][1];mesh[v].position[2]=cz+extent*corners[v][1];mesh[v].position[3]=1400-350.0f*corners[v][0]+100.0f*corners[v][1];mesh[v].normal[0]=.8f;mesh[v].normal[1]=.5f;mesh[v].normal[2]=.3f;mesh[v].normal1[0]=.4f;mesh[v].normal1[1]=.8f;mesh[v].normal1[2]=.6f;}c->UpdateSubresource(vb.p,0,NULL,mesh,0,0);
        for(unsigned variant=0;variant<2;++variant){const float clear[4]={-12345,-12345,-12345,-12345};for(unsigned t=0;t<Targets;++t)c->ClearRenderTargetView(views[t].p,clear);c->OMSetRenderTargets(Targets,bound,NULL);c->VSSetShader(vs[variant].p,NULL,0);c->PSSetShader(ps[variant].p,NULL,0);c->Draw(6,0);c->OMSetRenderTargets(0,NULL,NULL);
            for(unsigned t=0;t<Targets;++t){c->CopyResource(staging[t].p,textures[t].p);D3D11_MAPPED_SUBRESOURCE mapped={};HR(c->Map(staging[t].p,0,D3D11_MAP_READ,0,&mapped),"float fixture readback");for(unsigned y=0;y<Side;++y)std::memcpy(&result[variant][(t*Side*Side+y*Side)*4],static_cast<char*>(mapped.pData)+y*mapped.RowPitch,Side*16);c->Unmap(staging[t].p,0);}}
        for(unsigned t=0;t<Targets;++t)for(unsigned pixel=0;pixel<Side*Side;++pixel)for(unsigned channel=0;channel<4;++channel){size_t index=(t*Side*Side+pixel)*4+channel;float a=result[0][index],b=result[1][index];Check(_finite(a)&&_finite(b)&&a!=-12345&&b!=-12345,"finite complete native/owned field coverage");float delta=std::fabs(a-b);
            if(t==0&&channel<3){if(channel==0)maxHeight=(maxHeight>delta?maxHeight:delta);else maxDerivative=(maxDerivative>delta?maxDerivative:delta);unsigned oa=Ordered(a),ob=Ordered(b),ulp=oa>ob?oa-ob:ob-oa;maxUlp=maxUlp>ulp?maxUlp:ulp;if(delta>.000002f){std::fprintf(stderr,"case phase%u morph%u sid%u pixel%u channel%u native%.9g owned%.9g\n",phase,morph,sid,pixel,channel,a,b);Check(false,"native per-SID vertex height/interpolated gradient");}}
            else Check(a==b,"native geometry/morph/world normal/base UV/map packing unchanged");}
        ++cases;
    }
    c->ClearState();std::printf("PASS %u actual native-source/production packed VS cases; %u checks;15SIDs/nativeSlot0+4slots/morph0,.37,1/nonidentityWorld/cross-SID quads/zeroFrequencySignedOffset; maxHeight=%.9g maxDerivative=%.9g maxULP=%u toleranceAbs=2e-6; geometry exact\n",cases,checks,maxHeight,maxDerivative,maxUlp);
}
