// Included inside AIFixed's anonymous namespace.
namespace Unstuck
{
    enum { PhaseCount=32, BatchSize=16, HistorySize=2048, TickMs=125 };

    struct Identity
    {
        unsigned type, container, containerSerial, index, serial;
        Identity():type(0),container(0),containerSerial(0),index(0),serial(0){}
        explicit Identity(const hand& h):type(h.type),container(h.container),
            containerSerial(h.containerSerial),index(h.index),serial(h.serial){}
        hand Handle() const { return hand(index,serial,static_cast<itemType>(type),container,containerSerial); }
        bool operator==(const Identity& other) const
        {
            return type==other.type && index==other.index && serial==other.serial &&
                container==other.container && containerSerial==other.containerSerial;
        }
        unsigned Hash() const
        {
            unsigned value=(index*0x9e3779b9u)^(container*0x85ebca6bu)^serial^(containerSerial*31u);
            value=(value^(value>>16))*0x7feb352du;
            return value^(value>>15);
        }
    };

    struct Sample
    {
        Identity actor;
        bool used;
        Sample():used(false){}
    };

    struct History
    {
        Identity actor, subject;
        TaskType goal;
        AIFixedGate::Point destination;
        AIFixedUnstuck::Observation watch;
        unsigned probeSite;
        History():goal(NULL_TASK),probeSite(0){}
    };

    // The movement hook can run on another thread. It only publishes identity.
    // All queries, history, and relocation stay in the main world update.
    SRWLOCK samplesLock=SRWLOCK_INIT;
    volatile LONG phase=-1;
    volatile LONG faulted=0;
    Sample samples[BatchSize];
    History history[HistorySize];
    void (*originalMovement)(CharMovement*,float)=NULL;
    bool ready=false;
    LONG generation=-1;
    DWORD lastTick=0, activeMs=0, lastProbeMs=0;
    unsigned nextPhase=0;

    void Fault()
    {
        InterlockedExchange(&phase,-1);
        if (!InterlockedExchange(&faulted,1))
            ErrorLog("AIFixed: stuck NPC recovery suspended after a native access fault");
    }

    void Publish(CharMovement* movement)
    {
        const LONG observed=phase;
        if (observed<0 || g_loading || !movement || movement->movementMode==MOVE_COMBAT) return;
        const Character* c=movement->character;
        if (!c || c->movement!=movement || !c->isPhysicalMode || c->handle.type!=CHARACTER) return;
        const Identity actor(c->handle);
        const unsigned hash=actor.Hash();
        if ((hash&(PhaseCount-1))!=static_cast<unsigned>(observed&(PhaseCount-1))) return;
        if (!TryAcquireSRWLockExclusive(&samplesLock)) return;
        if (phase==observed && !g_loading)
        {
            const unsigned first=((hash>>5)^(hash>>17)*static_cast<unsigned>(observed>>5))&(BatchSize-1);
            for (unsigned i=0;i<4;++i)
            {
                Sample& slot=samples[(first+i)&(BatchSize-1)];
                if (slot.used)
                {
                    if (slot.actor==actor) break;
                    continue;
                }
                slot.actor=actor;
                slot.used=true;
                break;
            }
        }
        ReleaseSRWLockExclusive(&samplesLock);
    }

    void Movement_hook(CharMovement* movement,float time)
    {
        originalMovement(movement,time);
        if (phase<0 || faulted) return;
        __try { Publish(movement); }
        __except(EXCEPTION_EXECUTE_HANDLER) { Fault(); }
    }

    void Transfer(LONG following,Sample* output)
    {
        AcquireSRWLockExclusive(&samplesLock);
        for (unsigned i=0;i<BatchSize;++i)
        {
            if (output) output[i]=samples[i];
            samples[i].used=false;
        }
        if (following>=0 && (g_loading || g_loadGeneration!=generation || faulted)) following=-1;
        InterlockedExchange(&phase,following);
        ReleaseSRWLockExclusive(&samplesLock);
    }

