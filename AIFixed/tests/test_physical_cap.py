"""Compile real physical-cap helper against analytic capsule/ramp boundaries."""
from pathlib import Path
import os, subprocess
here=Path(__file__).resolve().parent
runtime=(here.parent/'UnstuckRuntime.h').read_text('utf-8')
start=runtime.index('    struct CapFooting')
code=runtime[start:runtime.index('    int QueryPlatformPocket(',start)]
fixture=r'''
#include <cmath>
#include <cstdio>
#include <float.h>
#include <UnstuckPolicy.h>
namespace Ogre {struct Vector3 {float x,y,z;Vector3(float a=0,float b=0,float c=0):x(a),y(b),z(c){} Vector3 operator+(const Vector3& b)const{return Vector3(x+b.x,y+b.y,z+b.z);} };}
struct Building{};struct NxShape{};Building support,other;NxShape shape,otherShape;
int mode=0,calls=0;bool abortTrace=false,missingShape=false,changeShape=false,wrongBuilding=false,wrongNormal=false;
struct physHit {bool hit;Ogre::Vector3 position,normal;float distance;NxShape* shape;physHit(bool,bool,float):hit(false),distance(0),shape(NULL){} bool traceWasAborted(){return abortTrace;}Building* getBuilding(){return wrongBuilding?&other:&support;}};
AIFixedGate::Point Point(const Ogre::Vector3& p){return AIFixedGate::Point(p.x,p.y,p.z);}
struct UtilityT {static void trace(physHit& h,const Ogre::Vector3& from,const Ogre::Vector3&,unsigned mask){
 if(mask!=(1u<<7))return;
 ++calls; h.shape=missingShape?NULL:(changeShape&&calls>2?&otherShape:&shape);
 if(mode==1||mode==2){h.hit=true;h.position=Ogre::Vector3(from.x,mode==1?260.f:260.f+(.514f/.819f)*(from.x-18780.21f)+(.253f/.819f)*(from.z-47722.70f),from.z);h.normal=mode==1?Ogre::Vector3(0,1,0):Ogre::Vector3(-.514f,.819f,-.253f);h.distance=from.y-h.position.y;return;}
 // Actual COL013 world capsule: endpoint plus axis toward lower endpoint.
 const double endpoint[]={18791.570265588,242.678533958,47728.289075247};
 const double axis[]={-.54124655696,-.766044497,.346739651995};
 const double radius=mode==3?50.:21.338205338, length=287.12701416;
 if(mode==4){double x=from.x-endpoint[0];if(std::fabs(x)>=radius)return;double y=std::sqrt(radius*radius-x*x);h.hit=true;h.position=Ogre::Vector3(from.x,(float)(endpoint[1]+y),from.z);h.normal=Ogre::Vector3((float)(x/radius),(float)(y/radius),0);h.distance=from.y-h.position.y;return;}
 double last=1e20;
 for(int step=0;step<=300;++step){double y=from.y-step*.25,dx=from.x-endpoint[0],dy=y-endpoint[1],dz=from.z-endpoint[2];
  double along=mode==4?length*.5:dx*axis[0]+dy*axis[1]+dz*axis[2];if(along<0)along=0;if(along>length)along=length;
  double nx=dx-axis[0]*along,ny=dy-axis[1]*along,nz=dz-axis[2]*along,d=std::sqrt(nx*nx+ny*ny+nz*nz)-radius;
  if(d<=0 && last>0){double low=y,high=y+.25;
   for(int j=0;j<30;++j){double mid=(low+high)*.5,yy=mid-endpoint[1],a=dx*axis[0]+yy*axis[1]+dz*axis[2];if(a<0)a=0;if(a>length)a=length;
    double xx=dx-axis[0]*a,zz=dz-axis[2]*a;yy-=axis[1]*a;
    if(std::sqrt(xx*xx+yy*yy+zz*zz)>radius)high=mid;else low=mid;}
   y=(low+high)*.5;dy=y-endpoint[1];along=dx*axis[0]+dy*axis[1]+dz*axis[2];if(along<0)along=0;if(along>length)along=length;
   nx=dx-axis[0]*along;ny=dy-axis[1]*along;nz=dz-axis[2]*along;
   h.hit=true;h.position=Ogre::Vector3(from.x,(float)y,from.z);h.normal=wrongNormal?Ogre::Vector3(0,0,0):Ogre::Vector3((float)(nx/radius),(float)(ny/radius),(float)(nz/radius));h.distance=from.y-h.position.y;return;}
  last=d;
 }
}};
// @PRODUCTION@
int checks=0,failures=0;void Check(bool ok,const char* message){++checks;if(!ok){++failures;printf("FAIL: %s (calls%d)\n",message,calls);}}
int main(){
 const Ogre::Vector3 current(18780.21f,259.86f,47722.70f);const AIFixedGate::Point normal(-.514f,.819f,-.253f);
 Check(ReadPhysicalPlatformCap(&support,current,normal,7),"recorded Drone capsule admits physical proof despite coarse native pocket0");Check(calls<=7,"bounded maximum seven rays");
 calls=0;Check(ReadPhysicalPlatformCap(&support,Ogre::Vector3(18780.7773f,260.4811f,47723.6094f),AIFixedGate::Point(-.4886283f,.8463781f,-.2118642f),7),"previous exact captured Drone position also proves cap");
 calls=0;abortTrace=true;Check(!ReadPhysicalPlatformCap(&support,current,normal,7),"aborted query rejects");abortTrace=false;
 calls=0;missingShape=true;Check(!ReadPhysicalPlatformCap(&support,current,normal,7),"no exact shape rejects");missingShape=false;
 calls=0;changeShape=true;Check(!ReadPhysicalPlatformCap(&support,current,normal,7),"adjacent same-building roof pieces cannot impersonate one cap");changeShape=false;
 calls=0;mode=1;Check(!ReadPhysicalPlatformCap(&support,current,normal,7),"ordinary flat post rejects");
 calls=0;mode=2;Check(!ReadPhysicalPlatformCap(&support,current,normal,7),"ordinary constant-normal ramp rejects");
 calls=0;mode=3;Check(!ReadPhysicalPlatformCap(&support,Ogre::Vector3(18765.87f,283.62f,47715.64f),normal,7),"large curved dome at physically consistent source rejects");
 calls=0;mode=4;Check(!ReadPhysicalPlatformCap(&support,current,AIFixedGate::Point(-.5324f,.8465f,0),7),"arched cylinder ramp has only one curvature axis and rejects");
 calls=0;mode=0;wrongBuilding=true;Check(!ReadPhysicalPlatformCap(&support,current,normal,7),"another building rejects");wrongBuilding=false;
 calls=0;wrongNormal=true;Check(!ReadPhysicalPlatformCap(&support,current,normal,7),"invalid native normal rejects");wrongNormal=false;
 calls=0;Check(!ReadPhysicalPlatformCap(&support,current,normal,32)&&calls==0,"unavailable footing group rejects without trace");
 calls=0;mode=0;Check(!ReadPhysicalPlatformCap(&support,current,AIFixedGate::Point(0,1,0),7)&&calls==0,"flat standing normal never probes");
 printf("Physical platform cap: %s (%d checks)\n",failures?"FAIL":"PASS",checks);return failures?1:0;
}
'''
out=here/'_generated';out.mkdir(exist_ok=True)
source=out/'physical_cap.cpp';source.write_text(fixture.replace('// @PRODUCTION@',code),'utf-8')
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=os.environ.copy();env['PATH']=str(vc/'bin/amd64')+';'+env['PATH'];env['INCLUDE']=str(vc/'include')+';'+str(sdk/'Include');env['LIB']=str(vc/'lib/amd64')+';'+str(sdk/'Lib/x64')
exe=out/'physical_cap.exe'
subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/Od','/I'+str(here.parent),str(source),'/Fe'+str(exe),'/Fo'+str(out/'physical_cap.obj')],cwd=out,env=env,check=True)
subprocess.run([str(exe)],check=True)
