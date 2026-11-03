#define main ExistingPackedTests
#include "../../atlas-models/loaded-instances/warp_tests.cpp"
#undef main
int main(){try{
 Device d;Fixture f(d,Dual43,Coloured64,false);Packet a,b;
 Hr(a.Adopt(d.d.p,&f.raw,1,f.origin,7),"first held packet");Hr(b.Adopt(d.d.p,&f.raw,1,f.origin,7),"same resources copied independently");
 C(a.SameContents(b)&&b.SameContents(a),"unchanged capture retains revision");
 D3D11_SAMPLER_DESC sd;f.sampler.p->GetDesc(&sd);Com<ID3D11SamplerState>sampler;Hr(d.d.p->CreateSamplerState(&sd,&sampler.p),"new sampler owner");RawDraw fresh=f.raw;fresh.sampler=sampler.p;
 Packet c;Hr(c.Adopt(d.d.p,&fresh,1,f.origin,7),"new sampler equal settings");C(a.SameContents(c),"sampler allocation does not invalidate town");
 fresh.rawWorld[3]+=1;Packet moved;Hr(moved.Adopt(d.d.p,&fresh,1,f.origin,7),"moved instance");C(!a.SameContents(moved),"changed pose invalidates town");
 fresh=f.raw;fresh.surface.dustColour[0]+=.1f;Packet material;Hr(material.Adopt(d.d.p,&fresh,1,f.origin,7),"changed native material");C(!a.SameContents(material),"material value invalidates town");
 float origin[3]={f.origin[0]+1,f.origin[1],f.origin[2]};Packet shifted;Hr(shifted.Adopt(d.d.p,&f.raw,1,origin,7),"new source origin");C(!a.SameContents(shifted),"floating origin invalidates town");
 Packet world;Hr(world.Adopt(d.d.p,&f.raw,1,f.origin,8),"new world generation");C(!a.SameContents(world),"world invalidates town");
 Packet empty;C(!a.SameContents(empty),"unloaded town invalidates old packet");
 sd.AddressU=D3D11_TEXTURE_ADDRESS_CLAMP;Com<ID3D11SamplerState>changedSampler;Hr(d.d.p->CreateSamplerState(&sd,&changedSampler.p),"changed sampler value");fresh=f.raw;fresh.sampler=changedSampler.p;Packet filtering;Hr(filtering.Adopt(d.d.p,&fresh,1,f.origin,7),"changed filtering");C(!a.SameContents(filtering),"sampler value invalidates town");
 std::printf("PASS %u loaded-town packet refresh controls\n",checks);return 0;
 }catch(...){return 1;}}
