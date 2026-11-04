from pathlib import Path
import os,subprocess
here=Path(__file__).resolve().parent;out=here/'_generated';out.mkdir(exist_ok=True)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=os.environ.copy();env['INCLUDE']=f'{vc}/include;{sdk}/Include;D:/DEV/Kenshi/KenshiLib-src/Include;D:/DEV/Kenshi/KenshiLib-src/Include/ogre;D:/DEV/Kenshi/deps/boost_1_60_0;D:/DEV/Kenshi/shared';env['LIB']=f'{vc}/lib/amd64;{sdk}/Lib/x64'
for opt in ['Od','O2']:
 for name in ['pump','progress']:
  r=subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/'+opt,'/MT',str(here/(name+'.cpp')),'/Fo'+str(out/(name+'.obj')),'/Fe'+str(out/(name+'.exe')),'/link','d3d11.lib','dxgi.lib'],cwd=out,env=env,capture_output=True,text=True);(out/('build-'+name+'-'+opt+'.log')).write_text(r.stdout+r.stderr)
  if r.returncode:print(r.stdout+r.stderr);raise SystemExit(r.returncode)
  r=subprocess.run([str(out/(name+'.exe'))],cwd=out,capture_output=True,text=True);(out/('run-'+name+'-'+opt+'.log')).write_text(r.stdout+r.stderr);print(r.stdout+r.stderr,end='')
  if r.returncode:raise SystemExit(r.returncode)
r=subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/c','/EHsc','/O2','/GL','/MD',str(here/'native.cpp'),'/Fo'+str(out/'native.obj')],cwd=out,env=env,capture_output=True,text=True);(out/'native-build.log').write_text(r.stdout+r.stderr)
if r.returncode:print(r.stdout+r.stderr);raise SystemExit(r.returncode)
print('PASS actual native-session progress getter compile /O2 /GL')
