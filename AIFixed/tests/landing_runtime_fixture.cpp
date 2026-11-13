#include <cstdio>
#include <cstring>
#include <cmath>
#include <float.h>
#include <GatePolicy.h>
#include <UnstuckPolicy.h>
namespace Ogre {
struct Vector3 {
    float x,y,z;
    Vector3(float a=0,float b=0,float c=0):x(a),y(b),z(c){}
    Vector3 operator+(const Vector3& b)const{return Vector3(x+b.x,y+b.y,z+b.z);}
    Vector3 operator-(const Vector3& b)const{return Vector3(x-b.x,y-b.y,z-b.z);}
    Vector3 operator*(float n)const{return Vector3(x*n,y*n,z*n);}
    float squaredLength()const{return x*x+y*y+z*z;}
};
}
struct DoorStuff {
    bool isSetupComplete;
    Ogre::Vector3 getDoorPosInside(){return Ogre::Vector3();}
    Ogre::Vector3 getDoorPosOutside(){return Ogre::Vector3();}
};
struct Building {bool gate;DoorStuff door;Building():gate(false){door.isSetupComplete=false;}Building* isGate(){return gate?this:NULL;}DoorStuff* getDoor(){return gate?&door:NULL;}};
struct StandingHandle {Building* building;operator bool(){return building!=NULL;}};
struct CharMovement {
    void* havokCharacter;
    StandingHandle standing;
    float getRadius(){return 2.0f;}
    StandingHandle isStandingOnSomething(){return standing;}
};
struct Character {CharMovement* movement;int amInsideTownWalls(){return 0;}};
struct ZoneMap {bool isLoadedMT(){return true;}bool isBeingLoadedMT(){return false;}};
ZoneMap zone;
struct ZoneManager {ZoneMap* getZoneMap(const Ogre::Vector3&){return &zone;}};
struct NavMesh {
    Ogre::Vector3 candidate;
    unsigned exteriorCalls;
    int sourceCode,targetCode,exteriorResult;
    bool isLoaded(ZoneMap*){return true;}bool isGenerating(){return false;}
    int getClosestPoint(const Ogre::Vector3&,float,float,bool,Ogre::Vector3&,unsigned&){return 0;}
    int getClosestExteriorPoint(const Ogre::Vector3&,float,float,Ogre::Vector3& out){++exteriorCalls;out=candidate;return exteriorResult;}
    int getGateCode(const Ogre::Vector3& p){return p.y>200?sourceCode:targetCode;}
};
struct GameWorld {NavMesh* navmesh;ZoneManager* zoneMgr;};
float ground=140;
bool slopeConfirmed=true,standingClear=true,barrier=false,water=false;
AIFixedGate::Point normal(-.8357168f,.5356244f,-.1211773f);
struct UtilityT {
    static float getTerrainHeight(float,float){return ground;}
    static bool getPositionInWater(float,float){return water;}
};
AIFixedGate::Point Point(const Ogre::Vector3& p){return AIFixedGate::Point(p.x,p.y,p.z);}
bool ReadPlatformSlope(Building*,const Ogre::Vector3&,AIFixedGate::Point& out){out=normal;return slopeConfirmed;}
Building* FindElevatedGate(const Ogre::Vector3&,float){return NULL;}
bool FindRockExit(const Ogre::Vector3&,float,unsigned,Ogre::Vector3&,unsigned&){return false;}
bool NativePositionAgrees(void*,const Ogre::Vector3&){return true;}
bool StandingSpaceClear(const Ogre::Vector3&,float,float){return standingClear;}
bool CrossesBarrier(GameWorld*,const Ogre::Vector3&,const Ogre::Vector3&,Building*,bool){return barrier;}
// @TRACE_STRUCT@
// @PRODUCTION@

