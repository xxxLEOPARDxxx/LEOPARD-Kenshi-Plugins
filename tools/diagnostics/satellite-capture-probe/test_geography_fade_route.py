"""Compile actual caller/routing/row adapter fragments with an engine boundary stub."""
from pathlib import Path
import subprocess
here=Path(__file__).resolve().parent;out=here/'build/geography'
source=(here/'CaptureProbe.cpp').read_text();terrain=(here/'TerrainExperiment.h').read_text();geo=(here/'PrivateGeography.h').read_text()
start=source.index('terrainExperiment.Begin(')+len('terrainExperiment.Begin(');at=start;depth=1;args=[];part=start
while depth:
    ch=source[at]
    if ch=='(':depth+=1
    elif ch==')':
        depth-=1
        if not depth:args.append(source[part:at].strip());break
    elif ch==',' and depth==1:args.append(source[part:at].strip());part=at+1
    at+=1
assert len(args)==12 and 'GeographicBiomes",0' in args[-2] and 'GeographicNativeFade",0' in args[-1]
route=next(line.strip() for line in terrain.splitlines() if 'if(geographic)GeographyReference().Begin(' in line)
assert 'nativeFadeRequest' in route
parameters=next(line.strip() for line in geo.splitlines() if line.strip()=='Parameters(root,nativeFadeRequest);')
row=geo.split('// Native fade row: this exact adapter is extracted by route tests.',1)[1].split('// Native fade row end.',1)[0]
assert 'data->idata,data->fdata' in row and 'parameters+b*60+28' in row
assert geo.index('Parameters(root,nativeFadeRequest);')<geo.index('for(unsigned bank=0;bank<5;++bank)',geo.index('void Begin('))
fixture=r'''
#include <map>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include "GeographyPolicy.h"
#include "GeographyFadeInputs.h"
static unsigned checks=0;void Check(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);std::exit(1);}}
struct Data{std::map<std::string,int>idata;std::map<std::string,float>fdata;};
struct Root{void*getCompositorManager2(){return this;}} dummy;
struct Geographic{Data records[15];float parameters[900];unsigned starts,allocations;Geographic():starts(0),allocations(0){std::memset(parameters,0,sizeof(parameters));}
 void Parameters(const std::string&,bool nativeFadeRequest){std::memset(parameters,0,sizeof(parameters));for(unsigned b=0;b<15;++b){Data*data=&records[b];
// ROW
 }}
 void Begin(const std::string&root,void*,bool nativeFadeRequest){++starts;
// PARAMETERS
 ++allocations;}
} geographicOwner;
Geographic&GeographyReference(){return geographicOwner;}
struct Biome{unsigned starts;Biome():starts(0){}void Begin(Data*,const std::string&){++starts;}} biome;
Biome&BiomeReference(){return biome;}
struct Experiment{void Begin(bool geographicRequest,bool nativeFadeRequest){bool geographic=geographicRequest;Root*root=&dummy;std::string gameRoot="fixture";Data*data=NULL;
// ROUTE
}} terrainExperiment;
std::map<std::string,int>request;std::string ini="fixture";
int GetPrivateProfileIntA(const char*,const char*key,int fallback,const char*){std::map<std::string,int>::const_iterator i=request.find(key);return i==request.end()?fallback:i->second;}
void CaptureRoute(){terrainExperiment.Begin(
// FLAGS
);}
int main(){for(unsigned b=0;b<15;++b){geographicOwner.records[b].idata["ground colour"]=int(FadeFixtures[b].argb);geographicOwner.records[b].fdata["fade distance"]=FadeFixtures[b].distance;}
 request["GeographicBiomes"]=1;
 for(unsigned phase=0;phase<3;++phase){if(phase==1)request["GeographicNativeFade"]=1;else request.erase("GeographicNativeFade");CaptureRoute();
 for(unsigned b=0;b<15;++b)for(unsigned c=0;c<4;++c)Check(geographicOwner.parameters[b*60+28+c]==(phase==1?FadeFixtures[b].expected[c]:0),"actual Capture->Terrain->Geography row7 off/on/off");}
 Check(geographicOwner.starts==3&&geographicOwner.allocations==3,"three successful production request routes");
 request["GeographicBiomes"]=0;request["GeographicNativeFade"]=1;CaptureRoute();Check(geographicOwner.starts==3&&biome.starts==1,"fade flag outside geographic route has no effect");
 request["GeographicBiomes"]=1;geographicOwner.records[3].idata.clear();bool refused=false;try{CaptureRoute();}catch(const std::runtime_error&){refused=true;}
 Check(refused&&geographicOwner.allocations==3&&geographicOwner.records[3].idata.empty(),"missing enabled field refuses before allocation without insertion");
 request.erase("GeographicNativeFade");for(unsigned b=0;b<15;++b){geographicOwner.records[b].idata.clear();geographicOwner.records[b].fdata.clear();}CaptureRoute();
 Check(geographicOwner.allocations==4,"disabled route needs no additional fields");for(unsigned i=0;i<900;++i)Check(geographicOwner.parameters[i]==0,"actual production reset removes enabled row data");
 std::printf("PASS %u extracted production Capture/Terrain/Geography fade route checks\n",checks);}
'''
fixture=fixture.replace('// ROW',row).replace('// PARAMETERS',parameters).replace('// ROUTE',route).replace('// FLAGS',',\n'.join(args[-2:]))
path=out/'geography-fade-route.cpp';path.write_text(fixture)
cl='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC/bin/amd64/cl.exe'
for command in ([cl,'/nologo','/EHsc','/MD','/O2','/I'+str(here),'/I'+str(out),'/Fe'+str(out/'geography-fade-route.exe'),'/Fo'+str(out/'geography-fade-route.obj'),str(path)],[str(out/'geography-fade-route.exe')]):
    result=subprocess.run(command,capture_output=True,text=True,creationflags=subprocess.CREATE_NO_WINDOW)
    print(result.stdout+result.stderr,end='');assert result.returncode==0,result.returncode
