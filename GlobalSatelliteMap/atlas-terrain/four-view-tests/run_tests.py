"""Focused real FineJob + extracted coordinator identity controls; no assets/GPU."""
from pathlib import Path
import os, subprocess, sys
HERE=Path(__file__).resolve().parent
OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
job=(HERE.parent/'fine-viewer/FineJob.h').read_text()
coordinator=(HERE.parent/'fine-coordinator/FineCoordinator.h').read_text()
begin=coordinator.index('bool PairMatches(');opening=coordinator.index('{',begin);depth=1;end=opening+1
while depth:
 depth+=(coordinator[end]=='{')-(coordinator[end]=='}');end+=1
code=(HERE/'controls.cpp').read_text().replace('// EXTRACTED_PAIR_MATCHES',coordinator[begin:end])
def block(start):
 begin=coordinator.index(start);opening=coordinator.index('{',begin);depth=1;end=opening+1
 while depth:
  depth+=(coordinator[end]=='{')-(coordinator[end]=='}');end+=1
 return coordinator[begin:end]
route=(HERE/'routing.inc').read_text()
route=route.replace('// EXTRACTED_WRITING',block('if(o.writing){if(!PollWritten())'))
route=route.replace('// EXTRACTED_QUEUED_GATE',block('if(o.queued&&!FineJob::CanRender('))
route=route.replace('// EXTRACTED_COMPLETE',block('if(o.queued&&p.State()==WorldCohort::Complete)'))
code=code.replace('// ROUTING_CONTROLS',route)
queue=(HERE/'queue.inc').read_text().replace('// EXTRACTED_QUEUE',block('bool Queue(const FineJob::Plan&'))
code=code.replace('// QUEUE_CONTROLS',queue)
legacy='unsigned pairSpan=1' not in job or '--legacy-pair' in sys.argv
code=code.replace('// MODE', '#define LEGACY' if legacy else '')
cpp=OUT/'controls.cpp';cpp.write_text(code)
exe=OUT/'controls.exe'
cmd=[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT','/W3',str(cpp),'/Fo'+str(OUT/'controls.obj'),'/Fe'+str(exe),'/link','advapi32.lib']
p=subprocess.run(cmd,cwd=OUT,env=env,capture_output=True,text=True);(OUT/'build.log').write_text(p.stdout+p.stderr)
if p.returncode:print(p.stdout+p.stderr);sys.exit(p.returncode)
p=subprocess.run([str(exe)],cwd=OUT,env=env,capture_output=True,text=True);(OUT/('legacy-red.log' if legacy else 'run.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='');sys.exit(p.returncode)
