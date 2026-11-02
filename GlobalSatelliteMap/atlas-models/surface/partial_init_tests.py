"""Exercise actual Init/Clear bodies with an allocation-failing device seam."""
from pathlib import Path
import os, subprocess
HERE=Path(__file__).resolve().parent
OUT=HERE/'_generated'
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
header=(HERE/'AtlasModelSurface.h').read_text()
def body(s,marker):
    a=s.index(marker); b=s.index('{',a);i=b+1;depth=1
    while depth:
        depth+=(s[i]=='{')-(s[i]=='}');i+=1
    return s[a:i]
template=r'''
#include <d3d11.h>
#include <cstdio>
#include <cstring>
typedef unsigned __int64 U64;
namespace AtlasView{const U64 GpuCap=201326592;}
const unsigned Side=256;const U64 PrivateBytes=1048576;
inline bool Fits(U64 n){return n<=AtlasView::GpuCap-PrivateBytes;}
inline DXGI_FORMAT Format(unsigned p){const DXGI_FORMAT f[]={DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_R32_FLOAT};return f[p];}
static unsigned live=0,checks=0;
static void C(bool b,const char*s){++checks;if(!b){std::printf("FAIL %s\n",s);throw 1;}}
struct Resource{Resource(){++live;}void Release(){--live;delete this;}};
template<class T>struct Pointer{T*p;Pointer():p(NULL){}~Pointer(){Reset();}void Reset(){if(p)p->Release();p=NULL;}};
struct Device{
 unsigned fail,operations,refs;bool unsupported;Device(unsigned f=0):fail(f),operations(0),refs(1),unsupported(false){}
 D3D_FEATURE_LEVEL GetFeatureLevel(){return D3D_FEATURE_LEVEL_11_0;}
 HRESULT CheckFormatSupport(DXGI_FORMAT,UINT*s){*s=unsupported?0:D3D11_FORMAT_SUPPORT_TEXTURE2D|D3D11_FORMAT_SUPPORT_SHADER_LOAD;return S_OK;}
 HRESULT CreateTexture2D(const D3D11_TEXTURE2D_DESC*,const D3D11_SUBRESOURCE_DATA*,Resource**p){if(++operations==fail)return E_OUTOFMEMORY;*p=new Resource;return S_OK;}
 HRESULT CreateShaderResourceView(Resource*,const D3D11_SHADER_RESOURCE_VIEW_DESC*,Resource**p){if(++operations==fail)return E_OUTOFMEMORY;*p=new Resource;return S_OK;}
 void AddRef(){++refs;}void Release(){--refs;}
};
struct Layer {Pointer<Device>device_;Pointer<Resource>texture_[3],view_[3];
CLEAR
INIT
};
int main(){try{
 for(unsigned fail=1;fail<=6;++fail){Device d(fail);{Layer s;C(s.Init(true,&d,0)==E_OUTOFMEMORY,"actual Init propagates each texture/SRV failure");C(!live&&!s.device_.p&&!s.texture_[0].p&&!s.texture_[1].p&&!s.texture_[2].p&&!s.view_[0].p&&!s.view_[1].p&&!s.view_[2].p,"partial Init releases ALL prior allocations");C(d.refs==1,"failed Init never credits device");d.fail=0;C(s.Init(true,&d,0)==S_OK&&live==6&&d.refs==2,"retry after partial failure creates exactly six objects");s.Clear();C(!live&&d.refs==1,"explicit Clear balances retry");}C(!live&&d.refs==1,"destruction after Clear balanced");}
 Device d;d.unsupported=true;{Layer s;C(FAILED(s.Init(true,&d,0))&&!d.operations&&!live&&d.refs==1,"format rejection before allocation");}
 {Layer s;C(s.Init(false,reinterpret_cast<Device*>(1),~U64(0))==S_FALSE,"OFF does not query sentinel device");C(FAILED(s.Init(true,NULL,0))&&!live,"null Init inert");C(FAILED(s.Init(true,&d,AtlasView::GpuCap-PrivateBytes+1))&&!d.operations,"budget failure before device/allocation");}
 d.unsupported=false;{Layer s;C(s.Init(true,&d,0)==S_OK&&live==6&&d.refs==2,"valid Init");C(FAILED(s.Init(true,&d,0))&&d.operations==6,"double Init does not allocate");}C(!live&&d.refs==1,"implicit destructor releases owned resources/device");
 std::printf("surface extracted Init/Clear %u checks PASS\n",checks);return 0;
}catch(...){return 1;}}
'''
env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
def run(source,name,expect=None):
    text=template.replace('CLEAR',body(source,'void Clear()')).replace('INIT',body(source,'HRESULT Init(').replace('ID3D11Device*d','Device*d'))
    cpp=OUT/(name+'.cpp');exe=OUT/(name+'.exe');cpp.write_text(text)
    args=[VC/'bin/amd64/cl.exe','/nologo','/EHsc','/MD','/O2','/W3','/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),cpp]
    p=subprocess.run([str(a) for a in args],env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    (OUT/(name+'-build.log')).write_text(p.stdout)
    if p.returncode:raise RuntimeError(p.stdout)
    p=subprocess.run([str(exe)],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
    (OUT/(name+'-run.log')).write_text(p.stdout)
    if expect:
        assert p.returncode and 'FAIL '+expect in p.stdout,(name,p.stdout)
        print('Expected RED:',name,'->',expect)
    else:
        assert p.returncode==0,p.stdout
        print(p.stdout.strip())
run(header,'partial-init')
for name,old,new,reason in [
 ('leak-partial','if(FAILED(hr)){Clear();return hr;}','if(FAILED(hr)){return hr;}','partial Init releases ALL prior allocations'),
 ('double-device-credit','device_.p=d;d->AddRef();','device_.p=d;d->AddRef();d->AddRef();','retry after partial failure creates exactly six objects'),
 ('omit-view-clear','view_[i].Reset();texture_[i].Reset();','texture_[i].Reset();','explicit Clear balances retry'),
]:
    assert old in header,name
    run(header.replace(old,new),name,reason)
