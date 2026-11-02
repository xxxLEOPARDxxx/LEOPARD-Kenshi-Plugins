"""Execute the actual native detail call, not a duplicate capture policy."""
import os
from pathlib import Path
import re
import subprocess

here = Path(__file__).resolve().parent
source = (here / 'TerrainExperiment.h').read_text()
calls = re.findall(r'api\.detail\(terrain,[^;]+;', source)
assert len(calls) == 1
fixture = r'''
#include <cstdio>
struct Api {float a,b,c;void detail(void*,float x,float y,float z){a=x;b=y;c=z;}} api;
struct Raster {float detail;} diagnosticRaster;
void Configure(bool atlas,float d){void* terrain=0;diagnosticRaster.detail=d;CALL}
int main(){
 unsigned checks=0;
 for(unsigned mode=0;mode<2;++mode)for(unsigned d=1;d<=64;++d){
  Configure(mode!=0,float(d));
  float expected=mode?32.0f:float(d);
  if(api.a!=expected||api.b!=expected||api.c!=0){
   std::printf("FAIL native detail mode=%u requested=%u actual=%g/%g/%g expected=%g\n",mode,d,api.a,api.b,api.c,expected);return 1;}
  ++checks;
 }
 const float distances[]={1,2,128,16000,32000,147456,420000};
 const float scales[]={0,.01f,1,62.5f,125,250,251,9800};
 for(unsigned d=0;d<7;++d)for(unsigned s=0;s<8;++s){
  const float reference=(512.0f/distances[d])*scales[s];
  const float atlasError=(2048.0f/distances[d])*scales[s];
  Configure(true,8);
  if((reference>8)!=(atlasError>api.a)||(reference<8)!=(atlasError<api.a))return 2;
  ++checks;
 }
 std::printf("PASS %u extracted native LOD threshold/equivalence checks\n",checks);
}
'''.replace('CALL', calls[0])
out = here / 'build'
out.mkdir(exist_ok=True)
path = out / 'atlas_lod.cpp'
path.write_text(fixture)
vc = Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
sdk = Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env = dict(os.environ, INCLUDE=f'{vc}/include;{sdk}/Include', LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
for cmd in ([str(vc/'bin/amd64/cl.exe'), '/nologo', '/EHsc', '/MD', '/O2', '/Fe'+str(out/'atlas_lod.exe'), '/Fo'+str(out/'atlas_lod.obj'), str(path)], [str(out/'atlas_lod.exe')]):
    result = subprocess.run(cmd, env=env, capture_output=True, text=True, creationflags=subprocess.CREATE_NO_WINDOW)
    print(result.stdout+result.stderr, end='')
    if result.returncode:
        raise SystemExit(result.returncode)
