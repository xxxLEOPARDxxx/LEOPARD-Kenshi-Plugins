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
        unsigned probeSite, reportedAt;
        bool guard;
        History():goal(NULL_TASK),probeSite(0),reportedAt(0),guard(false){}
    };

    struct LandingTrace
    {
        const char* reason;
        float ground, clearance, surface, landingGround;
        AIFixedGate::Point slope;
        int sourceFace, pocket;
        LandingTrace():reason("not-probed"),ground(0),clearance(0),surface(0),landingGround(0),slope(0,0,0),sourceFace(-2),pocket(-2){}
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
    DWORD lastTick=0, activeMs=0, lastProbeMs=0, lastRockProbeMs=0;
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

    const TaskMatch* CurrentGoal(Character* c)
    {
        const OrdersReceiver* orders=c->getOrdersReciever();
        return orders?&orders->getCurrentGoal():NULL;
    }

    TaskType GoalTask(const TaskMatch* goal)
    { return goal && goal->taskData ? goal->key() : NULL_TASK; }

    bool ReadIntent(Character* c,CharMovement* movement,const TaskMatch* goal,TaskType task,
                    Identity& subject,AIFixedGate::Point& destination)
    {
        if (task==NULL_TASK || task==IDLE)
        {
            // The engine can cancel an order while the NPC is inside solid rock.
            // Idle NPCs on a valid face need physical evidence at probe time.
            subject=Identity();
            destination=AIFixedGate::Point();
            return movement->havokCharacter->getState()==HavokCharacter::IDLE &&
                !movement->isStandingOnSomething();
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

    Building* ReadGuardCycle(Character* c,CharMovement* movement,TaskType task,bool tracked,
                            bool& attempt,bool& failed)
    {
        HavokCharacter* navigation=movement->havokCharacter;
        const HavokCharacter::CharacterState state=navigation->getState();
        if (!tracked && state!=HavokCharacter::IDLE) return NULL;
        const HavokCharacter::PathState path=navigation->getPathState();
        if (!tracked && path!=HavokCharacter::WAITING && path!=HavokCharacter::FAILED &&
            path!=HavokCharacter::BROKEN && path!=HavokCharacter::INCOMPLETE) return NULL;
        Building* support=movement->isStandingOnSomething().getBuilding();
        if (!support || support->getSpecialFunction()!=BF_FLUFF || support->isDisabled() ||
            !support->_buildState.isComplete || c->isAnimal()) return NULL;

        // The movement request may reset to the feet before the asynchronous
        // failure arrives. The native Havok goal still identifies that path.
        // Both vectors share the same space, with one Havok unit per metre.
        const float* dest=navigation->goal.m_quad.m128_f32;
        const float* here=navigation->position.m_quad.m128_f32;
        const AIFixedGate::Point delta(dest[0]-here[0],dest[1]-here[1],dest[2]-here[2]);
        if (!AIFixedGate::Finite(delta)) return NULL;
        const bool away=AIFixedGate::DistanceSq(delta,AIFixedGate::Point())>=4.0f;
        // Goals describe why an NPC travels, not whether it is travelling.
        // Shopping, bed and modded goals all use the same native move action.
        // A retained native failure alone is insufficient after that action
        // has ended; idle/at-post actors must never refresh this evidence.
        CharBody* body=c->getBody();
        const Tasker* action=body?body->getCurrentAction():NULL;
        // GOAP can clear its goal before the body receives the path failure.
        // The still-current native movement action is the movement witness;
        // the high-level goal is not an additional admission requirement.
        const bool movingTask=action && action->taskData &&
            (action->key()==MOVE_ON_FREE_WILL || action->key()==PATROL_TOWN);
        failed=movingTask && state==HavokCharacter::IDLE && away &&
            (path==HavokCharacter::FAILED || path==HavokCharacter::BROKEN || path==HavokCharacter::INCOMPLETE);
        attempt=movingTask && state==HavokCharacter::IDLE && (failed || (away && path==HavokCharacter::WAITING));
        // NONE/COMPLETE/UPDATING describe the native path pipeline, not arrival
        // at a guard post. A tracked actor can cycle through these phases at
        // the same stuck position while its native goal remains far away.
        // A cancelled request also briefly reports GOAL_REACHED or resets the
        // native goal to the feet without any physical progress. These phases
        // can retain an existing clock, but can never start/refresh evidence or
        // admit a landing. SampleGuard still expires stale movement evidence;
        // physical movement retires the old stationary anchor. Goal changes
        // retain only the physical history tied to this exact support handle.
        if (!tracked && !attempt) return NULL;
        return tracked || attempt ? support : NULL;
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

    const float EscapeDirections[8][2]={{1,0},{.7071068f,.7071068f},{0,1},{-.7071068f,.7071068f},
        {-1,0},{-.7071068f,-.7071068f},{0,-1},{.7071068f,-.7071068f}};

    Ogre::Vector3 EscapeDirection(unsigned index)
    { return Ogre::Vector3(EscapeDirections[index][0],0,EscapeDirections[index][1]); }

    Building* FindElevatedGate(const Ogre::Vector3& here,float ground)
    {
        if (!AIFixedUnstuck::AboveGateDeck(here.y,ground,ground)) return NULL;
        const unsigned gateMask=0x780200u | (1u<<13);
        const Ogre::Vector3 up(0,1,0),body=here+8.0f*up;
        physHit roof(false,true,here.y);
        UtilityT::sphereTrace(roof,body+100.0f*up,-100.0f*up,.75f,gateMask,true);
        if (roof.traceWasAborted() || !roof.hit || !(roof.position.y>body.y+1.5f)) return NULL;
        Building* object=roof.hitObject.getBuilding();
        Building* gate=object?object->isGate():NULL;
        DoorStuff* door=gate && !gate->isDisabled()?gate->getDoor():NULL;
        if (!door || !door->isSetupComplete) return NULL;
        const Ogre::Vector3 deck=(door->getDoorPosInside()+door->getDoorPosOutside())*.5f;
        const float dx=here.x-deck.x,dz=here.z-deck.z;
        if (!AIFixedUnstuck::AboveGateDeck(here.y,ground,deck.y) || !(dx*dx+dz*dz<=3600.0f)) return NULL;

        // A standing actor on a real upper walkway keeps its footing even
        // when the movement controller has not assigned a standing handle.
        physHit footing(false,true,here.y);
        UtilityT::sphereTrace(footing,here+3.0f*up,-8.0f*up,.75f,~0u,true);
        if (footing.traceWasAborted() || (footing.hit && footing.normal.y>.5f)) return NULL;
        for (unsigned i=0;i<8;i+=2)
        {
            const Ogre::Vector3 direction=EscapeDirection(i);
            physHit side(false,true,here.y);
            UtilityT::trace(side,body,direction,gateMask);
            if (side.traceWasAborted() || !side.hit || !_finite(side.distance) ||
                side.distance<0.0f || side.distance>16.0f) continue;
            Building* blocker=side.hitObject.getBuilding();
            if (blocker && blocker->isGate()==gate) return gate;
        }
        return NULL;
    }

    bool FindRockExit(const Ogre::Vector3& here,float clearance,unsigned site,Ogre::Vector3& seed,
                      unsigned& siteCount)
    {
        // Natural collision (including placed ore veins) is group 14. Keep
        // these traces separate from building collision and ground geometry.
        const unsigned naturalMask=1u<<14;
        const float span=AIFixedGate::WorldUnits(10.0f);
        const Ogre::Vector3 body=here+Ogre::Vector3(0,AIFixedGate::WorldUnits(.8f),0);
        physHit roof(false,true,here.y);
        UtilityT::sphereTrace(roof,body+Ogre::Vector3(0,span,0),Ogre::Vector3(0,-span,0),.75f,naturalMask,true);
        if (roof.traceWasAborted() || !roof.hit || roof.group()!=14 ||
            !AIFixedGate::Finite(Point(roof.position)) || roof.position.y<=body.y+1.5f) return false;
        Building* rock=roof.hitObject.getBuilding();
        if (rock && (rock->isDisabled() || (!rock->isFoliage && rock->getSpecialFunction()!=BF_MINE_NATURAL)))
            return false;

        struct Face { NxShape* shape; float distance; } faces[8]={};
        for (unsigned i=0;i<8;++i)
        {
            const Ogre::Vector3 out=EscapeDirection(i);
            physHit entry(false,true,here.y);
            UtilityT::trace(entry,body+out*span,-out,naturalMask);
            if (entry.traceWasAborted()) return false;
            if (!entry.hit || !entry.shape || (rock && entry.hitObject.getBuilding()!=rock) ||
                !AIFixedUnstuck::EntersSolid(entry.distance,entry.normal.dotProduct(-out),span)) continue;
            faces[i].shape=entry.shape;
            faces[i].distance=span-entry.distance;
        }
        // Seams and a ray starting inside a large rock can miss individual
        // faces. Require two complete axes from the same physical shape.
        unsigned mask=0;
        for (unsigned i=0;i<4 && !mask;++i)
        {
            if (!faces[i].shape || faces[i].shape!=faces[i+4].shape) continue;
            unsigned candidate=0;
            for (unsigned j=0;j<8;++j)
                if (faces[j].shape==faces[i].shape) candidate|=1u<<j;
            if (AIFixedUnstuck::OpposedRockFaces(candidate)) mask=candidate;
        }
        if (!mask) return false;
        if (!rock)
        {
            // Scenery can lack a building handle. Tie the overhead contact
            // to the same shape in that case, since a sweep has no shape ID.
            physHit top(false,true,here.y);
            const Ogre::Vector3 down(0,-1,0);
            UtilityT::trace(top,body-down*span,down,naturalMask);
            unsigned first=0;
            while (!(mask & (1u<<first))) ++first;
            if (top.traceWasAborted() || !top.hit || top.shape!=faces[first].shape ||
                !AIFixedUnstuck::EntersSolid(top.distance,top.normal.dotProduct(down),span)) return false;
        }
        std::pair<float,unsigned> exits[8];
        unsigned count=0;
        for (unsigned i=0;i<8;++i)
        {
            if (!(mask & (1u<<i))) continue;
            // A recess can look enclosed from outside. Reject an inward
            // face between the body and the corresponding exterior face.
            physHit outward(false,true,here.y);
            UtilityT::trace(outward,body,EscapeDirection(i),naturalMask);
            if (outward.traceWasAborted() || (outward.hit && outward.shape==faces[i].shape &&
                outward.distance<=faces[i].distance+1.5f)) return false;
            exits[count++]=std::make_pair(faces[i].distance,i);
        }
        std::sort(exits,exits+count);
        siteCount=count*2;
        const std::pair<float,unsigned>& exit=exits[(site%siteCount)/2];
        const float distance=exit.first+clearance+AIFixedGate::WorldUnits(site%2?2.5f:1.4f);
        if (distance>AIFixedGate::WorldUnits(8.0f)) return false;
        seed=here+EscapeDirection(exit.second)*distance;
        return true;
    }

    bool StandingSpaceClear(const Ogre::Vector3& landing,float clearance,float height)
    {
        const Ogre::Vector3 start=landing+Ogre::Vector3(0,height,0);
        const Ogre::Vector3 down(0,clearance+AIFixedGate::WorldUnits(.25f)-height,0);
        physHit solid(false,true,landing.y);
        UtilityT::sphereTrace(solid,start,down,clearance,~0u,true);
        return !solid.traceWasAborted() && !solid.hit;
    }

    bool ReadPlatformSlope(Building* support,const Ogre::Vector3& here,AIFixedGate::Point& normal)
    {
        physHit footing(false,true,here.y);
        UtilityT::sphereTrace(footing,here+Ogre::Vector3(0,6,0),Ogre::Vector3(0,-12,0),.75f,~0u,true);
        if (footing.traceWasAborted() || !footing.hit || footing.getBuilding()!=support ||
            !AIFixedGate::Finite(Point(footing.position)) || std::fabs(footing.position.y-here.y)>5.0f)
            return false;
        normal=Point(footing.normal);
        return AIFixedUnstuck::IsSteepGuardSlope(normal);
    }

    int QueryPlatformPocket(NavMesh* nav,HavokCharacter* navigation)
    {
        // Catch native faults inside the caller's RAII lock scope, so the
        // native nav reader lock is still released before recovery suspends.
        __try
        {
            return nav->isCharacterStuck(navigation->position,navigation->currentFace,navigation->radius,3.0f)?1:0;
        }
        __except(EXCEPTION_EXECUTE_HANDLER) { Fault();return -1; }
    }

    bool ReadPlatformPocket(NavMesh* nav,CharMovement* movement,const AIFixedGate::Point& normal,int& result)
    {
        // A rounded end of a tilted support can have a small walkable cap.
        // Keep ordinary flat posts and connected ramps out of this fallback.
        result=-2;
        if (!AIFixedUnstuck::IsRoundedGuardFooting(normal)) return false;
        HavokCharacter* navigation=movement->havokCharacter;
        if (!navigation || navigation->currentFace==~0u || !_finite(navigation->radius) ||
            navigation->radius<=0 || navigation->radius>2.0f) return false;
        const float* p=navigation->position.m_quad.m128_f32;
        if (!_finite(p[0]) || !_finite(p[1]) || !_finite(p[2])) return false;
        // Same non-blocking reader lock as native pathExists; the native
        // island query itself does not lock. No engine calls inside this lock
        // other than the audited local query (Steam/GOG native slot5006).
        boost::shared_lock<boost::shared_mutex> lock(nav->changeMutex,boost::try_to_lock);
        result=-1;
        if (!lock.owns_lock()) return false;
        result=QueryPlatformPocket(nav,navigation);
        return result==1;
    }

    bool RejectLanding(LandingTrace& trace,const char* reason)
    { trace.reason=reason;return false; }

    bool CorrectPlatformGround(NavMesh* nav,const Ogre::Vector3& source,float terrain,
                               float clearance,Ogre::Vector3& landing)
    {
        // Exterior navmesh simplifies the terrain. Its closest face can be
        // slightly below the real collision floor; teleport preserves Y.
        // Correct only a small downward discrepancy, with an independently
        // hit terrain floor AND the same nearby exterior face after correction.
        const float gap=terrain-source.y;
        if (!_finite(gap) || gap<=AIFixedGate::WorldUnits(.3f) ||
            gap>AIFixedGate::WorldUnits(1.0f)) return false;
        const Ogre::Vector3 start(source.x,terrain+AIFixedGate::WorldUnits(.5f),source.z);
        physHit floor(false,true,terrain);
        UtilityT::trace(floor,start,Ogre::Vector3(0,-1,0),1u<<2);
        if (floor.traceWasAborted() || !floor.hit || floor.group()!=2 || floor.getBuilding() ||
            !AIFixedGate::Finite(Point(floor.position)) || !AIFixedGate::Finite(Point(floor.normal)) ||
            floor.normal.y<.65f || !_finite(floor.distance) || floor.distance<0 ||
            floor.distance>AIFixedGate::WorldUnits(1.0f) ||
            std::fabs(floor.position.x-source.x)>AIFixedGate::WorldUnits(.1f) ||
            std::fabs(floor.position.z-source.z)>AIFixedGate::WorldUnits(.1f) ||
            std::fabs(floor.position.y-terrain)>AIFixedGate::WorldUnits(.2f)) return false;
        const Ogre::Vector3 physical(source.x,floor.position.y,source.z);
        Ogre::Vector3 confirmed;
        if (nav->getClosestExteriorPoint(physical,AIFixedGate::WorldUnits(1.0f),clearance,confirmed)!=1 ||
            !AIFixedGate::Finite(Point(confirmed)) ||
            AIFixedGate::DistanceSq(Point(source),Point(confirmed))>AIFixedGate::WorldUnits(.1f)*AIFixedGate::WorldUnits(.1f))
            return false;
        landing=physical;
        return true;
    }

    bool FindLanding(GameWorld* world,Character* actor,Building* support,const Ogre::Vector3& here,
                     unsigned probeSite,Ogre::Vector3& landing,unsigned& siteCount,bool guard,LandingTrace& trace)
    {
        NavMesh* nav=world->navmesh;
        ZoneMap* zone=world->zoneMgr?world->zoneMgr->getZoneMap(here):NULL;
        if (!nav || !zone || !zone->isLoadedMT() || zone->isBeingLoadedMT() ||
            !nav->isLoaded(zone) || nav->isGenerating()) return RejectLanding(trace,"zone-not-ready");
        CharMovement* movement=actor->movement;
        const float ground=UtilityT::getTerrainHeight(here.x,here.z);
        trace.ground=ground;
        if (!_finite(ground)) return RejectLanding(trace,"invalid-ground");
        if (here.y-ground<50.0f) support=NULL;
        AIFixedGate::Point slope;
        bool steep=false,pocket=false;
        if (guard)
        {
            if (!support) return RejectLanding(trace,"platform-support-missing");
            steep=ReadPlatformSlope(support,here,slope);
            trace.slope=slope;
            if (!steep)
            {
                // A native cached face may outlive the walkable surface that
                // supported it. Only an explicit current walkable-face miss
                // can replace slope proof; unknown query results fail closed.
                Ogre::Vector3 nearPoint;
                unsigned key=~0u;
                trace.sourceFace=nav->getClosestPoint(here,3.0f,0.0f,false,nearPoint,key);
                if (trace.sourceFace<0) return RejectLanding(trace,"support-face-unavailable");
                if (trace.sourceFace!=0)
                {
                    if (trace.sourceFace!=1 || !AIFixedGate::Finite(Point(nearPoint)) ||
                        AIFixedGate::DistanceSq(Point(here),Point(nearPoint))>25.0f ||
                        key!=movement->havokCharacter->currentFace ||
                        !NativePositionAgrees(movement->havokCharacter,here) ||
                        !ReadPlatformPocket(nav,movement,slope,trace.pocket))
                        return RejectLanding(trace,"support-not-steep");
                    pocket=true;
                }
            }
        }
        Building* inferredGate=!support && !movement->isStandingOnSomething()?FindElevatedGate(here,ground):NULL;
        if (inferredGate) support=inferredGate;
        // Only confirmed steep BF_FLUFF support receives the higher bound.
        // An explicit native face miss can prove other stranded footing, but
        // retains the normal14m support limit; gates and rock are unchanged.
        const float maximumDrop=guard && steep?18.0f:(support?14.0f:8.0f);
        if (here.y-ground<-15.0f || here.y-ground>maximumDrop*10.0f) return RejectLanding(trace,"terrain-height-limit");
        const float clearance=movement->getRadius();
        trace.clearance=clearance;
        if (!_finite(clearance) || clearance<=0 || clearance>AIFixedGate::WorldUnits(2.0f)) return RejectLanding(trace,"invalid-radius");
        Ogre::Vector3 rockExit;
        const bool embedded=!support && !movement->isStandingOnSomething() &&
            FindRockExit(here,clearance,probeSite,rockExit,siteCount);
        if (siteCount && !embedded) return RejectLanding(trace,"rock-exit-unconfirmed");
        if (!support)
        {
            if (!embedded)
            {
                if (NativePositionAgrees(movement->havokCharacter,here)) return RejectLanding(trace,"native-position-valid");
                // Only an explicit miss qualifies. -1 denotes an unavailable query.
                Ogre::Vector3 nearPoint;
                unsigned key=~0u;
                if (nav->getClosestPoint(here,3.0f,0.0f,false,nearPoint,key)!=0) return RejectLanding(trace,"near-face-not-explicit-miss");
            }
        }
        if (!embedded) { siteCount=5; probeSite%=siteCount; }
        // Start with the nearest surface around the NPC. If it fails, later
        // probes try a smaller area toward each side within the same 6 m limit.
        const float offset=30.0f;
        static const float shifts[5][2]={{0,0},{offset,0},{-offset,0},{0,offset},{0,-offset}};
        Ogre::Vector3 below=embedded?rockExit:
            Ogre::Vector3(here.x+shifts[probeSite][0],ground,here.z+shifts[probeSite][1]);
        if (guard && (steep || pocket) && probeSite==0)
        {
            below.x+=55.0f*slope.x;
            below.z+=55.0f*slope.z;
        }
        if (world->zoneMgr->getZoneMap(below)!=zone) return RejectLanding(trace,"seed-zone");
        below.y=UtilityT::getTerrainHeight(below.x,below.z);
        if (!_finite(below.y)) return RejectLanding(trace,"seed-ground");
        const float search=embedded?AIFixedGate::WorldUnits(1.0f):(probeSite==0&&!guard?60.0f:30.0f);
        if (nav->getClosestExteriorPoint(below,search,clearance,landing)!=1)
            return RejectLanding(trace,"no-exterior-point");
        if (!AIFixedGate::Finite(Point(landing))) return RejectLanding(trace,"invalid-landing");
        if (world->zoneMgr->getZoneMap(landing)!=zone) return RejectLanding(trace,"landing-zone");
        if (UtilityT::getPositionInWater(landing.x,landing.z)) return RejectLanding(trace,"landing-water");
        float surface=UtilityT::getTerrainHeight(landing.x,landing.z);
        trace.landingGround=surface;
        trace.surface=landing.y;
        if (guard && _finite(surface) && surface-landing.y>AIFixedGate::WorldUnits(.3f))
        {
            const Ogre::Vector3 source=landing;
            if (!CorrectPlatformGround(nav,source,surface,clearance,landing))
                return RejectLanding(trace,"platform-ground-unconfirmed");
        }
        if (support)
        {
            if (here.y-landing.y<50.0f) return RejectLanding(trace,"platform-drop-too-small");
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
        const bool allowed=AIFixedUnstuck::LandingAllowed(Point(here),Point(landing),surface,maximumDrop,embedded?8.0f:6.0f);
        if (!allowed) return RejectLanding(trace,"landing-distance-height-surface");
        if (embedded && !StandingSpaceClear(landing,clearance,80.0f)) return RejectLanding(trace,"rock-landing-clearance");
        const int destinationGate=nav->getGateCode(landing);
        if (destinationGate<0) return RejectLanding(trace,"invalid-gate-code");
        if (guard && destinationGate!=nav->getGateCode(here)) return RejectLanding(trace,"platform-gate-code");
        if (guard && (steep || pocket) && !AIFixedUnstuck::GuardLeavesSlope(Point(here),Point(landing),slope,pocket)) return RejectLanding(trace,"platform-uphill");
        if (guard && !StandingSpaceClear(landing,clearance,here.y-landing.y+clearance)) return RejectLanding(trace,"platform-landing-clearance");
        if (inferredGate)
        {
            DoorStuff* door=inferredGate->getDoor();
            if (!door || !door->isSetupComplete || destinationGate!=nav->getGateCode(here)) return RejectLanding(trace,"inferred-gate-code");
            const AIFixedGate::Point in=Point(door->getDoorPosInside()),out=Point(door->getDoorPosOutside());
            const int side=AIFixedGate::Side(Point(here),in,out);
            if (!side || side!=AIFixedGate::Side(Point(landing),in,out) ||
                !StandingSpaceClear(landing,clearance,30.0f)) return RejectLanding(trace,"inferred-gate-side-clearance");
        }
        if (destinationGate>0)
        {
            // The upper gate deck can have a different face code from the
            // gate below. Native standing support and the door side both
            // verify the same-gate recovery.
            const int sourceGate=support && support->isGate()
                ? actor->amInsideTownWalls() : nav->getGateCode(here);
            if (destinationGate!=sourceGate) return RejectLanding(trace,"town-gate-code");
        }
        if (CrossesBarrier(world,here,landing,guard?NULL:support,destinationGate==0)) return RejectLanding(trace,"barrier");
        if (!zone->isLoadedMT() || zone->isBeingLoadedMT() || !nav->isLoaded(zone)) return RejectLanding(trace,"zone-changed");
        trace.reason="accepted";
        return true;
    }

    void TracePlatform(History& entry,Character* c,double hours,const char* reason,
                       bool attempt,bool failed,bool fresh,const LandingTrace* trace=NULL,
                       const Ogre::Vector3* landing=NULL)
    {
        if (!g_debug) return;
        static unsigned lastReport=0;
        if (!trace && ((entry.reportedAt && activeMs-entry.reportedAt<15000) || activeMs-lastReport<200)) return;
        CharMovement* movement=c->movement;
        Building* support=movement->isStandingOnSomething().getBuilding();
        if (!support || support->getSpecialFunction()!=BF_FLUFF) return;
        entry.reportedAt=lastReport=activeMs;
        CharBody* body=c->getBody();
        const Tasker* action=body?body->getCurrentAction():NULL;
        const HavokCharacter* nav=movement->havokCharacter;
        const float* a=nav->position.m_quad.m128_f32,*b=nav->goal.m_quad.m128_f32;
        const Ogre::Vector3 here=c->getPosition(),target=landing?*landing:here;
        const Identity object(support->handle);
        char line[768];
        _snprintf_s(line,_TRUNCATE,
            "platform NPC %u/%u/%u:%u/%u support %u/%u/%u:%u/%u p=(%.2f,%.2f,%.2f) "
            "anchor=(%.2f,%.2f,%.2f) age=%.3f failureAge=%.3f active=%u goal=%d action=%d "
            "native=%d/%d native_delta=(%.2f,%.2f,%.2f) attempt=%u failed=%u fresh=%u reason=%s "
            "site=%u ground=%.2f radius=%.2f slopeY=%.3f slopeXZ=(%.3f,%.3f) sourceFace=%d "
            "pocket=%d navY=%.2f landingGround=%.2f landing=(%.2f,%.2f,%.2f)",
            entry.actor.type,entry.actor.container,entry.actor.containerSerial,entry.actor.index,entry.actor.serial,
            object.type,object.container,object.containerSerial,object.index,object.serial,
            here.x,here.y,here.z,entry.watch.anchor.x,entry.watch.anchor.y,entry.watch.anchor.z,
            entry.watch.active?hours-entry.watch.since:0.0,hours-entry.watch.guardFailure,entry.watch.active?1:0,
            static_cast<int>(GoalTask(CurrentGoal(c))),action&&action->taskData?static_cast<int>(action->key()):-1,
            static_cast<int>(movement->havokCharacter->getState()),static_cast<int>(movement->havokCharacter->getPathState()),
            b[0]-a[0],b[1]-a[1],b[2]-a[2],attempt?1:0,failed?1:0,fresh?1:0,reason,entry.probeSite,
            trace?trace->ground:0.0f,trace?trace->clearance:0.0f,
            trace?trace->slope.y:0.0f,trace?trace->slope.x:0.0f,trace?trace->slope.z:0.0f,
            trace?trace->sourceFace:-2,trace?trace->pocket:-2,trace?trace->surface:0.0f,trace?trace->landingGround:0.0f,
            target.x,target.y,target.z);
        DebugLog(line);
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
            !c->movement->havokCharacter)
        {
            if (entry.actor==actor) entry.watch.active=false;
            return;
        }
        if (!(entry.actor==actor)) { entry=History();entry.actor=actor; }
        const TaskMatch* goal=CurrentGoal(c);
        task=GoalTask(goal);
        bool attempt=false,failed=false;
        Building* guard=ReadGuardCycle(c,c->movement,task,entry.actor==actor && entry.guard &&
            entry.watch.active,attempt,failed);
        if (guard)
        {
            task=STAND_AT_GUARD_NODE_HOMETOWN_OUTSIDE;
            subject=Identity(guard->handle);
        }
        if ((!guard && !ReadIntent(c,c->movement,goal,task,subject,destination)) ||
            !CanTravel(c,c->movement,task))
        {
            if (entry.actor==actor) entry.watch.active=false;
            TracePlatform(entry,c,hours,"actor-or-intent-ineligible",attempt,failed,false);
            return;
        }
        const Ogre::Vector3 here=c->getPosition();
        const AIFixedGate::Point position=Point(here);
        if (!(entry.actor==actor) || entry.guard!=(guard!=NULL) || !SameIntent(entry,task,subject,destination))
        {
            entry.watch.active=false;
            entry.actor=actor;
            entry.goal=task;
            entry.subject=subject;
            entry.destination=destination;
            entry.guard=guard!=NULL;
        }
        const bool previouslyActive=entry.watch.active;
        const double priorStart=entry.watch.since;
        // Current native movement plus a current remote failed path is blocked
        // intent even when GOAP recreates the action or the path state stays
        // unchanged. A retained failure after movement ended is not evidence.
        const bool freshFailure=failed;
        const bool due=guard?entry.watch.SampleGuard(position,hours,activeMs,attempt,freshFailure,g_unstuckDelayHours):
            entry.watch.Sample(position,hours,activeMs,g_unstuckDelayHours);
        if (!previouslyActive || entry.watch.since!=priorStart) entry.probeSite=0;
        if (!due || (guard && !attempt))
        {
            TracePlatform(entry,c,hours,entry.watch.active?"observing":"evidence-expired",attempt,failed,freshFailure);
            return;
        }

        Building* support=guard?guard:BlockedSupport(c,c->movement,task,here);
        // Keep the new valid-face collision probes from delaying the existing
        // off-mesh and fortification recovery slots in crowded settlements.
        const bool validFace=!support && NativePositionAgrees(c->movement->havokCharacter,here);
        DWORD& last=validFace?lastRockProbeMs:lastProbeMs;
        if (activeMs-last<(validFace?1000u:500u)) return;
        last=activeMs;
        Ogre::Vector3 landing=here;
        LandingTrace trace;
        const unsigned probeSite=entry.probeSite;
        unsigned siteCount=0;
        const bool found=FindLanding(world,c,support,here,probeSite,landing,siteCount,guard!=NULL,trace);
        TracePlatform(entry,c,hours,trace.reason,attempt,failed,freshFailure,&trace,&landing);
        entry.probeSite=siteCount?(probeSite+1)%siteCount:0;
        entry.watch.Probed(entry.probeSite==0);
        if (!found) return;
        if (currentGeneration!=InterlockedCompareExchange(&g_loadGeneration,0,0) ||
            InterlockedCompareExchange(&g_loading,0,0) || world->isPaused() ||
            actor.Handle().getCharacter()!=c || !c->movement || !c->movement->havokCharacter ||
            AIFixedGate::DistanceSq(position,Point(c->getPosition()))>1.0f) return;
        goal=CurrentGoal(c);
        if (guard)
        {
            // Retained no-goal/reached/at-feet phases only keep observation.
            // Actual relocation requires an active remote native move now.
            Building* current=ReadGuardCycle(c,c->movement,GoalTask(goal),false,attempt,failed);
            if (!current || !(Identity(current->handle)==subject)) return;
        }
        else
        {
            task=GoalTask(goal);
            if (!ReadIntent(c,c->movement,goal,task,subject,destination) ||
                !SameIntent(entry,task,subject,destination)) return;
        }
        if (!CanTravel(c,c->movement,task)) return;
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
