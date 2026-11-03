"""Inject actual D3D allocation refusals into an isolated copy of the real Init."""
from pathlib import Path
import os,subprocess,re
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
def absolute(s):
 for a,b in [('../raster/',HERE.parent/'raster'),('../raw-rgb/',HERE.parent/'raw-rgb'),('../compact-material/',HERE.parent/'compact-material'),('../../atlas-view/',HERE.parents[1]/'atlas-view')]:s=s.replace('"'+a,'"'+b.as_posix()+'/')
 return s
def instrument(header):
 matches=list(re.finditer(r'd->(?:Create\w+|GetDeviceRemovedReason)\(',header))
 for m in reversed(matches):
  end=m.end();depth=1
  while depth:
   if header[end]=='(':depth+=1
   if header[end]==')':depth-=1
   end+=1
  header=header[:m.start()]+'(RgbFault::Next()?E_OUTOFMEMORY:'+header[m.start():end]+')'+header[end:]
 return header
raw=(HERE.parent/'raw-rgb/RawMaterial.h').read_text().replace('"../../atlas-view/','"'+(HERE.parents[1]/'atlas-view').as_posix()+'/')
rp=OUT/'fault-targets.h';rp.write_text(instrument(raw))
header=absolute((HERE/'RgbTerrainRaster.h').read_text())
header=header.replace('"'+(HERE.parent/'raw-rgb').as_posix()+'/RawMaterial.h"','"'+rp.as_posix()+'"')
header='namespace RgbFault {static unsigned faultAt=~0u,faultCount=0;static bool Next(){return faultCount++==faultAt;}}\n'+instrument(header)
hp=OUT/'fault-raster.h';hp.write_text(header)
body=r'''
#define _CRT_SECURE_NO_WARNINGS
#include "FAULT_HEADER"
#include "SHADERS_HEADER"
#include "RAW_SHADERS_HEADER"
#include "CONTEXT_HEADER"
#include <cstdio>
#include <fstream>
#include <vector>
static ULONG Refs(IUnknown*p){ULONG n=p->AddRef();p->Release();return n;}
int main(){using namespace TerrainRgbRaster;using namespace RgbFault;using AtlasView::Com;Com<ID3D11Device>d;Com<ID3D11DeviceContext>c;if(FAILED(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&d.p,NULL,&c.p)))return 2;
 std::ifstream f("../compact-material/_generated/height.u16",std::ios::binary);std::vector<unsigned short> h(133*133);f.read(reinterpret_cast<char*>(&h[0]),h.size()*2);if(!f)return 3;FixedTerrain::Samples source={7358,7358,133,133,133,&h[0],h.size()};Grid g={7360,7360,2,2,1};Shaders s={{CompactMaterial::VertexShader,sizeof(CompactMaterial::VertexShader)},{TerrainRaw::RgbShader,sizeof(TerrainRaw::RgbShader)}};
 faultAt=~0u;faultCount=0;{Raster r;if(FAILED(r.Init(d.p,s,source,g,CompactMaterial::SourceBytes)))return 4;}unsigned total=faultCount;
 for(unsigned n=0;n<total;++n){faultAt=n;faultCount=0;ULONG refs=Refs(d.p);Raster r;AtlasViewTest::ContextFixture before(c.p);HRESULT hr=r.Init(d.p,s,source,g,CompactMaterial::SourceBytes);AtlasViewTest::ContextFixture after(c.p);if(SUCCEEDED(hr)||r.OwnedGpuBytes()||r.Texture(0)||r.Texture(1)||r.Texture(2)||!before.Equal(after)||refs!=Refs(d.p)){std::fprintf(stderr,"FAIL partial allocation cleanup stage%u hr%lx\n",n,hr);return 1;}faultAt=~0u;faultCount=0;if(FAILED(r.Init(d.p,s,source,g,CompactMaterial::SourceBytes))){std::fprintf(stderr,"FAIL retry stage%u\n",n);return 1;}}
 std::printf("PASS %u exact Init allocation/health failure cleanup/retry paths\n",total);return 0;}
'''
body=body.replace('RAW_SHADERS_HEADER',(HERE.parent/'raw-rgb/_generated/RawShaders.h').as_posix()).replace('FAULT_HEADER',hp.as_posix()).replace('SHADERS_HEADER',(HERE.parent/'compact-material/_generated/Shaders.h').as_posix()).replace('CONTEXT_HEADER',(HERE.parents[1]/'atlas-view/ContextFixture.h').as_posix())
cpp=OUT/'faults.cpp';cpp.write_text(body);exe=OUT/'faults.exe'
p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT','/W3',str(cpp),'/Fo'+str(OUT/'faults.obj'),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib','advapi32.lib'],env=env,cwd=HERE,capture_output=True,text=True);(OUT/'faults-build.log').write_text(p.stdout+p.stderr);assert p.returncode==0,p.stdout+p.stderr
p=subprocess.run([str(exe)],cwd=HERE,capture_output=True,text=True);(OUT/'faults.log').write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='');assert p.returncode==0
# The fault test must also catch losing the actual production cleanup branch.
assert header.count('if(FAILED(hr)){Clear();return hr;}')==1
hp.write_text(header.replace('if(FAILED(hr)){Clear();return hr;}','if(FAILED(hr)){return hr;}'))
exe=OUT/'faults-no-cleanup.exe'
p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT','/W3',str(cpp),'/Fo'+str(OUT/'faults-no-cleanup.obj'),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib','advapi32.lib'],env=env,cwd=HERE,capture_output=True,text=True);(OUT/'faults-no-cleanup-build.log').write_text(p.stdout+p.stderr);assert p.returncode==0,p.stdout+p.stderr
p=subprocess.run([str(exe)],cwd=HERE,capture_output=True,text=True);(OUT/'faults-no-cleanup.log').write_text(p.stdout+p.stderr);assert p.returncode==1 and 'FAIL partial allocation cleanup' in p.stderr,(p.returncode,p.stdout,p.stderr)
hp.write_text(header)
print('Expected RED: partial cleanup',p.stderr.strip())
