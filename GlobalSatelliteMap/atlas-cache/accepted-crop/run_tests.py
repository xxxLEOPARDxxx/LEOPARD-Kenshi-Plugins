from pathlib import Path
import os, subprocess, sys
sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
OUT = Path('D:/DEV/Kenshi/research/gsm-large-parent-20261104/accepted-crop-tests')
OUT.mkdir(parents=True, exist_ok=True)
VC = Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
SDK = Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
ENV = dict(os.environ, INCLUDE=f'{VC}/include;{SDK}/Include', LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
for opt in ['/Od', '/O2']:
    name = 'crop-' + opt[1:]
    exe = OUT / (name + '.exe')
    build = subprocess.run([str(VC/'bin/amd64/cl.exe'), '/nologo', '/EHsc', '/MT', opt, '/W3', '/I'+str(HERE), str(HERE/'tests.cpp'), '/Fo'+str(OUT/(name+'.obj')), '/Fe'+str(exe), '/link', 'advapi32.lib'], cwd=HERE, env=ENV, capture_output=True, text=True)
    (OUT/(name+'-build.log')).write_text(build.stdout+build.stderr)
    if build.returncode:
        print(build.stdout+build.stderr)
        raise SystemExit(build.returncode)
    run = subprocess.run([str(exe)], cwd=HERE, capture_output=True, text=True)
    (OUT/(name+'-run.log')).write_text(run.stdout+run.stderr)
    print(name, run.stdout+run.stderr, end='', flush=True)
    if run.returncode:
        raise SystemExit(run.returncode)
if '--mutants' in sys.argv:
    subprocess.run([sys.executable, str(HERE/'test_mutants.py')], check=True)
