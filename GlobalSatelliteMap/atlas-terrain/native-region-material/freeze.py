from pathlib import Path
import hashlib,json,re,datetime
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
deps={}
for name in ['build-Od','build-O2','native-build']:
 for line in (OUT/(name+'.log')).read_text().splitlines():
  m=re.match(r'Note: including file:\s*(.+)',line)
  if m:
   p=Path(m.group(1).strip());assert p.is_file(),p;deps[str(p.resolve())]=sha(p)
for opt in ['Od','O2']:assert 'PASS 200880 regional material controls' in (OUT/('run-'+opt+'.log')).read_text()
mutants=json.loads((OUT/'mutants/result.json').read_text());assert len(mutants)==3
source={p.name:sha(p) for p in sorted(HERE.iterdir()) if p.is_file() and p.name!='verification.json'}
evidence={str(p.relative_to(HERE)):sha(p) for p in sorted(OUT.rglob('*')) if p.is_file() and (p.suffix in ['.log','.json'] or p.name in ['Fixture.h','source.fixture','record-inputs.bin','native-case2.bin'])}
artifacts={p.name:{'bytes':p.stat().st_size,'sha256':sha(p),'hostExecuted':p.suffix=='.exe'} for p in [OUT/'tests-Od.exe',OUT/'tests-O2.exe',OUT/'native-contract.dll']}
result={'scope':'Material-only extraction of proven native regional parameter packing; no live rendering or base publication','status':'author controls PASS; independent review pending','recordedUtc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'sourceFiles':source,'dependencies':dict(sorted(deps.items())),'evidence':evidence,'artifacts':artifacts,'controlsPerOptimization':200880,'runtimeMutantsKilled':mutants,'fixtures':json.loads((OUT/'fixture-pins.json').read_text()),'limitations':['Host Access substitutes native record/profile/cell boundary; native adapter compiled/linked only, never host-executed.','Same-ou load generation, full base/material provenance, device/map and queued output freshness belong to caller.','No FPS, long-run memory or final map-completion claim.']}
(HERE/'verification.json').write_bytes((json.dumps(result,indent=2)+'\n').encode());print(json.dumps({'RegionMaterial.h':sha(HERE/'RegionMaterial.h'),'NativeAdapter.h':sha(HERE/'NativeAdapter.h'),'NativePins.h':sha(HERE/'NativePins.h'),'manifest':sha(HERE/'verification.json'),'dependencyPins':len(deps)},indent=2))
