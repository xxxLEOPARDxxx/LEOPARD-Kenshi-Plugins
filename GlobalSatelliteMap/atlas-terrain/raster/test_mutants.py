"""Compiled real-body faults. A compiler error is never counted as rejection."""
from pathlib import Path
import os,subprocess
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
source=(HERE/'FixedTerrainRaster.h').read_text();body=(HERE/'warp_tests.cpp').read_text()
def edit(s,a,b):assert s.count(a)==1,(a,s.count(a));return s.replace(a,b)
def run(name,header):
 for a,p in [('../fixed-geometry/',HERE.parent/'fixed-geometry'),('../compact-material/',HERE.parent/'compact-material'),('../../atlas-models/',HERE.parents[1]/'atlas-models')]:header=header.replace('"'+a,'"'+p.as_posix()+'/')
 hp=OUT/(name+'.h');hp.write_text(header)
 test=body.replace('"FixedTerrainRaster.h"','"'+hp.as_posix()+'"').replace('"../compact-material/','"'+(HERE.parent/'compact-material').as_posix()+'/').replace('"../../atlas-view/','"'+(HERE.parents[1]/'atlas-view').as_posix()+'/')
 cpp=OUT/(name+'.cpp');cpp.write_text(test);exe=OUT/(name+'.exe')
 p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT','/W3',str(cpp),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib','advapi32.lib'],env=env,cwd=HERE,capture_output=True,text=True);(OUT/(name+'-build.log')).write_text(p.stdout+p.stderr);assert p.returncode==0,(name,'compile failure not kill',p.stdout,p.stderr)
 p=subprocess.run([str(exe)],cwd=HERE,capture_output=True,text=True);(OUT/(name+'-run.log')).write_text(p.stdout+p.stderr);assert p.returncode==1 and 'FAIL ' in p.stderr,(name,'mutant survived or non-assertion failure',p.returncode,p.stdout,p.stderr)
 print('Expected RED:',name,p.stderr.strip(),flush=True)
mutations=[
 ('init-budget','if(!Fits(other,g.columns*g.rows))return E_OUTOFMEMORY;',''),
 ('render-budget','if(!Fits(other,grid_.columns*grid_.rows))return E_OUTOFMEMORY;',''),
 ('shader-pin','return Code(s.vertex,4440,v)&&Code(s.pixel,37520,p);','return true;'),
 ('charge','1048576+VertexBytes+PixelBytes','0+VertexBytes+PixelBytes'),
 ('epoch','f.epoch!=grid_.epoch','false'),
 ('domain','double(f.x)+half>x1','false'),
 ('radial-far','return double(f.farClip)*f.farClip>double(f.cameraY)*f.cameraY+2*half*half;','return true;'),
 ('off','if(!enabled)return S_FALSE;',''),
 ('so','!AtlasModels::NoStreamOutput(c.p)','false'),
 ('output-alias','if(AtlasView::Bound(c.p,tex_[i].p,true))return E_INVALIDARG;','if(false)return E_INVALIDARG;'),
 ('source-alias','AtlasView::Identity(r.p,depth_.p)||AtlasView::Bound(c.p,r.p,true)','false'),
 ('predicate','c.p->SetPredication(NULL,FALSE);',''),
 ('pixel-reads','c_->PSSetShaderResources(1,7,views_);',''),
 ('sampler','c_->PSSetSamplers(4,1,&samplers_[1]);',''),
 ('topology','D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP','D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST'),
 ('stride','const UINT stride=40,offset=0','const UINT stride=36,offset=0'),
 ('morph','Put(vc,0,matrix,16);','vc[131]=63;Put(vc,0,matrix,16);'),
 ('normal-layout','DXGI_FORMAT_R32G32B32_FLOAT,0,16,','DXGI_FORMAT_R32G32B32_FLOAT,0,12,'),
]
for name,a,b in mutations:run(name,edit(source,a,b))
print('PASS',len(mutations),'compiled runtime raster mutants rejected')
