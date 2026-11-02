"""Compile actual production adapter bodies; engine boundary records arguments."""
from pathlib import Path
import os,subprocess,unittest
HERE=Path(__file__).resolve().parent;PROBE=HERE.parent
def function(text,signature):
    start=text.index(signature);opening=text.index('{',start);depth=1;end=opening+1
    while depth:
        if text[end]=='{':depth+=1
        elif text[end]=='}':depth-=1
        end+=1
    return text[start:end]
class AdapterTests(unittest.TestCase):
    def test_actual_region_and_primary_adapter_routes(self):
        geo=(PROBE/'PrivateGeography.h').read_text();paint=(PROBE/'PrivatePaintedReference.h').read_text()
        self.assertIn('void BeginRegion(',geo,'regional production adapter not implemented')
        self.assertIn('void BeginPrimary(',paint,'primary painted production adapter not implemented')
        bodies=[function(geo,'GeoFile Input('),function(geo,'void BeginRegion('),function(geo,'void Begin(')]
        painted=[function(paint,'static bool InputsForPrimary('),function(paint,'void BeginPrimary('),function(paint,'void Begin(')]
        uniform=next(line.strip() for line in geo.splitlines() if 'fp->setNamedConstant("geoSelectorRegion"' in line).split(';',1)[0]+';'
        template=r'''
#include <stdexcept>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "RegionCatalogData.h"
struct NativeCheck {unsigned rva;unsigned char code[24];};
typedef unsigned char BYTE;
#include "../build/GeographyInputs.h"
#include "../PrivatePaintedInputs.h"
namespace Ogre {struct CompositorManager2 {};struct Vector4 {float x,y,z,w;Vector4(float a,float b,float c,float d):x(a),y(b),z(c),w(d){}};struct GpuProgramParameters {float value[4];void setNamedConstant(const char*name,const Vector4&v){if(std::strcmp(name,"geoSelectorRegion"))std::abort();value[0]=v.x;value[1]=v.y;value[2]=v.z;value[3]=v.w;}};}
static unsigned checks=0;void Check(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);std::exit(1);}}
struct Geography {const SatelliteRegion::Region* selectedRegion;unsigned calls;bool fade;
 Geography():selectedRegion(NULL),calls(0),fade(false){}
 void BeginImpl(const std::string&,Ogre::CompositorManager2*,bool f,const SatelliteRegion::Region*r){++calls;selectedRegion=r;fade=f;}
 void BindUniform(Ogre::GpuProgramParameters*fp){// UNIFORM
 }
// GEO
};
struct Painted {unsigned calls;int x,z;PrivatePaintedInput captured[8];Painted():calls(0),x(-1),z(-1){}
 void BeginInputs(const std::string&,Ogre::CompositorManager2*,const PrivatePaintedInput (&inputs)[8],int px,int pz){++calls;x=px;z=pz;std::memcpy(captured,inputs,sizeof(captured));}
// PAINT
};
int main(){Geography g;g.Begin("",NULL,true);Check(g.calls==1&&!g.selectedRegion&&g.fade,"legacy Begin routes exact legacy dataset");
Ogre::GpuProgramParameters fp;g.BindUniform(&fp);Check(fp.value[0]==-73728&&fp.value[1]==-73728&&fp.value[2]==8&&fp.value[3]==8,"actual legacy uniform binding8x8");
for(unsigned f=0;f<92;++f){GeoFile a=g.Input(f);Check(!std::strcmp(a.path,GeoFiles[f].path)&&!std::strcmp(a.sha256,GeoFiles[f].sha256)&&a.bank==GeoFiles[f].bank&&a.layer==GeoFiles[f].layer,"legacy92 uploads unchanged");}
for(unsigned i=0;i<64;++i){const SatelliteRegion::Region&r=SatelliteRegion::Data.regions[i];g.BeginRegion("",NULL,r,(i&1)!=0);Check(g.selectedRegion==&r&&g.fade==((i&1)!=0),"exact immutable region and fade forwarded");
 g.BindUniform(&fp);Check(fp.value[0]==r.worldX&&fp.value[1]==r.worldZ&&fp.value[2]==4&&fp.value[3]==4,"actual regional uniform binding4x4");
 for(unsigned f=0;f<92;++f){GeoFile a=g.Input(f);const SatelliteRegion::Asset&expected=SatelliteRegion::Data.assets[r.banks[f]];Check(!std::strcmp(a.path,expected.path)&&!std::strcmp(a.sha256,expected.sha256)&&a.bytes==expected.bytes&&a.format==expected.format,"actual92 source refs including padding");Check(a.bank==(f<24?0u:f<46?1u:2u)&&a.layer==f-(f<24?0u:f<46?24u:46u),"exact24+22/46 layer mapping");}}
unsigned before=g.calls;SatelliteRegion::Region copy=SatelliteRegion::Data.regions[0];bool refused=false;try{g.BeginRegion("",NULL,copy,false);}catch(const std::runtime_error&){refused=true;}Check(refused&&g.calls==before,"region copy/foreign pointer refused before begin");
bool bad=false;try{g.Input(92);}catch(const std::runtime_error&){bad=true;}Check(bad,"out-of-bounds upload index refused");
Painted p;p.Begin("",NULL);Check(p.x==1&&p.z==1,"legacy painted Begin1,1");for(unsigned f=0;f<8;++f)Check(!std::strcmp(p.captured[f].sha256,PrivatePaintedInputs[f].sha256),"legacy actual PNG identities unchanged");
for(int z=0;z<4;++z)for(int x=0;x<4;++x){p.BeginPrimary("",NULL,x,z);Check(p.x==x&&p.z==z,"primary forwarded");for(unsigned k=0;k<2;++k)for(unsigned l=0;l<4;++l){const PrivatePaintedInput&a=p.captured[k*4+l];const SatelliteRegion::Painted&e=SatelliteRegion::Data.painted[k*64+(z*2+l/2)*8+x*2+l%2];Check(a.kind==k&&a.x==unsigned(x*2)+l%2&&a.z==unsigned(z*2)+l/2&&a.side==e.side&&a.bytes==e.bytes&&!std::strcmp(a.path,e.path)&&!std::strcmp(a.sha256,e.sha256),"actual primary layer/path/hash mapping");}}
before=p.calls;for(int i=0;i<4;++i){refused=false;try{p.BeginPrimary("",NULL,i==0?-1:i==1?4:0,i==2?-1:i==3?4:0);}catch(const std::runtime_error&){refused=true;}Check(refused&&p.calls==before,"invalid primary refuses before engine-bound begin");}
std::printf("PASS %u extracted actual region/painted adapter checks\n",checks);}
'''
        fixture=template.replace('// GEO','\n'.join(bodies)).replace('// PAINT','\n'.join(painted)).replace('// UNIFORM',uniform)
        out=HERE/'build';out.mkdir(exist_ok=True);source=out/'actual-adapters.cpp';source.write_text(fixture)
        vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
        env=dict(os.environ,INCLUDE=str(vc/'include')+';'+str(sdk/'Include'),LIB=str(vc/'lib/amd64')+';'+str(sdk/'Lib/x64'))
        for command in ([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/W4','/WX','/I'+str(HERE),'/Fe'+str(out/'actual-adapters.exe'),'/Fo'+str(out/'actual-adapters.obj'),str(source)],[str(out/'actual-adapters.exe')]):
            result=subprocess.run(command,env=env,capture_output=True,text=True);print(result.stdout+result.stderr,end='');self.assertEqual(result.returncode,0)
if __name__=='__main__':unittest.main(verbosity=2)
