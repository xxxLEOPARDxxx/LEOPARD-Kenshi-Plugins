"""Exercise actual regional parameter/identity production bodies on all57 records.
Only the engine GameData lookup/native-memory-read boundary is substituted.
"""
from pathlib import Path
import hashlib,json,os,struct,subprocess
from test_adapters import function
HERE=Path(__file__).resolve().parent;PROBE=HERE.parent
geo=(PROBE/'PrivateGeography.h').read_text();reference=(PROBE/'PrivateBiomeReference.h').read_text()
source=Path('D:/DEV/Kenshi/research/global-satellite-20261031/biome-current-36248/active-biomes.json')
assert hashlib.sha256(source.read_bytes()).hexdigest()=='511a8813928114dbf6b2697d5fa46a46e858c46690907cf8493e092c84633d0f'
records={r['sid']:r for r in json.loads(source.read_text())['records'] if r.get('sid')}
doc=json.loads((HERE/'catalog.json').read_text())
q=lambda s:json.dumps(s,ensure_ascii=True)
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def literal(x):
    v=format(f32(x),'.9g')
    return v+('' if any(c in v for c in '.eE') else '.0')+'f'
keys=[['tiling X 1','tiling Y 1','tiling X 2','tiling Y 2'],['tiling X 0','tiling Y 0','tiling X grass','tiling Y grass'],
 ['tiling X dirt','tiling Y dirt','tiling X road','tiling Y road'],['slope min 1','slope min 2','slope min 3','slope min grass'],
 ['slope max 1','slope max 2','slope max 3','slope max grass'],['slope fade 1','slope fade 2','slope fade 3','slope fade grass'],
 ['overlay mult vertical','overlay mult grass','overlay mult dirt','overlay mult road'],[None]*4,
 ['absorbance 0','absorbance 1','absorbance 2','absorbance grass'],['absorbance dirt','absorbance road',None,None]]
setup=[];expected=[];distortion=[];record_bytes=bytearray()
def text_bytes(value):
    value=value.encode('ascii');return struct.pack('<H',len(value))+value
for i,b in enumerate(doc['biomes']):
    r=records[b['sid']];setup+=['records[%d].stringID=%s;world.gamedata.bySID[%s]=&records[%d];'%(i,q(b['sid']),q(b['sid']),i)]
    for key,value in sorted(r['float'].items()):setup+=['records[%d].fdata[%s]=%s;'%(i,q(key),literal(value))]
    for key,value in sorted(r['int'].items()):setup+=['records[%d].idata[%s]=static_cast<int>(%du);'%(i,q(key),value&0xffffffff)]
    for key,value in sorted(r['file'].items()):setup+=['records[%d].filesdata[%s]=%s;'%(i,q(key),q(value))]
    record_bytes.extend(text_bytes(b['sid']))
    for kind in ('float','int','file'):
        record_bytes.extend(struct.pack('<H',len(r[kind])))
        for key,value in sorted(r[kind].items()):
            record_bytes.extend(text_bytes(key))
            record_bytes.extend(struct.pack('<f',value) if kind=='float' else struct.pack('<I',value&0xffffffff) if kind=='int' else text_bytes(value))
    row=[0.0]*44
    for ri,group in enumerate(keys):
        for col,key in enumerate(group):
            if key:row[ri*4+col]=f32(f32(r['float'].get(key,0))*f32(.01 if 3<=ri<=5 else 1))
    row[40]=r['float']['brightness fix'];argb=r['int']['ground colour'];distance=f32(r['float']['fade distance'])
    row[28:32]=[f32(v*f32(1/255)) for v in ((argb>>16)&255,(argb>>8)&255,argb&255)]+[f32(1/distance) if distance>0 else distance]
    expected.append('{'+','.join(literal(x) for x in row)+'}')
    wavelength=r['float']['distort wavelength'];distortion.append('{'+','.join(literal(x) for x in [f32(1/wavelength) if wavelength>0 else 0,f32(f32(r['float']['distort amplitude'])*f32(.01)),0,0])+'}')
