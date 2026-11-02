"""Compiled controls of real compact owner; compilation failures are not kills."""
from pathlib import Path
import os,subprocess
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=os.environ.copy();env['INCLUDE']=f'{VC}/include;{SDK}/Include';env['LIB']=f'{VC}/lib/amd64;{SDK}/Lib/x64'
header=(HERE/'CompactMaterial.h').read_text();test=(HERE/'warp_tests.cpp').read_text()
def edit(s,a,b):assert s.count(a)==1,(a,s.count(a));return s.replace(a,b)
def buildrun(name,h,body,green=False):
 hp=OUT/(name+'.h');h=h.replace('"../../atlas-view/AtlasRelight.h"','"'+(HERE.parents[1]/'atlas-view/AtlasRelight.h').as_posix()+'"').replace('"../../../tools/diagnostics/satellite-capture-probe/GeographyPolicy.h"','"'+(HERE.parents[2]/'tools/diagnostics/satellite-capture-probe/GeographyPolicy.h').as_posix()+'"').replace('"_generated/Case.h"','"'+(OUT/'Case.h').as_posix()+'"');hp.write_text(h)
 body=body.replace('"CompactMaterial.h"','"'+hp.as_posix()+'"').replace('"_generated/','"'+OUT.as_posix()+'/').replace('"../fixed-geometry/FixedTerrainGeometry.h"','"'+(HERE.parent/'fixed-geometry/FixedTerrainGeometry.h').as_posix()+'"').replace('"../../atlas-view/ContextFixture.h"','"'+(HERE.parents[1]/'atlas-view/ContextFixture.h').as_posix()+'"');sp=OUT/(name+'.cpp');sp.write_text(body);exe=OUT/(name+'.exe')
 p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT',str(sp),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib','advapi32.lib'],env=env,cwd=OUT,capture_output=True,text=True);(OUT/(name+'-build.log')).write_text(p.stdout+p.stderr);assert not p.returncode,(name,'compile failure not kill',p.stdout,p.stderr)
 p=subprocess.run([str(exe)],cwd=OUT,capture_output=True,text=True);(OUT/(name+'-run.log')).write_text(p.stdout+p.stderr);assert (p.returncode==0 if green else p.returncode!=0 and 'FAIL ' in p.stdout+p.stderr),(name,p.returncode,p.stdout,p.stderr);print(('PASS injected allocation failure: 'if green else 'Expected RED: ')+name+' '+(p.stdout+p.stderr).strip()[-170:],flush=True)
mutants=[('lost-cap','other<=Limit-SourceBytes&&other<=AtlasView::GpuCap-SourceBytes','true'),('lost-dds-hash','!HashMatches(data,bytes,DdsPins[index])','false'),('wrong-dds-layer','D3D11CalcSubresource(mip,index%6,9)','D3D11CalcSubresource(mip,(index+1)%6,9)'),('lost-rgba-hash','!HashMatches(data,bytes,RgbaPins[index])','false'),('early-publish','mask_!=32767','mask_==32767'),('lost-duplicate','(mask_&(1u<<index))','false'),('lost-clear-charge','device_.Reset();mask_=0;','mask_=0;'),('lost-state','broken_=true;for(unsigned mip','c.p->ClearState();broken_=true;for(unsigned mip'),('native-predicate','c_->SetPredication(NULL,FALSE);','')]
for n,a,b in mutants:buildrun(n,edit(header,a,b),test)
fault=r'''#include "CompactMaterial.h"
#include "../../atlas-view/ContextFixture.h"
#include <cstdio>
int main(){ID3D11Device*d=NULL;ID3D11DeviceContext*c=NULL;if(FAILED(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&d,NULL,&c)))return 1;ULONG before=d->AddRef();d->Release();{CompactMaterial::Inputs p;AtlasViewTest::ContextFixture a(c);if(SUCCEEDED(p.Init(d,0))||p.OwnedGpuBytes()||p.Ready())return 2;AtlasViewTest::ContextFixture b(c);if(!a.Equal(b))return 3;}ULONG after=d->AddRef();d->Release();if(before!=after)return 4;c->Release();d->Release();std::puts("PASS actual failed D3D allocation clears previous owned COM objects/state");}
'''
for i in range(7):buildrun('allocation-stage'+str(i),edit(header,'HRESULT hr=device_.p->CreateTexture2D','if(i==%d)t.Format=DXGI_FORMAT_UNKNOWN;HRESULT hr=device_.p->CreateTexture2D'%i),fault,True)
print('PASS %d functional mutants and7 actual allocation-failure paths'%len(mutants))
