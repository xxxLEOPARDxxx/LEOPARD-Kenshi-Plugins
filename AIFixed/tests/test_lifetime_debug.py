"""Compile exact AIFixed lifecycle/debug wrappers against narrow native-boundary fakes.

No game process is needed. Native world/medical data are boundary fakes;
production SelfRepairOne still decides and emits the final repair/get-up order.
"""
from pathlib import Path
import os
import re
import subprocess
import sys

here = Path(__file__).resolve().parent
source = (here.parent / 'AIFixed.cpp').read_text(encoding='utf-8-sig')

def function(name, optional=False):
    match = re.search(r'^(?:    |extern "C" __declspec\(dllexport\) )(?:void|bool|Building\*) ' + re.escape(name) + r'\(', source, re.M)
    if not match:
        if optional:
            return ''
        raise AssertionError('Missing production function: ' + name)
    opening = source.index('{', match.start())
    end, depth = opening + 1, 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end]

state_start = source.index('    const DWORD SELF_REPAIR_SCAN_MS')
state_end = source.index('    float WorstWear', state_start)
state = source[state_start:state_end]
identity_start = source.find('    struct HandIdentityLess')
identity = source[identity_start:source.index('    struct IntentGuard',identity_start)] if identity_start >= 0 else ''
helpers = '\n'.join(function(name, optional=True) for name in (
    'InvalidateWorld', 'WorldActive', 'ResetSessionState', 'SelfRepairSessionReady',
    'World_hook', 'LoadGame_hook', 'NewGame_hook', 'ResetGame_hook', 'ClearWorld_hook'))
wrappers = '\n'.join(function(name) for name in (
    'OwnRestOrder', 'OwnRestOrderMatches','LiveRepairBed', 'SelfRepairOne', 'ReadRestClock','ReadGateClock','SelfRepairScan', 'GateUpdate','GuiUpdate_hook', 'Drop_hook', 'SetBedMode_hook',
    'TaskImpossible_hook', 'Medical_hook', 'MCM_Describe'))
if '--baseline-repair' in sys.argv:
    current=source
    source=subprocess.check_output(['git','-C',str(here.parents[1]),'show','86642928f57ca18f0fb4a85b6809362652288adf:AIFixed/AIFixed.cpp']).decode('utf-8-sig')
    old=function('SelfRepairOne')
    source=current
    wrappers=wrappers.replace(function('SelfRepairOne'),old)
fixture = (here / 'lifetime_debug_fixture.cpp').read_text(encoding='utf-8')
fixture = fixture.replace('// @STATE@', identity + '\n' + state)
fixture = fixture.replace('// @PRODUCTION@', helpers + '\n' + wrappers)
fixture = fixture.replace('// @LIFECYCLE@', '#define HAS_LIFECYCLE 1' if function('InvalidateWorld', True) else '')
out = here / '_generated'
out.mkdir(exist_ok=True)
stem='lifetime_debug_baseline' if '--baseline-repair' in sys.argv else 'lifetime_debug'
cpp, exe = out / (stem+'.cpp'), out / (stem+'.exe')
cpp.write_text(fixture, encoding='utf-8')
vc = Path(r'C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC')
sdk = Path(r'C:\Program Files\Microsoft SDKs\Windows\v7.1')
env = os.environ.copy()
env['PATH'] = str(vc / 'bin/amd64') + ';' + env['PATH']
env['INCLUDE'] = str(vc / 'include') + ';' + str(sdk / 'Include') + ';' + str(here.parents[1] / 'shared')+';'+str(here.parent)
env['LIB'] = str(vc / 'lib/amd64') + ';' + str(sdk / 'Lib/x64')
subprocess.run([str(vc / 'bin/amd64/cl.exe'), '/nologo', '/EHsc', '/MD', '/Od',
                str(cpp), '/Fe' + str(exe), '/Fo' + str(out) + '\\'],
               env=env, cwd=out, check=True)
subprocess.run([str(exe)], cwd=out, check=True)
