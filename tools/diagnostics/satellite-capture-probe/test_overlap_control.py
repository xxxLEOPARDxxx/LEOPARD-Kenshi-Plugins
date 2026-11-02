"""Compile the production bounded seam-control selector, including refusal paths."""
from pathlib import Path
import os
import subprocess

here = Path(__file__).resolve().parent
build = here / 'build'
build.mkdir(exist_ok=True)
fixture = r'''
#include "AtlasCapturePolicy.h"
#include <cstdio>
int main() {
 AtlasCapture::Block b = {0}; unsigned checks=0;
 if(!AtlasCapture::OverlapControl(b))return 1;
 if(b.tileX!=1||b.tileZ!=1||b.x!=5120||b.z!=6144||b.regionX!=3||b.regionZ!=3||b.span!=18432||b.centerX!=-18432||b.centerZ!=-9216)return 2;
 for(unsigned z=0;z<2048;++z)for(unsigned x=0;x<2048;++x){
  const double actualX=b.centerX-b.span*.5+(x+.5)*9;
  const double actualZ=b.centerZ-b.span*.5+(z+.5)*9;
  if(actualX!=-73728+(5120+x+.5)*9||actualZ!=-73728+(6144+z+.5)*9)return 3;
  ++checks;
 }
 for(unsigned i=0;i<256;++i){AtlasCapture::Block normal;if(!AtlasCapture::Select(i,normal))return 4;++checks;}
 std::printf("PASS %u overlap/global sample alignment and unchanged normal selectors\n",checks);
}
'''
path = build / 'overlap_control.cpp'
path.write_text(fixture)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include;{here}',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
for cmd in ([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/Fe'+str(build/'overlap_control.exe'),'/Fo'+str(build/'overlap_control.obj'),str(path)],[str(build/'overlap_control.exe')]):
 r=subprocess.run(cmd,env=env,capture_output=True,text=True,creationflags=subprocess.CREATE_NO_WINDOW)
 print(r.stdout+r.stderr,end='')
 if r.returncode: raise SystemExit(r.returncode)
