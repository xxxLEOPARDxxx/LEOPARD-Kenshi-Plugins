#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <float.h>
#include <GatePolicy.h>
#include <UnstuckPolicy.h>
namespace Ogre {
struct Vector3 {
    float x,y,z;
    Vector3(float a=0,float b=0,float c=0):x(a),y(b),z(c){}
    Vector3 operator+(const Vector3& b)const{return Vector3(x+b.x,y+b.y,z+b.z);}
    Vector3 operator-(const Vector3& b)const{return Vector3(x-b.x,y-b.y,z-b.z);}
    Vector3 operator-()const{return Vector3(-x,-y,-z);}
    Vector3 operator*(float n)const{return Vector3(x*n,y*n,z*n);}
    float dotProduct(const Vector3& b)const{return x*b.x+y*b.y+z*b.z;}
    float squaredLength()const{return x*x+y*y+z*z;}
};
struct Aabb {Vector3 mCenter,mHalfSize;};
}
struct DoorStuff {
    bool isSetupComplete,broken;
    Ogre::Vector3 getDoorPosInside(){return Ogre::Vector3();}
    Ogre::Vector3 getDoorPosOutside(){return Ogre::Vector3();}
    bool isBroken(){return broken;}
    float getDoorOpenAmount(){return 0.0f;}
};
struct RootObject {};
struct Building:RootObject {
    bool gate,wall,disabled;int outsideGate;
    DoorStuff door;
    struct ConstructionState {bool isComplete;} state;
    struct Orientation {Ogre::Vector3 lateral;Ogre::Vector3 xAxis(){return lateral;}};
    Ogre::Aabb bounds;
    Ogre::Vector3 wallPosition,wallAxis,wallExterior;
    float wallLength;
    Building():gate(false),wall(false),disabled(false),outsideGate(0),wallPosition(-9200,140,27300),
        wallAxis(0,0,1),wallExterior(-1,0,0),wallLength(100){
        door.isSetupComplete=false;door.broken=true;state.isComplete=true;
    }
    Building* isGate(){return gate?this:NULL;}
    Building* isAWall(){return wall?this:NULL;}
    DoorStuff* getDoor(){return gate?&door:NULL;}
    bool isDisabled(){return disabled;}
    ConstructionState* getBuildState(){return &state;}
    Ogre::Vector3 getPosition(){return wallPosition;}
    Ogre::Vector3 getWallEndPosition(){return wallPosition+wallAxis*wallLength;}
    Orientation getOrientation(){Orientation result={wallExterior};return result;}
    int getOutsideGateCode(){return outsideGate;}
    const Ogre::Aabb& getAABB(){return bounds;}
};
typedef Building WallBuilding;
struct StandingHandle {Building* building;operator bool(){return building!=NULL;}Building* getBuilding(){return building;}};
struct HavokCharacter {unsigned currentFace;};
struct CharMovement {
    HavokCharacter* havokCharacter;
    StandingHandle standing;
    float radius;
    float getRadius(){return radius;}
    StandingHandle isStandingOnSomething(){return standing;}
};
struct Character {CharMovement* movement;int amInsideTownWalls(){return 0;}};
struct ZoneMap {
    ZoneMap *neighbour;
    bool loaded,loading,mesh;
    ZoneMap():neighbour(NULL),loaded(true),loading(false),mesh(true){}
    bool isLoadedMT(){return loaded;}bool isBeingLoadedMT(){return loading;}
    bool isANeighbour(ZoneMap* other){return other==neighbour;}
};
struct ZoneManager {
    ZoneMap zone,west;
    bool seam;
    ZoneManager():seam(false){zone.neighbour=&west;west.neighbour=&zone;}
    ZoneMap* getZoneMap(const Ogre::Vector3& p){return seam && p.x<-9216.0f?&west:&zone;}
};
struct NavMesh {
    Ogre::Vector3 candidate;
    unsigned exteriorCalls;
    int sourceCode,targetCode,exteriorResult,sourceFaceResult,requeryResult,alternateCode;
    unsigned sourceKey;
    float requeryShift,gateSplitX;
    bool isLoaded(ZoneMap* tile){return tile->mesh;}bool isGenerating(){return false;}
    int getClosestPoint(const Ogre::Vector3& p,float,float,bool,Ogre::Vector3& out,unsigned& key){out=p;key=sourceKey;return sourceFaceResult;}
    int getClosestExteriorPoint(const Ogre::Vector3&,float radius,float,Ogre::Vector3& out){++exteriorCalls;out=candidate;if(radius==10){out.x+=requeryShift;return requeryResult;}return exteriorResult;}
    int getGateCode(const Ogre::Vector3& p){
        if (gateSplitX && p.x>gateSplitX) return alternateCode;
        return p.y>200?sourceCode:targetCode;
    }
};
template<class T>struct lektor {T object;unsigned count;unsigned size(){return count;}T operator[](unsigned){return object;}};
enum {BUILDING};
struct GameWorld {
    NavMesh* navmesh;ZoneManager* zoneMgr;Building* nearby;
    void getObjectsWithinSphere(lektor<RootObject*>&,const Ogre::Vector3&,float,int,unsigned,void*);
};
lektor<RootObject*>& GateObjects(){static lektor<RootObject*> result;return result;}
float ground=140;
bool slopeConfirmed=true,standingClear=true,barrier=false,water=false;
ZoneMap *unloadAfterBarriers=NULL;
void GameWorld::getObjectsWithinSphere(lektor<RootObject*>& out,const Ogre::Vector3&,float,int,unsigned,void*){
    out.object=nearby;out.count=barrier?128u:nearby?1u:0u;
    if (unloadAfterBarriers) unloadAfterBarriers->loaded=false;
}
bool pocketConfirmed=false,physicalCapConfirmed=false;
int pocketResult=0,physicalCapCalls=0;
bool nativeAgrees=true;
bool floorHit=true,floorAborted=false,floorBuilding=false;
Building* colliderWall=NULL;
int wallShape=1,otherShape=2;
bool entryBlocked=false,wrongShape=false,wallHollow=false;
float wallTop=230.0f;
Ogre::Vector3 colliderOrigin(-9200,0,0),colliderNormal(1,0,0);
float nearFace=-12.0f,farFace=12.0f;
float floorY=140,floorNormalY=1,floorDistance=5;
unsigned floorGroup=2;
AIFixedGate::Point normal(-.8357168f,.5356244f,-.1211773f);
struct physHit {
    bool hit;
    Ogre::Vector3 position,normal;
    float distance;
    Building* building;
    int* shape;
    physHit(bool,bool,float):hit(floorHit),position(0,floorY,0),normal(0,floorNormalY,0),distance(floorDistance),building(NULL),shape(NULL){}
    bool traceWasAborted(){return floorAborted;}
    unsigned group(){return floorGroup;}
    Building* getBuilding(){static Building b;return building?building:(floorBuilding?&b:NULL);}
};
struct UtilityT {
    static float getTerrainHeight(float,float){return ground;}
    static bool getPositionInWater(float,float){return water;}
    static void trace(physHit& hit,const Ogre::Vector3& start,const Ogre::Vector3& direction,unsigned){
        hit.position.x=start.x;hit.position.z=start.z;
        if (!colliderWall) return;
        hit.hit=false;
        hit.building=colliderWall;
        hit.shape=&wallShape;
        if (direction.y<0.0f && start.y>=wallTop){
            hit.hit=true;hit.position.y=wallTop;hit.normal=Ogre::Vector3(0,1,0);
        } else if (!direction.y){
            const float from=(start-colliderOrigin).dotProduct(colliderNormal);
            const float ray=direction.dotProduct(colliderNormal);
            if (!entryBlocked && ((ray>0.0f && from<nearFace) || (ray<0.0f && from>farFace))) {
                const float edge=ray>0.0f?nearFace:farFace;
                hit.hit=true;hit.distance=(edge-from)/ray;hit.normal=-direction;
                if (wrongShape && ray<0.0f) hit.shape=&otherShape;
            } else if (wallHollow) {
                hit.hit=true;hit.distance=2.0f;hit.normal=-direction;
            }
        }
    }
};
AIFixedGate::Point Point(const Ogre::Vector3& p){return AIFixedGate::Point(p.x,p.y,p.z);}
bool ReadPlatformSlope(Building*,const Ogre::Vector3&,AIFixedGate::Point& out,unsigned& group){out=normal;group=7;return slopeConfirmed;}
bool ReadPlatformPocket(NavMesh*,CharMovement*,const AIFixedGate::Point&,int& result){result=pocketConfirmed?1:pocketResult;return pocketConfirmed;}
bool ReadPhysicalPlatformCap(Building*,const Ogre::Vector3&,const AIFixedGate::Point&,unsigned){++physicalCapCalls;return physicalCapConfirmed;}
Building* FindElevatedGate(const Ogre::Vector3&,float){return NULL;}
bool FindRockExit(const Ogre::Vector3&,float,unsigned,Ogre::Vector3&,unsigned&){return false;}
bool NativePositionAgrees(void*,const Ogre::Vector3&){return nativeAgrees;}
bool StandingSpaceClear(const Ogre::Vector3&,float,float){return standingClear;}
// @WALL_SIDE@
// @WALL_INTENT@
// @TRACE_STRUCT@
// @WALL_ESCAPE@
// @PRODUCTION@