    void Suspend() { Transfer(-1,NULL); }

    bool FollowGoal(TaskType task)
    {
        return task==FOLLOW_SQUADLEADER || task==FOLLOW_PLAYER_ORDER ||
            task==STAY_CLOSE_TO_TARGET || task==STAY_CLOSE_TO_TARGET_ANIMAL;
    }

    bool TravelGoal(TaskType task)
    {
        return FollowGoal(task) || task==MOVE_CUS_ORDERED || task==GET_NEAR_TO ||
            task==PATROL || task==PATROL_TOWN || task==WANDERING_TRADER ||
            task==TRAVEL_TO_TARGET_TOWN || task==TRAVEL_TO_TARGET_TOWN_FAST ||
            task==RETREAT_HOMETOWN || task==RUN_AWAY_HOMETOWN;
    }

    bool MovingDestination(TaskType task)
    {
        return FollowGoal(task) || task==PATROL || task==PATROL_TOWN ||
            task==RETREAT_HOMETOWN || task==RUN_AWAY_HOMETOWN;
    }

    bool Fortification(Building* support)
    {
        return support && (support->isGate() || support->isAWall());
    }

    bool IsWallFollower(Character* c,CharMovement* movement,TaskType task)
    {
        if (task!=FOLLOW_SQUADLEADER || !c->isAnimal()) return false;
        Building* support=movement->isStandingOnSomething().getBuilding();
        return support && support->isAWall();
    }

    bool CanTravel(Character* c,CharMovement* movement,TaskType task)
    {
        if (!movement || !movement->havokCharacter ||
            movement->animationOverride || movement->isTrackingAnimationMode() || movement->isIndoors() ||
            !c->isPhysicalMode || c->isPlayerCharacter() ||
            c->isDead() || c->isDown() || c->isUnconcious() || c->isRagdoll() ||
            c->isCrippled() || c->isBeingCarried() || c->isCarryingSomething ||
            c->inSomething!=IN_NOTHING || c->inWhat || c->isUsingTurret ||
            c->isChained || c->isInCombatMode(true,true) ||
            c->getWaterLevel()>=WaterState::THIGH_DEEP_WATER) return false;
        if (movement->movementMode!=MOVE_NORMAL &&
            (movement->movementMode!=MOVE_DIRECTION || !IsWallFollower(c,movement,task))) return false;
        MedicalSystem* med=c->getMedical();
        if (!med || med->isCrippled()) return false;
        for (int i=RobotLimbs::LEFT_LEG;i<=RobotLimbs::RIGHT_LEG;++i)
        {
            const LimbState limb=med->getLimbState(static_cast<RobotLimbs::Limb>(i));
            if (limb==LIMB_STUMP || limb==LIMB_CRUSHED) return false;
        }
        return true;
    }

    AIFixedGate::Point Point(const Ogre::Vector3& v) { return AIFixedGate::Point(v.x,v.y,v.z); }

    bool PathBlocked(HavokCharacter* navigation,bool includeWaiting)
    {
        if (navigation->getState()!=HavokCharacter::IDLE) return false;
        const HavokCharacter::PathState state=navigation->getPathState();
        return state==HavokCharacter::FAILED || state==HavokCharacter::INCOMPLETE ||
            state==HavokCharacter::BROKEN || (includeWaiting && state==HavokCharacter::WAITING);
    }

    bool WallFollowerBlocked(Character* c,CharMovement* movement,Character* leader,
                             Building* wall,const Ogre::Vector3& here)
    {
        if (!c->isAnimal() || !wall || !wall->isAWall() || !leader ||
            !leader->movement || !leader->movement->havokCharacter) return false;
        const HavokCharacter::CharacterState state=movement->havokCharacter->getState();
        const bool leaderBelow=here.y-leader->getPosition().y>=50.0f;
        if (state==HavokCharacter::GOAL_REACHED) return leaderBelow;
        // Animal follow AI switches to direct steering when it gets close.
        // Four hours without progress and an elevated wall are still required.
        const bool direct=movement->movementMode==MOVE_DIRECTION && state==HavokCharacter::MANUAL;
        if (state!=HavokCharacter::IDLE && !direct) return false;
        return PathBlocked(movement->havokCharacter,false) || leaderBelow ||
            (leader->movement->isStandingOnSomething().getBuilding()==wall &&
                PathBlocked(leader->movement->havokCharacter,true));
    }

