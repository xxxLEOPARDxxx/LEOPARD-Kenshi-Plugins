from pathlib import Path
import hashlib,json,re,sys
HERE=Path(__file__).resolve().parent;DEST=HERE/'verification.json'
def pin(p):return dict(bytes=p.stat().st_size,sha256=hashlib.sha256(p.read_bytes()).hexdigest())
def verify():
 d=json.loads(DEST.read_text())
 for group in ('source','dependencies','fixtures','logs'):
  for name,expected in d[group].items():
   p=(HERE/name).resolve() if group=='source' else Path(name)
   assert p.exists() and pin(p)==expected,(group,str(p))
 print('PASS frozen accepted-window source/dependency/fixture/log pins')
if '--verify' in sys.argv:verify();raise SystemExit
own={p.name:pin(p)for p in HERE.iterdir()if p.is_file()and p.name!='verification.json'}
for folder,header in (('../../atlas-terrain/world-pixels','WorldPixels.h'),('../window-writer512','WindowWriter512.h')):
 base=(HERE/folder).resolve();frozen=json.loads((base/'verification.json').read_text());assert pin(base/header)['sha256']==frozen['sourceFiles'][header],('frozen production dependency changed',header)
deps={};seen=set()
def visit(p):
 p=p.resolve()
 if p in seen:return
 seen.add(p)
 if p.parent!=HERE:deps[str(p)]=pin(p)
 for name in re.findall(r'^\s*#include\s+"([^"]+)"',p.read_text(errors='strict'),re.M):
  q=p.parent/name
  if q.exists():visit(q)
visit(HERE/'tests.cpp')
for name in ('../../atlas-terrain/world-pixels/verification.json','../window-writer512/verification.json','../build_mips.py','../gst2.py'):
 p=(HERE/name).resolve();deps[str(p)]=pin(p)
cases=(HERE/'../../atlas-terrain/world-pixels/_generated/Cases.h').resolve().read_text();fixtures={}
for prefix in re.findall(r'\{"[^"]+","([^"]+)"',cases):
 for suffix in ('-material.rgba16f','-normal.rgba8','-radial.r32f'):
  p=Path(prefix+suffix);fixtures[str(p)]=pin(p)
logs={str(p):pin(p)for p in (HERE/'_generated').glob('*.log')}
DEST.write_text(json.dumps(dict(kind='AcceptedWindow pinned base-snapshot CPU adapter',status='implementer PASS; independent review required',command='python run_tests.py',results=dict(checks_per_optimization=23452,optimizations=['VC10 x64 /MT /O2','VC10 x64 /MT /Od'],actual_fixtures=14,pixels=1900544,writer512_cases=5,cancellation_boundaries=20,allocation_faults=2,fault_assertions=104,functional_mutants=11,cancellation_mutants=2),source=own,dependencies=deps,fixtures=fixtures,logs=logs),indent=2)+'\n')
print('FROZEN',len(own),'own',len(deps),'dependencies',len(fixtures),'fixtures',len(logs),'logs',pin(DEST)['sha256'])
