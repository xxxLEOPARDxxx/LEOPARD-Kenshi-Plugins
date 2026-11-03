from pathlib import Path
import hashlib,json,re,sys
HERE=Path(__file__).resolve().parent;DEST=HERE/'verification.json'
def pin(p):
 h=hashlib.sha256()
 with p.open('rb')as f:
  for b in iter(lambda:f.read(65536),b''):h.update(b)
 return dict(bytes=p.stat().st_size,sha256=h.hexdigest())
if '--verify'in sys.argv:
 d=json.loads(DEST.read_text())
 for group in ['sources','dependencies','evidence']:
  for name,expected in d[group].items():assert pin(Path(name))==expected,(group,name)
 print('PASS frozen integrated world-build sources/dependencies/evidence');raise SystemExit
sources={str(p):pin(p)for p in HERE.iterdir()if p.is_file()and p.name!='verification.json'};deps={};seen=set()
def visit(p):
 p=p.resolve()
 if p in seen:return
 seen.add(p)
 if p.parent!=HERE:deps[str(p)]=pin(p)
 for name in re.findall(r'^\s*#include\s+"([^"]+)"',p.read_text(),re.M):
  q=p.parent/name
  if q.exists():visit(q)
visit(HERE/'build_tests.cpp')
for p in [HERE/'../world-native-rebind/verification.json',HERE/'../world-cohort/verification.json',HERE/'../../atlas-cache/world-window-session/verification.json',HERE/'../../atlas-cache/world-write-worker/verification.json',Path('D:/SteamLibrary/steamapps/common/Kenshi/data/newland/land/fullmap.tif')]:deps[str(p.resolve())]=pin(p)
for n in range(4):
 for plane in range(3):
  p=(HERE/f'../world-cohort/_generated/sequential-{n}-accepted-{plane}.bin').resolve();deps[str(p)]=pin(p)
evidence={str(p):pin(p)for p in (HERE/'_generated').rglob('*')if p.is_file()and p.suffix in ['.log','.exe']}
DEST.write_text(json.dumps(dict(scope='Isolated actual WIC/WARP cohort to accepted Session worker; no native caller/installation/atlas publication',status='author PASS; independent review pending',sources=sources,dependencies=deps,evidence=evidence),indent=2)+'\n')
print('FROZEN',len(sources),len(deps),len(evidence),pin(DEST)['sha256'])
