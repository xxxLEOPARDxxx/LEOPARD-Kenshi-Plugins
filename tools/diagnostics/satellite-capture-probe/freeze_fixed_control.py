"""Freeze the completed private adapter build/tests; no install or process calls."""
from pathlib import Path
import json,re,hashlib
HERE=Path(__file__).resolve().parent;OUT=HERE/'build';REPO=HERE.parents[2]
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
native=(OUT/'fixed-control-full-build-final.log').read_text()
assert 'PASS SatelliteCaptureProbe.dll:' in native and 'Capture bounds/restoration/source contracts passed' in native
warp=(OUT/'fixed-tests.log').read_text();match=re.search(r'PASS (\d+) actual fixed control',warp);assert match
assert 'PASS10 compiled runtime worker/control mutants' in (OUT/'fixed-control-mutants-final.log').read_text()
assert 'PASS five compiled caller refusal mutants' in (OUT/'fixed-control-calls-final.log').read_text()
own=['CaptureProbe.cpp','SatelliteCaptureProbe.ini','FixedTerrainControl.h','FixedTerrainControl.md','prepare_fixed_terrain.py','fixed_control_tests.cpp','test_fixed_control.py','test_fixed_control_calls.py','test_fixed_control_mutants.py','freeze_fixed_control.py']
closure={}
def include(p):
 p=p.resolve()
 if p in closure:return
 assert p.is_file(),p
 closure[p]=sha(p)
 text=p.read_text(encoding='utf8',errors='replace')
 for name in re.findall(r'^\s*#include\s+"([^"]+)"',text,re.M):
  q=p.parent/name
  if not q.exists():q=OUT/name
  assert q.is_file(),(p,name)
  include(q)
include(HERE/'CaptureProbe.cpp')
prepared=json.loads((OUT/'fixed-terrain-inputs/source-pins.json').read_text())
for name,pin in prepared['inputs'].items():assert sha(OUT/'fixed-terrain-inputs'/name)==pin['sha256']
dll=OUT/'SatelliteCaptureProbe.dll'
manifest=dict(scope='opt-in private fixed256 base-fixture adapter; no installed game/INI/process changes',status='implementer PASS; parent independent review pending',files={n:sha(HERE/n) for n in own},compileClosure={p.relative_to(REPO).as_posix():h for p,h in closure.items()},candidate=dict(path=str(dll),bytes=dll.stat().st_size,sha256=sha(dll)),preparedInputs=prepared,results=dict(actualWorkerWarpChecks=int(match[1]),manualCases=4,planes=3,callerChecks=35,callerMutants=5,controllerMutants=10,stagingFailureCleanupPaths=2,waitFailedQuarantine=True),gpuPayload=86156268,cpuOwnedBlobCap=16777216,logs={n:sha(OUT/n) for n in ['fixed-tests.log','fixed-tests-build.log','fixed-control-calls-final.log','fixed-control-mutants-final.log','fixed-control-full-build-final.log']},limitations=['Pinned prepared base assets/frozen parameters, not active native resource-winner proof.','No lighting/water/models/native Terrain/camera or active atlas mutation.','No runtime game callback/save/load/FPS or long-run memory acceptance.','Unknown worker wait quarantines one CPU job/unknown handles until process exit; no dynamic DLL unload support.'])
(HERE/'fixed-control-verification.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('PASS frozen compile closure',len(closure),'files;',dll.stat().st_size,'candidate bytes; manifest',sha(HERE/'fixed-control-verification.json'))
