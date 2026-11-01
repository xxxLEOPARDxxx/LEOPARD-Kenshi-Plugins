from pathlib import Path
import subprocess,os,re
here=Path(__file__).resolve().parent;source=(here.parent/'SavePreviews.h').read_text(encoding='utf-8')
def function(name):
    m=re.search(r'^    static (?:void|bool) '+name+r'\(',source,re.M);assert m,name
    a=m.start();end=source.index('{',a)+1;depth=1
    while depth:depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[a:end]
a=source.index('    struct SettingAlpha\n');b=source.index('    static bool OnMain()',a)
fixture=(here/'save_preview_capture_fixture.cpp').read_text(encoding='utf-8').replace('// EXTRACT_ALPHA',source[a:b]).replace('// EXTRACT_REQUIRE',function('RequireCapture'))
fixture=fixture.replace('// EXTRACT_CAPTURE','\n'.join(function(n)for n in ['AddModal','FindPauseRoot','DrawGui']))
out=here/'_generated';out.mkdir(exist_ok=True);cpp=out/'actual_preview_capture.cpp';cpp.write_text(fixture,encoding='utf-8')
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=os.environ.copy();env['INCLUDE']=f'{vc}/include;{sdk}/Include';env['LIB']=f'{vc}/lib/amd64;{sdk}/Lib/x64';env['PATH']=f'{vc}/bin/amd64;'+env['PATH']
exe=out/'actual_preview_capture.exe'
subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/W3',str(cpp),'/Fe'+str(exe),'/Fo'+str(out/'actual_preview_capture.obj')],env=env,check=True)
subprocess.run([str(exe)],env=env,check=True)
