"""Exercise actual FindLanding branches with explicit native boundary results."""
from pathlib import Path
import os
import subprocess

here = Path(__file__).resolve().parent
root = here.parent
out = here / '_generated'
out.mkdir(exist_ok=True)
runtime = (root / 'UnstuckRuntime.h').read_text(encoding='utf-8')
fixture = (here / 'landing_runtime_fixture.cpp').read_text(encoding='utf-8')
start = runtime.index('    struct LandingTrace')
fixture = fixture.replace('// @TRACE_STRUCT@', runtime[start:runtime.index('    // The movement hook', start)])
start = runtime.index('    bool RejectLanding(')
fixture = fixture.replace('// @PRODUCTION@', runtime[start:runtime.index('    void TracePlatform(', start)])
source = out / 'landing_runtime.cpp'
source.write_text(fixture, encoding='utf-8')
vc = Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
sdk = Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env = os.environ.copy()
env['PATH'] = str(vc / 'bin/amd64') + ';' + env['PATH']
env['INCLUDE'] = str(vc / 'include') + ';' + str(sdk / 'Include')
env['LIB'] = str(vc / 'lib/amd64') + ';' + str(sdk / 'Lib/x64')
exe = out / 'landing_runtime.exe'
subprocess.run([str(vc / 'bin/amd64/cl.exe'), '/nologo', '/EHsc', '/MD', '/Od',
                '/I' + str(root), str(source), '/Fe' + str(exe),
                '/Fo' + str(out / 'landing_runtime.obj')], cwd=out, env=env, check=True)
subprocess.run([str(exe)], check=True)
