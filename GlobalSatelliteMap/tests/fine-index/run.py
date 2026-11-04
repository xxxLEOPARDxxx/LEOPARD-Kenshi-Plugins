from pathlib import Path
import os,subprocess,hashlib,json
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=os.environ.copy();env['INCLUDE']=f'{VC}/include;{SDK}/Include';env['LIB']=f'{VC}/lib/amd64;{SDK}/Lib/x64'
def run(name,opt,defines=(),success=True):
 exe=OUT/(name+'.exe')
 p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT','/'+opt,*defines,str(HERE/'index_tests.cpp'),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe)],env=env,cwd=OUT,capture_output=True,text=True);assert p.returncode==0,p.stdout+p.stderr
 p=subprocess.run([str(exe)],cwd=OUT,capture_output=True,text=True);print(name+': '+p.stdout+p.stderr,end='');assert (p.returncode==0)==success
 (OUT/(name+'.log')).write_text(p.stdout+p.stderr);return p.stdout+p.stderr
results=[dict(opt=opt,result=run('index-'+opt,opt)) for opt in ['Od','O2']]
HEADER=HERE.parents[1]/'GsmFineIndex.h';text=HEADER.read_text();mutants=[
 ('stamp32','if(a.stamp.source!=b.stamp.source)return a.stamp.source<b.stamp.source;','if(unsigned(a.stamp.source)!=unsigned(b.stamp.source))return unsigned(a.stamp.source)<unsigned(b.stamp.source);'),
 ('duplicate-slot','||occupied_[slot])return false;','||false)return false;'),
 ('wrong-erase','at==entries_.end()||at->second!=slot','at==entries_.end()'),
 ('worst-spacing','if(FineWindows::Internal::Worst(view,spacing)>GsmCacheProfile::MaxVisible)continue;','if(false)continue;'),
]
for name,needle,replacement in mutants:
 assert text.count(needle)==1,name
 source=text.replace(needle,replacement).replace('#include "GsmCacheProfile.h"','#include "'+str(HEADER.parent/'GsmCacheProfile.h').replace('\\','/')+'"').replace('#include "atlas-terrain/fine-windows/FineWindowPlanner.h"','#include "'+str(HEADER.parent/'atlas-terrain/fine-windows/FineWindowPlanner.h').replace('\\','/')+'"')
 path=OUT/('mutant-'+name+'.h');path.write_text(source)
 run('mutant-'+name,'O2',['/DFINE_INDEX_HEADER="'+str(path).replace('\\','/')+'"'],False)
report=dict(scope='pure actual GsmFineIndex header; no viewer/game integration',results=results,functionalMutants=[m[0] for m in mutants],sha256={str(p.relative_to(HEADER.parent)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [HEADER,HERE/'index_tests.cpp',HERE/'run.py']})
(HERE/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
