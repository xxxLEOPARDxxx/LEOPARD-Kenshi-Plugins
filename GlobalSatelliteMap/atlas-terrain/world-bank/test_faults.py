"""Real GPU allocations/uploads with controlled failures in isolated header copies."""
from pathlib import Path
import os,subprocess,re
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
source=(HERE/'WorldSourceBank.h').read_text();test=(HERE/'warp_tests.cpp').read_text();prefix=test[:test.index('int main()')]
def absolute(s):
 def convert(m):
  name=m[2]
  for a,p in [('../world-material/',HERE.parent/'world-material'),('../../atlas-view/',HERE.parents[1]/'atlas-view'),('../../../tools/',HERE.parents[2]/'tools'),('_generated/',OUT)]:
   if name.startswith(a):name=p.as_posix()+'/'+name[len(a):];break
  return m[1]+name+m[3]
 return re.sub(r'(^\s*#include\s+")([^"]+)(")',convert,s,flags=re.M)
def buildrun(name,h,body,green=True):
 hp=OUT/(name+'.h');hp.write_text(absolute(h));cpp=OUT/(name+'.cpp');cpp.write_text(absolute(body.replace('"WorldSourceBank.h"','"'+hp.as_posix()+'"')));exe=OUT/(name+'.exe')
 p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT','/W3',str(cpp),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib','advapi32.lib'],cwd=HERE,env=env,capture_output=True,text=True);(OUT/(name+'-build.log')).write_text(p.stdout+p.stderr);assert not p.returncode,(name,'compile failure not kill',p.stdout,p.stderr)
 p=subprocess.run([str(exe)],cwd=HERE,capture_output=True,text=True);(OUT/(name+'-run.log')).write_text(p.stdout+p.stderr);assert (not p.returncode if green else p.returncode==1 and 'FAIL 'in p.stderr),(name,p.returncode,p.stdout,p.stderr);print(('PASS 'if green else 'Expected RED ')+name+': '+(p.stdout+p.stderr).strip(),flush=True)
h=source
matches=list(re.finditer(r'device_\.p->Create\w+\(',h))
for m in reversed(matches):
 end=m.end();depth=1
 while depth:
  if h[end]=='(':depth+=1
  elif h[end]==')':depth-=1
  end+=1
 h=h[:m.start()]+'(FailNext()?E_OUTOFMEMORY:'+h[m.start():end]+')'+h[end:]
