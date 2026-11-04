from pathlib import Path
import os,subprocess,sys
here=Path(__file__).resolve().parent;out=here/'_generated';out.mkdir(exist_ok=True)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=os.environ.copy();env['INCLUDE']=f'{vc}/include;{sdk}/Include;D:/DEV/Kenshi/KenshiLib-src/Include;D:/DEV/Kenshi/KenshiLib-src/Include/ogre;D:/DEV/Kenshi/deps/boost_1_60_0;D:/DEV/Kenshi/shared';env['LIB']=f'{vc}/lib/amd64;{sdk}/Lib/x64'
legacy='--legacy' in sys.argv
for opt in ['Od','O2']:
 args=[str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/'+opt,'/MT']+(['/DMEMORY_LEGACY'] if legacy else [])+[str(here/'policy.cpp'),'/Fo'+str(out/'policy.obj'),'/Fe'+str(out/'policy.exe'),'/link','d3d11.lib','dxgi.lib']
 p=subprocess.run(args,cwd=out,env=env,capture_output=True,text=True);(out/('build-'+opt+'.log')).write_text(p.stdout+p.stderr)
 if p.returncode:print(p.stdout+p.stderr);raise SystemExit(p.returncode)
 for gpu,ram in ([(1,8)] if legacy else [(gpu,9-gpu) for gpu in range(1,9)]+[(1,9),(8,16),(4,31),(8,32)]):
  p=subprocess.run([str(out/'policy.exe'),str(gpu),str(ram)],cwd=out,capture_output=True,text=True);(out/(('legacy-'if legacy else'run-')+opt+'-'+str(gpu)+'-'+str(ram)+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='')
  if legacy:assert p.returncode==1 and 'FAIL default independent VRAM1/RAM2 GiB' in p.stdout
  elif p.returncode:raise SystemExit(p.returncode)
if not legacy:
 p=subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/c','/EHsc','/MD','/O2','/GL',str(here/'native.cpp'),'/Fo'+str(out/'native.obj')],cwd=out,env=env,capture_output=True,text=True);(out/'native-build.log').write_text(p.stdout+p.stderr)
 if p.returncode:print(p.stdout+p.stderr);raise SystemExit(p.returncode)
 print('PASS native actual-session admission and RAMstats/debt header compilation')