    bool ReadIntent(Character* c,CharMovement* movement,TaskType& task,Identity& subject,
                    AIFixedGate::Point& destination)
    {
        const OrdersReceiver* orders=c->getOrdersReciever();
        const TaskMatch* goal=orders?&orders->getCurrentGoal():NULL;
        task=goal && goal->taskData ? goal->key() : NULL_TASK;
        if (task==NULL_TASK || task==IDLE)
        {
            // An engine-cancelled order can leave an NPC idle. In this case
            // require an absent native face as well as the later position query.
            subject=Identity();
            destination=AIFixedGate::Point();
            return !movement->havokCharacter->isOnNavmesh() && !movement->isStandingOnSomething();
        }
        if (!TravelGoal(task)) return false;
        const bool reached=movement->isDestinationReached();
        const Ogre::Vector3 here=c->getPosition();
        subject=Identity(goal->subject);
        destination=Point(movement->getDestination());
        if (task==FOLLOW_SQUADLEADER)
        {
            Character* leader=c->getSquadLeader();
            if (!leader || leader==c || leader->isDead()) return false;
            subject=Identity(leader->handle);
            destination=Point(leader->getPosition());
            if (!AIFixedGate::Finite(destination)) return false;
            if (reached || AIFixedGate::DistanceSq(Point(here),destination)<400.0f)
                return WallFollowerBlocked(c,movement,leader,
                    movement->isStandingOnSomething().getBuilding(),here);
        }
        return !reached && AIFixedGate::Finite(destination) &&
            AIFixedGate::DistanceSq(Point(here),destination)>=400.0f;
    }

    bool SameIntent(const History& entry,TaskType task,const Identity& subject,const AIFixedGate::Point& destination)
    {
        return entry.goal==task && entry.subject==subject &&
            (MovingDestination(task) || AIFixedGate::DistanceSq(entry.destination,destination)<=25.0f);
    }

    bool WallAllowsLanding(WallBuilding* wall,const Ogre::Vector3& origin,
                           const Ogre::Vector3& landing,bool supporting,bool confirmedExterior)
    {
        if (!wall) return false;
        const Ogre::Vector3 a=wall->getPosition(),b=wall->getWallEndPosition();
        const float x=b.x-a.x,z=b.z-a.z;
        const float length=std::sqrt(x*x+z*z);
        if (!_finite(length) || length<1.0f) return false;
        const float from=(x*(origin.z-a.z)-z*(origin.x-a.x))/length;
        const float to=(x*(landing.z-a.z)-z*(landing.x-a.x))/length;
        if (AIFixedUnstuck::LeavesSupportingWall(from,to,0,false)) return true;
        const float along=(x*(origin.x-a.x)+z*(origin.z-a.z))/length;
        if (!supporting)
        {
            const float targetAlong=(x*(landing.x-a.x)+z*(landing.z-a.z))/length;
            return AIFixedUnstuck::PastSameWallEnd(along,targetAlong,length);
        }
        if (!confirmedExterior) return false;
        if (!_finite(along) || along<0.0f || along>length) return false;
        // The game uses local +X for a wall's exterior waypoint. The
        // gate codes also have to confirm that the landing is outside.
        const Ogre::Vector3 direction=wall->getOrientation().xAxis();
        const float outside=(x*direction.z-z*direction.x)/length;
        return AIFixedUnstuck::LeavesSupportingWall(from,to,outside,true) &&
            wall->getOutsideGateCode()==0;
    }

