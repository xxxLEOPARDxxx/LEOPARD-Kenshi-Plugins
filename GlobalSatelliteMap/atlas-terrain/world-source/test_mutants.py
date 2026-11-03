"""Compile actual supplier bodies; assertion failures only, not compiler errors/crashes."""
from pathlib import Path
import os,subprocess
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
source=(HERE/'WorldHeightSource.h').read_text();body=(HERE/'source_tests.cpp').read_text()
def edit(s,a,b):assert s.count(a)==1,(a,s.count(a));return s.replace(a,b)
mutants=[
 ('hash','std::memcmp(digest,SourceSha,32)','false'),
 ('format','!Profile(header,sizeof(header),U64(length.QuadPart))','false'),
 ('halo','r.x0-2','r.x0-1'),
 ('row-stride','U64(z+row)*WorldSide','U64(z+row)*(WorldSide-1)'),
 ('row-offset','StripOffset+2*(U64(z+row)','StripOffset+1+2*(U64(z+row)'),
 ('epoch','expectedEpoch!=identity_.epoch','false'),
 ('cache-identity','Same(identity_,id)','true'),
 ('partial-clear','if(!ok)out.Clear();','if(false)out.Clear();'),
 ('world-edge','r.x0<2?0:r.x0-2','r.x0<2?1:r.x0-2'),
 ('inclusive-cap','MaxVertexSide=1025','MaxVertexSide=1024'),
 ('cancel-cache','if(AtlasView::Cancelled(o.io_)){out.Clear();return false;}','if(false){out.Clear();return false;}'),
 ('worker-owner','if(owner_!=GetCurrentThreadId())','if(false)'),
 ('growth','if(out.data_.capacity()<count)out.Clear();',''),
 ('hidden-whole-image','unsigned char scratch[AuthScratch];','unsigned char scratch[AuthScratch];std::vector<unsigned char>whole(static_cast<size_t>(FileBytes));'),
]
for name,a,b in mutants:
 h=edit(source,a,b).replace('"../../atlas-view/AtlasReader.h"','"'+(HERE.parents[1]/'atlas-view/AtlasReader.h').as_posix()+'"').replace('"../fixed-geometry/FixedTerrainGeometry.h"','"'+(HERE.parent/'fixed-geometry/FixedTerrainGeometry.h').as_posix()+'"');hp=OUT/(name+'.h');hp.write_text(h)
 cpp=OUT/(name+'.cpp');cpp.write_text(body.replace('"WorldHeightSource.h"','"'+hp.as_posix()+'"'));exe=OUT/(name+'.exe')
 p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT','/W3',str(cpp),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),'/link','advapi32.lib'],env=env,cwd=HERE,capture_output=True,text=True);(OUT/(name+'-build.log')).write_text(p.stdout+p.stderr);assert p.returncode==0,(name,'compile failure not kill',p.stdout,p.stderr)
 p=subprocess.run([str(exe)],cwd=HERE,capture_output=True,text=True);(OUT/(name+'-run.log')).write_text(p.stdout+p.stderr);assert p.returncode==1 and 'FAIL ' in p.stderr,(name,'not an assertion rejection',p.returncode,p.stdout,p.stderr);print('Expected RED',name,p.stderr.strip(),flush=True)
print('PASS',len(mutants),'compiled production world-source mutants rejected')
