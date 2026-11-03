#pragma once
#include "../../atlas-view/AtlasPolicy.h"
#include "../../../tools/diagnostics/satellite-capture-probe/region-catalog/RegionCatalogData.h"
namespace WorldMaterial {
typedef AtlasView::U64 U64;
enum{RefuseBudget,MonolithicCandidate,OrderedStageCandidate};
struct Bank{unsigned count;unsigned short canonical[24],upload[24];};
struct Binding{unsigned source,shaderRow;unsigned char layers[2][6];};
struct Stage{unsigned binding,channel,slot,shaderRow,selectorCode,pixelDiffuseOffset,pixelNormalOffset,vertexDistortionOffset;bool residual;Bank banks[2];unsigned char layers[2][6];};
struct Plan{U64 epoch,monoSourceBytes,stageSourceBytes;unsigned region,cell,sidCount,mode,sourceCount,rootMask,treeBytes;const char*treeSha256;Binding binding[4];Bank banks[2];Stage stage[4];unsigned char selector[4],nativeSlots[5];unsigned short sourceFiles[48],painted[2];};
// Base-snapshot metadata only. No runtime winner/admission Boolean is invented.
const U64 DiffuseLayerBytes=5592384ULL,NormalLayerBytes=2796192ULL;
const U64 PaintedPairBytes=27962024ULL,BlendBytes=5592404ULL;
const U64 FixedSourceBytes=PaintedPairBytes+BlendBytes+64+8;
const U64 SingleSidBankBytes=6*DiffuseLayerBytes+6*NormalLayerBytes;
enum{StageSide=256};
const U64 AccumulatorBytes=4ULL*StageSide*StageSide*16; // Two RGBA32F planes, two ping-pong sets; 256 only.
inline bool Fits(U64 other,U64 owned){return owned<=AtlasView::GpuCap&&other<=AtlasView::GpuCap-owned;}
class Planner{
 U64 epoch_;
 static unsigned Canonical(unsigned source){const SatelliteRegion::Catalog&c=SatelliteRegion::Data;const SatelliteRegion::Asset&a=c.assets[source];for(unsigned i=0;i<c.assetCount;++i)if(c.assets[i].format==a.format&&!std::strcmp(c.assets[i].sha256,a.sha256))return i;return source;}
 static unsigned Find(const Bank&b,unsigned key){for(unsigned i=0;i<b.count;++i)if(b.canonical[i]==key)return i;return 24;}
 static bool Banks(const unsigned*ids,unsigned count,Bank(&banks)[2],Binding*bindings){
  const SatelliteRegion::Catalog&c=SatelliteRegion::Data;
  for(unsigned kind=0;kind<2;++kind){Bank&b=banks[kind];for(unsigned sid=0;sid<count;++sid)for(unsigned role=0;role<6;++role){unsigned key=Canonical(c.biomes[ids[sid]].sources[kind][role]);if(Find(b,key)==24){if(b.count==24)return false;b.canonical[b.count++]=static_cast<unsigned short>(key);}}
   for(unsigned i=1;i<b.count;++i){unsigned short v=b.canonical[i];unsigned j=i;while(j&&b.canonical[j-1]>v){b.canonical[j]=b.canonical[j-1];--j;}b.canonical[j]=v;}
   for(unsigned layer=0;layer<b.count;++layer){unsigned upload=219;for(unsigned sid=0;sid<count;++sid)for(unsigned role=0;role<6;++role){unsigned actual=c.biomes[ids[sid]].sources[kind][role];if(Canonical(actual)==b.canonical[layer]&&actual<upload)upload=actual;}if(upload==219)return false;b.upload[layer]=static_cast<unsigned short>(upload);}
   for(unsigned sid=0;sid<count;++sid)for(unsigned role=0;role<6;++role){unsigned layer=Find(b,Canonical(c.biomes[ids[sid]].sources[kind][role]));if(layer>=b.count)return false;bindings[sid].layers[kind][role]=static_cast<unsigned char>(layer);}
  }return true;
 }
public:
 Planner():epoch_(0){}
 bool Init(U64 e){if(epoch_||!e||!SatelliteRegion::Validate(SatelliteRegion::Data)||std::strcmp(SatelliteRegion::SnapshotSha256,"5cb085baa67a7100181daf4edf25bdf3e47f5c162f7ef551fa8eb5910e565d60")||std::strcmp(SatelliteRegion::ActiveSha256,"511a8813928114dbf6b2697d5fa46a46e858c46690907cf8493e092c84633d0f"))return false;epoch_=e;return true;}
 void Clear(){epoch_=0;}
 bool Build(unsigned region,unsigned cell,U64 epoch,U64 other,Plan&out)const{
  if(!epoch_||epoch!=epoch_||region>=64||cell>=16)return false;const SatelliteRegion::Catalog&c=SatelliteRegion::Data;const SatelliteRegion::Region&r=c.regions[region];const SatelliteRegion::Cell&selected=r.cells[cell];Plan p={};p.region=region;p.cell=cell;p.epoch=epoch_;p.rootMask=selected.rootMask;p.treeBytes=selected.treeBytes;p.treeSha256=selected.treeSha256;std::memcpy(p.nativeSlots,selected.slots,5);
  unsigned ids[4]={};for(unsigned slot=0;slot<4;++slot){unsigned code=selected.selector[slot];if(!(code&15))continue;if((code&15)>r.biomeCount)return false;unsigned sid=r.biomes[(code&15)-1].source,channel=code>>4;if(channel>=5||(slot&&channel>=4)||selected.slots[channel]!=sid+1||p.sidCount==4)return false;ids[p.sidCount++]=sid;}
  if(!p.sidCount)return false;for(unsigned i=1;i<p.sidCount;++i){unsigned v=ids[i],j=i;while(j&&ids[j-1]>v){ids[j]=ids[j-1];--j;}ids[j]=v;}for(unsigned i=0;i<p.sidCount;++i){if(i&&ids[i]==ids[i-1])return false;p.binding[i].source=ids[i];unsigned original=0;while(original<r.biomeCount&&r.biomes[original].source!=ids[i])++original;if(original==r.biomeCount)return false;p.binding[i].shaderRow=original;}
  if(!Banks(ids,p.sidCount,p.banks,p.binding))return false;
  bool needed[219]={};for(unsigned sid=0;sid<p.sidCount;++sid)for(unsigned kind=0;kind<2;++kind)for(unsigned role=0;role<6;++role)needed[c.biomes[ids[sid]].sources[kind][role]]=true;for(unsigned a=0;a<219;++a)if(needed[a]){if(p.sourceCount==48)return false;p.sourceFiles[p.sourceCount++]=static_cast<unsigned short>(a);}
  for(unsigned slot=0;slot<p.sidCount;++slot){unsigned native=selected.selector[slot],sid=r.biomes[(native&15)-1].source,row=0;while(row<p.sidCount&&ids[row]!=sid)++row;if(row==p.sidCount)return false;p.selector[slot]=static_cast<unsigned char>(native);Stage&s=p.stage[slot];s.binding=row;s.channel=native>>4;s.slot=slot;s.shaderRow=(native&15)-1;s.selectorCode=native;s.pixelDiffuseOffset=240+(s.shaderRow*60+44)*4;s.pixelNormalOffset=240+(s.shaderRow*60+52)*4;s.vertexDistortionOffset=144+s.shaderRow*16;s.residual=slot==0;Binding single={};single.source=sid;if(!Banks(&sid,1,s.banks,&single))return false;std::memcpy(s.layers,single.layers,sizeof(s.layers));}
  const unsigned paint=(region/8)*8+region%8;p.painted[0]=static_cast<unsigned short>(paint);p.painted[1]=static_cast<unsigned short>(64+paint);
  p.monoSourceBytes=p.banks[0].count*DiffuseLayerBytes+p.banks[1].count*NormalLayerBytes+FixedSourceBytes;
  p.stageSourceBytes=SingleSidBankBytes+FixedSourceBytes+AccumulatorBytes;
  p.mode=Fits(other,p.monoSourceBytes)?MonolithicCandidate:Fits(other,p.stageSourceBytes)?OrderedStageCandidate:RefuseBudget;
  out=p;return true;
 }
};
}
