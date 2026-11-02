"""Execute the real request admission expression, including manual reentry."""
import os
from pathlib import Path
import subprocess

here = Path(__file__).resolve().parent
source = (here / 'CaptureProbe.cpp').read_text()
gate = next(line.strip() for line in source.splitlines()
            if line.strip().startswith('if (!press || pending || terrainExperiment.root'))
assert source[source.index(gate):].splitlines()[1].strip() == 'return;'
fixture = r'''
#include <cstdio>
#include <cstdlib>
struct State {void *root,*thread,*pixels;bool quarantined;} terrainExperiment,atlasRun;
struct Map {bool visible;bool getVisible(){return visible;}} actualMap;
Map *map=&actualMap;bool press=true,pending=false,chainSafe=true;
bool CanBegin(){
GATE
return false;return true;}
int main(){unsigned checks=0;
for(unsigned mask=0;mask<128;++mask){
 press=!(mask&1);pending=(mask&2)!=0;
 terrainExperiment.root=(mask&4)?(void*)1:0;
 atlasRun.thread=(mask&8)?(void*)1:0;atlasRun.pixels=(mask&16)?(void*)1:0;
 atlasRun.quarantined=(mask&32)!=0;actualMap.visible=!(mask&64);
 bool expected=mask==0;
 if(CanBegin()!=expected){std::printf("FAIL manual/batch admission mask=%u\n",mask);return 1;}++checks;
}
 press=true;pending=false;terrainExperiment.root=0;atlasRun.thread=0;atlasRun.pixels=0;atlasRun.quarantined=false;actualMap.visible=true;
 chainSafe=false;if(CanBegin())return 2;++checks;chainSafe=true;map=0;if(CanBegin())return 3;++checks;
 std::printf("PASS %u extracted atlas/manual request admission checks\n",checks);
}
'''.replace('GATE', gate)
out = here / 'build'
path = out / 'atlas_route.cpp'
path.write_text(fixture)
vc = Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
sdk = Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env = dict(os.environ, INCLUDE=f'{vc}/include;{sdk}/Include', LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
for cmd in ([str(vc/'bin/amd64/cl.exe'), '/nologo', '/EHsc', '/MD', '/O2', '/Fe'+str(out/'atlas_route.exe'), '/Fo'+str(out/'atlas_route.obj'), str(path)], [str(out/'atlas_route.exe')]):
    result = subprocess.run(cmd, env=env, capture_output=True, text=True, creationflags=subprocess.CREATE_NO_WINDOW)
    print(result.stdout+result.stderr, end='')
    assert result.returncode == 0
