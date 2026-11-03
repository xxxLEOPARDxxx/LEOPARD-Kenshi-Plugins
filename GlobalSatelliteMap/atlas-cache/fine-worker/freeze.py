from pathlib import Path
import hashlib,json,re,sys
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
files=set()
def include(p):
    p=p.resolve()
    if p in files:return
    assert p.is_relative_to(ROOT) and p.is_file(),str(p)
    files.add(p)
    if p.suffix in ('.h','.cpp'):
        for rel in re.findall(r'^\s*#include\s+"([^"]+)"',p.read_text(),re.M):include(p.parent/rel)
for p in HERE.iterdir():
    if p.is_file() and p.name!='verification.json':include(p)
value={'format':'GSM_FINE_WORKER_VERIFICATION','version':1,'files':[{'path':p.relative_to(ROOT).as_posix(),'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in sorted(files)],'scope':'single persistent CPU file worker; no native game/GPU/viewer/FPS claim'}
target=HERE/'verification.json'
if '--verify' in sys.argv:
    assert json.loads(target.read_text())==value,'frozen input changed'
    print('PASS frozen',len(files),'fine-worker input files')
else:
    target.write_text(json.dumps(value,indent=2)+'\n')
    print('FROZEN',hashlib.sha256(target.read_bytes()).hexdigest())
