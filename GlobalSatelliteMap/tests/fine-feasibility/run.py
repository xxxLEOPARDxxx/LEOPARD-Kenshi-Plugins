from pathlib import Path
import os,subprocess,sys,json,hashlib
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True);VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64');results={}
for mode in ['Od','O2']:
 exe=OUT/(mode+'.exe');p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT','/'+mode,'/W3',str(HERE/'tests.cpp'),'/Fo'+str(OUT/(mode+'.obj')),'/Fe'+str(exe),'/link','advapi32.lib'],cwd=HERE,env=env,capture_output=True,text=True);(OUT/('build-'+mode+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr)
 if p.returncode:sys.exit(p.returncode)
 p=subprocess.run([str(exe)],capture_output=True,text=True);(OUT/('run-'+mode+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr)
 if p.returncode:sys.exit(p.returncode)
 results[mode]=p.stdout.strip()
(HERE/'verification.json').write_text(json.dumps({'productionPolicySha256':hashlib.sha256((HERE.parent.parent/'GsmFinePolicy.h').read_bytes()).hexdigest(),'results':results,'scope':'Actual planner worst bounds and feasibility, live9207primary2160x1044viewport; no engine/UI/source capture.'},indent=2)+'\n')
