"""Verify output-only source surgery and the frozen default bytecodes/admission."""
from pathlib import Path
import hashlib,json
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';GAME=Path('D:/SteamLibrary/steamapps/common/Kenshi')
contract=json.loads((OUT/'source-contract.json').read_text());baseline=json.loads((HERE/'verification.json').read_text())
assert contract['shaders']==baseline['shaders'],'all four default native shader bytes/defines must stay identical'
assert len(contract['rgb_shaders'])==2 and len(contract['files'])==21 and len(contract['windows'])==47
source=(OUT/'InstanceRawRgb.hlsl').read_bytes()
for c in contract['rgb_source']['writerChanges']:
 before=c['before'].encode();after=c['after'].encode();assert source.count(after)==1;source=source.replace(after,before)
assert source==(GAME/'data/materials/deferred/objects.hlsl').read_bytes(),'exactly the two final writers may change'
for n in ['NativeInstances.h','InstanceAdmission.h','warp_tests.cpp','policy_tests.cpp','admission_tests.cpp','test_native_extracted.py','test_mutants.py','native_compile.cpp']:
 assert hashlib.sha256((HERE/n).read_bytes()).hexdigest()==baseline['production'][n],n+' unchanged default/native contract'
for shader in contract['rgb_shaders']:
 reflection=(OUT/(shader['name']+'.reflection.txt')).read_text()
 assert 'ps_4_0' in reflection and 'cb0' in reflection and 'o0' in reflection and 'o1' in reflection and 'o2' in reflection
print('PASS RGB two-writer byte reversal;four frozen default bytecodes;eight unchanged default/native sources;21 pins/47 windows')
