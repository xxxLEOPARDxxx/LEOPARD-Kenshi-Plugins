"""Small compiled rejection controls against the real Tile implementation."""
from pathlib import Path
import os, subprocess, sys
sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
OUT = Path('D:/DEV/Kenshi/research/gsm-large-parent-20261104/accepted-crop-tests')
OUT.mkdir(parents=True, exist_ok=True)
VC = Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
SDK = Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
ENV = dict(os.environ, INCLUDE=f'{VC}/include;{SDK}/Include', LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
header = (HERE/'AcceptedCrop.h').read_text()
tests = (HERE/'tests.cpp').read_text()
mutants = [
    ('child-centre', 'child.x+=float(int(pixelX)+Side/2-int(v.side)/2)*parent.spacing;', 'child.x=parent.x;'),
    ('height-plane', 'v.height+src,Side*4', 'v.normal+src*4,Side*4'),
    ('coverage-alpha', 'normal[x*4+3]=255;', 'normal[x*4+3]=original[x*4+3];'),
    ('budget', 'if(!Budget(p,w,ledger))', 'if(false)'),
    ('aligned', '(pixelX%Side)||(pixelZ%Side)||', ''),
    ('scope64', '!SourceReceipts::SameScope(scope_,now)||', ''),
    ('main-copy', 'if(GetCurrentThreadId()==now.thread)', 'if(false)'),
]
for name, before, after in mutants:
    assert header.count(before) == 1, name
    mutant = OUT/(name+'.h')
    mutant.write_text(header.replace(before,after).replace('#include "../accepted-window/AcceptedWindow.h"', '#include "'+(HERE.parent/'accepted-window/AcceptedWindow.h').as_posix()+'"'))
    cpp = OUT/(name+'.cpp')
    cpp.write_text(tests.replace('#include "AcceptedCrop.h"', '#include "'+mutant.as_posix()+'"'))
    exe = OUT/(name+'.exe')
    build = subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT','/O2','/W3',str(cpp),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),'/link','advapi32.lib'],cwd=HERE,env=ENV,capture_output=True,text=True)
    (OUT/(name+'-build.log')).write_text(build.stdout+build.stderr)
    assert build.returncode == 0, (name, build.stdout, build.stderr)
    run = subprocess.run([str(exe)],cwd=HERE,capture_output=True,text=True)
    (OUT/(name+'-run.log')).write_text(run.stdout+run.stderr)
    assert run.returncode == 1 and 'FAIL ' in run.stderr, (name,run.returncode,run.stdout,run.stderr)
    print('REJECTED',name,run.stderr.strip(),flush=True)
print('PASS',len(mutants),'compiled AcceptedCrop negative controls')
