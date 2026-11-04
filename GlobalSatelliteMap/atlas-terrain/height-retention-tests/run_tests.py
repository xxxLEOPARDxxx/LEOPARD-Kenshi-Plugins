"""Tiny controls extracting the production worker; no game assets or D3D device."""
from pathlib import Path
import os, re, subprocess, sys

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
OUT = HERE / '_generated'
OUT.mkdir(exist_ok=True)
VC = Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
SDK = Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env = dict(os.environ, INCLUDE=f'{VC}/include;{SDK}/Include', LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
stream = (ROOT / 'world-stream/WorldStream.h').read_text()
worker = stream[stream.index('enum WorkerState'):stream.index('inline bool Add')]
cohort = (ROOT / 'world-cohort/WorldCohort.h').read_text()

def block(source, start):
    begin = source.index(start)
    opening = source.index('{', begin)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[begin:end]

controller = '\n'.join(block(cohort, start) for start in ['bool Send(WorldStream::Command', 'void Cancel(){', 'bool DrainCompleted(){']) if 'bool DrainCompleted(){' in cohort else ''
cancelling = block(cohort, 'if(state_==Cancelling){')
ordinary = 'Trim' if 'Stop,Trim}' in worker else 'Close'
if '--legacy-close' in sys.argv:
    ordinary = 'Close'
code = (HERE / 'worker_controls.cpp').read_text().replace('// EXTRACTED_WORKER', worker).replace('ORDINARY_COMMAND', ordinary)
if controller:
    code = code.replace('// EXTRACTED_CONTROLLER', controller).replace('// EXTRACTED_CANCEL_STEP', cancelling)
    code = code.replace('// EXTRACTED_CONTROL_TESTS', (HERE / 'cohort_controls.inc').read_text())
else:
    code = code.replace('// EXTRACTED_CANCEL_STEP', 'return E_FAIL;')
    code = code.replace('// EXTRACTED_CONTROL_TESTS', '')
cpp = OUT / 'worker.cpp'
cpp.write_text(code)
exe = OUT / 'worker.exe'
cmd = [str(VC / 'bin/amd64/cl.exe'), '/nologo', '/EHsc', '/O2', '/MT', '/W3', str(cpp), '/Fo' + str(OUT / 'worker.obj'), '/Fe' + str(exe)]
p = subprocess.run(cmd, cwd=OUT, env=env, capture_output=True, text=True)
(OUT / 'build.log').write_text(p.stdout + p.stderr)
if p.returncode:
    print(p.stdout + p.stderr)
    sys.exit(p.returncode)
p = subprocess.run([str(exe)], cwd=OUT, env=env, capture_output=True, text=True)
(OUT / ('legacy-red.log' if ordinary == 'Close' else 'run.log')).write_text(p.stdout + p.stderr)
print(p.stdout + p.stderr, end='')
sys.exit(p.returncode)
