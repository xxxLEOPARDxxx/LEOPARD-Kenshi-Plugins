from pathlib import Path
import os,subprocess,sys,json,hashlib
here=Path(__file__).resolve().parent;out=here/'_generated';out.mkdir(exist_ok=True)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=os.environ.copy();env['INCLUDE']=f'{vc}/include;{sdk}/Include;{here}';env['LIB']=f'{vc}/lib/amd64;{sdk}/Lib/x64'
subprocess.run([sys.executable,str(here/'generate_town.py')],check=True)
def build_run(name,opt,defines=(),success=True):
 exe=out/(name+'.exe')
 p=subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/'+opt,'/MT',*defines,str(here/'town_tests.cpp'),'/Fo'+str(out/(name+'.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib'],env=env,cwd=out,capture_output=True,text=True);assert p.returncode==0,p.stdout+p.stderr
 p=subprocess.run([str(exe)],cwd=out,capture_output=True,text=True);print(name+': '+p.stdout+p.stderr,end='');assert (p.returncode==0)==success
 (out/(name+'.log')).write_text(p.stdout+p.stderr);return p.stdout+p.stderr
results=[]
for opt in ['Od','O2']:results.append(dict(opt=opt,result=build_run('town-'+opt,opt)))
# Focused functional negative controls compile actual altered shader/header,
# run the same WARP test and require an assertion failure (not compile failure).
fxc=Path('C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe')
shader=(here/'TownCompose.hlsl').read_text();mutants=[
 ('translucency','float translucency=encoded.w<.5?encoded.w*2:0;','float translucency=0;'),
 ('occlusion','y-errorY>height','true'),
 ('origin','cameraFar.y-vertical+sceneToCache.y','cameraFar.y-vertical'),
 ('environment','environment.diffuse*=ambient.rgb*scale;','environment.diffuse*=0;'),
 ('hdr','hdr=float4(colour,1);','hdr=float4(saturate(colour),1);'),
]
for name,needle,replacement in mutants:
 assert shader.count(needle)==1,name
 path=out/('town-'+name+'.hlsl');path.write_text(shader.replace(needle,replacement));cso=out/('town-'+name+'.cso')
 p=subprocess.run([str(fxc),'/nologo','/T','cs_5_0','/E','main','/O3','/I','D:/SteamLibrary/steamapps/common/Kenshi/data/materials/common','/Fo',str(cso),str(path)],capture_output=True,text=True);assert p.returncode==0,p.stdout+p.stderr
 header=out/('TownMutant-'+name+'.h');header.write_text('static const unsigned char TownComposeShader[]={'+','.join(str(b) for b in cso.read_bytes())+'};\n')
 build_run('mutant-'+name,'O2',['/DTOWN_MUTANT_SHADER="'+str(header).replace('\\','/')+'"'],False)
header=(here/'TownCompose.h').read_text()
for name,needle,replacement in [('budget','return caller<=AtlasView::GpuCap-PrivateBytes;','return true;'),('state','c_->CSSetShaderResources(0,10,views_);','')]:
 assert header.count(needle)==1,name
 path=out/('TownHeader-'+name+'.h');path.write_text(header.replace(needle,replacement).replace('#include "../../atlas-water/AtlasWater.h"','#include "../../../atlas-water/AtlasWater.h"'))
 build_run('mutant-'+name,'O2',['/DTOWN_HEADER="'+str(path).replace('\\','/')+'"'],False)
report=dict(scope='TownCompose isolated HDR/native lighting/acceptedY, actual frozen raw raster triangles; no live native admission or map integration proof',results=results,functionalMutants=7,sourceSha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [here/'TownCompose.h',here/'TownCompose.hlsl',here/'town_tests.cpp',here/'generate_town.py',here/'run_town.py']})
(here/'town-verification.json').write_text(json.dumps(report,indent=2)+'\n')