template=r'''
#include <windows.h>
#include <wincrypt.h>
#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include "RegionCatalogData.h"
#include "../GeographyPolicy.h"
enum {BIOMES=1};struct GameData {int type;std::string stringID;std::map<std::string,float> fdata;std::map<std::string,int> idata;std::map<std::string,std::string> filesdata;GameData():type(BIOMES){}};
struct Store{std::map<std::string,GameData*>bySID;unsigned calls;Store():calls(0){}GameData*getData(const std::string&s){++calls;std::map<std::string,GameData*>::iterator i=bySID.find(s);return i==bySID.end()?NULL:i->second;}};
struct World{Store gamedata;}world;World*ou=&world;GameData records[57];
static unsigned checks=0;void Check(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);std::exit(1);}}
struct Raw {BYTE slots[5];std::vector<BYTE>tree;}raw[1024];bool corruptSlot=false,corruptTree=false;
bool ReadGeographicRegionCell(const SatelliteRegion::Region&r,unsigned index,void*(&ids)[5],BYTE(&tree)[2048],unsigned&bytes){
const Raw& c=raw[(r.z*4+index/4)*32+r.x*4+index%4];for(unsigned s=0;s<5;++s)ids[s]=c.slots[s]?&records[c.slots[s]-1]:NULL;
bytes=static_cast<unsigned>(c.tree.size());std::memcpy(tree,&c.tree[0],bytes);if(corruptSlot)ids[0]=reinterpret_cast<void*>(1);if(corruptTree)tree[bytes-1]^=1;return true;}
struct PrivateBiomeReference {
// HELPERS
};
struct Geography {const SatelliteRegion::Region*selectedRegion;float parameters[900],distortion[60];
// ACTUAL
}g;
static const float expected[57][44]={// EXPECTED
};static const float distort[57][4]={// DISTORTION
};
unsigned Read16(std::ifstream&f){unsigned short v=0;f.read(reinterpret_cast<char*>(&v),2);Check(f.good(),"bounded record count");return v;}
std::string Text(std::ifstream&f){unsigned n=Read16(f);Check(n<4096,"record string bound");std::string s(n,'\0');if(n)f.read(&s[0],n);Check(f.good(),"record string read");return s;}
int main(int argc,char**argv){Check(argc==3,"source fixtures");
std::ifstream input(argv[2],std::ios::binary);Check(input.good(),"57actual record inputs");
for(unsigned i=0;i<57;++i){GameData&r=records[i];r.stringID=Text(input);Check(r.stringID==SatelliteRegion::Data.biomes[i].sid,"record identity");world.gamedata.bySID[r.stringID]=&r;
unsigned n=Read16(input);Check(n<256,"float field bound");for(unsigned k=0;k<n;++k){std::string key=Text(input);float value=0;input.read(reinterpret_cast<char*>(&value),4);r.fdata[key]=value;}
n=Read16(input);Check(n<256,"integer field bound");for(unsigned k=0;k<n;++k){std::string key=Text(input);int value=0;input.read(reinterpret_cast<char*>(&value),4);r.idata[key]=value;}
n=Read16(input);Check(n<256,"file field bound");for(unsigned k=0;k<n;++k){std::string key=Text(input);r.filesdata[key]=Text(input);}}
Check(input.peek()==std::char_traits<char>::eof(),"record fixture complete");
std::ifstream f(argv[1],std::ios::binary);Check(f.good(),"raw fixture open");f.seekg(8);unsigned short size;f.read(reinterpret_cast<char*>(&size),2);Check(size==57,"57sourceIDs");for(unsigned i=0;i<57;++i){f.read(reinterpret_cast<char*>(&size),2);f.seekg(size,std::ios::cur);}for(unsigned i=0;i<1024;++i){f.read(reinterpret_cast<char*>(raw[i].slots),5);f.read(reinterpret_cast<char*>(&size),2);raw[i].tree.resize(size);f.read(reinterpret_cast<char*>(&raw[i].tree[0]),size);f.seekg(64,std::ios::cur);}Check(f.good(),"raw native masks read");
for(unsigned phase=0;phase<3;++phase)for(unsigned ri=0;ri<64;++ri){const SatelliteRegion::Region&r=SatelliteRegion::Data.regions[ri];g.selectedRegion=&r;for(unsigned i=0;i<900;++i)g.parameters[i]=12345;for(unsigned i=0;i<60;++i)g.distortion[i]=12345;world.gamedata.calls=0;
g.ParametersRegion("D:/SteamLibrary/steamapps/common/Kenshi",phase==1);bool needed[57]={0};for(unsigned c=0;c<16;++c)for(unsigned s=0;s<5;++s)if(r.cells[c].slots[s])needed[r.cells[c].slots[s]-1]=true;unsigned count=0;for(unsigned s=0;s<57;++s)if(needed[s])++count;Check(world.gamedata.calls==count,"lookup only selected native source refs inclunused");
for(unsigned b=0;b<15;++b){for(unsigned p=0;p<60;++p){float want=0;if(b<r.biomeCount){unsigned src=r.biomes[b].source;if(p<44)want=(p>=28&&p<32&&phase!=1)?0:expected[src][p];else if(p>=44&&p<50)want=float(r.biomes[b].layers[0][p-44]);else if(p>=52&&p<58)want=float(r.biomes[b].layers[1][p-52]);}Check(g.parameters[b*60+p]==want,"all actual57 parameter rows/local layer refs/zero rest");}for(unsigned p=0;p<4;++p)Check(g.distortion[b*4+p]==(b<r.biomeCount?distort[r.biomes[b].source][p]:0),"all actual57 distortion records/zero rest");}}
g.selectedRegion=&SatelliteRegion::Data.regions[1];const std::string unused="4415-Newwworld.mod";GameData*saved=world.gamedata.bySID[unused];world.gamedata.bySID.erase(unused);bool refused=false;try{g.ParametersRegion("D:/SteamLibrary/steamapps/common/Kenshi",false);}catch(const std::runtime_error&){refused=true;}Check(refused,"missing unused native identity refuses");world.gamedata.bySID[unused]=saved;
for(unsigned trial=0;trial<2;++trial){corruptSlot=trial==0;corruptTree=trial==1;refused=false;try{g.ParametersRegion("D:/SteamLibrary/steamapps/common/Kenshi",false);}catch(const std::runtime_error&){refused=true;}Check(refused,"native ordered slot/tree mismatch refuses");}corruptSlot=corruptTree=false;
std::printf("PASS %u actual ParametersRegion/FillRecord checks;64regions/off-on-off/all57records/unusedidentity/hashrefusal/zero-unused-rows\n",checks);}
'''
template=template.replace('// HELPERS','\n'.join(function(reference,s) for s in ('static float Float(','static std::string File(','static std::string Canonical(','static std::string Hash(')))
template=template.replace('// ACTUAL','\n'.join(function(geo,s) for s in ('void FillRecord(','void ParametersRegion(')))
template=template.replace('// EXPECTED',',\n'.join(expected)).replace('// DISTORTION',',\n'.join(distortion))
out=HERE/'build';file=out/'actual-parameters.cpp';file.write_text(template)
(out/'record-inputs.bin').write_bytes(record_bytes)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=str(vc/'include')+';'+str(sdk/'Include'),LIB=str(vc/'lib/amd64')+';'+str(sdk/'Lib/x64'))
for command in ([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/W4','/WX','/I'+str(HERE),'/Fe'+str(out/'actual-parameters.exe'),'/Fo'+str(out/'actual-parameters.obj'),str(file),'/link','advapi32.lib'],[str(out/'actual-parameters.exe'),str(out/'source.fixture'),str(out/'record-inputs.bin')]):
    result=subprocess.run(command,env=env,capture_output=True,text=True);print(result.stdout+result.stderr,end='')
    if result.returncode:raise SystemExit(result.returncode)
