"""Exact on-disk native evidence and compiled negative production fixtures."""
from pathlib import Path
import hashlib
import os
import re
import struct
import subprocess
import sys
from test_pe_contract import PE

base = Path(__file__).resolve().parents[1]
header = (base / 'GsmTownDiagnostic.h').read_text()
engine = PE(Path('D:/SteamLibrary/steamapps/common/Kenshi/RE_Kenshi/Kenshi_x64.exe'))
assert hashlib.sha256(engine.data).hexdigest() == '504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e'
pins = re.findall(r'\{0x([0-9a-f]+),\{([^}]+)\}\}', header)
assert len(pins) == 5
for rva, values in pins:
    expected = bytes(int(v, 16) for v in values.split(','))
    assert len(expected) == 24
    at = engine.offset(int(rva, 16))
    assert engine.data[at:at+24] == expected
assert struct.unpack_from('<Q', engine.data, engine.offset(0x1734c08+0x268))[0] == 0x1400253a6
print('Town native contract: exact504b EXE/5code24/primaryVTthunk PASS')
if '--skip-mutations' in sys.argv:
    raise SystemExit(0)

out = base / 'build/town-diagnostic-review/mutants'
out.mkdir(parents=True, exist_ok=True)
fixture = (base / 'tests/town_diagnostic_tests.cpp').read_text()
vc = Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
sdk = Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env = os.environ.copy()
env['INCLUDE'] = str(vc/'include')+';'+str(sdk/'Include')
env['LIB'] = str(vc/'lib/amd64')+';'+str(sdk/'Lib/x64')
mutations = [
    ('reject-completed-state2', 'return state==2||state==3;', 'return state==3;'),
    ('accept-pending-state1', 'return state==2||state==3;', 'return state>=1&&state<=3;'),
    ('query-off', 'if(!enabled)return S_FALSE;', ''),
    ('query-unverified', 'if(!sourceVerified)return E_ACCESSDENIED;', ''),
    ('capacity4097', 'Limit=4096', 'Limit=4097'),
    ('other-town-type', 'if(Pointer(town)!=engine+TownVtable)', 'if(false)'),
    ('skip-header-postcheck', '!Header(king,after)||!SameHeader(first,after)', 'false'),
    ('skip-candidate-postcheck', 'chosen&&!SameCandidate', 'false&&!SameCandidate'),
    ('reuse-nonvolatile-header', 'const void*const volatile*', 'const void*const*'),
    ('skip-native-profile', 'if(!Profile(engine))', 'if(false)'),
    ('last-candidate', 'if(!chosen)', 'if(true)'),
]
for name, before, after in mutations:
    assert before in header, name
    mutated = header.replace(before, after)
    if name == 'reuse-nonvolatile-header':
        mutated = mutated.replace('const volatile unsigned*', 'const unsigned*')
    mutant = out / (name+'.h')
    mutant.write_text(mutated)
    cpp = out / (name+'.cpp')
    cpp.write_text(fixture.replace('#include "../GsmTownDiagnostic.h"', '#include "'+mutant.as_posix()+'"'))
    exe = out / (name+'.exe')
    with (out/(name+'-compile.log')).open('w') as log:
        subprocess.run([str(vc/'bin/amd64/cl.exe'), '/nologo', '/EHsc', '/MD', '/O2', '/W3', '/Fe'+str(exe), '/Fo'+str(out/(name+'.obj')), str(cpp)], env=env, check=True, stdout=log, stderr=subprocess.STDOUT)
    with (out/(name+'-run.log')).open('w') as log:
        result = subprocess.run([str(exe)], stdout=log, stderr=subprocess.STDOUT)
    assert result.returncode != 0, name+' survived'
    print('Town compiled functional mutant rejected:', name)
