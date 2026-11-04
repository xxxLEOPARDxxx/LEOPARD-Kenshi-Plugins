#define main ExistingPackedTests
#include "../../atlas-models/loaded-instances/warp_tests.cpp"
#undef main
#include "../../atlas-models/loaded-instances/InstanceAdmission.h"
struct Route {
 unsigned ends,published,clears;bool end,changed;HRESULT result;volatile LONG*live;
 Route():ends(0),published(0),clears(0),end(true),changed(false),result(S_FALSE),live(NULL){}
 void Clear(){++clears;}HRESULT Begin(){return S_OK;}unsigned Keys(){return 1;}unsigned Entries(unsigned){return 1;}
 HRESULT Inspect(unsigned,unsigned,Stats&s){++s.selected;return S_OK;}
 bool End(){++ends;return end;}HRESULT Publish(){++published;if(changed)InterlockedIncrement(live);return result;}
};
int main(){try{
 Device d;Fixture f(d,Dual43,Coloured64,false);Packet held;Hr(held.Adopt(d.d.p,&f.raw,1,f.origin,7),"held immutable packet");
 D3D11_SAMPLER_DESC sampler={};f.sampler.p->GetDesc(&sampler);RawDraw raw=f.raw;raw.sampler=NULL;
 C(held.MatchesRaw(d.d.p,&raw,&sampler,1,f.origin,7),"fully validated unchanged raw matches before sampler creation");
 C(!held.MatchesRaw(NULL,&raw,&sampler,1,f.origin,7)&&!held.MatchesRaw(d.d.p,NULL,&sampler,1,f.origin,7)&&!held.MatchesRaw(d.d.p,&raw,NULL,1,f.origin,7)&&!held.MatchesRaw(d.d.p,&raw,&sampler,0,f.origin,7)&&!held.MatchesRaw(d.d.p,&raw,&sampler,257,f.origin,7)&&!held.MatchesRaw(d.d.p,&raw,&sampler,1,NULL,7),"invalid arguments reject before raw pointer traversal");
 Packet empty;C(!empty.MatchesRaw(d.d.p,&raw,&sampler,1,f.origin,7),"empty old packet cannot yield unchanged");
 for(unsigned i=0;i<16;++i){raw=f.raw;raw.rawWorld[i]+=1;C(!held.MatchesRaw(d.d.p,&raw,&sampler,1,f.origin,7),"every raw pose component invalidates");}
 for(unsigned i=0;i<6;++i){raw=f.raw;raw.localBounds[i]+=1;C(!held.MatchesRaw(d.d.p,&raw,&sampler,1,f.origin,7),"every local bound invalidates");}
 for(unsigned i=0;i<13;++i){raw=f.raw;reinterpret_cast<float*>(&raw.surface)[i]+=.125f;C(!held.MatchesRaw(d.d.p,&raw,&sampler,1,f.origin,7),"every native surface parameter invalidates");}
 for(unsigned i=0;i<sizeof(Shape)/4;++i){raw=f.raw;reinterpret_cast<unsigned*>(&raw.shape)[i]^=1;C(!held.MatchesRaw(d.d.p,&raw,&sampler,1,f.origin,7),"every retained geometry shape scalar invalidates");}
 raw=f.raw;for(unsigned i=0;i<sizeof(D3D11_SAMPLER_DESC);++i){D3D11_SAMPLER_DESC changed=sampler;reinterpret_cast<BYTE*>(&changed)[i]^=1;C(!held.MatchesRaw(d.d.p,&raw,&changed,1,f.origin,7),"every sampler descriptor byte invalidates");}raw=f.raw;
 for(unsigned i=0;i<7;++i){raw=f.raw;raw.texture[i]=reinterpret_cast<ID3D11ShaderResourceView*>(1);C(!held.MatchesRaw(d.d.p,&raw,&sampler,1,f.origin,7),"changed or invalid view token rejects without dereference");}
 raw=f.raw;raw.vertex=reinterpret_cast<ID3D11Buffer*>(1);C(!held.MatchesRaw(d.d.p,&raw,&sampler,1,f.origin,7),"changed vertex identity rejects without raw dereference");raw=f.raw;raw.index=reinterpret_cast<ID3D11Buffer*>(1);C(!held.MatchesRaw(d.d.p,&raw,&sampler,1,f.origin,7),"changed index identity rejects without raw dereference");
 raw=f.raw;raw.kind=Alpha48;C(!held.MatchesRaw(d.d.p,&raw,&sampler,1,f.origin,7),"changed material kind invalidates");raw=f.raw;raw.cullMode=1;C(!held.MatchesRaw(d.d.p,&raw,&sampler,1,f.origin,7),"changed native culling invalidates");raw=f.raw;raw.verifiedBlack=false;C(!held.MatchesRaw(d.d.p,&raw,&sampler,1,f.origin,7),"unverified material cannot yield unchanged");raw=f.raw;
 float origin[3];std::memcpy(origin,f.origin,12);for(unsigned i=0;i<3;++i){origin[i]+=1;C(!held.MatchesRaw(d.d.p,&raw,&sampler,1,origin,7),"every origin component invalidates");origin[i]-=1;}C(!held.MatchesRaw(d.d.p,&raw,&sampler,1,f.origin,8),"changed world generation invalidates");
 Device foreign;C(!held.MatchesRaw(foreign.d.p,&raw,&sampler,1,f.origin,7),"foreign device invalidates even same raw identities");
 Com<ID3D11ShaderResourceView>newView;Hr(d.d.p->CreateShaderResourceView(f.tex[0].p,NULL,&newView.p),"distinct view of same resource");raw.texture[0]=newView.p;C(!held.MatchesRaw(d.d.p,&raw,&sampler,1,f.origin,7),"same texture under different SRV is conservatively refreshed");raw=f.raw;
 RawDraw two[2]={f.raw,f.raw};two[1].rawWorld[3]+=1;D3D11_SAMPLER_DESC descriptors[2]={sampler,sampler};descriptors[1].MipLODBias+=.25f;Com<ID3D11SamplerState>secondSampler;Hr(d.d.p->CreateSamplerState(&descriptors[1],&secondSampler.p),"different second draw sampler");two[1].sampler=secondSampler.p;Packet multi;Hr(multi.Adopt(d.d.p,two,2,f.origin,7),"two retained draws");two[0].sampler=two[1].sampler=NULL;C(multi.MatchesRaw(d.d.p,two,descriptors,2,f.origin,7),"all raw draws match their ordered sampler descriptors");C(!held.MatchesRaw(d.d.p,two,descriptors,2,f.origin,7),"changed selected count cannot reuse shorter packet");two[1].rawWorld[3]+=1;C(!multi.MatchesRaw(d.d.p,two,descriptors,2,f.origin,7),"second draw pose checked");two[1].rawWorld[3]-=1;descriptors[1].MipLODBias+=.25f;C(!multi.MatchesRaw(d.d.p,two,descriptors,2,f.origin,7),"second sampler descriptor checked");descriptors[1].MipLODBias-=.25f;
 Fixture alpha(d,Alpha48,Plain60,false);Packet alphaPacket;Hr(alphaPacket.Adopt(d.d.p,&alpha.raw,1,alpha.origin,7),"alpha retained packet");RawDraw alphaRaw=alpha.raw;D3D11_SAMPLER_DESC alphaSampler;alpha.sampler.p->GetDesc(&alphaSampler);C(alphaPacket.MatchesRaw(d.d.p,&alphaRaw,&alphaSampler,1,alpha.origin,7),"alpha packet exactly reusable");alphaRaw.texture[4]=reinterpret_cast<ID3D11ShaderResourceView*>(1);C(!alphaPacket.MatchesRaw(d.d.p,&alphaRaw,&alphaSampler,1,alpha.origin,7),"unexpected unused alpha texture rejects without dereference");
 Packet before;Hr(before.Adopt(d.d.p,&raw,1,f.origin,7),"unchanged old geometry control");for(unsigned i=0;i<20;++i)C(held.MatchesRaw(d.d.p,&raw,&sampler,1,f.origin,7)&&held.SameContents(before),"repeated matching never withdraws old geometry");
 volatile LONG live=7;Route route;route.live=&live;Stats stats;C(ReadRoute(true,true,7,&live,route,stats)==S_FALSE&&route.ends==1&&route.published==1,"actual route preserves explicit unchanged result after End");route.end=false;unsigned published=route.published;C(ReadRoute(true,true,7,&live,route,stats)==E_ABORT&&route.published==published,"failed world End cannot take unchanged path");route.end=true;route.changed=true;C(ReadRoute(true,true,7,&live,route,stats)==E_ABORT,"generation change during unchanged publication aborts");route.changed=false;live=7;route.result=S_OK;C(ReadRoute(true,true,7,&live,route,stats)==S_OK,"old fresh publish route remains compatible");
 std::printf("PASS %u loaded-town exact raw reuse/route controls\n",checks);return 0;
 }catch(...){return 1;}}
