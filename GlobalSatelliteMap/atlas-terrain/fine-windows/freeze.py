"""Pin completed tests; no builds, generators, game or installation writes."""
from pathlib import Path
import hashlib,json
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
for mode in ['O2','Od']:
 assert 'PASS 458358 production fine-window checks' in (OUT/('run-'+mode+'.log')).read_text()
assert 'PASS 9 compiled planner mutants rejected' in (OUT/'mutants.log').read_text()
dependencies=json.loads((HERE/'dependencies.json').read_text())
for name,want in dependencies.items():assert sha(HERE.parents[2]/name)==want,name
m=dict(scope='pure bounded1/3/9m visible-window math and exact completion admission; no Main/game/cache/thread/install',status='implementer PASS; independent review pending',command='C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe run_tests.py',sourceFiles={p.name:sha(p) for p in sorted(HERE.iterdir()) if p.is_file() and p.name!='verification.json'},dependencies=dependencies,outputs={p.name:sha(p) for p in sorted(OUT.iterdir()) if p.is_file() and (p.suffix=='.log' or p.name in ['planner_O2.exe','planner_Od.exe'])},results=dict(optimizedChecks=458358,unoptimizedChecks=458358,compiledMutants=9,cppHeapAllocations=0,maxJobs=128,worldExtent=294912,worldOrigin=-147456,globalGrid9=128,globalGrid3=384,globalGrid1=1152,tileExtent=73728),limits=dict(maxGpuCap=201326592,minimumRawWindowBytes=1048576,maxScreenAxis=16384,maxJobs=128),limitations=['Not Main/viewer/renderer or cache integration.','Budget.limit must honor the smallest applicable pipeline cap; existing90MiB raster is unchanged.','Explicit source/raster/per-window residency proof remains external and must cover all proposed keys including apron/worstphase.','Same-view pan stability assumes unchanged extents/screen/reservation; genuinely changed source residency can alter admission.','1m output uses actual18m height grid and does not invent height detail; screen density may warrant only9m even at zoom8.','Key equality does not authenticate completed pixels; source/resource/worker/retirement safety remains caller responsibility.','Coarse fallback ownership is never released by this planner; no whole-view completion claim.','No FPS, whole-process peak or in-game visual acceptance claim.'])
(HERE/'verification.json').write_text(json.dumps(m,indent=2)+'\n')
print('PASS frozen',len(m['sourceFiles']),'own files,2 unchanged dependencies; SHA256',sha(HERE/'verification.json'))
