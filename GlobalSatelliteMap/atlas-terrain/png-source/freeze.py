from pathlib import Path
import hashlib,json
here=Path(__file__).resolve().parent
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
own={p.name:sha(p) for p in sorted(here.iterdir()) if p.is_file() and p.name!='verification.json'}
deps={str(p.resolve()):sha(p) for p in [here.parent/'world-bank/WorldSourceBank.h',here.parent/'world-bank/verification.json',here.parent/'world-bank/_generated/DecodedPins.h']}
logs={p.name:sha(p) for p in sorted((here/'_generated').glob('*.log'))}
(here/'verification.json').write_text(json.dumps({'scope':'worker-only canonical PNG input; no current winner/Main/game/install claim','command':'python run_tests.py','sourceFiles':own,'dependencies':deps,'checks':660,'realImages':129,'compiledRuntimeMutants':5,'logs':logs},indent=2)+'\n',encoding='utf-8')
print(sha(here/'verification.json'))