h=h.replace('device_.p->GetDeviceRemovedReason()','(FailNext()?E_FAIL:device_.p->GetDeviceRemovedReason())').replace('namespace WorldBank {','namespace WorldBank {static unsigned failAt=~0u,failCount=0;static bool FailNext(){return failCount++==failAt;}')
body=prefix+r'''
int main(){try{Com<ID3D11Device>d;Com<ID3D11DeviceContext>c;HR(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&d.p,NULL,&c.p),"WARP faults");SourceEpoch e={7,PinnedBaseSnapshot};Ledger l={};failAt=~0u;failCount=0;{Sources s;HR(s.Init(d.p,e,l),"baseline Init");}unsigned total=failCount;C(total==15,"14 exact resource creates plus Init health");for(unsigned n=0;n<total;++n){failAt=n;failCount=0;Sources s;ULONG before=Refs(d.p);AtlasViewTest::ContextFixture state(c.p);C(FAILED(s.Init(d.p,e,l))&&!s.OwnedGpuBytes()&&!s.Ready()&&!s.ResourceToken(),"partial Init failure releases all source resources");AtlasViewTest::ContextFixture after(c.p);C(state.Equal(after)&&before==Refs(d.p),"partial Init state and COM references exact");failAt=~0u;HR(s.Init(d.p,e,l),"actual failed Init retry");s.Clear();}std::puts("PASS15 actual allocation/Init-health failure cleanup+retry paths");return 0;}catch(...){return 1;}}
'''
buildrun('fault-init',h,body)
buildrun('mut-partial-cleanup',h.replace('view_[i].Reset();tex_[i].Reset();','tex_[i].Reset();'),body,False)
health=source.replace('namespace WorldBank {','namespace WorldBank {static bool failHealth=false;').replace('HRESULT hr=device_.p->GetDeviceRemovedReason();','HRESULT hr=failHealth?E_FAIL:device_.p->GetDeviceRemovedReason();')
body=prefix+r'''
static void Fill(Sources&s,const SourceEpoch&e,const Ledger&l){for(unsigned kind=0;kind<3;++kind){char name[80];std::sprintf(name,"_generated/rgba-%u.bin",kind==2?128:kind*64+27);std::vector<BYTE>b=Read(name);HR(s.UploadRgba(e,0,kind,&b[0],b.size(),l),"health fixture RGBA");}for(unsigned kind=0;kind<2;++kind)for(unsigned layer=0;layer<6;++layer){const SatelliteRegion::Asset*a=s.ExpectedDds(e,0,kind,layer);std::vector<BYTE>b=Read(std::string("D:/SteamLibrary/steamapps/common/Kenshi/")+a->path);HR(s.UploadDds(e,0,kind,layer,&b[0],b.size(),l),"health fixture DDS");}}
int main(){try{Com<ID3D11Device>d;Com<ID3D11DeviceContext>c;HR(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&d.p,NULL,&c.p),"health WARP");SourceEpoch e={7,PinnedBaseSnapshot};Ledger l={};WorldMaterial::Planner planner;C(planner.Init(7),"planner");WorldMaterial::Plan p;C(planner.Build(27,10,7,0,p),"real plan");for(unsigned phase=0;phase<5;++phase){Sources s;failHealth=false;ULONG before=Refs(d.p);HR(s.Init(d.p,e,l),"clean Init");if(phase)HR(s.BeginStage(p,e,0,l),"clean Begin");if(phase>=3)Fill(s,e,l);if(phase==4)HR(s.Complete(e,0,l),"clean Complete");AtlasViewTest::ContextFixture state(c.p);failHealth=true;HRESULT hr=S_OK;Bindings b={};if(!phase)hr=s.BeginStage(p,e,0,l);else if(phase==1){const SatelliteRegion::Asset*a=s.ExpectedDds(e,0,0,0);std::vector<BYTE>v=Read(std::string("D:/SteamLibrary/steamapps/common/Kenshi/")+a->path);hr=s.UploadDds(e,0,0,0,&v[0],v.size(),l);}else if(phase==2){std::vector<BYTE>v=Read("_generated/rgba-27.bin");hr=s.UploadRgba(e,0,0,&v[0],v.size(),l);}else if(phase==3)hr=s.Complete(e,0,l);else hr=s.AcquireBase(e,0,l,b);failHealth=false;C(FAILED(hr)&&s.Broken()&&!s.Ready()&&FAILED(s.Complete(e,0,l))&&FAILED(s.AcquireBase(e,0,l,b)),"post-command/device health failure latches until Clear");AtlasViewTest::ContextFixture after(c.p);C(state.Equal(after),"failure restores foreign pipeline/predicate");s.Clear();C(before==Refs(d.p)&&!s.OwnedGpuBytes(),"health failure explicit Clear releases all GPU refs");HR(s.Init(d.p,e,l),"health failure can reInit after explicit Clear");s.Clear();}std::puts("PASS5 stage/DDS/RGBA/complete/acquire health-latch paths");return 0;}catch(...){return 1;}}
'''
buildrun('fault-health',health,body)
buildrun('mut-health-latch',health.replace('broken_=true;ready_=false;','broken_=false;ready_=false;'),body,False)
body=prefix+r'''
int main(){try{Com<ID3D11Device>d;Com<ID3D11DeviceContext>c;HR(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&d.p,NULL,&c.p),"counter WARP");SourceEpoch e={7,PinnedBaseSnapshot};Ledger l={};Sources s;ULONG before=Refs(d.p);AtlasViewTest::ContextFixture state(c.p);C(s.Init(d.p,e,l)==E_ABORT&&!s.OwnedGpuBytes()&&!s.ResourceToken(),"allocation generation exhaustion refuses before owner creation");AtlasViewTest::ContextFixture after(c.p);C(state.Equal(after)&&before==Refs(d.p),"exhaustion cannot mutate pipeline or refs");std::puts("PASS allocation counter exhaustion failclosed");return 0;}catch(...){return 1;}}
'''
buildrun('fault-allocation-exhaustion',source.replace('allocationSerial_(0)','allocationSerial_(~U64(0))'),body)
body=prefix+r'''
int main(){try{Com<ID3D11Device>d;Com<ID3D11DeviceContext>c;HR(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&d.p,NULL,&c.p),"counter WARP");SourceEpoch e={7,PinnedBaseSnapshot};Ledger l={};Sources s;HR(s.Init(d.p,e,l),"clean Init");WorldMaterial::Planner planner;C(planner.Init(7),"planner");WorldMaterial::Plan p;C(planner.Build(27,10,7,0,p),"plan");AtlasViewTest::ContextFixture state(c.p);C(s.BeginStage(p,e,0,l)==E_ABORT&&!s.Ready(),"stage revision exhaustion refuses before selector mutation");AtlasViewTest::ContextFixture after(c.p);C(state.Equal(after),"revision exhaustion preserves pipeline");std::puts("PASS stage revision exhaustion failclosed");return 0;}catch(...){return 1;}}
'''
buildrun('fault-revision-exhaustion',source.replace('revision_(0)','revision_(~U64(0))'),body)
print('PASS22 source allocation/health/counter fault paths +2 compiled cleanup/latch mutants')
