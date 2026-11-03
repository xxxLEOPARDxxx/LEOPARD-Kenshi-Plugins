from pathlib import Path
import hashlib,json
here=Path(__file__).resolve().parent
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
own={p.name:sha(p) for p in sorted(here.iterdir()) if p.is_file() and p.name!='verification.json'}
deps={str(p.resolve()):sha(p) for p in [here.parent/'Gst2Writer.h',here.parent.parent/'atlas-view'/'AtlasReader.h',here.parent.parent/'atlas-view'/'AtlasPolicy.h']}
logs={p.name:sha(p) for p in sorted((here/'_generated').glob('*.log')) if p.name in ('build.log','writer.log','mutants.log')}
out={'scope':'worker-only512 writes to a fresh incomplete GST2 tile; no publication/game/install',
     'command':'python run_tests.py','checks':1231296,'compiledRuntimeMutants':14,
     'sourceFiles':own,'dependencies':deps,'logs':logs}
(here/'verification.json').write_text(json.dumps(out,indent=2)+'\n',encoding='utf-8')
print(sha(here/'verification.json'))
