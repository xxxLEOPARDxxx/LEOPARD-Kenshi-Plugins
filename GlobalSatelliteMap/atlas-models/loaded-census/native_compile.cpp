#include "NativeCensus.h"
typedef char ManagerAbi[offsetof(TownBase,buildingsManager)==0x250?1:-1];
typedef char MapAbi[offsetof(TownBuildingsManager,buildingEntities)==8?1:-1];
typedef char ManagerTownAbi[offsetof(TownBuildingsManager,town)==0xa8?1:-1];
typedef char FloorAbi[offsetof(TownBuildingsManager::BuildingInfo,visibleFloor)==0x60?1:-1];
typedef char PhysicalAbi[offsetof(Building,physical)==0x228?1:-1];
typedef char EntAbi[offsetof(PhysicalEntity,ent)==8?1:-1];
typedef char CensusOutputBudget[sizeof(NativeLoadedCensus::Result)<65536?1:-1];
typedef char CensusStackBudget[sizeof(NativeLoadedCensus::NativeOps)<100000?1:-1];
HRESULT CompileLoaded(bool on,Town*t,Ogre::SceneManager*s,LONG g,volatile LONG*live,bool verified,NativeLoadedCensus::Result&r){return NativeLoadedCensus::Read(on,t,s,g,live,verified,r);}
