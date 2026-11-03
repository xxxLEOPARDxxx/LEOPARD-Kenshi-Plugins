from pathlib import Path
import os,re,subprocess,sys,hashlib,json
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True);source=HERE.parent.parent/'GsmFineViewer.h'
s=source.read_text();at=s.index('void FineObserve(');tail=s[at:];start=tail.index(' // A few fallback cells remain.') if ' // A few fallback cells remain.' in tail else tail.index(' // Fixed overview cells');end=tail.index('\n}',start)
body=tail[start:end];cpp=(HERE.parent.parent/'GlobalSatelliteMap.cpp').read_text();capacity=re.search(r'unsigned BaseDetailCapacity\(\)\{[^}]+\}',cpp).group();reserve=re.search(r'bool FineReservesBaseDetail\(\)\{[^}]+\}',s).group();feasible=re.search(r'bool FineDetailFeasible\(\)\{[^}]+\}',s).group();(OUT/'Residency.inc').write_bytes((feasible+'\n'+reserve+'\n'+capacity+'\nvoid Maintain(MapScreen*map){\n'+body+'\n}\n').encode())
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64');results={}
for mode in ['Od','O2']:
 exe=OUT/(mode+'.exe');cmd=[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT','/'+mode,'/W3',str(HERE/'tests.cpp'),'/Fo'+str(OUT/(mode+'.obj')),'/Fe'+str(exe)]
 p=subprocess.run(cmd,cwd=HERE,env=env,capture_output=True,text=True);(OUT/('build-'+mode+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr)
 if p.returncode:sys.exit(p.returncode)
 p=subprocess.run([str(exe)],cwd=HERE,capture_output=True,text=True);(OUT/('run-'+mode+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr);results[mode]=p.stdout.strip()
 if p.returncode:sys.exit(p.returncode)
(HERE/'verification.json').write_text(json.dumps({'productionTailSha256':hashlib.sha256(body.encode()).hexdigest(),'baseDetailCapacity':capacity,'reserveDetailBudget':reserve,'baseDetailCapacitySha256':hashlib.sha256(capacity.encode()).hexdigest(),'source':str(source),'results':results,'scope':'Actual extracted FineObserve residency maintenance plus real AtlasWindowPolicy; no engine/GPU/game/large fixtures.'},indent=2)+'\n')