    bool GateAllowsLanding(Building* b,const Ogre::Vector3& origin,const Ogre::Vector3& landing)
    {
        DoorStuff* door=b->getDoor();
        if (!door || !door->isSetupComplete) return false;
        if (door->isBroken() || door->getDoorOpenAmount()>=.95f) return true;
        const AIFixedGate::Point inside=Point(door->getDoorPosInside()),outside=Point(door->getDoorPosOutside());
        const int start=AIFixedGate::Side(Point(origin),inside,outside);
        return start && start==AIFixedGate::Side(Point(landing),inside,outside);
    }

    bool CrossesBarrier(GameWorld* world,const Ogre::Vector3& origin,const Ogre::Vector3& landing,
                        Building* support,bool exterior)
    {
        // Spatial searches can omit long walls whose origins are far away.
        // Validate the actual structure supporting the NPC independently.
        if (support)
        {
            if (support->isGate())
            {
                if (!GateAllowsLanding(support,origin,landing)) return true;
            }
            else if (!WallAllowsLanding(support->isAWall(),origin,landing,true,exterior)) return true;
        }
        lektor<RootObject*>& objects=GateObjects();
        world->getObjectsWithinSphere(objects,landing,AIFixedGate::WorldUnits(18.0f),BUILDING,128,NULL);
        if (objects.size()>=128) return true;
        for (unsigned i=0;i<objects.size();++i)
        {
            Building* b=static_cast<Building*>(objects[i]);
            if (!b || b==support) continue;
            if (!b->isGate())
            {
                WallBuilding* wall=b->isAWall();
                if (!wall && !b->getDoor()) continue;
                const Ogre::Aabb& box=b->getAABB();
                if (!AIFixedGate::Finite(Point(box.mCenter)) || !AIFixedGate::Finite(Point(box.mHalfSize)))
                    return true;
                if (AIFixedUnstuck::OverlapsStructure(Point(origin),Point(landing),Point(box.mCenter),Point(box.mHalfSize)))
                {
                    // Adjacent pieces can overlap the recovery corridor at a
                    // join. Their actual wall spans are narrower than the AABB.
                    if (support && wall && exterior &&
                        WallAllowsLanding(wall,origin,landing,false,false)) continue;
                    return true;
                }
                continue;
            }
            if (!GateAllowsLanding(b,origin,landing)) return true;
        }
        return false;
    }

    Building* BlockedSupport(Character* c,CharMovement* movement,TaskType task,const Ogre::Vector3& here)
    {
        Building* support=movement->isStandingOnSomething().getBuilding();
        if (!Fortification(support)) return NULL;
        if (PathBlocked(movement->havokCharacter,true)) return support;
        return task==FOLLOW_SQUADLEADER && WallFollowerBlocked(c,movement,c->getSquadLeader(),support,here)
            ? support : NULL;
    }

    bool NativePositionAgrees(HavokCharacter* navigation,const Ogre::Vector3& here)
    {
        return navigation->isOnNavmesh() &&
            AIFixedGate::DistanceSq(Point(here),Point(navigation->getPosition()))<=25.0f;
    }

