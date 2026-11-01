"""Compile the exact production worker with WIC/real Win32 threads and owned test files."""
from pathlib import Path
import subprocess,os,re
here=Path(__file__).resolve().parent
source=(here.parent/'SavePreviews.h').read_text(encoding='utf-8')
begin=source.index('    struct Writer\n')
end=source.index('    static bool enabled=',begin)
writer=source[begin:end]
begin=source.index('    static unsigned __stdcall WriteThread(')
opening=source.index('{',begin);end=opening+1;depth=1
while depth:
    depth+=(source[end]=='{')-(source[end]=='}');end+=1
worker=source[begin:end]
out=here/'_generated';out.mkdir(exist_ok=True)
fixture=(here/'save_preview_writer_fixture.cpp').read_text(encoding='utf-8').replace('// EXTRACT_WRITER',writer+'\n'+worker)
cpp=out/'actual_preview_writer.cpp';cpp.write_text(fixture,encoding='utf-8')
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=os.environ.copy();env['INCLUDE']=f'{vc}/include;{sdk}/Include';env['LIB']=f'{vc}/lib/amd64;{sdk}/Lib/x64';env['PATH']=f'{vc}/bin/amd64;'+env['PATH']
exe=out/'actual_preview_writer.exe'
subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/W3',str(cpp),'/Fe'+str(exe),'/Fo'+str(out/'actual_preview_writer.obj')],env=env,check=True)
subprocess.run([str(exe),str(out/'owned-save-fixture')],env=env,check=True)
