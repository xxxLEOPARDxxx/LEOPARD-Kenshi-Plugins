"""Compiled controls reject omissions from the actual packet equality gate."""
from pathlib import Path
import os, subprocess, sys
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent
BASE=HERE.parent.parent/'atlas-models/loaded-instances'
OUT=Path('D:/DEV/Kenshi/research/gsm-loaded-reuse-20261104');OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');ENV=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include;{HERE}',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
source=(BASE/'InstanceRaster.h').read_text();start=source.index(' bool MatchesRaw(');end=source.index(' bool SameContents(',start);gate=source[start:end]
controls=[
 ('pose','std::memcmp(a.rawWorld,b.rawWorld,64)||',''),
 ('surface','std::memcmp(&a.surface,&b.surface,sizeof(Surface))||',''),
 ('resource','a.vertex.p!=b.vertex||',''),
 ('sampler','if(std::memcmp(&sampler,&samplers[i],sizeof(sampler)))','if(false)'),
 ('generation','generation_!=generation||',''),
 ('device','||!AtlasView::SameDevice(draw_[0].vertex.p,d)',''),
 ('verified','!b.verifiedBlack||',''),
]
for name,before,after in controls:
 assert gate.count(before)==1,name
 header=OUT/'mutant-raster.h';header.write_text((source[:start]+gate.replace(before,after)+source[end:]).replace('#include "InstancePolicy.h"','#include "'+(BASE/'InstancePolicy.h').as_posix()+'"').replace('#include "../GraphicsState.h"','#include "'+(BASE.parent/'GraphicsState.h').as_posix()+'"'))
 cpp=OUT/'mutant-reuse.cpp';cpp.write_text('#define INSTANCE_RASTER_HEADER "'+header.as_posix()+'"\n'+(HERE/'reuse_tests.cpp').read_text())
 exe=OUT/'mutant-reuse.exe';p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT','/O2',str(cpp),'/Fo'+str(OUT/'mutant-reuse.obj'),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib'],cwd=HERE,env=ENV,capture_output=True,text=True);(OUT/(name+'-mutant-build.log')).write_text(p.stdout+p.stderr)
 assert p.returncode==0,(name,p.stdout,p.stderr)
 p=subprocess.run([str(exe)],capture_output=True,text=True);(OUT/(name+'-mutant-run.log')).write_text(p.stdout+p.stderr)
 assert p.returncode==1 and 'FAIL 'in p.stderr,(name,p.returncode,p.stdout,p.stderr)
 print('REJECTED',name,p.stderr.strip(),flush=True)
print('PASS',len(controls),'compiled raw reuse negative controls')
