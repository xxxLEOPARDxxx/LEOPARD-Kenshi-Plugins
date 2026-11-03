"""Small compiled real-body negative controls; compiler errors are not kills."""
from pathlib import Path
import os,subprocess
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated'
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
source=(HERE/'FineWindowPlanner.h').read_text();body=(HERE/'planner_tests.cpp').read_text()
mutants=[
 ('negative-priority','+128.0*spacing-','+128*spacing-'),
 ('phase-reserve','n+1','n'),
 ('source-ledger','head-=b.sourcePeak','head-=0'),
 ('source-proof','!b.sourceAdmitted||!b.sourcePeak||!b.rasterPeak','false'),
 ('material-epoch','a.material==b.material','true'),
 ('absolute-center','wx=r.x+(double(x)+.5)*k.spacing','wx=r.x+1+(double(x)+.5)*k.spacing'),
 ('apron-reserve','limit=p.visibleCount+p.usableSlots-p.worstVisible','limit=p.usableSlots'),
 ('completion','Equal(requested,completed)&&',''),
 ('capacity','b.capacity-b.unavailable','b.capacity'),
]
for name,a,b in mutants:
 assert source.count(a)==(2 if name=='negative-priority' else 1),(name,a,source.count(a))
 header=source.replace(a,b).replace('"../../atlas-view/AtlasPolicy.h"','"'+(HERE.parents[1]/'atlas-view/AtlasPolicy.h').as_posix()+'"')
 hp=OUT/(name+'.h');hp.write_text(header);cpp=OUT/(name+'.cpp');cpp.write_text(body.replace('"FineWindowPlanner.h"','"'+hp.as_posix()+'"'));exe=OUT/(name+'.exe')
 p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT','/W3',str(cpp),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe)],cwd=HERE,env=env,capture_output=True,text=True)
 (OUT/(name+'-build.log')).write_text(p.stdout+p.stderr);assert p.returncode==0,(name,'compile failure is not rejection',p.stdout,p.stderr)
 p=subprocess.run([str(exe)],cwd=HERE,capture_output=True,text=True);(OUT/(name+'-run.log')).write_text(p.stdout+p.stderr)
 assert p.returncode==1 and 'FAIL ' in p.stderr,(name,'mutant survived or crashed',p.returncode,p.stdout,p.stderr)
 print('Expected RED',name,p.stderr.strip(),flush=True)
print('PASS 9 compiled planner mutants rejected')
