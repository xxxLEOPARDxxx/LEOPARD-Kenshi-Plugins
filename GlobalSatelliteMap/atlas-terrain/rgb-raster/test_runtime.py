"""Compile real Render body with bounded faults; existing scopes remain read-only."""
from pathlib import Path
import os, subprocess
HERE=Path(__file__).resolve().parent
OUT=HERE/'_generated'
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
ENV=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
source=(HERE/'RgbTerrainRaster.h').read_text()
body=(HERE/'warp_tests.cpp').read_text()
def edit(s,a,b):
 assert s.count(a)==1,(a,s.count(a))
 return s.replace(a,b)
def build_run(name,header,test,red=False):
 for prefix,path in [('../raster/',HERE.parent/'raster'),('../raw-rgb/',HERE.parent/'raw-rgb')]:
  header=header.replace('"'+prefix,'"'+path.as_posix()+'/')
 hp=OUT/(name+'.h');hp.write_text(header)
 test=test.replace('"RgbTerrainRaster.h"','"'+hp.as_posix()+'"')
 for prefix,path in [('../compact-material/',HERE.parent/'compact-material'),('../raw-rgb/',HERE.parent/'raw-rgb'),('../../atlas-view/',HERE.parents[1]/'atlas-view')]:
  test=test.replace('"'+prefix,'"'+path.as_posix()+'/')
 cpp=OUT/(name+'.cpp');cpp.write_text(test);exe=OUT/(name+'.exe')
 result=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT','/W3',str(cpp),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib','advapi32.lib'],env=ENV,cwd=HERE,capture_output=True,text=True)
 (OUT/(name+'-build.log')).write_text(result.stdout+result.stderr)
 assert result.returncode==0,(name,'compile error is not rejection',result.stdout,result.stderr)
 result=subprocess.run([str(exe)],cwd=HERE,capture_output=True,text=True)
 (OUT/(name+'-run.log')).write_text(result.stdout+result.stderr)
 if red:
  assert result.returncode==1 and 'FAIL ' in result.stderr,(name,result.returncode,result.stdout,result.stderr)
  print('Expected RED:',name,result.stderr.strip(),flush=True)
 else:
  assert result.returncode==0,(name,result.stdout,result.stderr)
  print(result.stdout.strip(),flush=True)

instrumented=source.replace('namespace TerrainRgbRaster {','namespace TerrainRgbRaster {\nstatic unsigned runtimeFault=0;')
instrumented=edit(instrumented,'c.p->DrawIndexed(FixedTerrain::IndexCount,0,0);','c.p->DrawIndexed(FixedTerrain::IndexCount,0,0);if(runtimeFault==2)throw 2;')
instrumented=edit(instrumented,'HRESULT result=device_.p->GetDeviceRemovedReason();','HRESULT result=runtimeFault==1?DXGI_ERROR_DEVICE_REMOVED:device_.p->GetDeviceRemovedReason();')
extra=r'''
 for(unsigned mode=1;mode<=2;++mode){
  c.p->SetPredication(pred.p,FALSE);
  ULONG refs=Refs(d.p),srcRefs=Refs(sources.View(0));
  {AtlasViewTest::ContextFixture before(c.p);
  runtimeFault=mode;bool caught=false;HRESULT result=S_OK;
  try{result=raster.Render(true,sources,frame,edge);}catch(int x){caught=x==2;}
  runtimeFault=0;AtlasViewTest::ContextFixture after(c.p);
  C(mode==1?result==DXGI_ERROR_DEVICE_REMOVED:caught,"actual post-draw health/exception surfaced");
  C(before.Equal(after),"health failure and C++ unwind restore exact foreign GPU state");
  }
  C(refs==Refs(d.p)&&srcRefs==Refs(sources.View(0)),"failure/unwind COM references balanced");
  C(!raster.HasFrame()&&!raster.Output(0)&&!raster.Output(1)&&!raster.Output(2),"failed/partial draw cannot publish output");
  HR(raster.Render(true,sources,frame,edge),"healthy retry after interrupted frame");
  C(raster.HasFrame()&&raster.Output(0),"healthy retry republished complete frame");
 }
 c.p->SetPredication(NULL,FALSE);
'''
test=edit(body,' // Caller-held output lifetime is explicit;',extra+' // Caller-held output lifetime is explicit;')
build_run('runtime-failures',instrumented,test)
for name,a,b in [
 ('shader-pin','return Code(s.vertex,4440,v)&&Code(s.pixel,36968,p);','return true;'),
 ('extra-charge','+TerrainRaw::PrivateBytes+VertexBytes','+TerrainRaw::PackedBytes+VertexBytes'),
 ('predicate','c.p->SetPredication(NULL,FALSE);',''),
 ('brightness','*f.brightness','*1.f'),
 ('premature-publication','hasFrame_=false;\n  AtlasView::SavedOutputReads','hasFrame_=true;\n  AtlasView::SavedOutputReads'),
]:
 build_run(name,edit(instrumented,a,b),test,red=True)
print('PASS post-draw device-health/C++ unwind recovery and 5 compiled negative controls')
