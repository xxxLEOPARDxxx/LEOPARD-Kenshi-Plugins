"""Compile production descriptor/adoption/lifecycle bodies against counted owners.

This tests CPU control flow and ownership, not native Ogre or D3D rendering.
"""
from pathlib import Path
import os
import subprocess

HERE = Path(__file__).resolve().parent
SOURCE = (HERE.parent / 'NativeModel.h').read_text()
OUT = HERE / '_generated'
OUT.mkdir(exist_ok=True)
VC = Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
SDK = Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
ENV = dict(os.environ, INCLUDE=f'{VC}/include;{SDK}/Include',
           LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')


def method(source, name):
    start = source.index(name)
    opening = source.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


def fixture(source):
    descriptor = source[source.index('struct Descriptor {'):source.index('inline ULONG_PTR ComIdentity')]
    bodies = '\n'.join(method(source, name) for name in (
        'void AdoptDescriptor(', 'void Clear(){', 'HRESULT Capture(){',
        'HRESULT Read(bool enabled', 'HRESULT Refresh(bool enabled'))
    return r'''
#include <windows.h>
#include <cstring>
#include <stdexcept>
#include <cstdio>
unsigned creates=0,adopts=0,failCreate=0; bool failAdopt=false;
namespace AtlasModels {
struct Shape {unsigned data[90];};
struct RawDraw {Shape shape;void*vertex[16];void*index;void*diffuse;void*sampler;unsigned cullMode;};
}
struct D3D11_SAMPLER_DESC {unsigned data[13];};
struct Town{};namespace Ogre{struct SceneManager{};}
struct Ref {void*p;Ref():p(NULL){}void Reset(){p=NULL;}};
struct Device {HRESULT CreateSamplerState(const D3D11_SAMPLER_DESC*,void**out){++creates;if(failCreate==creates)return E_FAIL;*out=reinterpret_cast<void*>(ULONG_PTR(creates));return S_OK;}};
struct DeviceRef {Device*p;DeviceRef():p(NULL){}void Reset(){p=NULL;}};
struct Packet {
 unsigned count,generation;Packet():count(0),generation(0){}
 void Clear(){count=generation=0;}unsigned Count()const{return count;}unsigned Generation()const{return generation;}
 HRESULT Adopt(Device*,const AtlasModels::RawDraw*,unsigned n,const float*,const float*,unsigned g,const void*,size_t){++adopts;if(failAdopt)return E_FAIL;count=n;generation=g;return S_OK;}
};
const unsigned char AtlasModelVertexShader[]={0};
template<class O>HRESULT ReadRoute(bool e,bool v,unsigned g,O&o){if(!e){o.Clear();return S_FALSE;}if(!v||!g){o.Clear();return E_ACCESSDENIED;}return o.Capture();}
''' + descriptor + r'''
struct Snapshot {
 Packet packet_;Ref samplers_[16];DeviceRef device_;float localBounds_[6];Town*town_;Ogre::SceneManager*scene_;unsigned requested_;char error_[256];Descriptor cached_;bool cacheValid_,force_,lastChanged_;
 Descriptor incoming;Device dev;bool nativeFail;unsigned captures;
 Snapshot():town_(NULL),scene_(NULL),requested_(0),cacheValid_(false),force_(false),lastChanged_(false),nativeFail(false),captures(0){std::memset(localBounds_,0,sizeof(localBounds_));error_[0]=0;}
 void Error(const char*){}
 void Current(){++captures;if(nativeFail)throw std::runtime_error("retired source");device_.p=&dev;incoming.generation=requested_;AdoptDescriptor(incoming);}
 static bool TryCurrent(Snapshot*s){s->Current();return true;}
''' + bodies + r'''
};
unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::printf("FAIL line %u: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){Town town;Ogre::SceneManager scene;Snapshot s;s.incoming.count=2;s.incoming.device=1;s.incoming.material=2;
 bool changed=true;
 CHECK(s.Refresh(true,&town,&scene,1,true,&changed)==S_OK&&changed&&creates==2&&adopts==1);
 for(unsigned i=0;i<50;++i){changed=true;CHECK(s.Refresh(true,&town,&scene,1,true,&changed)==S_OK&&!changed&&creates==2&&adopts==1);}
 CHECK(!s.town_&&!s.scene_&&!s.requested_&&!s.device_.p);
 for(unsigned field=0;field<8;++field){switch(field){case 0:s.incoming.matrix[3]+=1;break;case 1:s.incoming.origin[2]+=1;break;case 2:s.incoming.bounds[4]+=1;break;case 3:s.incoming.material+=1;break;case 4:s.incoming.device+=1;break;case 5:s.incoming.identity[1][17]+=1;break;case 6:s.incoming.sampler[1].data[12]+=1;break;case 7:s.incoming.raw[1].shape.data[4]+=1;break;}
  unsigned before=adopts;CHECK(s.Refresh(true,&town,&scene,1,true,&changed)==S_OK&&changed&&adopts==before+1);
  CHECK(s.Refresh(true,&town,&scene,1,true,&changed)==S_OK&&!changed&&adopts==before+1);
 }
 unsigned before=adopts;CHECK(s.Refresh(true,&town,&scene,2,true,&changed)==S_OK&&changed&&adopts==before+1);
 // Read remains the explicit forced-fresh API, unlike Refresh.
 before=adopts;CHECK(s.Read(true,&town,&scene,2,true)==S_OK&&adopts==before+1);
 unsigned calls=s.captures;changed=true;CHECK(s.Refresh(false,&town,&scene,2,true,&changed)==S_FALSE&&!changed&&!s.packet_.Count()&&s.captures==calls);
 CHECK(!s.cacheValid_&&!s.town_&&!s.scene_&&!s.requested_);
 CHECK(s.Refresh(true,&town,&scene,2,false,&changed)==E_ACCESSDENIED&&s.captures==calls);
 CHECK(s.Refresh(true,&town,&scene,0,true,&changed)==E_ACCESSDENIED&&s.captures==calls);
 CHECK(s.Refresh(true,&town,&scene,3,true,&changed)==S_OK&&changed);
 s.nativeFail=true;CHECK(FAILED(s.Refresh(true,&town,&scene,3,true,&changed))&&!changed&&!s.packet_.Count()&&!s.cacheValid_);
 s.nativeFail=false;CHECK(s.Refresh(true,&town,&scene,3,true,&changed)==S_OK&&changed);
 failCreate=creates+1;s.incoming.matrix[3]+=1;CHECK(FAILED(s.Refresh(true,&town,&scene,3,true,&changed))&&!changed&&!s.packet_.Count()&&!s.cacheValid_&&!s.device_.p);
 failCreate=0;CHECK(s.Refresh(true,&town,&scene,3,true,&changed)==S_OK&&changed);
 failAdopt=true;s.incoming.bounds[4]+=1;CHECK(FAILED(s.Refresh(true,&town,&scene,3,true,&changed))&&!s.packet_.Count()&&!s.cacheValid_);
 for(unsigned i=0;i<16;++i)CHECK(!s.samplers_[i].p);
 std::printf("Actual Refresh/adoption/lifecycle: %u checks PASS\n",checks);return 0;
}
'''


def run(source, name, expected):
    cpp = OUT / (name + '.cpp')
    cpp.write_text(fixture(source))
    subprocess.run([str(VC / 'bin/amd64/cl.exe'), '/nologo', '/EHsc', '/O2', '/MT',
                    str(cpp), f'/Fe{OUT/name}.exe', f'/Fo{OUT/name}.obj'],
                   env=ENV, check=True, capture_output=True)
    result = subprocess.run([str(OUT / (name + '.exe'))], capture_output=True, text=True)
    if (result.returncode == 0) != expected:
        raise AssertionError(f'{name}: {result.stdout} {result.stderr}')
    print(name, result.stdout.strip())


run(SOURCE, 'actual_refresh', True)
for name, old, new in (
    ('always_rebuild', '!force_&&cacheValid_', 'false&&cacheValid_'),
    ('ignore_bounds', '||std::memcmp(a.bounds,b.bounds,sizeof(a.bounds))', ''),
    ('ignore_pose', 'std::memcmp(a.matrix,b.matrix,sizeof(a.matrix))||', ''),
    ('ignore_diffuse', 'std::memcmp(a.identity[n],b.identity[n],sizeof(a.identity[n]))||', ''),
    ('ignore_sampler', '||std::memcmp(&a.sampler[n],&b.sampler[n],sizeof(a.sampler[n]))', ''),
    ('stale_on_failure', 'Error(e.what());Clear();return E_FAIL;', 'Error(e.what());return E_FAIL;'),
):
    assert old in SOURCE, name
    run(SOURCE.replace(old, new), name, False)
print('6 compiled Refresh mutants rejected')