int checks=0,failures=0;
void Check(bool value,const char* message){++checks;if(!value){++failures;printf("FAIL: %s\n",message);}}
bool Probe(GameWorld& w,Character& actor,Building& platform,const Ogre::Vector3& p,const char* reason,bool platformRecovery=true){
    LandingTrace trace;unsigned sites=0;Ogre::Vector3 landing=p;
    const bool accepted=FindLanding(&w,&actor,&platform,p,0,landing,sites,platformRecovery,trace);
    Check(!strcmp(trace.reason,reason),reason);
    return accepted;
}
int main(){
    NavMesh nav;nav.exteriorCalls=0;nav.sourceCode=nav.targetCode=0;nav.exteriorResult=1;
    nav.candidate=Ogre::Vector3(18740,140,47720);
    ZoneManager manager;GameWorld world={&nav,&manager};Building platform;
    CharMovement movement;movement.havokCharacter=NULL;movement.standing.building=&platform;
    Character actor={&movement};
    const Ogre::Vector3 original(18778,258.5387f,47723.859f);
    Check(Probe(world,actor,platform,original,"accepted"),"captured lower guard height has a permitted dry downhill landing when all native queries confirm it");
    // Actual new actor coordinates and authenticated base terrain samples.
    // Walkability, slope, town side and clearance are EXPLICIT native fakes.
    const Ogre::Vector3 upper[]={Ogre::Vector3(18762.498f,299.07416f,47775.023f),
        Ogre::Vector3(18749.109f,315.03497f,47782.895f),Ogre::Vector3(18754.861f,305.96454f,47777.359f)};
    for(unsigned i=0;i<3;++i){
        nav.candidate=Ogre::Vector3(upper[i].x-38.0f,139.96796f,upper[i].z-4.0f);
        Check(Probe(world,actor,platform,upper[i],"accepted"),
            "each saved upper actor can use a native-confirmed safe platform landing within18m");
    }
    nav.candidate=Ogre::Vector3(18740,140,47720);
    Check(Probe(world,actor,platform,Ogre::Vector3(18778,320,47724),"accepted"),
        "confirmed steep-platform18m boundary is admitted");
    const unsigned calls=nav.exteriorCalls;
    Check(!Probe(world,actor,platform,Ogre::Vector3(18778,320.02f,47724),"terrain-height-limit"),
        "even a confirmed steep platform cannot exceed18m");
    Check(nav.exteriorCalls==calls,"excessive height rejection precedes exterior navmesh search");
    platform.gate=true;
    Check(!Probe(world,actor,platform,Ogre::Vector3(18778,281,47724),"terrain-height-limit",false),
        "gate recovery retains its14m limit");
    Check(Probe(world,actor,platform,Ogre::Vector3(18778,280,47724),"accepted",false),
        "gate14m boundary stays accepted");
    platform.gate=false;
    LandingTrace rockTrace;Ogre::Vector3 rockLanding;unsigned rockSites=0;
    Check(!FindLanding(&world,&actor,NULL,Ogre::Vector3(18778,221,47724),0,rockLanding,rockSites,false,rockTrace)&&
        !strcmp(rockTrace.reason,"terrain-height-limit"),"ordinary rock/off-mesh recovery retains its8m height limit");
    standingClear=false;Check(!Probe(world,actor,platform,original,"platform-landing-clearance"),"blocked landing cannot be accepted");standingClear=true;
    water=true;Check(!Probe(world,actor,platform,original,"landing-water"),"water cannot be accepted");water=false;
    nav.targetCode=4;Check(!Probe(world,actor,platform,original,"platform-gate-code"),"different town side cannot be accepted");nav.targetCode=0;
    barrier=true;Check(!Probe(world,actor,platform,original,"barrier"),"a structure across the corridor cannot be ignored");barrier=false;
    nav.candidate.x=18810;Check(!Probe(world,actor,platform,original,"platform-uphill"),"wrong slope side cannot be accepted");nav.candidate.x=18740;
    nav.candidate.x=18700;Check(!Probe(world,actor,platform,original,"landing-distance-height-surface"),"a farther point cannot exceed6m");nav.candidate.x=18740;
    slopeConfirmed=false;Check(!Probe(world,actor,platform,original,"support-not-steep"),"ordinary posts cannot enter platform recovery");
    printf("AIFixed extracted landing flow: %s (%d checks; native queries faked explicitly)\n",failures?"FAIL":"PASS",checks);
    return failures?1:0;
}
