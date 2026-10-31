"""Extract actual RMB policy/hooks; compile isolated VC10 regression, no game access."""
from pathlib import Path
import hashlib
import json
import os
import re
import subprocess

tests = Path(__file__).resolve().parent
source = tests.parent / 'UITweaks.cpp'
out = tests / '_generated'
out.mkdir(exist_ok=True)
text = source.read_text(encoding='utf-8-sig')

def function(name):
    match = re.search(r'^    (?:bool|void) ' + re.escape(name) + r'\(', text, re.M)
    assert match, name
    opening = text.index('{', match.start())
    end, depth = opening + 1, 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end]

policy = text[text.index('    bool g_rmbWasDown ='):text.index('    DWORD g_lastDropLogMs =')]
hooks = text[text.index('    void (*g_origAddOrder)'):text.index('    void HookByExport(')]
fixture = (tests / 'right_click_fixture.cpp').read_text(encoding='utf-8')
generated = fixture.replace('// EXTRACT_PRODUCTION', policy + '\n' + function('DropOrder') + '\n' + hooks)
cpp = out / 'actual_right_click.cpp'
cpp.write_text(generated, encoding='utf-8')
vc = Path(r'C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC')
sdk = Path(r'C:\Program Files\Microsoft SDKs\Windows\v7.1')
env = os.environ.copy()
env['INCLUDE'] = ';'.join(map(str, [vc / 'include', sdk / 'Include']))
env['LIB'] = ';'.join(map(str, [vc / 'lib/amd64', sdk / 'Lib/x64']))
env['PATH'] = str(vc / 'bin/amd64') + ';' + env['PATH']
exe = out / 'actual_right_click.exe'
compiled = subprocess.run([str(vc / 'bin/amd64/cl.exe'), '/nologo', '/EHsc', '/MD', '/O2',
                           str(cpp), '/Fe' + str(exe), '/Fo' + str(out / 'actual_right_click.obj')],
                          env=env, capture_output=True, text=True)
if compiled.returncode:
    print(compiled.stdout, compiled.stderr)
    raise SystemExit(compiled.returncode)
result = subprocess.run([str(exe)], env=env, capture_output=True, text=True)
print(result.stdout, result.stderr, end='')
(out / 'right_click_verification.json').write_text(json.dumps({
    'source': str(source), 'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
    'fixture_sha256': hashlib.sha256((tests / 'right_click_fixture.cpp').read_bytes()).hexdigest(),
    'compile_exit_code': compiled.returncode, 'test_exit_code': result.returncode,
    'stdout': result.stdout, 'stderr': result.stderr,
    'extracted': ['PressOverInterface', 'TrackRightButton', 'DropOrder', 'AddOrder_hook',
                  'NewTask_hook', 'MoveWaypoint_hook', 'PlayerMove_hook', 'CharMoveOrder_hook'],
}, indent=2), encoding='utf-8')
raise SystemExit(result.returncode)
