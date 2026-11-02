"""Isolated header copies; every changed admission predicate must fail a check."""
from pathlib import Path
import os,subprocess
BASE=Path(__file__).resolve().parents[1];OUT=BASE/'build/quality-policy-tests/mutants'
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
FILES=['atlas-view/AtlasPolicy.h','atlas-view/AtlasWindowPolicy.h','atlas-view/AtlasRelight.h','atlas-view/AtlasReader.h','atlas-water/AtlasWater.h','atlas-water/WaterPipeline.h','tests/atlas_quality_policy_tests.cpp']
CASES=[
 ('old-detail56','atlas-view/AtlasWindowPolicy.h','DetailSlots=128','DetailSlots=56','measured4096 primary2160x1044 viewport retains densityLevel1'),
 ('split-water-cap','atlas-water/AtlasWater.h','Cap=AtlasView::GpuCap','Cap=96ULL*1024*1024','one authoritative192MiB cap shared by water and terrain'),
 ('omit-default-scratch','atlas-view/AtlasWindowPolicy.h','WindowSlots*CellBytes(5)+U64(WindowSide)*WindowSide*4+UniformGpuBytes','WindowSlots*CellBytes(5)+UniformGpuBytes','144window base includes default scratch and constants'),
 ('pipeline-plus1','atlas-water/WaterPipeline.h','callerOwnedBytes<=AtlasView::GpuCap-PipelinePrivateBytes','callerOwnedBytes<=AtlasView::GpuCap-PipelinePrivateBytes+1','pipeline plus-one boundary remains strict'),
 ('water-plus1','atlas-water/AtlasWater.h','callerOwnedBytes<=Cap-PrivateBytes','callerOwnedBytes<=Cap-PrivateBytes+1','water layer uses same inclusive plus-one bound'),
 ('default-terrain-plus1','atlas-view/AtlasPolicy.h','U64(s)*s*4<=GpuCap-UniformGpuBytes-cellBytes','U64(s)*s*4<=GpuCap-UniformGpuBytes-cellBytes+1','default terrain plus-one boundary remains strict'),
 ('hdr-terrain-plus1','atlas-view/AtlasRelight.h','callerBytes<=GpuCap-hdrPrivate','callerBytes<=GpuCap-hdrPrivate+1','HDR layer plus-one boundary remains strict'),
]
def run():
 env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
 for name,file,old,new,expected in CASES:
  root=OUT/name
  for relative in FILES:
   target=root/relative;target.parent.mkdir(parents=True,exist_ok=True);text=(BASE/relative).read_text()
   if relative==file:
    if text.count(old)!=1:raise ValueError('quality mutation anchor changed: '+name)
    text=text.replace(old,new)
   target.write_text(text)
  result=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/W3',f'/Fo{root}/test.obj',f'/Fe{root}/test.exe',str(root/'tests/atlas_quality_policy_tests.cpp'),'/link','d3d11.lib'],env=env,capture_output=True,text=True)
  (root/'compile.log').write_text(result.stdout+result.stderr)
  if result.returncode:raise RuntimeError('quality mutant compile failed: '+name+'\n'+result.stdout+result.stderr)
  result=subprocess.run([str(root/'test.exe')],capture_output=True,text=True)
  log=result.stdout+result.stderr;(root/'run.log').write_text(log)
  if result.returncode==0 or 'FAIL '+expected not in log:raise AssertionError('quality mutant survived/failed unfairly: '+name+'\n'+log)
  print('REJECTED',name,'->',expected)
 print(f'PASS {len(CASES)} compiled functional quality/budget mutants')
if __name__=='__main__':run()
