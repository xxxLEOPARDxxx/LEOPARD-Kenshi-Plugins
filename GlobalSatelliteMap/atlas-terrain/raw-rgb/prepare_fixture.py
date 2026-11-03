"""Use compact's ignored source recipe; relocate remaining fixtures here."""
from pathlib import Path
import hashlib,json,importlib.util,sys
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;COMPACT=HERE.parent/'compact-material';OUT=HERE/'_generated';FIX=OUT/'fixture';FIX.mkdir(parents=True,exist_ok=True)
sha=lambda b:hashlib.sha256(b).hexdigest()
spec=importlib.util.spec_from_file_location('compact_source_preparation',COMPACT/'prepare_sources.py');preparation=importlib.util.module_from_spec(spec);spec.loader.exec_module(preparation);preparation.prepare_sources(COMPACT)
frozen=json.loads((COMPACT/'verification.json').read_text())
for n,h in frozen['production'].items():assert sha((COMPACT/n).read_bytes())==h,n
source=(COMPACT/'generate.py').read_text();needle="OUT=HERE/'_generated'";assert source.count(needle)==1
relocated=source.replace(needle,'OUT=Path('+repr(FIX.as_posix())+')')
sys.path.insert(0,str(COMPACT))
try:exec(compile(relocated,str(COMPACT/'generate.py'),'exec'),{'__file__':str(COMPACT/'generate.py'),'__name__':'relocated_fixture'})
finally:sys.path.remove(str(COMPACT))
# The production source owner is compiled unchanged except include locations;
# generated case/shaders and every test output stay under raw-rgb/_generated.
header=(COMPACT/'CompactMaterial.h').read_text()
header=header.replace('"../../atlas-view/AtlasRelight.h"','"'+(COMPACT.parents[1]/'atlas-view/AtlasRelight.h').as_posix()+'"').replace('"../../../tools/diagnostics/satellite-capture-probe/GeographyPolicy.h"','"'+(COMPACT.parents[2]/'tools/diagnostics/satellite-capture-probe/GeographyPolicy.h').as_posix()+'"').replace('"_generated/Case.h"','"'+(FIX/'Case.h').as_posix()+'"')
(FIX/'CompactMaterial.h').write_text(header)
for n,h in frozen['production'].items():assert sha((COMPACT/n).read_bytes())==h,n
(OUT/'fixture-provenance.json').write_text(json.dumps(dict(compactFreezeSha256=sha((COMPACT/'verification.json').read_bytes()),compactSourceHashes=frozen['production'],relocation='one output-directory assignment + include paths; compact source snapshots use its own ignored reconstruction recipe, other fixtures written only here'),indent=2)+'\n')
print('PASS compact source recipe/pins preserved; remaining fixtures prepared in raw-rgb only')
