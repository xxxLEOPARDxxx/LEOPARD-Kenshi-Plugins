from pathlib import Path
import os,subprocess
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');FXC='C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe'
env=os.environ.copy();env['INCLUDE']=str(VC/'include')+';'+str(SDK/'Include')+';'+str(HERE);env['LIB']=str(VC/'lib/amd64')+';'+str(SDK/'Lib/x64')
header=(HERE/'AtlasModelDecode.h').read_text();shader=(HERE/'ModelDecode.hlsl').read_text()
cases=[
 ('same-native-planes','header','if(AtlasView::Identity(res[i].p,res[j].p))return E_INVALIDARG;','','native packed and normal source alias refused'),
 ('unproven-mapping','header','if(!m.verified||','if(','unverified mapping refused'),
 ('lose-uavs12','header','SavedUavs uavs(c.p);','','complete native context and output aliases restored'),
 ('ignore-total-budget','header','return caller<=AtlasView::GpuCap-PrivateBytes;','return true;','exact inclusive shared budget'),
 ('linear-depth','shader','cameraFar.y-vertical+sceneToCache.y','cameraFar.y-radial+sceneToCache.y','native radial roof height translated correctly'),
 ('omit-translation','shader','cameraFar.y-vertical+sceneToCache.y','cameraFar.y-vertical','native radial roof height translated correctly'),
 ('overwrite-foreground','shader','normal.a<=0||!isfinite(height)||modelHeight-errorY>height','true','hidden model retains existing material and normal'),
 ('omit-chroma','shader','SatelliteDecodeAlbedo(int2(p),256)','SatelliteAlbedoSample(int2(p),256).rrr','native packed colour and gloss decoded'),
 ('inherit-predicate','header','AtlasWater::SavedPredicate predicate(c.p);','','native radial roof height translated correctly'),
]
def call(args,name,allow_failure=False):
 p=subprocess.run([str(x) for x in args],env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);(OUT/(name+'.log')).write_text(p.stdout)
 if p.returncode and not allow_failure:raise RuntimeError(name+': '+p.stdout[-1200:])
 return p
for name,kind,old,new,reason in cases:
 text=header if kind=='header' else shader;assert old in text,name;text=text.replace(old,new)
 if kind=='header':
  text=text.replace('"../AtlasModel.h"','"'+(HERE.parent/'AtlasModel.h').as_posix()+'"').replace('"../../atlas-water/AtlasWater.h"','"'+(HERE.parents[1]/'atlas-water/AtlasWater.h').as_posix()+'"');path=OUT/(name+'.h');path.write_text(text);define='/DMODEL_DECODE_HEADER="'+path.as_posix()+'"'
 else:
  path=OUT/(name+'.hlsl');path.write_text(text);code=OUT/(name+'.cso');call([FXC,'/nologo','/T','cs_5_0','/E','main','/O3','/I',OUT,'/Fo',code,path],name+'-shader-build');raw=code.read_bytes();inc=OUT/(name+'-shader.h');inc.write_text('static const unsigned char AtlasModelDecodeShader[]={'+','.join(str(v) for v in raw)+'};\n');define='/DMODEL_DECODE_SHADER="'+inc.as_posix()+'"'
 exe=OUT/(name+'.exe');call([VC/'bin/amd64/cl.exe','/nologo','/EHsc','/MD','/O2','/W1',define,'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),HERE/'warp_tests.cpp','/link','d3d11.lib','dxgi.lib'],name+'-build')
 p=call([exe],name+'-run',True)
 if not p.returncode or 'FAIL '+reason not in p.stdout:raise RuntimeError('survived or unfair '+name+': '+p.stdout[-1200:])
 print('Expected RED:',name,'->',reason)
print('PASS nine compiled functional model decode mutants rejected')
