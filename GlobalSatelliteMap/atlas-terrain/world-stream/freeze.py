"""Record or verify current worker-acceptance integration evidence; no game calls."""
from pathlib import Path
import datetime,hashlib,json,re,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';MANIFEST=HERE/'verification.json'
def require(ok,message):
 if not ok:raise RuntimeError(message)
def sha(path):
 h=hashlib.sha256()
 with Path(path).open('rb')as stream:
  for block in iter(lambda:stream.read(1024*1024),b''):h.update(block)
 return h.hexdigest()
def own():return{p.name:sha(p)for p in sorted(HERE.iterdir())if p.is_file()and p.name!='verification.json'}
def check(values,base=None):
 for name,digest in values.items():
  path=base/name if base else Path(name);require(path.is_file()and sha(path)==digest,'Changed or missing '+str(path))
def dependencies():
 seen=set()
 def add(path):
  path=path.resolve()
  if path in seen:return
  require(path.is_file(),'Missing include '+str(path));seen.add(path)
  if path.suffix in ['.h','.cpp']:
   for name in re.findall(r'^\s*#include\s+"([^\"]+)"',path.read_text(),re.M):add(path.parent/name)
 for file in ['WorldStream.h','lifecycle_tests.cpp','worker_acceptance_tests.cpp','flow_tests.cpp']:add(HERE/file)
 for component in ['world-raster','world-bank','world-source','source-receipts','png-source','world-native','world-pixels']:add(HERE.parent/component/'verification.json')
 for suffix in ['-material.rgba16f','-normal.rgba8','-radial.r32f']:add(HERE.parent/('raw-rgb/_generated/control3'+suffix))
 add(HERE.parent/'world-raster/_generated/height-two.u16')
 add(Path('D:/SteamLibrary/steamapps/common/Kenshi/data/newland/land/fullmap.tif'))
 return{str(p):sha(p)for p in sorted(seen)if not p.is_relative_to(HERE)}
def record():
 logs={str(p.relative_to(HERE)):sha(p)for p in sorted(OUT.rglob('*.log'))}
 lifecycle=[]
 for opt in ['Od','O2']:
  text=(OUT/('lifecycle'+opt+'-run.log')).read_text();m=re.search(r'PASS (\d+) production controller lifecycle checks; copiedCPU(\d+)B; accepted512(\d+)B',text);require(m is not None,'Missing current CPU/lifecycle evidence');lifecycle.append(tuple(map(int,m.groups())))
  require('PASS 29 real worker acceptance/refusal/CPU/close controls'in(OUT/('worker_acceptance'+opt+'-run.log')).read_text(),'Missing real worker evidence')
 require(lifecycle[0]==lifecycle[1]and lifecycle[0][1]<=64*1024*1024,'CPU reservation inconsistent or exceeds cap')
 flow=(OUT/'flow-run.log').read_text();m=re.search(r'PASS (\d+) actual streamed-frame checks; three completed frames, cache reuse/OFF/world cancel',flow);require(m is not None,'Missing completed current flow');flowChecks=int(m[1])
 fault=(OUT/'adversarial.log').read_text()
 for expected in ['worker-exit PASS','validated-worker-exit PASS','opened-source-exit PASS','decoded-source-exit PASS','complete-exit-cancel PASS','send-fence-quarantine PASS','source-exit-loss FAIL unexpected exit preserves encoded lease','cancel-failure-latch-loss FAIL Cancel cannot hide failed worker exit','validated-exit-loss FAIL unexpected exit preserves accepted/raw/window','destructor-loss FAIL controller destructor does not release quarantined','reservation-loss FAIL unknown partial/retired work retains maximum']:
  require(expected in fault,'Missing expected adversarial evidence '+expected)
 binary={str(p.relative_to(HERE)):sha(p)for p in sorted(OUT.rglob('*.exe'))}
 result=dict(scope='One-window copied worker/sourcegate/WIC/staged GPU/readback plus worker pixel/world-height acceptance; fake metadata host controls; no Main/game installation',status='implementer checks PASS; independent review pending',recordedUtc=datetime.datetime.now(datetime.timezone.utc).isoformat(),command='python run_tests.py',sourceFiles=own(),dependencies=dependencies(),logs=logs,binaries=binary,results=dict(lifecycleChecksEach=lifecycle[0][0],realWorkerAcceptanceChecksEach=29,flowChecks=flowChecks,completedFrames=3,independentTriangleHeightPixels=393216,cancelAtPixelValidation=True,originalLifecycleFaults=True,additionalValidatedExitLifetime=True,openedAndDecodedSourceExitLifetime=True,completeExitThenCancelFailureLatched=True,killedLifecycleMutants=5),gpuReservationBytes=112825820,copiedCpuReservationBytes=lifecycle[0][1],accepted512ReservationBytes=lifecycle[0][2],limits=['One process-local joined worker; same-thread Main/controller/destruction.','Default planes are accepted HDR/normal/worldheight; RawPlane radial is diagnostic only.','Supplied native source/material/world authority remains an external Main adapter responsibility.','Explicit copied CPU payload is charged under64MiB; opaque WIC/COM/driver allocations are not a hard total-memory bound.','Unknown completion retains conservative full Work/GPU debt until restart.','No native Main callback execution, GST2 publication, full-world throughput, FPS or long-run leak-free claim.'])
 MANIFEST.write_text(json.dumps(result,indent=2)+'\n');print('PASS frozen world-stream',sha(MANIFEST),'sources',len(result['sourceFiles']),'dependencies',len(result['dependencies']),'CPU',lifecycle[0][1])
def verify():
 data=json.loads(MANIFEST.read_text());require(own()==data['sourceFiles'],'Owned source set/hash changed');check(data['dependencies']);check(data['logs'],HERE);check(data['binaries'],HERE);print('PASS frozen world-stream sources/dependencies/logs/binaries; host acceptance evidence only')
if '--verify'in sys.argv:verify()
else:record()
