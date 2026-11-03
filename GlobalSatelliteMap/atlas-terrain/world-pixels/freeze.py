from pathlib import Path
import hashlib,json,re,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated'
def sha(p):
 h=hashlib.sha256()
 with p.open('rb')as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return h.hexdigest()
def verify(j):
 for group in ['sourceFiles','generated','evidence']:
  for n,h in j[group].items():assert sha(HERE/n)==h,(group,n)
 for n,h in j['dependencies'].items():assert sha(Path(n))==h,n
 print('PASS all frozen pixel source/dependency/fixture/evidence pins unchanged')
if '--verify'in sys.argv:verify(json.loads((HERE/'verification.json').read_text()));sys.exit()
for name in ['pixels-O2-run.log','pixels-Od-run.log']:assert 'PASS 317 production pixel controls;14actualframes;1900544 pixels;'in(OUT/name).read_text()
assert 'PASS14 compiled functional pixel mutants rejected'in(OUT/'test_mutants.py.log').read_text()
assert 'PASS 10 real cancellation boundaries plus2 allocation faults;72 assertions'in(OUT/'test_faults.py.log').read_text()
fixtures=json.loads((OUT/'fixtures.json').read_text());deps={};generated={};seen=set()
def closure(p):
 p=p.resolve()
 if p in seen:return
 seen.add(p)
 if not p.is_relative_to(HERE):deps[str(p)]=sha(p)
 if p.suffix in ['.h','.cpp']:
  for child in re.findall(r'^\s*#include\s+"([^"]+)"',p.read_text(),re.M):closure(p.parent/child)
closure(HERE/'WorldPixels.h');closure(HERE/'tests.cpp')
for n,h in fixtures['pins'].items():
 p=Path(n);assert sha(p)==h,p
 if p.is_relative_to(HERE):generated[str(p.relative_to(HERE))]=h
 else:deps[n]=h
for name in ['world-raster','world-source','world-bank','fixed-geometry','source-receipts']:
 p=HERE.parent/name/'verification.json'
 if not p.is_file():p=HERE.parent/name/'frozen.json'
 assert p.is_file(),p;deps[str(p)]=sha(p)
tiff=Path('D:/SteamLibrary/steamapps/common/Kenshi/data/newland/land/fullmap.tif');digest=sha(tiff);assert tiff.stat().st_size==536980180 and digest=='6f7d4bc7baaf80dbf8db91e6f367db71f87fe38f053a34108f192584c9637b18';deps[str(tiff)]=digest
for n in ['Cases.h','fixtures.json','radial-evidence.json','capture.cpp']:generated['_generated/'+n]=sha(OUT/n)
j=dict(scope='CPU-only authenticated fixed18m terrain triangle height plus raw pixel acceptance; no Main/game/source-winner/publication claim',status='implementer PASS; independent review pending',sourceFiles={p.name:sha(p)for p in sorted(HERE.iterdir())if p.is_file()and p.name!='verification.json'},dependencies=deps,generated=generated,evidence={str(p.relative_to(HERE)):sha(p)for p in sorted(OUT.iterdir())if p.is_file()and p.suffix=='.log'},results=dict(optimizedChecks=317,unoptimizedChecks=317,actualFrames=14,actualPixels=1900544,cancellationBoundaries=10,allocationFailures=2,faultAssertions=72,functionalMutants=14,cancellationMutants=2,maxRadialErrorMetres=0.002003347100981,radialToleranceMetres=.05),limitations=['The source is the exact canonical fullmap base snapshot, not an active native winner receipt.','Scope/frame labels require checked producer/consumer context and do not authenticate arbitrary RGB/normal image origin.','The radial tolerance is fixed measured acceptance policy, not universal native/hardware equality.','Material alpha is gloss; normal alpha must remain native zero; no RGB/gloss clamping.','All output is private and unintegrated; no map readiness, GPU upload, performance or universal leak claim.'])
(HERE/'verification.json').write_text(json.dumps(j,indent=2)+'\n');verify(j);print('Frozen',sha(HERE/'verification.json'),'own',len(j['sourceFiles']),'dependencies',len(deps),'generated',len(generated))
