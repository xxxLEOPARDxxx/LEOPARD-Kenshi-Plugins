"""Record a completed run. Does not build, mutate dependencies, or install."""
from pathlib import Path
import hashlib,json
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert 'PASS 3145979 actual raster checks' in (OUT/'warp.log').read_text()
assert 'PASS 27 exact Init allocation failure/cleanup/retry paths' in (OUT/'faults.log').read_text()
assert 'Expected RED: partial cleanup' in (OUT/'faults.log').read_text()
assert 'PASS 18 compiled runtime raster mutants rejected' in (OUT/'mutants.log').read_text()
own={p.name:sha(p) for p in HERE.iterdir() if p.is_file() and p.name!='verification.json'}
base=HERE.parents[1]
deps=[base/'atlas-models/GraphicsState.h',base/'atlas-view/AtlasRelight.h',base/'atlas-view/AtlasReader.h',base/'atlas-view/AtlasPolicy.h',base/'atlas-view/ContextFixture.h',HERE.parent/'fixed-geometry/FixedTerrainGeometry.h',HERE.parent/'compact-material/CompactMaterial.h',base.parent/'tools/diagnostics/satellite-capture-probe/GeographyPolicy.h']
compact=HERE.parent/'compact-material'
deps += [compact/'_generated'/name for name in ['Case.h','Shaders.h','contract.json','height.u16']+[f'{case}-gbuffer{plane}.bin' for case in ['control3','control9','shift3','control1'] for plane in range(3)]]
deps += [compact/'verification.json']
manifest=dict(scope='owned fixed256 packed terrain pass only; no Main/probe/native/process/install',status='implementer PASS; independent review pending',command='python run_tests.py',sourceFiles=own,dependencies={p.as_posix():sha(p) for p in deps},outputs={p.name:sha(p) for p in OUT.iterdir() if p.is_file() and (p.suffix=='.log' or p.name=='warp_tests.exe')},results=dict(actualWarpChecks=3145979,independentManualCases=4,planesPerCase=3,compiledRuntimeMutants=18,allocationFailureCleanupRetryPaths=27,cleanupLossMutantRejected=True),budget=dict(source=83885956,fourPatchRaster=1746024,sixteenPatchRaster=3774024,controlCap=94371840,viewerCap=201326592),limitations=['Frozen two-SID source/default material domain only, not current game provenance.','All GPU and immediate-context access requires externally serialized owning thread.','Driver overhead and retained caller output references require caller accounting.','No device-removal-in-driver/OOM-allocation exhaustive proof, FPS or long-run memory claim.','Packed native material GBuffer, no lighting/water/models/final RGB map acceptance.'])
(HERE/'verification.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('PASS frozen',len(own),'own files,',len(deps),'dependencies; manifest',sha(HERE/'verification.json'))
