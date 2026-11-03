from pathlib import Path
import os,subprocess,re
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
source=(HERE/'WorldSourceBank.h').read_text();test=(HERE/'warp_tests.cpp').read_text()
def edit(s,a,b):assert s.count(a)==1,(a,s.count(a));return s.replace(a,b)
def in_method(name,s):
 at=s.index(' HRESULT '+name+'(');return s[:at]+s[at:].replace('if(!Fits(l))return E_OUTOFMEMORY;','',1)
def absolute(s):
 def cv(m):
  name=m[2]
  for a,p in [('../world-material/',HERE.parent/'world-material'),('../../atlas-view/',HERE.parents[1]/'atlas-view'),('../../../tools/',HERE.parents[2]/'tools'),('_generated/',OUT)]:
   if name.startswith(a):name=p.as_posix()+'/'+name[len(a):];break
  return m[1]+name+m[3]
 return re.sub(r'(^\s*#include\s+")([^"]+)(")',cv,s,flags=re.M)
def run(name,h):
 hp=OUT/(name+'.h');hp.write_text(absolute(h));cpp=OUT/(name+'.cpp');cpp.write_text(absolute(test.replace('"WorldSourceBank.h"','"'+hp.as_posix()+'"')));exe=OUT/(name+'.exe')
 p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT','/W3',str(cpp),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib','advapi32.lib'],cwd=HERE,env=env,capture_output=True,text=True);(OUT/(name+'-build.log')).write_text(p.stdout+p.stderr);assert not p.returncode,(name,'compile failure not kill',p.stdout,p.stderr)
 p=subprocess.run([str(exe)],cwd=HERE,capture_output=True,text=True);(OUT/(name+'-run.log')).write_text(p.stdout+p.stderr);assert p.returncode==1 and 'FAIL 'in p.stderr,(name,p.returncode,p.stdout,p.stderr);print('Expected RED',name,p.stderr.strip(),flush=True)
mutants=[('old90-total','if(sum>AtlasView::GpuCap-SourceBytes)','if(sum>BankLimit-SourceBytes)'),('retired','l.pending,l.retired,l.other','l.pending,0,l.other'),('profile','e.profile==PinnedBaseSnapshot','true'),('epoch','a.value==b.value','DWORD(a.value)==DWORD(b.value)'),('dds-hash','!AssetHash(data,bytes,*a)','false'),('rgba-hash','!Hash(data,bytes,p->bytes,p->sha)','false'),('dds-layer','D3D11CalcSubresource(mip,layer,9)','D3D11CalcSubresource(mip,(layer+1)%6,9)'),('mips','c.p->GenerateMips(view_[index].p);',''),('early-ready','ddsMask_!=4095||rgbaMask_!=7','false'),('ready-reset','ready_=false;++revision_;','++revision_;'),('predicate','c_->SetPredication(NULL,FALSE);',''),('aliases','if(AtlasView::Bound(c,tex_[i].p,true))return false;','if(false)return false;'),('role-offset','const unsigned offset=k?s.pixelNormalOffset:s.pixelDiffuseOffset;','const unsigned offset=0;'),('paint-reset','if(changed)rgbaMask_&=4;',''),('recipe','!EqualPlan(p,expected)','false'),('allocation-token','planner_.Clear();std::memset','planner_.Clear();allocationSerial_=0;std::memset')]
for name,a,b in mutants:run('mut-'+name,edit(source,a,b))
for method in ['BeginStage','UploadDds','UploadRgba','Complete','AcquireBase','PatchRoles']:run('mut-budget-'+method,in_method(method,source))
print('PASS22 compiled functional source-bank mutants rejected')
