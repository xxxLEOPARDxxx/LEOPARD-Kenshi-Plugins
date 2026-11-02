#pragma once
#include <Windows.h>
#include <cstring>
namespace NativeLoadedCensus {
const unsigned EntryLimit=4096,EntityLimit=16,MaterialLimit=64,SubLimit=16,NameBytes=128,DeclLimit=16;
struct DeclElement {unsigned short source,type,semantic,index;unsigned __int64 offset;};
struct Material {char name[NameBytes],vertex[NameBytes],pixel[NameBytes];unsigned entity,sub,techniques,passes,textures,elements,semanticMask,typeMask,topology,indexed;unsigned __int64 vertexStart,vertices,indexStart,indices;DeclElement declaration[DeclLimit];};
struct Entity {unsigned firstMaterial,materials,subentities,registryFloor;int physicalFloor;bool shell,emissive,animated;};
struct Result {unsigned keys,visited,physicalVisited,ordinary,instanced,other,missingOwner,unregistered,unready,unsupported,duplicates,nameOverflow,selected,materialCount;bool incomplete,physicalBudgetExhausted;Entity entities[EntityLimit];Material materials[MaterialLimit];Result(){std::memset(this,0,sizeof(*this));}};
inline bool SelectionRoom(Result&r){if(r.selected>=EntityLimit){r.incomplete=true;return false;}return true;}
inline bool MaterialRoom(Result&r,size_t count){if(!count||count>SubLimit){++r.unsupported;r.incomplete=true;return false;}if(r.materialCount>MaterialLimit||count>MaterialLimit-r.materialCount){r.incomplete=true;return false;}return true;}
inline bool ValidArray(unsigned __int64 pointer,unsigned __int64 count,unsigned __int64 capacity,unsigned stride){if(!stride||count>EntryLimit||capacity<count)return false;if(!count)return true;return pointer&&pointer%8==0&&count<=(~0ULL-pointer)/stride;}
inline bool CopyName(const char*name,size_t size,char out[NameBytes]){out[0]=0;if(!name||size>=NameBytes)return false;for(size_t i=0;i<size;++i)if(static_cast<unsigned char>(name[i])<32||name[i]==127)return false;std::memcpy(out,name,size);out[size]=0;return true;}
template<class Ops>HRESULT ReadRoute(bool enabled,bool verified,LONG generation,Ops&ops,Result&out,void*&candidate){out=Result();candidate=NULL;if(!enabled)return S_FALSE;if(!verified||!generation)return E_ACCESSDENIED;Result next;HRESULT hr=ops.Begin();if(FAILED(hr))return hr;
 const unsigned keys=ops.Keys();if(keys>EntryLimit)return E_INVALIDARG;next.keys=keys;void*chosen=NULL;
 for(unsigned k=0;k<keys;++k){const unsigned count=ops.Entries(k);if(count>EntryLimit)return E_INVALIDARG;
  for(unsigned n=0;n<count;++n){if(next.visited==EntryLimit){next.incomplete=true;break;}++next.visited;void*found=NULL;hr=ops.Inspect(k,n,next,found);if(FAILED(hr))return hr;if(!chosen&&found)chosen=found;}
  if(next.visited==EntryLimit){if(k+1<keys)next.incomplete=true;break;}
 }
 if(!ops.End(chosen))return E_ABORT;out=next;candidate=chosen;return S_OK;}
}