int checks=0,failures=0;
void Check(bool value,const char* message){++checks;if(!value){++failures;printf("FAIL: %s\n",message);}}
bool Probe(GameWorld& w,Character& actor,Building& platform,const Ogre::Vector3& p,const char* reason,bool platformRecovery=true,
           WallIntent intent=NoWallIntent,unsigned site=0){
    LandingTrace trace;unsigned sites=0;Ogre::Vector3 landing=p;
    const bool accepted=FindLanding(&w,&actor,&platform,p,site,landing,sites,platformRecovery,trace,intent);
    Check(!strcmp(trace.reason,reason),reason);
    return accepted;
}
int main(){
    NavMesh nav;nav.sourceKey=0x02000307;nav.sourceFaceResult=1;nav.exteriorCalls=0;nav.sourceCode=nav.targetCode=0;nav.exteriorResult=nav.requeryResult=1;nav.requeryShift=0;nav.gateSplitX=0;nav.alternateCode=0;
    nav.candidate=Ogre::Vector3(18740,140,47720);
    ZoneManager manager;GameWorld world={&nav,&manager,NULL};Building platform;
    HavokCharacter navigation={0x02000307};
    CharMovement movement;movement.havokCharacter=&navigation;movement.standing.building=&platform;movement.radius=2.0f;
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
    platform.gate=true;platform.door.isSetupComplete=true;
    Check(!Probe(world,actor,platform,Ogre::Vector3(18778,281,47724),"terrain-height-limit",false),
        "gate recovery retains its14m limit");
    Check(Probe(world,actor,platform,Ogre::Vector3(18778,280,47724),"accepted",false),
        "gate14m boundary stays accepted");
    platform.gate=false;
    Building embeddedWall;embeddedWall.wall=true;
    movement.standing.building=&embeddedWall;
    colliderWall=&embeddedWall;
    nav.candidate=Ogre::Vector3(-9240,140,27344.7f);
    const Ogre::Vector3 embedded(-9204.69f,140,27344.7f);
    Check(Probe(world,actor,embeddedWall,embedded,"accepted",false),
        "physical enclosure in a wall can be escaped at ground level despite a cached native face");
    movement.standing.building=NULL;
    Check(Probe(world,actor,embeddedWall,embedded,"accepted",false,EmbeddedWallOnly),
        "matching solid enclosure can provide its own wall when the movement handle is absent");
    nav.candidate.x=-9165;
    Check(!Probe(world,actor,embeddedWall,embedded,"wall-landing-clearance",false,EmbeddedWallOnly),
        "solid enclosure may only be escaped toward the original side");
    nav.candidate.x=-9240;
    Building joiningWall;joiningWall.wall=true;
    embeddedWall.wallLength=50;
    joiningWall.wallPosition.z+=embeddedWall.wallLength;
    joiningWall.bounds.mCenter=Ogre::Vector3(-9205,155,27345);
    joiningWall.bounds.mHalfSize=Ogre::Vector3(15,20,20);
    world.nearby=&joiningWall;nav.targetCode=27;
    Check(Probe(world,actor,embeddedWall,embedded,"accepted",false,EmbeddedWallOnly),
        "an adjacent wall's AABB may overlap a proven same-side interior wall exit");
    joiningWall.wallPosition.x+=12;
    Check(!Probe(world,actor,embeddedWall,embedded,"barrier",false,EmbeddedWallOnly),
        "an unjoined wall with the same AABB still blocks the interior exit");
    joiningWall.wall=false;joiningWall.gate=true;joiningWall.door.isSetupComplete=true;joiningWall.door.broken=false;
    Check(!Probe(world,actor,embeddedWall,embedded,"barrier",false,EmbeddedWallOnly),
        "wall enclosure cannot bypass a closed gate without a confirmed same side");
    world.nearby=NULL;nav.targetCode=0;
    wrongShape=true;
    Check(!Probe(world,actor,embeddedWall,embedded,"wall-support-unconfirmed",false,EmbeddedWallOnly),
        "overhead and both side hits must come from the same collider");
    LandingTrace unconfirmedWall;Ogre::Vector3 unconfirmedLanding;unsigned unconfirmedSites=0;
    Check(!FindLanding(&world,&actor,NULL,embedded,0,unconfirmedLanding,unconfirmedSites,false,unconfirmedWall)&&
        unconfirmedWall.wallHit&&!strcmp(unconfirmedWall.reason,"wall-support-unconfirmed"),
        "generic off-mesh recovery also needs enclosure proof when a physical wall was hit");
    wrongShape=false;wallHollow=true;
    Check(!Probe(world,actor,embeddedWall,embedded,"wall-support-unconfirmed",false,EmbeddedWallOnly),
        "a tunnel under the same wall collider is not solid enclosure");
    wallHollow=false;standingClear=false;
    Check(!Probe(world,actor,embeddedWall,embedded,"wall-landing-clearance",false,EmbeddedWallOnly),
        "a blocked landing cannot be used to escape an embedded wall");
    standingClear=true;barrier=true;
    Check(!Probe(world,actor,embeddedWall,embedded,"barrier",false,EmbeddedWallOnly),
        "an additional barrier blocks the wall exit");
    barrier=false;
    const Ogre::Vector3 crest(-9200,wallTop,27292.5f);
    nav.candidate=Ogre::Vector3(-9235,140,27292.5f);
    nav.sourceFaceResult=1;
    Check(!Probe(world,actor,embeddedWall,crest,"wall-support-unconfirmed",false,CrestWallOnly),
        "a fresh walkable crest face keeps the retreating actor on a real walkway");
    Check(!Probe(world,actor,embeddedWall,crest,"wall-support-unconfirmed",false,CrestWall),
        "ordinary movement fallback cannot override rejected retreat on a real supported wall walkway");
    nav.sourceFaceResult=0;
    Check(Probe(world,actor,embeddedWall,crest,"accepted",false,CrestWallOnly),
        "an unsupported retreating animal at a small wall endpoint join can use the proven exterior");
    nav.targetCode=27;
    Check(!Probe(world,actor,embeddedWall,crest,"wall-gate-code",false,CrestWallOnly),
        "the elevation fallback cannot enter a walled town");
    nav.targetCode=0;
    wallTop=210;
    nav.candidate=Ogre::Vector3(-9235,140,27320);
    embeddedWall.outsideGate=27;
    LandingTrace unsafeCrest;Ogre::Vector3 crestLanding;unsigned crestSites=0;
    Check(!FindLanding(&world,&actor,NULL,Ogre::Vector3(-9201,wallTop,27320),0,crestLanding,crestSites,false,unsafeCrest,CrestWall)&&
        unsafeCrest.wallHit&&!strcmp(unsafeCrest.reason,"wall-support-unconfirmed"),
        "a rejected physical crest cannot be treated as general off-mesh terrain without its standing handle");
    embeddedWall.outsideGate=0;wallTop=230;
    nav.candidate=Ogre::Vector3(-9155,140,27320);
    nav.sourceCode=-1;nav.targetCode=27;
    Check(Probe(world,actor,embeddedWall,Ogre::Vector3(-9192.42f,wallTop,27320),
        "accepted",false,CrestWallOnly),
        "a stranded animal 7.58 units inside the crest can descend on the same side");
    joiningWall.wall=true;joiningWall.gate=false;joiningWall.wallPosition=Ogre::Vector3(-9200,140,27265);
    joiningWall.wallLength=35;
    movement.standing.building=&joiningWall;
    nav.candidate.z=27300;
    Check(Probe(world,actor,embeddedWall,Ogre::Vector3(-9192.42f,wallTop,27300),
        "accepted",false,CrestWallOnly),
        "native standing support can name the other joined section at a crest seam");
    nav.candidate.z=27320;
    movement.standing.building=NULL;
    nav.gateSplitX=-9160;
    Check(!Probe(world,actor,embeddedWall,Ogre::Vector3(-9192.42f,wallTop,27320),
        "wall-gate-code",false,CrestWallOnly),
        "the native closest-point search cannot switch the interior crest seed's town side");
    nav.gateSplitX=0;
    nav.sourceCode=nav.targetCode=0;
    nav.candidate.z=27292.5f;
    nav.candidate.x=-9165;
    Check(!Probe(world,actor,embeddedWall,crest,"wall-landing-clearance",false,CrestWallOnly),
        "even an exterior code cannot override the measured wall side");
    nav.candidate.x=-9235;
    // Captured patrol actor/candidate, saved wall direction and adjacent
    // zone boundary. The native gate codes and load results are explicit fakes.
    const Ogre::Vector3 patrol(-9204.73f,271.80f,27344.70f);
    Building recordedWall;recordedWall.wall=true;
    recordedWall.wallPosition=Ogre::Vector3(-9203.34f,297.6f,27291.58f);
    recordedWall.wallAxis=Ogre::Vector3(.74378f,0,.66843f);
    recordedWall.wallExterior=Ogre::Vector3(.66843f,0,-.74378f);
    recordedWall.wallLength=39.95f;
    ground=271.97f;wallTop=374.28f;movement.radius=4.0f;
    colliderWall=&recordedWall;colliderOrigin=patrol;colliderNormal=-recordedWall.wallExterior;
    nearFace=-44.92f;farFace=4.47f;
    nav.candidate=Ogre::Vector3(-9214.47f,272.46f,27354.27f);
    manager.seam=true;nav.gateSplitX=-9210.0f;nav.sourceCode=27;
    Check(Probe(world,actor,recordedWall,patrol,"accepted",false),
        "the logged patrol exit uses the grounded seed side when the embedded source has the opposite gate code");
    nav.gateSplitX=-9215.0f;
    Check(!Probe(world,actor,recordedWall,patrol,"wall-gate-code",false),
        "nearest-nav snapping cannot cross the measured ground seed's gate code");
    nav.gateSplitX=0;
    nav.candidate=Ogre::Vector3(-9219.75f,272.46f,27361.41f);
    Check(!Probe(world,actor,recordedWall,patrol,"landing-zone",false),
        "the first seed cannot snap into a different tile");
    Check(Probe(world,actor,recordedWall,patrol,"accepted",false,NoWallIntent,1),
        "later seeds and landings can use the loaded immediate neighbouring tile at the recorded seam");
    manager.west.mesh=false;
    Check(!Probe(world,actor,recordedWall,patrol,"seed-zone",false,NoWallIntent,1),
        "a neighbouring zone without a loaded mesh is unavailable");
    manager.west.mesh=true;manager.west.loading=true;
    Check(!Probe(world,actor,recordedWall,patrol,"seed-zone",false,NoWallIntent,1),
        "a neighbouring zone still loading is unavailable");
    manager.west.loading=false;manager.zone.neighbour=NULL;
    Check(!Probe(world,actor,recordedWall,patrol,"seed-zone",false,NoWallIntent,1),
        "an unrelated loaded zone cannot admit a wall landing");
    manager.zone.neighbour=&manager.west;unloadAfterBarriers=&manager.west;
    Check(!Probe(world,actor,recordedWall,patrol,"zone-changed",false,NoWallIntent,1),
        "the neighbouring tile is checked again after the barrier query");
    unloadAfterBarriers=NULL;manager.west.loaded=true;
    colliderWall=NULL;nativeAgrees=false;
    LandingTrace ordinaryProbe;Ogre::Vector3 ordinaryLanding;unsigned ordinarySites=0;
    Check(!FindLanding(&world,&actor,NULL,patrol,2,ordinaryLanding,ordinarySites,false,ordinaryProbe)&&
        !strcmp(ordinaryProbe.reason,"seed-zone"),"unconfirmed terrain recovery cannot cross a tile seam");
    nativeAgrees=true;manager.seam=false;
    ground=140;movement.radius=2.0f;nav.sourceCode=nav.targetCode=0;
    colliderWall=NULL;
    nav.sourceFaceResult=1;
    movement.standing.building=&platform;
    nav.candidate=Ogre::Vector3(18740,140,47720);
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
