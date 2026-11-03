"""Pin a completed local run. No builds, source regeneration or installation."""
from pathlib import Path
import hashlib,json
HERE=Path(__file__).resolve().parent
OUT=HERE/'_generated'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert 'PASS 9437694 actual RGB raster checks' in (OUT/'warp.log').read_text()
assert 'PASS 29 exact Init allocation/health failure cleanup/retry paths' in (OUT/'faults.log').read_text()
assert 'Expected RED: partial cleanup' in (OUT/'faults.log').read_text()
runtime=(OUT/'runtime.log').read_text()
assert 'PASS 9437706 actual RGB raster checks' in runtime
assert runtime.count('Expected RED:')==5
assert 'PASS post-draw device-health/C++ unwind recovery' in runtime
dependencies=json.loads((HERE/'dependencies.json').read_text())
for name,want in dependencies.items():
 assert sha(HERE.parents[2]/name)==want,name
manifest=dict(
 scope='owned direct-RGB fixed256 terrain raster, fixed two-SID case; no Main/probe/game/install',
 status='implementer PASS; independent review pending',
 command='C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe run_tests.py',
 sourceFiles={p.name:sha(p) for p in sorted(HERE.iterdir()) if p.is_file() and p.name!='verification.json'},
 dependencies=dependencies,
 outputs={p.name:sha(p) for p in sorted(OUT.iterdir()) if p.is_file() and (p.suffix=='.log' or p.name=='warp_tests.exe')},
 results=dict(warpChecks=9437694,runtimeFailureChecks=9437706,cases=9,planesPerCase=3,initAllocationHealthFaults=29,compiledRuntimeMutants=5,cleanupLossMutants=1,allBorrowedFilesUnchanged=True),
 budget=dict(source=83885956,targets=1310720,extraVersusPacked=262144,fourPatchRaster=2008168,sixteenPatchRaster=4036168,sourcePlusFour=85894124,sourcePlusSixteen=87922124,controlCap=94371840,viewerCap=201326592),
 limitations=['Static WARP control, not Main or in-game readiness.','Fixed compact two-SID defaults only; current native source/input provenance remains outside this primitive.','Successful Render means healthy command submission, not GPU completion or arbitrary material finiteness.','Original prewrite camera drift and half quantization remain; no perfect world-camera float equality claim.','Serialized owning-render-thread use; caller accounts retained outputs/in-flight allocations/driver overhead.','C++ unwind tested, not recovery from SEH or native driver crashes.','No FPS or long-run game memory measurement.'])
(HERE/'verification.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('PASS frozen',len(manifest['sourceFiles']),'own files,',len(dependencies),'borrowed files; SHA256',sha(HERE/'verification.json'))
