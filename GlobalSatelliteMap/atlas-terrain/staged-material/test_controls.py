"""Bounded actual WARP fault and compiled shader/orchestration negative controls."""
from pathlib import Path
import os,re,subprocess
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
FXC=Path('C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe')
ENV=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
def execute(name,body,red=False):
 cpp=OUT/(name+'.cpp');cpp.write_text(body);exe=OUT/(name+'.exe')
 p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT','/W3','/I'+str(HERE),str(cpp),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib','advapi32.lib'],cwd=HERE,env=ENV,capture_output=True,text=True)
 (OUT/(name+'-build.log')).write_text(p.stdout+p.stderr);assert p.returncode==0,('compile failure is not a negative control',name,p.stdout,p.stderr)
 p=subprocess.run([str(exe)],cwd=HERE,capture_output=True,text=True);(OUT/(name+'-run.log')).write_text(p.stdout+p.stderr)
 if red:
  assert p.returncode==1 and 'FAIL ' in p.stderr,(name,p.returncode,p.stdout,p.stderr)
  print('Expected RED:',name,p.stderr.strip(),flush=True)
 else:
  assert p.returncode==0,(name,p.returncode,p.stdout,p.stderr)
  print(p.stdout.strip(),flush=True)

header=(HERE/'StagedMaterial.h').read_text()
matches=list(re.finditer(r'd->(?:Create\w+|GetDeviceRemovedReason)\(',header));faultHeader=header
for m in reversed(matches):
 end=m.end();depth=1
 while depth:
  if faultHeader[end]=='(':depth+=1
  if faultHeader[end]==')':depth-=1
  end+=1
 faultHeader=faultHeader[:m.start()]+'(Fault::Next()?E_OUTOFMEMORY:'+faultHeader[m.start():end]+')'+faultHeader[end:]
faultHeader='namespace Fault {static unsigned at=~0u,count=0;static bool Next(){return count++==at;}}\n'+faultHeader
hp=OUT/'fault-owner.h';hp.write_text(faultHeader.replace('"../raster/','"'+(HERE.parent/'raster').as_posix()+'/'))
faultTest=r'''
#include "FAULT_HEADER"
#include "../../atlas-view/ContextFixture.h"
#include <cstdio>
static ULONG Refs(IUnknown*p){ULONG n=p->AddRef();p->Release();return n;}
int main(){using namespace StagedMaterial;using AtlasView::Com;Com<ID3D11Device>d;Com<ID3D11DeviceContext>c;if(FAILED(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&d.p,NULL,&c.p)))return 2;
 Fault::at=~0u;Fault::count=0;{Carry x;if(FAILED(x.Init(d.p,0)))return 2;}unsigned count=Fault::count;if(count!=14){std::fprintf(stderr,"FAIL incomplete fault cutpoints%u\n",count);return 1;}
 for(unsigned i=0;i<count;++i){Fault::at=i;Fault::count=0;ULONG refs=Refs(d.p);Carry x;AtlasViewTest::ContextFixture before(c.p);HRESULT hr=x.Init(d.p,0);AtlasViewTest::ContextFixture after(c.p);if(SUCCEEDED(hr)||x.OwnedGpuBytes()||x.Constants()||x.Texture(0,0)||x.View(1,1)||!before.Equal(after)||Refs(d.p)!=refs){std::fprintf(stderr,"FAIL allocation cleanup cutpoint%u\n",i);return 1;}Fault::at=~0u;Fault::count=0;if(FAILED(x.Init(d.p,0)))return 1;}
 std::printf("PASS 14 actual carry allocation/health failure cutpoints, cleanup, state and retry\n");return 0;}
'''.replace('FAULT_HEADER',hp.as_posix())
execute('allocation-faults',faultTest)
assert faultHeader.count('if(FAILED(hr)){Clear();return hr;}')==1
hp.write_text(faultHeader.replace('"../raster/','"'+(HERE.parent/'raster').as_posix()+'/').replace('if(FAILED(hr)){Clear();return hr;}','if(FAILED(hr)){return hr;}'))
execute('cleanup-mutant',faultTest,True)
hp.write_text(faultHeader.replace('"../raster/','"'+(HERE.parent/'raster').as_posix()+'/'))

cpp=(HERE/'warp_tests.cpp').read_text()
source={n:(OUT/(n+'.hlsl')).read_text() for n in ['Partial','Tail']}
def shader_mutant(name,which,old,new):
 assert source[which].count(old)==1,(name,source[which].count(old))
 path=OUT/(name+'.hlsl');path.write_text(source[which].replace(old,new));blob=OUT/(name+'.cso')
 p=subprocess.run([str(FXC),'/nologo','/T','ps_4_0','/E','main_fs','/O3','/Gec','/D','DX11=1','/Fo',str(blob),str(path)],cwd=HERE,capture_output=True,text=True);(OUT/(name+'-shader.log')).write_text(p.stdout+p.stderr);assert p.returncode==0,(name,p.stdout,p.stderr)
 arrays=['#pragma once','namespace StagedMaterial {']
 for kind in ['Partial','Tail']:
  b=blob.read_bytes() if kind==which else (OUT/(kind+'.cso')).read_bytes()
  arrays+=['static const unsigned char '+kind+'Shader[]={'+','.join(str(v)for v in b)+'};']
 arrays+=['}'];h=OUT/(name+'.h');h.write_text('\n'.join(arrays))
 execute(name,cpp.replace('"_generated/StagedShaders.h"','"'+h.as_posix()+'"'),True)

shader_mutant('lost-albedo','Partial','biome.albedo=previousAlbedo.Load(int3(carryPixel,0));','biome.albedo=0;')
shader_mutant('normalize-per-slot','Partial','float4(biome.normal.rgb,biome.absorbance)','float4(normalize(biome.normal.rgb),biome.absorbance)')
shader_mutant('wet-per-slot','Partial','part.albedo.rgb*=satBiome[row+10].x;','part.albedo.rgb*=satBiome[row+10].x;makeWet(part.albedo,wetness,1-part.albedo.a+part.absorbance,waterHeightRel-worldPos.y);')
shader_mutant('clamp-residual','Partial','float weight=selectedWeights[slot];','float weight=saturate(selectedWeights[slot]);')
shader_mutant('dedup-slots','Partial','if((code&15u)!=0) {','if((code&15u)!=0 && (slot==0 || (code&15u)!=(selected[0]&15u))) {')
shader_mutant('reverse-order','Partial','uint slot=StageSlot;','uint slot=3-StageSlot;')
shader_mutant('omit-wet-tail','Tail','makeWet(biome.albedo, wetness, absorb, waterHeightRel - worldPos.y);','/* missing once-only wet tail */')
assert cpp.count('c.p->SetPredication(NULL,FALSE);')==1
execute('predicate-mutant',cpp.replace('c.p->SetPredication(NULL,FALSE);',''),True)
assert cpp.count('carry.View((slotIndex-1)&1,0)')==1
execute('ping-pong-mutant',cpp.replace('carry.View((slotIndex-1)&1,0)','carry.View(slotIndex&1,0)'),True)
print('PASS 14 real allocation failures and 10 compiled shader/orchestration/cleanup negative controls')
