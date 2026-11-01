from pathlib import Path
import os, subprocess,re
here=Path(__file__).resolve().parent
source=(here.parent/'AIFixed.cpp').read_text(encoding='utf-8-sig')
production=source[source.index('    struct HandIdentityLess'):source.index('    bool IsRobot')]
def function(name,optional=False):
    match=re.search(r'^    (?:void|bool) '+re.escape(name)+r'\(',source,re.M)
    if not match:
        if optional:return ''
        raise AssertionError(name)
    opening=source.index('{',match.start());end=opening+1;depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[match.start():end]
production+='\n'+'\n'.join(function(name,True) for name in ('ReadGateClock','ResetSessionState','GateUpdate','GuiUpdate_hook'))
fixture=(here/'gate_runtime_fixture.cpp').read_text(encoding='utf-8').replace('// @PRODUCTION@',production)
out=here/'_generated';out.mkdir(exist_ok=True)
cpp=out/'gate_runtime.cpp';cpp.write_text(fixture,encoding='utf-8')
vc=Path(r'C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC');sdk=Path(r'C:\Program Files\Microsoft SDKs\Windows\v7.1')
env=os.environ.copy();env['PATH']=str(vc/'bin/amd64')+';'+env['PATH']
env['INCLUDE']=str(vc/'include')+';'+str(sdk/'Include')+';'+str(here.parent)
env['LIB']=str(vc/'lib/amd64')+';'+str(sdk/'Lib/x64')
exe=out/'gate_runtime.exe'
subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/Od',str(cpp),'/Fe'+str(exe),'/Fo'+str(out)+'\\'],env=env,cwd=out,check=True)
subprocess.run([str(exe)],check=True)
