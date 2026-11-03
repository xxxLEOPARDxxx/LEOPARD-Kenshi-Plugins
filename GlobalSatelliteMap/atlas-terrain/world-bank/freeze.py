"""Freeze only this owner's sources, immutable include closure, profiles and evidence."""
from pathlib import Path
import hashlib,json,re,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated'
def sha(p):
 h=hashlib.sha256()
 with p.open('rb')as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return h.hexdigest()
def verify(j):
 for section in ['sourceFiles','dependencies','generated','evidence']:
  for name,pin in j[section].items():
   p=Path(name)if section=='dependencies'else HERE/name
   assert p.is_file()and sha(p)==pin,(section,str(p))
 for entry in j['encodedRgbaAssets']:
  p=Path(entry['path']);assert p.stat().st_size==entry['bytes']and sha(p)==entry['sha256'],p
 print('PASS all frozen source/dependency/profile/evidence and129 encoded RGBA pins')
if '--verify'in sys.argv:
 verify(json.loads((HERE/'verification.json').read_text()));sys.exit()
assert 'PASS 76179 world bank checks'in (OUT/'warp.log').read_text()
assert 'PASS22 source allocation/health/counter fault paths +2 compiled'in (OUT/'faults.log').read_text()
assert 'PASS22 compiled functional source-bank mutants rejected'in (OUT/'mutants.log').read_text()
planner=HERE.parent/'world-material/verification.json'
assert sha(planner)=='2f4e4ce3b117a1ec4fd38a63d698afea1bdd7638ac3c9f8e6ddc0d5facd74e97'
deps={};seen=set()
def closure(p):
 p=p.resolve()
 if p in seen:return
 seen.add(p)
 if not p.is_relative_to(HERE):deps[str(p)]=sha(p)
 if p.suffix in ['.h','.cpp']:
  for relative in re.findall(r'^\s*#include\s+"([^"]+)"',p.read_text(),re.M):
   child=(p.parent/relative).resolve();assert child.is_file(),child;closure(child)
closure(HERE/'WorldSourceBank.h');closure(HERE/'warp_tests.cpp')
deps[str(planner)]=sha(planner)
prior=json.loads(planner.read_text())
for name,pin in prior['dependencies'].items():
 assert sha(Path(name))==pin,name
 deps[name]=pin
for asset,pin in prior['authenticatedRealFiles'].items():
 p=Path(asset);assert p.stat().st_size==pin['bytes']and sha(p)==pin['sha256'],p
 deps[asset]=pin['sha256']
pins=json.loads((OUT/'decoded-pins.json').read_text())
game=Path('D:/SteamLibrary/steamapps/common/Kenshi')
assets=[dict(path=str(game/p['path']),bytes=p['encodedBytes'],sha256=p['encodedSha256'])for p in pins['pins']]
own={p.name:sha(p)for p in sorted(HERE.iterdir())if p.is_file()and p.name!='verification.json'}
generated={str(p.relative_to(HERE)):sha(p)for p in sorted(OUT.iterdir())if p.name in ['DecodedPins.h','decoded-pins.json']or p.name.startswith('rgba-')and p.suffix=='.bin'}
evidence={str(p.relative_to(HERE)):sha(p)for p in sorted(OUT.iterdir())if p.suffix=='.log'}
j=dict(scope='Single-SID D3D11 staged source bank; pinned base snapshot only; no Main/probe/game integration',status='implementer PASS; independent review pending',sourceFiles=own,dependencies=deps,generated=generated,evidence=evidence,encodedRgbaAssets=assets,results=dict(warpChecks=76179,realStages=6,compressedSubresources=648,rgbaUploads=5,faultPaths=22,functionalMutants=22,faultMutants=2),budget=dict(sourcePayload=83885956,sourceLimit=94371840,sharedCap=201326592,observerReadback=16777216),limitations=['Pinned base snapshot does not establish current authoritative native resource winners.','Caller must disclose all live GPU owners, including references retained after Clear.','No renderer, whole-atlas acceptance, active mod resolution, or runtime performance claim.'])
(HERE/'verification.json').write_text(json.dumps(j,indent=2)+'\n');verify(j)
print('Frozen manifest',sha(HERE/'verification.json'),'own',len(own),'dependencies',len(deps),'generated',len(generated))
