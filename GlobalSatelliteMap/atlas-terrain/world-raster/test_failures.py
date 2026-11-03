"""Real Init cutpoints and one small phase-failure fixture; no full image reruns."""
from pathlib import Path
import os,re,subprocess
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated'
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');ENV=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
source=(HERE/'WorldTerrainRaster.h').read_text()
def run(name,header,body,red=False):
 hp=OUT/(name+'.h');hp.write_text(header);cpp=OUT/(name+'.cpp');cpp.write_text(body.replace('"WorldTerrainRaster.h"','"'+hp.as_posix()+'"'));exe=OUT/(name+'.exe')
 p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT','/W3','/I'+str(HERE),str(cpp),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib','advapi32.lib'],cwd=HERE,env=ENV,capture_output=True,text=True);(OUT/(name+'-build.log')).write_text(p.stdout+p.stderr);assert p.returncode==0,(name,'compile errors not rejection',p.stdout,p.stderr)
 p=subprocess.run([str(exe)],cwd=HERE,capture_output=True,text=True);(OUT/(name+'-run.log')).write_text(p.stdout+p.stderr)
 if red:assert p.returncode==1 and 'FAIL 'in p.stderr,(name,p.returncode,p.stdout,p.stderr);print('Expected RED',name,p.stderr.strip(),flush=True)
 else:assert p.returncode==0,(name,p.returncode,p.stdout,p.stderr);print(p.stdout.strip(),flush=True)
fault=source
for m in reversed(list(re.finditer(r'd->(?:Create\w+|GetDeviceRemovedReason)\(',fault))):
 end=m.end();depth=1
 while depth:
  if fault[end]=='(':depth+=1
  if fault[end]==')':depth-=1
  end+=1
 fault=fault[:m.start()]+'(OwnerFault::Next()?E_OUTOFMEMORY:'+fault[m.start():end]+')'+fault[end:]