    bool FindLanding(GameWorld* world,Character* actor,Building* support,const Ogre::Vector3& here,
                     unsigned probeSite,Ogre::Vector3& landing)
    {
        NavMesh* nav=world->navmesh;
        ZoneMap* zone=world->zoneMgr?world->zoneMgr->getZoneMap(here):NULL;
        if (!nav || !zone || !zone->isLoadedMT() || zone->isBeingLoadedMT() ||
            !nav->isLoaded(zone) || nav->isGenerating()) return false;
        CharMovement* movement=actor->movement;
        const float ground=UtilityT::getTerrainHeight(here.x,here.z);
        if (!_finite(ground)) return false;
        if (here.y-ground<50.0f) support=NULL;
        if (!support)
        {
            if (NativePositionAgrees(movement->havokCharacter,here)) return false;

            // Only an explicit miss qualifies. -1 denotes an unavailable query.
            Ogre::Vector3 nearPoint;
            unsigned key=~0u;
            if (nav->getClosestPoint(here,3.0f,0.0f,false,nearPoint,key)!=0) return false;
        }
        const float maximumDrop=support?14.0f:8.0f;
        if (here.y-ground<-15.0f || here.y-ground>maximumDrop*10.0f) return false;
        // Start with the nearest surface around the NPC. If it fails, later
        // probes try a smaller area toward each side within the same 6 m limit.
        const float offset=30.0f;
        static const float shifts[5][2]={{0,0},{offset,0},{-offset,0},{0,offset},{0,-offset}};
        const float x=here.x+shifts[probeSite][0],z=here.z+shifts[probeSite][1];
        Ogre::Vector3 below(x,ground,z);
        if (world->zoneMgr->getZoneMap(below)!=zone) return false;
        below.y=UtilityT::getTerrainHeight(x,z);
        if (!_finite(below.y)) return false;
        const float clearance=movement->getRadius();
        if (!_finite(clearance) || clearance<=0 || clearance>AIFixedGate::WorldUnits(2.0f)) return false;
        if (nav->getClosestExteriorPoint(below,probeSite==0?60.0f:30.0f,clearance,landing)!=1 ||
            !AIFixedGate::Finite(Point(landing)) ||
            world->zoneMgr->getZoneMap(landing)!=zone || UtilityT::getPositionInWater(landing.x,landing.z))
            return false;
        float surface=UtilityT::getTerrainHeight(landing.x,landing.z);
        if (support)
        {
            if (here.y-landing.y<50.0f) return false;
            DoorStuff* door=support->isGate()?support->getDoor():NULL;
            if (door && door->isSetupComplete)
            {
                const Ogre::Vector3 inside=door->getDoorPosInside(),outside=door->getDoorPosOutside();
                const float platform=(inside.y+outside.y)*.5f;
                if (std::fabs(landing.y-platform)<=3.0f &&
                    (landing-inside).squaredLength()<1600.0f)
                    surface=platform;
            }
        }
        const bool allowed=AIFixedUnstuck::LandingAllowed(Point(here),Point(landing),surface,maximumDrop);
        if (!allowed) return false;
        const int destinationGate=nav->getGateCode(landing);
        if (destinationGate<0) return false;
        if (destinationGate>0)
        {
            // The upper gate deck can have a different face code from the
            // gate below. Native standing support and the door side both
            // verify the same-gate recovery.
            const int sourceGate=support && support->isGate()
                ? actor->amInsideTownWalls() : nav->getGateCode(here);
            if (destinationGate!=sourceGate) return false;
        }
        return !CrossesBarrier(world,here,landing,support,destinationGate==0) && zone->isLoadedMT() &&
            !zone->isBeingLoadedMT() && nav->isLoaded(zone);
    }

