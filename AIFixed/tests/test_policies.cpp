#include <cstdio>
#include <limits>
#include "../RestPolicy.h"
#include "../GatePolicy.h"
#include "../UnstuckPolicy.h"
int checks=0,failures=0;
void Check(bool value,const char* name){++checks;if(!value){++failures;printf("FAIL: %s\n",name);}}
int main()
{
    {
        using namespace AIFixedUnstuck;
        Observation watch;
        const AIFixedGate::Point at(100,200,300);
        Check(!watch.Sample(at, 10.0, 1000), "first observation starts tracking");
        for (unsigned i=1; i<16; ++i)
            Check(!watch.Sample(at, 10.0+i*.25, 1000+i*1000), "less than four game hours");
        Check(watch.Sample(at, 14.0, 17000), "four continuous game hours");
        watch.Probed(false);
        Check(!watch.Sample(at, 14.04, 17400), "neighbor probes are spaced apart");
        Check(watch.Sample(at, 14.06, 17600), "next neighbor is tried within minutes");
        watch.Probed(true);
        Check(!watch.Sample(at, 14.09, 17900), "exhausted sites back off before retrying");
        Check(!watch.Sample(AIFixedGate::Point(107,200,300), 14.1, 18000), "progress restarts the timer");
        Check(!watch.Sample(at, 18.1, 19000), "unobserved game time is discarded");
        Check(!watch.Sample(at, 18.2, 34000), "unobserved real time is discarded");
        Check(!watch.Sample(at, 1.0, 35000), "clock rewind starts new observation");
        Check(LandingAllowed(at, AIFixedGate::Point(102,126,300), 126), "nearby terrain landing below a gate");
        Check(LandingAllowed(at, AIFixedGate::Point(140,200,300), 200), "nearby sideways recovery from a rock");
        Check(!LandingAllowed(at, AIFixedGate::Point(161,126,300), 126), "no wide horizontal jump");
        Check(!LandingAllowed(at, AIFixedGate::Point(102,126,300), 112), "no placement on a building above terrain");
        Check(!LandingAllowed(at, AIFixedGate::Point(100,110,300), 110), "no excessive drop");
        Check(LandingAllowed(AIFixedGate::Point(-8180,252.9f,28086.4f),
            AIFixedGate::Point(-8156.3f,127.3f,28077.2f),127.3f,14.0f),
            "stranded iron hound can leave a tall defensive wall");
        Check(LandingAllowed(AIFixedGate::Point(-8918,281.4f,28656.7f),
            AIFixedGate::Point(-8914.5f,207.1f,28665),207.6f,14.0f),
            "gate platform is a lower walking surface");
        Check(!LandingAllowed(at, at, 200), "no zero-distance teleport");
        Check(EntersSolid(36.0f, -.7f, 100.0f), "rock face points outward between probe start and NPC");
        Check(!EntersSolid(101.0f, -.7f, 100.0f), "a face beyond the NPC does not enclose it");
        Check(!EntersSolid(36.0f, .7f, 100.0f), "an exit face is not an entrance into a solid");
        Check(!EntersSolid(0.0f, -.7f, 100.0f), "initial overlap is not an entrance into a solid");
        Check(OpposedRockFaces(0xbfu), "three axes enclose the live ore NPC despite missing the south ray");
        Check(!OpposedRockFaces(0x1fu), "one complete axis does not establish rock enclosure");
        Check(!OpposedRockFaces(0x0fu), "nearby faces on one side do not establish enclosure");
        Check(LandingAllowed(at,AIFixedGate::Point(175,200,300),200,8.0f,8.0f),
            "an enclosed actor can reach a clear spot beyond the normal recovery distance");
        Check(!LandingAllowed(at,AIFixedGate::Point(181,200,300),200,8.0f,8.0f),
            "even a proven ore exit respects the extended recovery limit");
        Check(AboveGateDeck(281.3f,168.1f,207.6f), "live unsupported buyer is high above the lower gate platform");
        Check(!AboveGateDeck(207.8f,168.1f,207.6f), "actor already on the gate platform is excluded");
        Check(!AboveGateDeck(340.0f,168.1f,207.6f), "unsupported recovery cannot drop from arbitrarily high above terrain");
        Observation guard;
        for (unsigned i=0; i<=16; ++i)
            Check(!guard.SampleGuard(at, 30.0+i*.25, 1000+i*1000, false, false), "a guard at a valid post does not start recovery");
        Check(!guard.SampleGuard(at, 35.0, 20000, true, false), "an outstanding path request starts observation but does not prove failure");
        for (unsigned i=1; i<16; ++i)
            Check(!guard.SampleGuard(at, 35.0+i*.25, 20000+i*1000, i%4<=1, i%4==1), "post and no-goal cycles preserve but do not shorten the four-hour wait");
        Check(guard.SampleGuard(at, 39.0, 36000, true, false), "fresh failure survives a resumed guard request after four stationary hours");
        Check(!guard.SampleGuard(at, 39.5, 37000, false, false), "old failure cannot move a guard who has returned to a post");
        Check(!guard.SampleGuard(AIFixedGate::Point(106,200,300), 39.75, 38000, false, true), "movement discards stale failed guard intent");
        Observation staleGuard;
        Check(!staleGuard.SampleGuard(at, 40.0, 40000, true, true), "an active failed guard request starts its own four-hour watch");
        for (unsigned i=1; i<=20; ++i)
            Check(!staleGuard.SampleGuard(at, 40.0+i*.25, 40000+i*1000, false, true),
                "a stale failed path retained during no-goal idle never renews failure evidence");
        Check(staleGuard.guardFailure==40.0, "no-goal samples preserve the original failure timestamp");
        const AIFixedGate::Point slope(-.8357f,.5356f,-.1212f);
        Check(GuardLeavesSlope(at,AIFixedGate::Point(88,82,297),slope), "a downhill landing leaves the observed platform slope");
        Check(!GuardLeavesSlope(at,AIFixedGate::Point(112,82,303),slope), "an uphill landing cannot move through a platform");
        Check(!GuardLeavesSlope(at,AIFixedGate::Point(88,82,297),AIFixedGate::Point(0,1,0)), "ordinary level guard posts are never eligible");
        Check(LeavesSupportingWall(8.9f, -14.4f, -1.0f, true), "wall top can drop to confirmed exterior");
        Check(!LeavesSupportingWall(8.9f, -14.4f, -1.0f, false), "no cross-wall landing without exterior proof");
        Check(!LeavesSupportingWall(-8.9f, 14.4f, -1.0f, true), "never drop from wall top into enclosure");
        Check(!LeavesSupportingWall(22.0f, -14.4f, -1.0f, true), "preserve side away from wall center");
        Check(LeavesSupportingWall(8.9f, 54.9f, -1.0f, false), "drop away on current side");
        Check(PastSameWallEnd(-19.7f, -43.3f, 39.8f), "exit past adjacent wall end despite overlapping bounds");
        Check(!PastSameWallEnd(-19.7f, 8.0f, 39.8f), "cannot move along wall end into its span");
        Check(!PastSameWallEnd(-2.0f, -43.3f, 39.8f), "preserve clearance at a wall join");
        Check(OverlapsStructure(at,AIFixedGate::Point(140,200,300),AIFixedGate::Point(120,200,300),
            AIFixedGate::Point(3,10,30)), "wall blocks sideways recovery");
        Check(!OverlapsStructure(at,AIFixedGate::Point(100,130,300),AIFixedGate::Point(120,200,300),
            AIFixedGate::Point(3,10,30)), "landing on the same side can clear the wall");
    }
    using namespace AIFixedRest;
    Health healthy;healthy.Part(false,false,false,1,0,100,0,100);
    Check(NutritionAllows(false,1),"native normal nutrition permits resting");
    Check(!NutritionAllows(false,.75f),"native hunger penalty gives feeding priority");
    Check(!NutritionAllows(false,std::numeric_limits<float>::quiet_NaN()),"invalid nutrition fails closed");
    Check(NutritionAllows(true,0),"skeletons do not require food");
    SimulationClock clock;unsigned stamp=0;
    Check(clock.Sample(100,false,stamp)&&stamp==1,"simulation clock anchors without charging previous world age");
    Check(clock.Sample(101,false,stamp)&&stamp==1001,"simulation clock counts actual native time progression");
    Check(!clock.Sample(3701,true,stamp)&&stamp==1001,"paused native clock change does not consume a deadline");
    Check(clock.Sample(3702,false,stamp)&&stamp==1001,"first resume gap is excluded");
    Check(clock.Sample(3703,false,stamp)&&stamp==2001,"subsequent active simulation advances normally");
    Check(clock.Sample(10,false,stamp)&&stamp==2001,"native clock rewind reanchors without expiring orders");
    Check(!clock.Sample(std::numeric_limits<double>::quiet_NaN(),false,stamp),"invalid native clock fails closed");
    Check(clock.Sample(10000,false,stamp)&&stamp==2001,"recovery from invalid clock does not charge unknown interval");
    Check(!clock.Sample((std::numeric_limits<double>::max)(),false,stamp),"finite clock overflow fails closed before timestamp conversion");
    clock.Reset();Check(clock.Sample(0,false,stamp)&&stamp==1,"world reset restores sentinel-safe timer origin");
    clock.Sample(4294967.290,false,stamp);const unsigned beforeWrap=stamp;
    clock.Sample(4294967.310,false,stamp);
    Check(unsigned(stamp-beforeWrap)>=19&&unsigned(stamp-beforeWrap)<=21,"simulation timestamp wrap preserves short unsigned deadlines");
    Check(Needed(healthy,true,true,true,.9f,.9f,50)==None,"healthy standing worker is not sent to bed");
    Health flesh;flesh.Part(false,false,false,.6f,0,60,40,100);
    Check(Needed(flesh,true,true,true,.9f,.9f,50)==Sleep,"bandaged flesh uses ordinary sleep");
    Health bleeding;bleeding.Part(false,false,false,.6f,0,60,0,100);
    Check(Needed(bleeding,true,true,true,.9f,.9f,50)==None,"untreated wounds wait for medical care");
    Health mixed=flesh;mixed.Part(true,false,false,.2f,70,20,0,100);
    Check(Needed(mixed,true,true,true,.9f,.9f,50)==Sleep,"human prosthetics do not select repair before flesh care");
    mixed.fleshRatio=1;
    Health lowerWake=mixed;lowerWake.fleshRatio=.95f;
    Check(Needed(lowerWake,false,true,true,.9f,.9f,50,.9f)==Repair,"repair transition respects configured flesh completion threshold");
    Check(Complete(mixed,Sleep,.99f),"ordinary bed completion ignores unrepairable prosthetics");
    Check(!Complete(mixed,Repair,.99f),"robotic health and wear both gate repair completion");
    Check(Needed(mixed,false,true,false,.9f,.9f,50)==Repair,"wear repair remains independent module");
    mixed.wear=0;
    Check(Needed(mixed,false,true,false,.9f,.9f,50)==None,"robotic damage alone does not enable disabled health module");
    Check(Needed(mixed,false,false,true,.9f,.9f,50)==Repair,"robotic damage can trigger health repair without wear");
    mixed.robotRatio=1;mixed.robotMissing=0;mixed.wear=.4f;
    Check(Complete(mixed,Repair,.99f),"small completion epsilon permits repaired limbs");
    mixed.wear=2;
    Check(!Complete(mixed,Repair,.99f),"nonzero significant wear cannot finish repair");
    Health highHP;highHP.Part(true,false,false,.995f,0,298.5f,0,300);
    Check(!Complete(highHP,Repair,.99f),"high maximum robotic HP does not relax absolute completion epsilon");
    Health stunned;stunned.Part(true,false,false,1,0,300,0,300,1);
    Check(!Complete(stunned,Repair,.99f),"prosthetic stun damage must also recover");
    Health almost;almost.Part(true,false,false,.998f,0,299.6f,0,300,0);
    Check(Complete(almost,Repair,.99f),"absolute half HP epsilon matches existing repair guarantee");
    Health missing;missing.Part(true,true,false,0,90,0,0,0);
    Check(Needed(missing,false,true,true,.9f,.9f,50)==None,"missing limb is not an endless repair target");
    Health invalid;invalid.Part(false,false,false,std::numeric_limits<float>::quiet_NaN(),0,100,0,100);
    Check(Needed(invalid,true,true,true,.9f,.9f,50)==None,"invalid medical data fails closed");
    Bed bed;bed.properFunction=bed.furniture=bed.useable=bed.free=bed.powered=bed.own=bed.sameTown=bed.sameFloor=true;
    bed.distanceSq=10000;
    Check(Allowed(bed,Repair,true,100),"bed at radius boundary is accepted");
    Check(!Allowed(bed,Repair,true,99),"distant repair bed cannot cause continent travel");
    bed.properFunction=false;Check(!Allowed(bed,Sleep,true,100),"barrel or wrong building function rejected");bed.properFunction=true;
    bed.furniture=false;Check(!Allowed(bed,Sleep,true,100),"function flag without furniture rejected");bed.furniture=true;
    bed.useable=false;Check(!Allowed(bed,Sleep,true,100),"unusable modded furniture rejected");bed.useable=true;
    bed.sameFloor=false;Check(!Allowed(bed,Sleep,true,100),"wrong floor rejected");bed.sameFloor=true;
    bed.sameTown=false;Check(!Allowed(bed,Sleep,true,100),"different town rejected");bed.sameTown=true;
    bed.own=false;bed.allied=true;
    Check(!Allowed(bed,Sleep,true,100),"own beds only excludes allied bed");
    Check(Allowed(bed,Sleep,false,100),"optional allied beds accepted");bed.own=true;
    bed.powered=false;Check(!Allowed(bed,Repair,true,100)&&Allowed(bed,Sleep,true,100),"repair power required only for repair");bed.powered=true;
    bed.free=false;Check(!Allowed(bed,Sleep,true,100),"occupied or reserved slot rejected");
    Check(!IntentAllows(true,false,false),"manual travel / NeedSleep orders take priority");
    Check(!IntentAllows(false,true,true),"cancelled injury episode cannot reissue sleep");
    Check(IntentAllows(true,true,false),"exact owned rest order can complete");
    using namespace AIFixedGate;
    Point in(-2,0,0),out(2,0,0),from(-4,0,0),to(4,0,0);
    Check(Crosses(from,to,in,out,8),"gate corridor crossing selected");
    Check(WorldUnits(50.0f)==500.0f,"gate search radius uses native units");
    Check(Crosses(Point(-8857.8f,207.9f,28705.3f),Point(-9314.8f,163.0f,28867.9f),
        Point(-8908.4f,207.6f,28704.4f),Point(-8880.9f,207.7f,28692.6f),WorldUnits(8.0f)),
        "live base approach crosses the gate within the movement corridor");
    Check(!Crosses(from,Point(-8,0,0),in,out,8),"same side movement does not open gate");
    Check(!Crosses(Point(-4,0,20),Point(4,0,20),in,out,8),"unrelated nearby gate rejected");
    Check(!Crosses(Point(-4,20,0),to,in,out,8),"gate on different floor rejected");
    Check(!Crosses(from,to,in,in,8),"invalid native gate direction rejected");
    Check(!Ready(1000,800,500,0),"click storm quiet period enforced");
    Check(!Ready(1000,0,900,0),"stationary sample duration enforced");
    Check(!Ready(1000,0,500,1),"moving actor not interrupted");
    Check(Ready(1000,0,500,.25f),"stalled actor eligible after quiet period");
    Check(Ready(200,0xffffff00u,0xffffff00u,0),"tick wrap uses unsigned time");
    Check(!Expired(30000,0)&&Expired(30001,0),"gate episode has bounded lifetime");
    printf("AIFixed policies: %s (%d checks)\n",failures?"FAIL":"PASS",checks);
    return failures?1:0;
}
