from pathlib import Path
import hashlib,json
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
for n in ['plan.log','debug.log']:assert 'PASS 61105 real material planner checks'in (OUT/n).read_text()
assert 'PASS 11 compiled real material planner mutants rejected'in (OUT/'mutants.log').read_text()
auth=json.loads((OUT/'authenticated-plans.json').read_text());deps=dict(auth['pins'])
deps[str(HERE.parents[1]/'atlas-view/AtlasPolicy.h')]=sha(HERE.parents[1]/'atlas-view/AtlasPolicy.h')
for p,h in deps.items():assert sha(Path(p))==h
own={p.name:sha(p)for p in HERE.iterdir()if p.is_file()and p.name!='verification.json'}
j=dict(scope='CPU base-snapshot cell/bank planner +qualified ordered-algebra/source/paint/residency design; no renderer/game/frozen-folder edits',status='implementer PASS; parent independent review pending',sourceFiles=own,dependencies=deps,realCases=auth['cases'],authenticatedRealFiles=auth['checkedRealAssets'],results=dict(optimizedChecks=61105,unoptimizedChecks=61105,compiledMutants=11,cells=1024,fourSidCells=49),budget=dict(cap=201326592,worstFourSidDds=192937248,worstMonoSources=226491748,fixedSidSourcesWithFullPaintAndBlend=83885956,staged256SourcesPlusPingPong=88080260,fourPatchRgbBeforeReadback=90088444,rgbMaterialReadback=524288,rgbNormalReadback=262144,rgbHeightReadback=262144,fourPatchRgbWithThreeReadbacks=91137020,separateFp32ProofBeforeObserver=90612732,fp32ProofOneObserver=1048576),outputs={p.name:sha(p)for p in OUT.iterdir()if p.is_file()and(p.suffix=='.log'or p.name in ['Expected.h','authenticated-plans.json'])},limitations=['Current authoritative native resource-winner mapping remains a required production receipt.','Budget candidates are arithmetic only; stage-side256. No GPU loader or material values fabricated.','Exact original selectors/slots15CBrows retained; bank-role indices only are remapped.','Multi-cell/paint-scissor derivative parity and production whole-owner residency require separate proof.','Staged parity is against pinned private monolithic reference, not stock native DXBC byte equality.'])
(HERE/'verification.json').write_text(json.dumps(j,indent=2)+'\n')
print('PASS frozen',len(own),'own files;',len(deps),'dependencies;',len(auth['checkedRealAssets']),'real case files;manifest',sha(HERE/'verification.json'))
