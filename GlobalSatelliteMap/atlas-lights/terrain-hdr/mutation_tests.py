"""Compile every modified production branch; require its relevant real WARP failure."""
from pathlib import Path
import os,subprocess
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');FXC='C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe'
env=os.environ.copy();base_includes=str(VC/'include')+';'+str(SDK/'Include')+';'+str(HERE);env['INCLUDE']=base_includes;env['LIB']=str(VC/'lib/amd64')+';'+str(SDK/'Lib/x64')
def run(args,name,expect=0):
 p=subprocess.run([str(a) for a in args],env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);(OUT/(name+'.log')).write_text(p.stdout)
 if expect==0 and p.returncode:raise RuntimeError(name+' failed; '+p.stdout[-1600:])
 return p
header=(HERE/'AtlasLocalLights.h').read_text();shader=(OUT/'LocalLights.generated.hlsl').read_text()
mutants=[
 ('split-gpu-cap','header','const U64 Cap=AtlasView::GpuCap;','const U64 Cap=96ULL*1024*1024;','single authoritative GPU payload cap'),
 ('cap32','header','i<snapshot.Count()','i<(std::min)(32u,snapshot.Count())','all4096 processed in32-light batches'),
 ('unproven-mapping','header','if(!verified||generation!=g||!s.Valid(g))','if(generation!=g||!s.Valid(g))','unproven mapping refused'),
 ('drop-native-state','header','AtlasView::SavedCompute saved(c.p);','','complete native context and six output SRVs preserved'),
 ('ignore-ledger','header','return caller<=Cap-PrivateBytes;','return true;','exact inclusive private payload budget'),
 ('omit-power','shader','color = color * attenuation * power;','color = color * attenuation;','installed native punctual fragment comparison'),
 ('omit-spot','shader','if(light.directionType.w==2)','if(false)','installed native punctual fragment comparison'),
 ('omit-native-attenuation','shader','color = color * attenuation * power;','color = color * power;','installed native punctual fragment comparison'),
 ('coverage-overwrite','shader','float4(rgb,base.a)','float4(rgb,1)','coverage alpha preserved'),
 ('invalid-source-write','shader','if(n.a<=0||!all(isfinite(m))||!all(isfinite(normal))||!isfinite(h)||dot(normal,normal)<1e-8)','if(false)','invalid source pixel retains HDR and coverage'),
 ('wrong-coordinate-translation','header','v.worldPosition[i]+m.sceneToCacheTranslation[i]','v.worldPosition[i]','installed native punctual fragment comparison'),
]
for name,kind,old,new,reason in mutants:
 text=header if kind=='header' else shader
 if old not in text:raise ValueError('missing mutant anchor '+name)
 text=text.replace(old,new)
 if kind=='header':
  text=text.replace('"../NativeLocalLightSnapshot.h"','"'+(HERE.parent/'NativeLocalLightSnapshot.h').as_posix()+'"').replace('"../../atlas-water/AtlasWater.h"','"'+(HERE.parent.parent/'atlas-water/AtlasWater.h').as_posix()+'"')
  path=OUT/(name+'.h');path.write_text(text);define='/DLOCAL_LAYER_HEADER="'+path.as_posix()+'"'
  env['INCLUDE']=base_includes+';D:/DEV/Kenshi/KenshiLib-src/Include;D:/DEV/Kenshi/KenshiLib-src/Include/ogre;D:/DEV/Kenshi/deps/boost_1_60_0;D:/DEV/Kenshi/shared'
  run([VC/'bin/amd64/cl.exe','/nologo','/c','/EHsc','/MD','/O2','/W1','/DUNICODE','/D_UNICODE',define,'/Fo'+str(OUT/(name+'-adapter.obj')),HERE/'compile_adapter.cpp'],name+'-adapter-build')
  env['INCLUDE']=base_includes
  exe=OUT/(name+'.exe');run([VC/'bin/amd64/cl.exe','/nologo','/EHsc','/MD','/O2','/W1',define,'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),HERE/'warp_tests.cpp','/link','d3d11.lib','dxgi.lib'],name+'-test-build');code=OUT/'LocalLights.cso'
 else:
  path=OUT/(name+'.hlsl');path.write_text(text);code=OUT/(name+'.cso');run([FXC,'/nologo','/T','cs_5_0','/E','main','/O3','/Gec','/Fo',code,path],name+'-shader-build');exe=OUT/'warp.exe'
 p=run([exe,code,OUT/'NativePoint.cso',OUT/'NativeSpot.cso',OUT/'Vertex.cso'],name+'-run',expect=1)
 if not p.returncode or 'FAIL '+reason not in p.stdout:raise RuntimeError('survived or unfair mutant '+name+': '+p.stdout[-1000:])
 print('Expected RED:',name,'->',reason)
print('PASS eleven compiled functional production mutants rejected')
