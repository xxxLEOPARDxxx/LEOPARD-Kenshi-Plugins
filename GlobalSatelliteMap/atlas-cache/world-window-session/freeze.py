from pathlib import Path
import hashlib,json,re,sys
HERE=Path(__file__).resolve().parent;DEST=HERE/'verification.json'
def pin(p):return dict(bytes=p.stat().st_size,sha256=hashlib.sha256(p.read_bytes()).hexdigest())
def verify():
 d=json.loads(DEST.read_text())
 for group in ('source','dependencies','generated'):
  for name,expected in d[group].items():
   p=HERE/name if group=='source' else Path(name)
   assert p.exists()and pin(p)==expected,(group,str(p))
 print('PASS frozen world-window-session source/dependency/generated pins')
if '--verify'in sys.argv:verify();raise SystemExit
for folder,header,key in (('../accepted-window','AcceptedWindow.h','source'),('../window-writer512','WindowWriter512.h','sourceFiles')):
 base=(HERE/folder).resolve();d=json.loads((base/'verification.json').read_text());entry=d[key][header];assert pin(base/header)['sha256']==(entry['sha256']if isinstance(entry,dict)else entry),('frozen dependency changed',header)
own={p.name:pin(p)for p in HERE.iterdir()if p.is_file()and p.name!='verification.json'};deps={};seen=set()
def visit(p):
 p=p.resolve()
 if p in seen:return
 seen.add(p)
 if p.parent!=HERE:deps[str(p)]=pin(p)
 if p.suffix in ['.h','.cpp']:
  for name in re.findall(r'^\s*#include\s+"([^"]+)"',p.read_text(),re.M):
   q=p.parent/name
   if q.exists():visit(q)
visit(HERE/'tests.cpp')
for path in json.loads((HERE/'_generated/fixtures.json').read_text())['pins']:
 p=Path(path)
 if p.parent!=HERE/'_generated':deps[str(p)]=pin(p)
for name in ('../accepted-window/verification.json','../window-writer512/verification.json'):
 p=(HERE/name).resolve();deps[str(p)]=pin(p)
generated={str(p):pin(p)for p in (HERE/'_generated').iterdir()if p.is_file()and(p.suffix in ['.log','.json','.rgba16f','.rgba8','.r32f']or p.name=='capture.cpp')}
DEST.write_text(json.dumps(dict(scope='worker-owned accepted512@9m to new incomplete GST2 files; no publication/native/Main integration',status='implementer PASS; independent review required',command='python run_tests.py',results=dict(primary_per_optimization=8775,adversarial_per_optimization=36498,optimizations=['VC10 x64 /MT /O2','VC10 x64 /MT /Od'],real_aligned_warp_windows=2,actual_files=1,source_height_pixels=524288,maximum_source_height_error_metres=.00006103515625,canonical_addresses=4096,io_and_exception_faults=26,in_io_real_cancellation_cuts=13,compiled_runtime_mutants=12),source=own,dependencies=deps,generated=generated),indent=2)+'\n')
print('FROZEN',len(own),'source',len(deps),'dependencies',len(generated),'generated',pin(DEST)['sha256'])
