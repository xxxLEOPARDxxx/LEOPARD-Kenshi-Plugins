"""Pin local worker/layout evidence without changing borrowed components."""
from pathlib import Path
import hashlib,json,re,sys
ROOT=Path(__file__).resolve().parents[3]
NAMES={'worker':'GlobalSatelliteMap/atlas-cache/world-write-worker',
       'layout':'GlobalSatelliteMap/atlas-terrain/world-layout'}
def pin(path):
 h=hashlib.sha256()
 with path.open('rb')as f:
  for data in iter(lambda:f.read(65536),b''):h.update(data)
 return {'bytes':path.stat().st_size,'sha256':h.hexdigest()}
name=sys.argv[1];base=ROOT/NAMES[name];manifest=base/'verification.json'
if '--verify'in sys.argv:
 data=json.loads(manifest.read_text())
 for section in ['sources','dependencies','evidence']:
  for path,expected in data[section].items():assert pin(Path(path))==expected,(section,path)
 print('PASS exact frozen '+name+' sources/dependencies/evidence');raise SystemExit
source={str(p):pin(p)for p in base.iterdir()if p.is_file()and p.name!='verification.json'}
deps={str(Path(__file__).resolve()):pin(Path(__file__).resolve())};seen=set()
def visit(path):
 path=path.resolve()
 if path in seen:return
 seen.add(path)
 if path.parent!=base:deps[str(path)]=pin(path)
 for relative in re.findall(r'^\s*#include\s+"([^"]+)"',path.read_text(),re.M):
  p=path.parent/relative
  if p.exists():visit(p)
for p in base.glob('*.h'):visit(p)
if name=='worker':
 for path in ['GlobalSatelliteMap/atlas-cache/world-window-session/verification.json',
              'GlobalSatelliteMap/atlas-terrain/world-cohort/verification.json']:
  p=ROOT/path;deps[str(p)]=pin(p)
 for suffix in ['-material.rgba16f','-normal.rgba8','-radial.r32f']:
  p=base.parent/('world-window-session/_generated/aligned0'+suffix);deps[str(p)]=pin(p)
 p=Path('D:/SteamLibrary/steamapps/common/Kenshi/data/newland/land/fullmap.tif');deps[str(p)]=pin(p)
evidence={str(p):pin(p)for p in (base/'_generated').rglob('*')if p.is_file()and p.suffix in ['.log','.exe']}
manifest.write_text(json.dumps(dict(scope='Isolated '+name+' primitive; no native install or atlas publication',status='author PASS; independent review pending',sources=source,dependencies=deps,evidence=evidence),indent=2)+'\n')
print('FROZEN',name,len(source),len(deps),len(evidence),pin(manifest)['sha256'])
