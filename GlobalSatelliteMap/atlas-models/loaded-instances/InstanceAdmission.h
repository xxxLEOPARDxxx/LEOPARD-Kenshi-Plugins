#pragma once
#include "InstancePolicy.h"
namespace LoadedInstances {
struct Stats {
 unsigned visited,selected,unsupported,filtered,unready,missingOwner,physicalReads,memberReads,perFamily[5];bool incomplete;
 Stats():visited(0),selected(0),unsupported(0),filtered(0),unready(0),missingOwner(0),physicalReads(0),memberReads(0),incomplete(false){std::memset(perFamily,0,sizeof(perFamily));}
};
template<class Ops>HRESULT ReadRoute(bool enabled,bool verified,LONG generation,volatile LONG*live,Ops&ops,Stats&out){
 ops.Clear();out=Stats();if(!enabled)return S_FALSE;if(!verified)return E_ACCESSDENIED;if(generation<=0||!live||InterlockedCompareExchange(live,0,0)!=generation)return E_ABORT;
 HRESULT hr=ops.Begin();if(FAILED(hr)){ops.Clear();return hr;}unsigned keys=ops.Keys();if(keys>EntryLimit){ops.Clear();return E_INVALIDARG;}
 bool full=false;unsigned physicalEntries=0,counts[EntryLimit];
 for(unsigned k=0;k<keys;++k){counts[k]=ops.Entries(k);if(counts[k]>EntryLimit-physicalEntries){out.incomplete=true;ops.Clear();return E_INVALIDARG;}physicalEntries+=counts[k];}
 for(unsigned k=0;k<keys&&!full;++k){unsigned count=counts[k];
  for(unsigned i=0;i<count;++i){if(out.selected==MaxRecords){out.incomplete=true;full=true;break;}++out.visited;hr=ops.Inspect(k,i,out);if(FAILED(hr)){ops.Clear();return hr;}}
 }
 if(InterlockedCompareExchange(live,0,0)!=generation||!ops.End()){ops.Clear();return E_ABORT;}
 if(!out.selected)return S_FALSE;hr=ops.Publish();if(FAILED(hr)||InterlockedCompareExchange(live,0,0)!=generation){ops.Clear();return FAILED(hr)?hr:E_ABORT;}return S_OK;
}
}
