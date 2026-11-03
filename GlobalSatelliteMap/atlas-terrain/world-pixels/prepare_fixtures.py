"""Owned WARP fixture capture only; no generator or write in borrowed helpers."""
from pathlib import Path
import hashlib,json,re,os,subprocess
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True);RASTER=HERE.parent/'world-raster';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
source=(RASTER/'warp_tests.cpp').read_text().split('int main()')[0]
source=re.sub(r'(^\s*#include\s+")([^"]+)(")',lambda m:m[1]+(RASTER/m[2]).resolve().as_posix()+m[3],source,flags=re.M)
main=r'''
int main(int argc,char**argv){try{C(argc==2,"owned fixture output path");Com<ID3D11Device>d;Com<ID3D11DeviceContext>c;HR(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&d.p,NULL,&c.p),"owned fixture WARP");WorldBank::SourceEpoch e={0x100000007ULL,WorldBank::PinnedBaseSnapshot};WorldMaterial::Planner planner;WorldMaterial::Plan plan;C(planner.Init(e.value)&&planner.Build(27,10,e.value,0,plan),"actual two-SID fixture plan");std::vector<BYTE>height=Read("_generated/height-two.u16");FixedTerrain::Samples source={7294,7294,261,261,261,reinterpret_cast<const unsigned short*>(&height[0]),261*261};Grid grid={7296,7296,4,4,e.value};Ledger ledger=Money(512,16);WorldBank::Sources bank;HR(bank.Init(d.p,e,ledger),"one actual bank");Raster raster;HR(raster.Init(d.p,512,ShaderSet(),source,grid,ledger),"actual512 raster");const unsigned cases[5]={0,1,3,2,8};const char*names[5]={"control3","control9","control1","shift3","odd3"};const char*suffix[3]={"-material.rgba16f","-normal.rgba8","-radial.r32f"};for(unsigned i=0;i<5;++i){Frame frame=CaseFrame(e,100+i,512,cases[i]);Material material=TwoMaterial(e,frame.materialGeneration,cases[i]);Steps(raster,bank,plan,material,frame,ledger,c.p);for(unsigned plane=0;plane<3;++plane){std::vector<BYTE>pixels=Pixels(d.p,c.p,raster.Texture(plane));std::string path=std::string(argv[1])+"/512-"+names[i]+suffix[plane];std::ofstream f(path.c_str(),std::ios::binary);f.write(reinterpret_cast<const char*>(&pixels[0]),pixels.size());C(!!f,"owned fixture complete");}}printf("PASS five actual512 WARP fixtures captured\n");return 0;}catch(...){return 1;}}
'''
(OUT/'capture.cpp').write_text(source+main)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT',str(OUT/'capture.cpp'),'/Fo'+str(OUT/'capture.obj'),'/Fe'+str(OUT/'capture.exe'),'/link','d3d11.lib','dxgi.lib','advapi32.lib'],cwd=RASTER,env=env,capture_output=True,text=True);(OUT/'capture-build.log').write_text(p.stdout+p.stderr);assert not p.returncode,(p.stdout,p.stderr)
p=subprocess.run([str(OUT/'capture.exe'),str(OUT)],cwd=RASTER,capture_output=True,text=True);(OUT/'capture-run.log').write_text(p.stdout+p.stderr);assert not p.returncode,(p.stdout,p.stderr);print(p.stdout)
labels=['control3','control9','shift3','control1','wet3','underwater3','hdr3','hdr1','odd3'];steps=[3,9,3,1,3,3,3,1,3];centers=[-13824,-13824,-13632,-13824,-13824,-13824,-13824,-13824,-13821];cases=[]
for i,label in enumerate(labels):cases.append(dict(name='256-'+label,side=256,passIndex=i,spacing=steps[i],center=centers[i],generation=i+1,prefix=str(HERE.parent/'raw-rgb/_generated'/label)))
for i,idx in enumerate([0,1,3,2,8]):cases.append(dict(name='512-'+labels[idx],side=512,passIndex=idx,spacing=steps[idx],center=centers[idx],generation=100+i,prefix=str(OUT/('512-'+labels[idx]))))
pins={str(RASTER/'warp_tests.cpp'):sha(RASTER/'warp_tests.cpp'),str(RASTER/'_generated/height-two.u16'):sha(RASTER/'_generated/height-two.u16')}
seen=set()
def closure(p):
 p=p.resolve()
 if p in seen:return
 seen.add(p);pins[str(p)]=sha(p)
 if p.suffix in ['.h','.cpp']:
  for name in re.findall(r'^\s*#include\s+"([^"]+)"',p.read_text(),re.M):closure(p.parent/name)
closure(RASTER/'warp_tests.cpp')
for case in cases:
 for suffix in ['-material.rgba16f','-normal.rgba8','-radial.r32f']:
  p=Path(case['prefix']+suffix);pins[str(p)]=sha(p)
(OUT/'fixtures.json').write_text(json.dumps(dict(scope='actual private WARP outputs; not live native source winners',cases=cases,pins=pins),indent=2)+'\n')
lines=['#pragma once','struct PixelCase{const char*name;const char*prefix;unsigned side,index,generation;float spacing,center;};','static const PixelCase PixelCases[]={']
for c in cases:lines.append('{%s,%s,%d,%d,%d,%sf,%sf},'%(json.dumps(c['name']),json.dumps(c['prefix'].replace('\\','/')),c['side'],c['passIndex'],c['generation'],float(c['spacing']),float(c['center'])))
lines+=['};'];(OUT/'Cases.h').write_text('\n'.join(lines)+'\n')
print('PASS14 actual256/512 fixtures and pinned source-height window')
