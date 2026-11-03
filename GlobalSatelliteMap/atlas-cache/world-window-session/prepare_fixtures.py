from pathlib import Path
import hashlib,json,re,os,subprocess,sys
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True);RASTER=HERE.parents[1]/'atlas-terrain/world-raster'
prefix=(RASTER/'warp_tests.cpp').read_text().split('int main()')[0]
prefix=re.sub(r'(^\s*#include\s+")([^"]+)(")',lambda m:m[1]+(RASTER/m[2]).resolve().as_posix()+m[3],prefix,flags=re.M)
body=r'''
#include "D:/DEV/Kenshi/ours/GlobalSatelliteMap/atlas-terrain/world-source/WorldHeightSource.h"
int main(int argc,char**argv){try{C(argc==2,"owned output root");Com<ID3D11Device>d;Com<ID3D11DeviceContext>c;HR(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&d.p,NULL,&c.p),"owned aligned WARP");WorldBank::SourceEpoch e={0x100000007ULL,WorldBank::PinnedBaseSnapshot};WorldMaterial::Planner planner;WorldMaterial::Plan plan;C(planner.Init(e.value)&&planner.Build(27,10,e.value,0,plan),"actual cell10 source plan");WorldHeight::Source height;WorldHeight::Operation op;C(height.Open(L"D:/SteamLibrary/steamapps/common/Kenshi/data/newland/land/fullmap.tif",e.value,op),"real fullmap authentication");
for(unsigned i=0;i<2;++i){WorldHeight::Window samples;WorldHeight::Region region={7168+int(i)*256,7168,7424+int(i)*256,7424};C(height.Read(region,e.value,samples,op),"bounded exact native geometry samples");Grid grid={unsigned(region.x0),unsigned(region.z0),4,4,e.value};Ledger ledger=Money(512,16);WorldBank::Sources bank;HR(bank.Init(d.p,e,ledger),"real single-SID bank");Raster raster;HR(raster.Init(d.p,512,ShaderSet(),samples.View(),grid,ledger),"aligned actual512 raster");Frame frame=CaseFrame(e,5,512,1);frame.x=-16128.f+4608.f*i;frame.z=-16128.f;frame.materialGeneration=0x200000001ULL+i;Material material=TwoMaterial(e,frame.materialGeneration,1);Steps(raster,bank,plan,material,frame,ledger,c.p);const char*suffix[3]={"-material.rgba16f","-normal.rgba8","-radial.r32f"};for(unsigned p=0;p<3;++p){std::vector<BYTE>b=Pixels(d.p,c.p,raster.Texture(p));char label[32];sprintf(label,"/aligned%u",i);std::ofstream f((std::string(argv[1])+label+suffix[p]).c_str(),std::ios::binary);f.write(reinterpret_cast<const char*>(&b[0]),b.size());C(!!f,"owned aligned fixture written");}}
printf("PASS two real atlas-aligned512/9m WARP windows, original source bank/shaders\n");return 0;}catch(...){return 1;}}
'''
(OUT/'capture.cpp').write_text(prefix+body)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT',str(OUT/'capture.cpp'),'/Fo'+str(OUT/'capture.obj'),'/Fe'+str(OUT/'capture.exe'),'/link','d3d11.lib','dxgi.lib','advapi32.lib'],cwd=RASTER,env=env,capture_output=True,text=True);(OUT/'capture-build.log').write_text(p.stdout+p.stderr);assert not p.returncode,(p.stdout,p.stderr)
p=subprocess.run([str(OUT/'capture.exe'),str(OUT)],cwd=RASTER,capture_output=True,text=True);(OUT/'capture-run.log').write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='');assert not p.returncode
pins={};seen=set()
def visit(p):
 p=p.resolve()
 if p in seen:return
 seen.add(p);pins[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest()
 if p.suffix in ['.h','.cpp']:
  for name in re.findall(r'^\s*#include\s+"([^"]+)"',p.read_text(),re.M):
   q=p.parent/name
   if q.exists():visit(q)
visit(RASTER/'warp_tests.cpp');visit(HERE.parents[1]/'atlas-terrain/world-source/WorldHeightSource.h')
for p in OUT.glob('aligned*'):pins[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest()
(OUT/'fixtures.json').write_text(json.dumps(dict(scope='actual owned WARP output; pinned source snapshot, not current native winners',pins=pins),indent=2)+'\n')
