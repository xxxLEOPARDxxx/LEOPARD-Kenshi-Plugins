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
struct HavokCharacter {unsigned currentFace;};
struct CharMovement {
    HavokCharacter* havokCharacter;
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
    int sourceCode,targetCode,exteriorResult,sourceFaceResult,requeryResult;
    unsigned sourceKey;
    float requeryShift;
    bool isLoaded(ZoneMap*){return true;}bool isGenerating(){return false;}
    int getClosestPoint(const Ogre::Vector3& p,float,float,bool,Ogre::Vector3& out,unsigned& key){out=p;key=sourceKey;return sourceFaceResult;}
    int getClosestExteriorPoint(const Ogre::Vector3&,float radius,float,Ogre::Vector3& out){++exteriorCalls;out=candidate;if(radius==10){out.x+=requeryShift;return requeryResult;}return exteriorResult;}
    int getGateCode(const Ogre::Vector3& p){return p.y>200?sourceCode:targetCode;}
};
struct GameWorld {NavMesh* navmesh;ZoneManager* zoneMgr;};
float ground=140;
bool slopeConfirmed=true,standingClear=true,barrier=false,water=false;
bool pocketConfirmed=false,physicalCapConfirmed=false;
int pocketResult=0,physicalCapCalls=0;
bool nativeAgrees=true;
bool floorHit=true,floorAborted=false,floorBuilding=false;
float floorY=140,floorNormalY=1,floorDistance=5;
unsigned floorGroup=2;
AIFixedGate::Point normal(-.8357168f,.5356244f,-.1211773f);
struct physHit {
    bool hit;
    Ogre::Vector3 position,normal;
    float distance;
    physHit(bool,bool,float):hit(floorHit),position(0,floorY,0),normal(0,floorNormalY,0),distance(floorDistance){}
    bool traceWasAborted(){return floorAborted;}
    unsigned group(){return floorGroup;}
    Building* getBuilding(){static Building b;return floorBuilding?&b:NULL;}
};
struct UtilityT {
    static float getTerrainHeight(float,float){return ground;}
    static bool getPositionInWater(float,float){return water;}
    static void trace(physHit& hit,const Ogre::Vector3& start,const Ogre::Vector3&,unsigned){hit.position.x=start.x;hit.position.z=start.z;}
};
AIFixedGate::Point Point(const Ogre::Vector3& p){return AIFixedGate::Point(p.x,p.y,p.z);}
bool ReadPlatformSlope(Building*,const Ogre::Vector3&,AIFixedGate::Point& out,unsigned& group){out=normal;group=7;return slopeConfirmed;}
bool ReadPlatformPocket(NavMesh*,CharMovement*,const AIFixedGate::Point&,int& result){result=pocketConfirmed?1:pocketResult;return pocketConfirmed;}
bool ReadPhysicalPlatformCap(Building*,const Ogre::Vector3&,const AIFixedGate::Point&,unsigned){++physicalCapCalls;return physicalCapConfirmed;}
Building* FindElevatedGate(const Ogre::Vector3&,float){return NULL;}
bool FindRockExit(const Ogre::Vector3&,float,unsigned,Ogre::Vector3&,unsigned&){return false;}
bool NativePositionAgrees(void*,const Ogre::Vector3&){return nativeAgrees;}
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
    NavMesh nav;nav.sourceKey=0x02000307;nav.sourceFaceResult=1;nav.exteriorCalls=0;nav.sourceCode=nav.targetCode=0;nav.exteriorResult=nav.requeryResult=1;nav.requeryShift=0;
    nav.candidate=Ogre::Vector3(18740,140,47720);
    ZoneManager manager;GameWorld world={&nav,&manager};Building platform;
    HavokCharacter navigation={0x02000307};
    CharMovement movement;movement.havokCharacter=&navigation;movement.standing.building=&platform;
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
    normal=AIFixedGate::Point(0,1,0);nav.sourceFaceResult=0;
    Check(Probe(world,actor,platform,original,"accepted"),"an explicit native walkable-face miss can prove a stranded non-steep platform position");
    standingClear=false;Check(!Probe(world,actor,platform,original,"platform-landing-clearance"),"native face miss cannot bypass landing clearance");standingClear=true;
    Check(!Probe(world,actor,platform,upper[1],"terrain-height-limit"),"non-steep explicit-miss recovery cannot use the18m steep-platform limit");
    nav.sourceFaceResult=-1;Check(!Probe(world,actor,platform,original,"support-face-unavailable"),"an unavailable native face query is never a miss");
    nav.sourceFaceResult=1;Check(!Probe(world,actor,platform,original,"support-not-steep"),"an ordinary valid walking face cannot use missing-face recovery");
    // Recorded13.11 candidate: simplified exterior navmesh is0.55m under
    // the independently sampled physical terrain at the same X/Z.
    slopeConfirmed=true;normal=AIFixedGate::Point(-.80f,.589f,-.08f);
    nav.candidate=Ogre::Vector3(18729.38f,134.50f,47725.18f);
    Check(Probe(world,actor,platform,Ogre::Vector3(18773.75f,254.39f,47727.40f),"accepted"),
        "recorded lower guard can land on physically confirmed terrain above simplified navmesh");
    const Ogre::Vector3 guard(18773.75f,254.39f,47727.40f);
    floorHit=false;Check(!Probe(world,actor,platform,guard,"platform-ground-unconfirmed"),"missing collision floor rejects correction");floorHit=true;
    floorAborted=true;Check(!Probe(world,actor,platform,guard,"platform-ground-unconfirmed"),"unavailable collision floor rejects correction");floorAborted=false;
    floorBuilding=true;Check(!Probe(world,actor,platform,guard,"platform-ground-unconfirmed"),"building roof is not terrain");floorBuilding=false;
    floorGroup=14;Check(!Probe(world,actor,platform,guard,"platform-ground-unconfirmed"),"natural scenery is not the terrain collision group");floorGroup=2;
    floorNormalY=.5f;Check(!Probe(world,actor,platform,guard,"platform-ground-unconfirmed"),"steep landing cannot be corrected");floorNormalY=1;
    floorDistance=11;Check(!Probe(world,actor,platform,guard,"platform-ground-unconfirmed"),"unbounded native ray must hit within explicit1m band");floorDistance=5;
    floorY=136;Check(!Probe(world,actor,platform,guard,"platform-ground-unconfirmed"),"floor must agree with independently queried terrain");floorY=140;
    nav.candidate.y=129;Check(!Probe(world,actor,platform,guard,"platform-ground-unconfirmed"),"no correction beyond1m");nav.candidate.y=134.5f;
    nav.requeryResult=-1;Check(!Probe(world,actor,platform,guard,"platform-ground-unconfirmed"),"unavailable exterior requery fails closed");nav.requeryResult=0;
    Check(!Probe(world,actor,platform,guard,"platform-ground-unconfirmed"),"missing exterior requery fails closed");nav.requeryResult=1;
    nav.requeryShift=2;Check(!Probe(world,actor,platform,guard,"platform-ground-unconfirmed"),"different nearby face cannot authorize original landing");nav.requeryShift=0;
    slopeConfirmed=false;normal=AIFixedGate::Point(-.49f,.84638f,-.20f);pocketConfirmed=true;
    nav.candidate=Ogre::Vector3(18743.75f,140,47723.61f);
    Check(Probe(world,actor,platform,Ogre::Vector3(18780.78f,260.48f,47723.61f),"accepted"),
        "captured rounded strut cap requires independent native local-pocket proof");
    nativeAgrees=false;Check(!Probe(world,actor,platform,Ogre::Vector3(18780.78f,260.48f,47723.61f),"support-not-steep"),
        "stale native position cannot authorize recovery of different physical footing");nativeAgrees=true;
    nav.sourceKey=0x02000306;Check(!Probe(world,actor,platform,Ogre::Vector3(18780.78f,260.48f,47723.61f),"support-not-steep"),
        "nearby physical query must match the current native face");nav.sourceKey=0x02000307;
    pocketConfirmed=false;Check(!Probe(world,actor,platform,Ogre::Vector3(18780.78f,260.48f,47723.61f),"support-not-steep"),
        "rounded footing without either native island or physical convex-cap proof rejects");
    physicalCapConfirmed=true;
    const Ogre::Vector3 drone(18780.21f,259.86f,47722.70f);
    normal=AIFixedGate::Point(-.514f,.819f,-.253f);
    Check(Probe(world,actor,platform,drone,"accepted"),"recorded Drone pocket0 may use independently proven small physical cap");
    pocketResult=-1;const int previousPhysicalCalls=physicalCapCalls;
    Check(!Probe(world,actor,platform,drone,"support-not-steep")&&physicalCapCalls==previousPhysicalCalls,"busy or faulted nav query cannot fall back to physical cap");pocketResult=0;
    nativeAgrees=false;Check(!Probe(world,actor,platform,drone,"support-not-steep"),"physical cap cannot bypass native position agreement");nativeAgrees=true;
    water=true;Check(!Probe(world,actor,platform,drone,"landing-water"),"physical cap cannot bypass dry landing");water=false;
    barrier=true;Check(!Probe(world,actor,platform,drone,"barrier"),"physical cap cannot bypass barrier checks");barrier=false;
    standingClear=false;Check(!Probe(world,actor,platform,drone,"platform-landing-clearance"),"physical cap cannot bypass standing clearance");standingClear=true;
    Check(!Probe(world,actor,platform,Ogre::Vector3(drone.x,280.1f,drone.z),"terrain-height-limit"),"physical cap retains14m drop limit");
    printf("AIFixed extracted landing flow: %s (%d checks; native queries faked explicitly)\n",failures?"FAIL":"PASS",checks);
    return failures?1:0;
}
