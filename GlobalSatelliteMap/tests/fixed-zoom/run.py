from pathlib import Path
import os,subprocess,sys,json,hashlib
here=Path(__file__).resolve().parent;out=here/'_generated';out.mkdir(exist_ok=True);vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64');results={}
for mode in ('Od','O2'):
 exe=out/(mode+'.exe');p=subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT','/'+mode,'/W3',str(here/'tests.cpp'),'/Fo'+str(out/(mode+'.obj')),'/Fe'+str(exe)],env=env,capture_output=True,text=True);(out/('build-'+mode+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr)
 if p.returncode:sys.exit(p.returncode)
 p=subprocess.run([str(exe)],capture_output=True,text=True);(out/('run-'+mode+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr)
 if p.returncode:sys.exit(p.returncode)
 results[mode]=p.stdout.strip()
(here/'verification.json').write_text(json.dumps({'productionStepSha256':hashlib.sha256((here.parents[1]/'GsmZoomStep.h').read_bytes()).hexdigest(),'results':results,'scope':'Production deterministic zoom ladder and scoped exact native clamp; no game/UI.'},indent=2)+'\n')
