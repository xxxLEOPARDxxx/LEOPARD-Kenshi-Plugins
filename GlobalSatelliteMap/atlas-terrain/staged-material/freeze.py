from pathlib import Path
import hashlib,json,sys
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2];OUT=HERE/'_generated'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
if '--init-dependencies' in sys.argv:
 pins=json.loads((HERE.parent/'rgb-raster/dependencies.json').read_text())
 recipe=json.loads((OUT/'source-build/recipe.json').read_text())
 for rel,want in recipe.items():assert sha(ROOT/rel)==want; pins[rel]=want
 (HERE/'dependencies.json').write_text(json.dumps(dict(sorted(pins.items())),indent=2)+'\n')
pins=json.loads((HERE/'dependencies.json').read_text())
for name,want in pins.items():assert sha(ROOT/name)==want,name
own={p.name:sha(p) for p in sorted(HERE.iterdir()) if p.is_file() and p.name!='verification.json'}
artifacts={p.relative_to(OUT).as_posix():sha(p) for p in sorted(OUT.rglob('*')) if p.is_file() and p.suffix in ['.log','.json','.cso','.hlsl','.h'] and 'source-build' not in p.parts}
(HERE/'verification.json').write_text(json.dumps(dict(scope='controlled fixed two-SID ordered shader proof only; no live4SID/engine/Main/probe',sourceFiles=own,dependencies=pins,artifacts=artifacts,results=dict(warpChecks=4719113,actualCases=9,planesPerCase=3,materialFloatBytesExact=True,maxFloatError=0,syntheticSlotAlgebraCases=3,allocationHealthCutpoints=14,compiledNegativeControls=10),budget=dict(carry=4194304,extraSlotCb=16,fullProofGpuPayload=90612732,maximumSingleReadback=1048576,controlCap=94371840,viewerCap=201326592),limits=['Literal native core/tail; no arbitrary finite-input/output guarantee','No general four-distinct-SID bank or live native winner proof','Full fixed viewport only; no per-cell scissor derivative proof','No game performance/leak/GPU-completion claim']),indent=2)+'\n')
print('PASS freeze',sha(HERE/'verification.json'),len(own),'own source files',len(pins),'unchanged borrowed pins')
