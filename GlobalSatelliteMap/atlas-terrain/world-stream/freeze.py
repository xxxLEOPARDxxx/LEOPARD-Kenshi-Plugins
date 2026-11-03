from pathlib import Path
import hashlib,json
here=Path(__file__).resolve().parent
def sha(p):
 h=hashlib.sha256()
 with p.open('rb')as f:
  for b in iter(lambda:f.read(65536),b''):h.update(b)
 return h.hexdigest()
own={p.name:sha(p) for p in sorted(here.iterdir()) if p.is_file() and p.name!='verification.json'}
deps={}
for folder,files in [('world-raster',['WorldTerrainRaster.h','verification.json']),('world-bank',['WorldSourceBank.h','verification.json']),('world-source',['WorldHeightSource.h','verification.json']),('source-receipts',['MainGate.h','WorkerFileLease.h','ReceiptPolicy.h','verification.json']),('png-source',['PinnedPng.h','verification.json']),('world-native',['SourceFactory.h','StageAdapter.h','NativeProvider.h','verification.json'])]:
 for file in files:
  p=here.parent/folder/file;deps[str(p.resolve())]=sha(p)
logs={p.name:sha(p) for p in sorted((here/'_generated').glob('*.log'))}
out={'scope':'one-window worker/sourcegate/WIC/staged GPU/readback development core; raw radial output; fake metadata host control; no game/Main installation','command':'python run_tests.py','sourceFiles':own,'dependencies':deps,'logs':logs,'gpuReservationBytes':112825820,'limits':['one process-local worker slot','same-thread Main/destruction; joined worker','unknown failure retains conservative debt until restart','raw outputs require separate finite/world-height acceptance','not a full-world throughput scheduler']}
(here/'verification.json').write_text(json.dumps(out,indent=2)+'\n',encoding='utf-8');print(sha(here/'verification.json'))