fault='namespace OwnerFault{static unsigned at=~0u,count=0;static bool throwing=false;static bool Next(){if(count++!=at)return false;if(throwing)throw 7;return true;}}\n'+fault
body=(HERE/'warp_tests.cpp').read_text().split('int main(){try{')[0]
init=body+r'''
int main(){try{Com<ID3D11Device>d;Com<ID3D11DeviceContext>c;HR(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&d.p,NULL,&c.p),"fault WARP");std::vector<BYTE>h=Read("_generated/height-two.u16");FixedTerrain::Samples s={7294,7294,261,261,261,reinterpret_cast<const unsigned short*>(&h[0]),261*261};Grid g={7360,7360,2,2,7};Ledger l=Money(256,4);OwnerFault::count=0;{Raster x;HR(x.Init(d.p,256,ShaderSet(),s,g,l),"healthy cutpoint counter");}unsigned total=OwnerFault::count;C(total==42,"all real Init cutpoints counted");
 for(unsigned mode=0;mode<2;++mode)for(unsigned i=0;i<total;++i){OwnerFault::at=i;OwnerFault::count=0;OwnerFault::throwing=mode!=0;ULONG refs=Refs(d.p);{Raster x;{AtlasViewTest::ContextFixture before(c.p);C(FAILED(x.Init(d.p,256,ShaderSet(),s,g,l))&&!x.OwnedGpuBytes()&&!x.Texture(0)&&!x.CarryTexture(0,0),"every Init HRESULT/exception clears all partial resources");AtlasViewTest::ContextFixture after(c.p);C(before.Equal(after),"Init failure preserves exact foreign context");}C(Refs(d.p)==refs,"Init failure device refs balanced");OwnerFault::at=~0u;OwnerFault::throwing=false;HR(x.Init(d.p,256,ShaderSet(),s,g,l),"Init failure supports full retry");}C(Refs(d.p)==refs,"retry destructor balances references");}
 printf("PASS 42 Init HRESULT and42 C++ exception cutpoints, cleanup/state/refs/retry\n");return 0;}catch(...){return 1;}}
'''
run('init-failures',fault,init)
assert fault.count('if(FAILED(hr))Clear();return hr;')==1
run('cleanup-mutant',fault.replace('if(FAILED(hr))Clear();return hr;','return hr;'),init,True)
runtime=source.replace('namespace WorldRaster {','namespace WorldRaster {\nstatic unsigned passFault=0;')
old='c->DrawIndexed(FixedTerrain::IndexCount,0,0);'
assert runtime.count(old)==1
runtime=runtime.replace(old,old+'if(passFault==2)throw 7;if(passFault==3&&i==0)bank.BeginStage(plan_,frame_.source,0,l);')
old='HRESULT hr=device_.p->GetDeviceRemovedReason();'
assert runtime.count(old)==1
runtime=runtime.replace(old,'HRESULT hr=passFault==1?DXGI_ERROR_DEVICE_REMOVED:device_.p->GetDeviceRemovedReason();')
phase=body+r'''
int main(){try{Com<ID3D11Device>d;Com<ID3D11DeviceContext>c;HR(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&d.p,NULL,&c.p),"phase WARP");WorldBank::SourceEpoch e={0x100000007ULL,WorldBank::PinnedBaseSnapshot};WorldMaterial::Planner planner;C(planner.Init(e.value),"phase plan");WorldMaterial::Plan p;C(planner.Build(27,10,e.value,0,p),"phase actual plan");std::vector<BYTE>h=Read("_generated/height-two.u16");FixedTerrain::Samples s={7294,7294,261,261,261,reinterpret_cast<const unsigned short*>(&h[0]),261*261};Grid g={7360,7360,2,2,e.value};Ledger l=Money(256,4);Raster x;HR(x.Init(d.p,256,ShaderSet(),s,g,l),"phase raster");WorldBank::Sources bank;HR(bank.Init(d.p,e,l),"phase sourcebank");Frame f=CaseFrame(e,1,256,0);Material m=TwoMaterial(e,f.materialGeneration,0);Com<ID3D11Predicate>pred;D3D11_QUERY_DESC qd={D3D11_QUERY_OCCLUSION_PREDICATE,0};HR(d.p->CreatePredicate(&qd,&pred.p),"phase predicate");c.p->Begin(pred.p);c.p->End(pred.p);c.p->Flush();BOOL v=TRUE;HRESULT hr=S_FALSE;for(unsigned i=0;i<10000&&hr==S_FALSE;++i)hr=c.p->GetData(pred.p,&v,sizeof(v),0);C(hr==S_OK&&!v,"phase false predicate ready");c.p->SetPredication(pred.p,FALSE);ID3D11ShaderResourceView*read=x.CarryView(1,1);c.p->CSSetShaderResources(127,1,&read);c.p->PSSetShaderResources(9,1,&read);ID3D11ShaderResourceView*read0=x.CarryView(0,0);c.p->VSSetShaderResources(17,1,&read0);c.p->HSSetShaderResources(31,1,&read0);
 // Exact same metadata/initial revision on another bank is not this frame's
 // owner. Charge both complete banks before creating the temporary second one.
 {Ledger two=l;two.retained=WorldBank::SourceBytes;Prepare(bank,p,e,0,two);HR(x.BeginFrame(true,f,p,m,bank,two),"begin original physical bank");WorldBank::Sources other;HR(other.Init(d.p,e,two),"fully charged temporary other bank");Prepare(other,p,e,0,two);C(FAILED(x.RenderStage(0,other,two))&&!x.Ready(f),"same metadata/revision cannot replace captured bank owner");}l.retired+=WorldBank::SourceBytes; // temporary bank uploads can still be in flight

 // Seed owned read aliases immediately before the first healthy draw. Earlier
 // unobserved partial draws must not erase the mutant's test precondition.
 Prepare(bank,p,e,0,l);HR(x.BeginFrame(true,f,p,m,bank,l),"read restoration controlled begin");
 read0=x.CarryView(0,0);c.p->VSSetShaderResources(17,1,&read0);c.p->HSSetShaderResources(31,1,&read0);
 {Com<ID3D11ShaderResourceView>observed;c.p->VSGetShaderResources(17,1,&observed.p);C(observed.p==read0&&read0!=NULL,"owned alias is actually bound before restoration observer");}
 {AtlasViewTest::ContextFixture before(c.p);HR(x.RenderStage(0,bank,l),"single healthy pass with seeded owned aliases");AtlasViewTest::ContextFixture after(c.p);C(before.Equal(after),"first healthy pass restores seeded carry reads across stages");}x.Cancel();

 Prepare(bank,p,e,0,l);HR(x.BeginFrame(true,f,p,m,bank,l),"premature tail begin");C(FAILED(x.Finish(bank,l))&&!x.Ready(f),"tail cannot finish before four contributions");
 Prepare(bank,p,e,0,l);HR(x.BeginFrame(true,f,p,m,bank,l),"premature trailing-empty begin");HR(x.RenderStage(0,bank,l),"premature first actual contribution");Prepare(bank,p,e,1,l);HR(x.RenderStage(1,bank,l),"premature second actual contribution");C(FAILED(x.Finish(bank,l))&&!x.Ready(f),"tail cannot omit required trailing empty passes");
 Prepare(bank,p,e,0,l);HR(x.BeginFrame(true,f,p,m,bank,l),"same-slot revision begin");Prepare(bank,p,e,0,l);C(FAILED(x.RenderStage(0,bank,l))&&!x.Ready(f),"same-slot upload cannot bypass Begin revision");
 for(unsigned mode=1;mode<=3;++mode){Prepare(bank,p,e,0,l);HR(x.BeginFrame(true,f,p,m,bank,l),"phase begin");ULONG refs=Refs(d.p),viewRefs=Refs(read);{AtlasViewTest::ContextFixture before(c.p);passFault=mode;HRESULT result=x.RenderStage(0,bank,l);passFault=0;C(FAILED(result),"real stage health/exception/source mutation fails");AtlasViewTest::ContextFixture after(c.p);C(before.Equal(after),"interrupted stage restores all foreign state/predicate/reads");C(!x.Ready(f)&&!x.Output(f,0),"interrupted stage cannot publish any completed output");}C(Refs(d.p)==refs&&Refs(read)==viewRefs,"interrupted stage refs balanced");}
 for(unsigned mode=1;mode<=3;++mode){Prepare(bank,p,e,0,l);HR(x.BeginFrame(true,f,p,m,bank,l),"tail failure begin");for(unsigned slot=0;slot<4;++slot){if(slot&&slot<p.sidCount)Prepare(bank,p,e,slot,l);HR(x.RenderStage(slot,bank,l),"tail failure prior complete contribution");}{AtlasViewTest::ContextFixture before(c.p);passFault=mode;HRESULT result=x.Finish(bank,l);passFault=0;C(FAILED(result)&&!x.Ready(f)&&!x.Output(f,0),"tail failure/exception/source change cannot publish");AtlasViewTest::ContextFixture after(c.p);C(before.Equal(after),"interrupted tail restores exact foreign state");}}
 Prepare(bank,p,e,0,l);HR(x.BeginFrame(true,f,p,m,bank,l),"last revision begin");for(unsigned slot=0;slot<4;++slot){if(slot&&slot<p.sidCount)Prepare(bank,p,e,slot,l);HR(x.RenderStage(slot,bank,l),"last revision stage");}Prepare(bank,p,e,1,l);C(FAILED(x.Finish(bank,l))&&!x.Ready(f),"last same-slot revision change blocks Finish");
 Steps(x,bank,p,m,f,l,c.p);C(x.Ready(f),"complete recovery after every failure");C(Pixels(d.p,c.p,x.Texture(0))==Read("../raw-rgb/_generated/control3-material.rgba16f"),"recovered focused frame matches frozen RGB bytes");l.retired-=WorldBank::SourceBytes; // Map above waits all prior queued bank work
c.p->ClearState();printf("PASS %u focused production phase failure/cancel/state checks\n",checks);return 0;}catch(...){return 1;}}
'''
run('phase-failures',runtime,phase)
# Functional controls reuse just this one bounded phase fixture, not32M image checks.
mutants=[
 ('bank-identity','||&bank!=bankIdentity_',''),
 ('begin-lease','b.revision!=lease_.revision:b.revision<=lease_.revision','false:b.revision<=lease_.revision'),
 ('finish-lease','if(SUCCEEDED(hr)&&b.revision!=lease_.revision)hr=E_ABORT;',''),
 ('premature-tail','!active_||next_!=4','!active_'),
 ('role-patch','if(!finish&&slot<plan_.sidCount)','if(false)'),
 ('reverse-slot','const unsigned control[4]={slot,0,0,0}','const unsigned control[4]={3-slot,0,0,0}'),
 ('missing-tail','Pass(true,4,bank,l,b)','Pass(false,3,bank,l,b)'),
 ('partial-publication','lease_=b;++next_;return S_OK;','lease_=b;++next_;ready_=true;return S_OK;'),
 ('predicate','c->SetPredication(NULL,FALSE);',''),
 ('read-restore','Set(s,i,v_[s][i]);v_[s][i]->Release();','v_[s][i]->Release();'),
]
for name,a,b in mutants:
 assert runtime.count(a)==1,(name,runtime.count(a))
 run(name,runtime.replace(a,b),phase,True)
print('PASS bounded owner/phase faults and10 compiled functional controls')
