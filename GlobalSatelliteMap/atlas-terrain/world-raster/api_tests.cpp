#include "WorldTerrainRaster.h"
#include "../compact-material/_generated/Shaders.h"
#include "../staged-material/_generated/StagedShaders.h"
#include <cstdio>
static unsigned checks=0;static void C(bool b,const char*s){++checks;if(!b){fprintf(stderr,"FAIL %s\n",s);throw 1;}}
int main(){try{using namespace WorldRaster;
 C(CarryBytes(256)==4194304&&CarryBytes(512)==16777216&&!CarryBytes(1024),"explicit bounded target sizes");C(TargetBytes(256)==1310720&&TargetBytes(512)==5242880,"all RGB normal radial and depth targets charged");C(MeshBytes(4)==697464&&MeshBytes(16)==2725464,"geometry index and allthree CBs charged");C(RequiredBytes(512,16)+WorldBank::SourceBytes==108631516,"complete512bank+producer ledger");
 Ledger l={};l.carry=CarryBytes(512);l.raster=MeshBytes(16);l.outputs=TargetBytes(512);C(Fits(512,16,l),"unified192 admits actual512 above old90 diagnostic total");l.retained=AtlasView::GpuCap-RequiredBytes(512,16)-WorldBank::SourceBytes;C(Fits(512,16,l),"exact shared ledger boundary");l.retired=1;C(!Fits(512,16,l),"one retired byte refuses");l.retired=~U64(0);C(!Fits(512,16,l),"overflow refuses");std::memset(&l,0,sizeof(l));C(!Fits(256,4,l),"omitted owner categories refuse");
 Shaders s={{CompactMaterial::VertexShader,sizeof(CompactMaterial::VertexShader)},{StagedMaterial::PartialShader,sizeof(StagedMaterial::PartialShader)},{StagedMaterial::TailShader,sizeof(StagedMaterial::TailShader)}};C(ValidShaders(s),"allthree frozen shader bytecodes");s.partial.size--;C(!ValidShaders(s),"wrong shader span refuses");
 Raster r;WorldBank::Sources bank;Frame f={};WorldMaterial::Plan p={};Material m={};C(r.BeginFrame(false,f,p,m,bank,l)==S_FALSE,"OFF before all admission/query");C(!r.Ready(f)&&!r.Output(f,0),"defaultowner cannot publish");r.Cancel();C(!r.Ready(f),"cancel default safe");
 printf("PASS %u world-raster API checks\n",checks);return 0;
}catch(...){return 1;}}
