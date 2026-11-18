"""Compile production host callbacks against narrow game-boundary fakes."""
from pathlib import Path
import os, subprocess
here=Path(__file__).resolve().parent
root=here.parent
out=here/'_generated';out.mkdir(exist_ok=True)
host=root/'EngineerHostRuntime.h'
missing='''bool EngineerCurrent(unsigned*){return false;}
bool EngineerAllowed(Character*){return false;}
bool EngineerAllowActor(AI*){return false;}
bool EngineerSnapshot(Building*,AIFixedEngineerPriority::Candidate*){return false;}
Building* EngineerResolve(const AIFixedEngineerPriority::Key&){return 0;}'''
production=host.read_text(encoding='utf8') if host.exists() else missing
source=(root/'AIFixed.cpp').read_text(encoding='utf-8-sig')
start=source.index('    void EngineerMaintenance()')
opening=source.index('{',start);end=opening+1;depth=1
while depth:
    depth+=(source[end]=='{')-(source[end]=='}');end+=1
production+='\n'+source[start:end]
cpp=out/'engineer_host.cpp'
cpp.write_text((here/'engineer_host_fixture.cpp').read_text(encoding='utf8').replace('// @PRODUCTION@',production),encoding='utf8')
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include;{root}',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
for opt in ('Od','O2'):
    binary=out/f'engineer_host_{opt}.exe'
    subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT','/'+opt,str(cpp),'/Fo'+str(out)+'/', '/Fe'+str(binary)],env=env,cwd=out,check=True)
    subprocess.run([str(binary)],check=True)
