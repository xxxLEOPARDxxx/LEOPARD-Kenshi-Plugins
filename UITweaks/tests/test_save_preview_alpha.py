"""Compile the exact hook and production alpha transaction without game/UI calls."""
from pathlib import Path
import subprocess,os
here=Path(__file__).resolve().parent;source=(here.parent/'SavePreviews.h').read_text(encoding='utf-8')
start=source.index('    struct SettingAlpha\n');end=source.index('    static bool OnMain()',start)
out=here/'_generated';out.mkdir(exist_ok=True)
cpp=out/'actual_preview_alpha.cpp';cpp.write_text((here/'save_preview_alpha_fixture.cpp').read_text(encoding='utf-8').replace('// EXTRACT_ALPHA_HOOK',source[start:end]),encoding='utf-8')
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=os.environ.copy();env['INCLUDE']=f'{vc}/include;{sdk}/Include';env['LIB']=f'{vc}/lib/amd64;{sdk}/Lib/x64';env['PATH']=f'{vc}/bin/amd64;'+env['PATH']
exe=out/'actual_preview_alpha.exe'
subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/W3',str(cpp),'/Fe'+str(exe),'/Fo'+str(out/'actual_preview_alpha.obj')],env=env,check=True)
subprocess.run([str(exe)],env=env,check=True)
