"""Compile the actual height adapter against bounded native-widget seams."""
from pathlib import Path
import subprocess, os, re
here=Path(__file__).resolve().parent
source=(here.parent/'SavePreviews.h').read_text(encoding='utf-8')
match=re.search(r'^    static void SizeMenu\(',source,re.M)
opening=source.index('{',match.start());end=opening+1;depth=1
while depth:
    depth+=(source[end]=='{')-(source[end]=='}');end+=1
out=here/'_generated';out.mkdir(exist_ok=True)
cpp=out/'actual_preview_height.cpp'
cpp.write_text((here/'save_preview_height_fixture.cpp').read_text(encoding='utf-8').replace('// EXTRACT_HEIGHT',source[match.start():end]),encoding='utf-8')
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=os.environ.copy();env['INCLUDE']=f'{vc}/include;{sdk}/Include';env['LIB']=f'{vc}/lib/amd64;{sdk}/Lib/x64';env['PATH']=f'{vc}/bin/amd64;'+env['PATH']
exe=out/'actual_preview_height.exe'
subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2',str(cpp),'/Fe'+str(exe),'/Fo'+str(out/'actual_preview_height.obj')],env=env,check=True)
subprocess.run([str(exe)],env=env,check=True)
