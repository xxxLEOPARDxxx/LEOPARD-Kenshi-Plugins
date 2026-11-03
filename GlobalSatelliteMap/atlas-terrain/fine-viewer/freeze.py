from pathlib import Path
import hashlib,json,re,datetime
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
sources={p.name:sha(p) for p in sorted(HERE.iterdir()) if p.is_file() and p.name!='verification.json'}
deps={}
for opt in ['Od','O2']:
 log=(OUT/('compile-'+opt+'.log')).read_text()
 for line in log.splitlines():
  m=re.match(r'Note: including file:\s*(.+)',line)
  if m:
   p=Path(m.group(1).strip());assert p.is_file(),p;deps[str(p.resolve())]=sha(p)
 log=(OUT/('run-'+opt+'.log')).read_text();assert 'PASS 34600 FineJob production controls' in log
checks={str(p.relative_to(HERE)):sha(p) for p in sorted(OUT.rglob('*.log')) if p.name not in ['compile.log','imports.log'] or 'mutants' in p.parts}
mutants=json.loads((OUT/'mutants/result.json').read_text());assert len(mutants)==2
report={'scope':'Pure fine terrain placement/native ABI job adapter and two-phase resource policy; no live viewer, native execution or IO','status':'author controls PASS; independent review pending','recordedUtc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'sourceFiles':sources,'dependencies':dict(sorted(deps.items())),'evidence':checks,'checksPerOptimization':34600,'runtimeMutantsKilled':mutants,'limits':{'fineSlots':96,'coarseViewerProductionLimitBytes':33554432,'gpuCapBytes':201326592,'cpuCapBytes':67108864,'producerK2GpuBytes':137571380}}
(HERE/'verification.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'FineJob.h':sha(HERE/'FineJob.h'),'verification.json':sha(HERE/'verification.json'),'dependencyPins':len(deps)},indent=2))
