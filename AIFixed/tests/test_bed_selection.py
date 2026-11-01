from pathlib import Path
import os,subprocess
here=Path(__file__).resolve().parent
source=(here.parent/'AIFixed.cpp').read_text(encoding='utf-8-sig')
identity=source[source.index('    struct HandIdentityLess'):source.index('    struct IntentGuard')]
begin=source.index('    struct RestBedContext')
end=source.index('\n\n\n    // Журнал',begin)
fixture=(here/'bed_selection_fixture.cpp').read_text(encoding='utf-8').replace('// @PRODUCTION@',identity+source[begin:end])
out=here/'_generated';out.mkdir(exist_ok=True);cpp=out/'bed_selection.cpp';cpp.write_text(fixture,encoding='utf-8')
vc=Path(r'C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC');sdk=Path(r'C:\Program Files\Microsoft SDKs\Windows\v7.1')
env=os.environ.copy();env['PATH']=str(vc/'bin/amd64')+';'+env['PATH'];env['INCLUDE']=str(vc/'include')+';'+str(sdk/'Include')+';'+str(here.parent);env['LIB']=str(vc/'lib/amd64')+';'+str(sdk/'Lib/x64')
exe=out/'bed_selection.exe'
subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/Od',str(cpp),'/Fe'+str(exe),'/Fo'+str(out)+'\\'],env=env,cwd=out,check=True)
subprocess.run([str(exe)],check=True)
