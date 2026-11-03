"""Pin the reusable raster's sources, borrowed closure and bounded test evidence."""
from pathlib import Path
import hashlib,json,re,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated'
def sha(p):
 h=hashlib.sha256()
 with p.open('rb')as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return h.hexdigest()
def check(pins):
 for name,pin in pins.items():
  p=Path(name);assert p.is_file()and sha(p)==pin,name
def dependencies():
 check(json.loads((HERE/'dependencies.json').read_text()))
 print('PASS immutable borrowed source/shader/oracle/asset dependency pins')
if '--dependencies'in sys.argv:dependencies();sys.exit()
if '--pin-dependencies'in sys.argv:
 deps={};seen=set()
 def add(p):
  p=p.resolve()
  if p in seen:return
  seen.add(p)
  if not p.is_relative_to(HERE):deps[str(p)]=sha(p)
  if p.suffix in ['.h','.cpp']:
   for relative in re.findall(r'^\s*#include\s+"([^"]+)"',p.read_text(),re.M):add(p.parent/relative)
 for name in ['WorldTerrainRaster.h','warp_tests.cpp','api_tests.cpp']:add(HERE/name)
 bank=HERE.parent/'world-bank/verification.json';prior=json.loads(bank.read_text());add(bank)
 for name,pin in prior['dependencies'].items():
  p=Path(name);assert sha(p)==pin,name;deps[name]=pin
 for entry in prior['encodedRgbaAssets']:
  p=Path(entry['path']);assert p.stat().st_size==entry['bytes']and sha(p)==entry['sha256'];deps[str(p)]=entry['sha256']
 for i in [27,54,91,118,128]:add(HERE.parent/f'world-bank/_generated/rgba-{i}.bin')
 add(HERE.parent/'world-bank/_generated/decoded-pins.json')
 for stem in ['control3','control9','shift3','control1','wet3','underwater3','hdr3','hdr1','odd3']:
  for suffix in ['-material.rgba16f','-normal.rgba8','-radial.r32f']:add(HERE.parent/('raw-rgb/_generated/'+stem+suffix))
 add(Path('D:/SteamLibrary/steamapps/common/Kenshi/data/newland/land/fullmap.tif'))
 (HERE/'dependencies.json').write_text(json.dumps(dict(sorted(deps.items())),indent=2)+'\n');dependencies();sys.exit()
if '--verify'in sys.argv:
 j=json.loads((HERE/'verification.json').read_text())
 for section in ['sourceFiles','generated','evidence']:
  check({str(HERE/k):v for k,v in j[section].items()})
 dependencies();print('PASS frozen world-raster sources, fixtures and test evidence');sys.exit()
dependencies()
assert 'PASS 14 world-raster API checks'in(OUT/'api_tests.log').read_text()
warp=(OUT/'warp_tests.log').read_text();assert '9actual256+5actual512oracle+1actualfourSID'in warp
failure=(OUT/'failures.log').read_text()
for text in ['PASS 42 Init HRESULT and42 C++ exception','PASS 1489 focused','Expected RED read-restore','PASS bounded owner/phase faults and10 compiled functional controls']:assert text in failure,text
own={p.name:sha(p)for p in sorted(HERE.iterdir())if p.is_file()and p.name!='verification.json'}
generated={str(p.relative_to(HERE)):sha(p)for p in sorted(OUT.iterdir())if p.name in ['Materials.h','height-two.u16','height-four.u16','fixtures.json']}
evidence={str(p.relative_to(HERE)):sha(p)for p in sorted(OUT.iterdir())if p.name in ['api_tests.log','warp_tests.log','failures.log','prepare_fixtures.log','dependencies-before.log','dependencies-after.log']}
j=dict(scope='Reusable ordered staged world raster; pinned base snapshot; no Main/probe/live native integration',status='implementer PASS; independent review pending',command='C:/Users/User/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe run_tests.py',sourceFiles=own,generated=generated,evidence=evidence,results=dict(apiChecks=14,warpChecks=int(re.search(r'PASS (\d+) reusable',warp)[1]),actual256Controls=9,actual512MonolithicControls=5,actualFourSidSmoke=1,initHresultFaults=42,initExceptionFaults=42,phaseChecks=1489,functionalMutants=10,cleanupMutants=1),budget=dict(sharedCap=201326592,sourceBank=83885956,raster256SixteenPatches=8230488,raster512SixteenPatches=24745560),limitations=['Pinned base source/material parameters do not prove current native winners or live weather.','Four-SID case is real staged-source coverage/finite/state smoke, not a full-bank monolithic parity proof.','Native inherited camera-float drift remains; finer output adds no height samples beyond the 18m source.','Ready proves healthy command submission, not GPU completion; caller owns fences, serialized context, complete GPU ledger and source/material/frame authority.','No game FPS, universal finite-output, whole-world seam or leak-free claim.'])
(HERE/'verification.json').write_text(json.dumps(j,indent=2)+'\n');print('Frozen world-raster',sha(HERE/'verification.json'),'own',len(own),'borrowed',len(json.loads((HERE/'dependencies.json').read_text())))
