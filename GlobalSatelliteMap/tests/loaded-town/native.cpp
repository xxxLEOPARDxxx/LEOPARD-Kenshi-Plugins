#define NOMINMAX
#include "../../GsmInstanceSources.h"
#include "../../GsmLoadedTownState.h"
bool CompileInstanceSources(const std::string&game,std::string&error){return GsmInstanceSources::ActiveBaseSources(game,error);}
HRESULT CompileTownState(GsmLoadedTown::State&s,const GsmTownValues::Result&values,const float*camera,const float*origin,Ogre::SceneManager*scene,const void*king,LONG generation,volatile LONG*live,ID3D11Device*d,std::string&error){return s.Refresh(values,camera,origin,scene,king,generation,live,true,d,0,0,error);}