    void Inspect(GameWorld* world,const Identity& actor,double hours,LONG currentGeneration)
    {
        const unsigned start=actor.Hash()&(HistorySize-4);
        History* slot=&history[start];
        for (unsigned i=0;i<4;++i)
        {
            History* alternative=&history[start+i];
            if (alternative->actor==actor) { slot=alternative;break; }
            if (!alternative->watch.active ||
                (slot->watch.active && activeMs-alternative->watch.sampledAt>activeMs-slot->watch.sampledAt))
                slot=alternative;
        }
        History& entry=*slot;
        Character* c=actor.Handle().getCharacter();
        TaskType task;
        Identity subject;
        AIFixedGate::Point destination;
        if (!c || !(Identity(c->handle)==actor) || c->isPlayerCharacter() || !c->movement ||
            !c->movement->havokCharacter || !ReadIntent(c,c->movement,task,subject,destination) ||
            !CanTravel(c,c->movement,task))
        {
            if (entry.actor==actor) entry.watch.active=false;
            return;
        }
        const Ogre::Vector3 here=c->getPosition();
        const AIFixedGate::Point position=Point(here);
        if (!(entry.actor==actor) || !SameIntent(entry,task,subject,destination))
        {
            entry.watch.active=false;
            entry.actor=actor;
            entry.goal=task;
            entry.subject=subject;
            entry.destination=destination;
        }
        const bool previouslyActive=entry.watch.active;
        const double priorStart=entry.watch.since;
        const bool due=entry.watch.Sample(position,hours,activeMs);
        if (!previouslyActive || entry.watch.since!=priorStart) entry.probeSite=0;
        if (!due || activeMs-lastProbeMs<500) return;

        Building* support=BlockedSupport(c,c->movement,task,here);
        if (!support && NativePositionAgrees(c->movement->havokCharacter,here))
        {
            entry.watch.Probed(true);
            return;
        }
        lastProbeMs=activeMs;
        Ogre::Vector3 landing;
        const unsigned probeSite=entry.probeSite;
        entry.probeSite=(entry.probeSite+1)%5;
        entry.watch.Probed(entry.probeSite==0);
        if (!FindLanding(world,c,support,here,probeSite,landing)) return;
        if (currentGeneration!=InterlockedCompareExchange(&g_loadGeneration,0,0) ||
            InterlockedCompareExchange(&g_loading,0,0) || world->isPaused() ||
            actor.Handle().getCharacter()!=c || !c->movement || !c->movement->havokCharacter ||
            !ReadIntent(c,c->movement,task,subject,destination) || !CanTravel(c,c->movement,task) ||
            !SameIntent(entry,task,subject,destination) ||
            AIFixedGate::DistanceSq(position,Point(c->getPosition()))>1.0f) return;
        entry.watch.active=false;
        c->teleport(landing);
        if (g_debug)
        {
            Character* moved=actor.Handle().getCharacter();
            const Ogre::Vector3 arrived=moved==c?moved->getPosition():landing;
            char line[256];
            _snprintf_s(line,_TRUNCATE,
                "freed NPC %u/%u:%u after %.2f game hours (%.1f, %.1f, %.1f) to (%.1f, %.1f, %.1f)",
                actor.container,actor.index,actor.serial,hours-entry.watch.since,
                here.x,here.y,here.z,arrived.x,arrived.y,arrived.z);
            DebugLog(line);
        }
    }

    void Update(GameWorld* world,LONG currentGeneration)
    {
        if (!ready || faulted) return;
        if (!g_unstuck)
        {
            if (phase>=0) Suspend();
            generation=-1;
            return;
        }
        const DWORD now=GetTickCount();
        if (generation!=currentGeneration)
        {
            Transfer(-1,NULL);
            for (unsigned i=0;i<HistorySize;++i) history[i].watch.active=false;
            generation=currentGeneration;
            lastTick=now;
        }
        const DWORD elapsed=now-lastTick;
        if (elapsed<TickMs) return;
        lastTick=now;
        if (world->isPaused())
        {
            if (phase>=0) Transfer(-1,NULL);
            return;
        }
        activeMs+=elapsed;
        const double hours=world->getTimeStamp_inGameHours().getTotalHours();
        Sample batch[BatchSize];
        Transfer(static_cast<LONG>(nextPhase++&0x7fffffffu),batch);
        if (!_finite(hours)) return;
        for (unsigned i=0;i<BatchSize;++i)
            if (batch[i].used) Inspect(world,batch[i].actor,hours,currentGeneration);
    }

    void SafeUpdate(GameWorld* world,LONG currentGeneration)
    {
        __try { Update(world,currentGeneration); }
        __except(EXCEPTION_EXECUTE_HANDLER) { Fault(); }
    }

    void Install()
    {
        if (!g_unstuck) return;
        ready=g_lifecycleReady && KenshiLib::SUCCESS==KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&CharMovement::_NV_update),Movement_hook,&originalMovement);
        if (!ready) ErrorLog("AIFixed: stuck NPC recovery could not start");
    }
}
