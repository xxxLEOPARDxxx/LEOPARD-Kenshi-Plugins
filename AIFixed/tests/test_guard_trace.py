"""Replay the complete retained capture histogram through actual Inspect/CanTravel.

The source capture retained 9 first-seen snapshots + counts, NOT all 316 samples
in temporal order. Both histogram expansion order and >4-hour clock are synthetic.
No native process is opened and no game code is loaded/executed.
"""
from pathlib import Path
import json
import os
import subprocess
import sys

here=Path(__file__).resolve().parent
root=here.parent
out=here/'_generated'
out.mkdir(exist_ok=True)
runtime=(root/'UnstuckRuntime.h').read_text(encoding='utf-8')
if '--baseline-reset' in sys.argv:
    runtime=subprocess.check_output(['git','show','3bc09133341a2a87a32bfaca800be086a2bfae3e:AIFixed/UnstuckRuntime.h'],cwd=root,text=True,encoding='utf-8')
capture=json.loads((here/'fixtures/guard_cycle_20261112.json').read_text(encoding='utf-8'))
assert capture['sample_count']==316 and len(capture['snapshots'])==9
assert sum(s['count'] for s in capture['snapshots'])==316
assert all(s['support']==capture['support'] and s['pos']==capture['snapshots'][0]['pos'] for s in capture['snapshots'])
assert capture['actor']==[1,15,98510744,6,3684570368]
def part(first,last):
    start=runtime.index(first)
    return runtime[start:runtime.index(last,start)]
def number(value):
    s=format(value,'.9g')
    return s+('' if '.' in s or 'e' in s.lower() else '.0')+'f'
def vector(values):
    return '{'+','.join(number(v) for v in values)+'}'
rows=[]
for s in capture['snapshots']:
    rows.append('    {'+','.join(['static_cast<TaskType>('+str(s['goal'] or 0)+')',str(s['action'] or 0),str(s['state']),str(s['path']),vector(s['pos']),vector(s['nav_pos']),vector(s['nav_goal']),str(s['count'])])+'}')
fixture=(here/'guard_trace_runtime_fixture.cpp').read_text(encoding='utf-8')
sections={
    '// @STRUCTS@':part('    struct Identity','    // The movement hook')+('' if 'struct LandingTrace' in runtime else '\nstruct LandingTrace {};\n'),
    '// @CAN_TRAVEL@':part('    bool CanTravel(','    AIFixedGate::Point Point('),
    '// @GUARD_CYCLE@':part('    Building* ReadGuardCycle(','    bool WallAllowsLanding('),
    '// @TRACE@':part('    void TracePlatform(','    void Inspect(') if '    void TracePlatform(' in runtime else '',
    '// @INSPECT@':part('    void Inspect(','    void Update('),
    '// @CAPTURED@':'const Captured captured[]={\n'+',\n'.join(rows)+'\n};',
}
for marker,code in sections.items():
    assert marker in fixture
    fixture=fixture.replace(marker,code)
generated=out/'guard_trace_runtime.cpp'
generated.write_text(fixture,encoding='utf-8')
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=os.environ.copy()
env['PATH']=str(vc/'bin/amd64')+';'+env['PATH']
env['INCLUDE']=str(vc/'include')+';'+str(sdk/'Include')
env['LIB']=str(vc/'lib/amd64')+';'+str(sdk/'Lib/x64')
exe=out/'guard_trace_runtime.exe'
subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/Od','/I'+str(root),str(generated),'/Fe'+str(exe),'/Fo'+str(out/'guard_trace_runtime.obj')],cwd=out,env=env,check=True)
subprocess.run([str(exe)],check=True)
