"""Compile actual model preparation/reset bodies with counted GPU-call seams."""
from pathlib import Path
import os, subprocess
BASE = Path(__file__).resolve().parents[1]
OUT = BASE/'build/model-caller-tests'
OUT.mkdir(exist_ok=True)
SOURCE = (BASE/'GlobalSatelliteMap.cpp').read_text(encoding='utf-8')
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')

def body(name):
    a=SOURCE.index(name); b=SOURCE.index('{',a); depth=1; end=b+1
    while depth:
        depth+=(SOURCE[end]=='{')-(SOURCE[end]=='}');end+=1
    return SOURCE[a:end]

CODE=r'''
#include <windows.h>
#include <cstring>
#include <string>
#include <cstdio>
struct Resource{unsigned refs;int value;Resource():refs(1),value(0){}void AddRef(){++refs;}void Release(){--refs;}};
typedef Resource ID3D11Texture2D;typedef Resource ID3D11ShaderResourceView;
template<class T>struct Ref{T*p;Ref():p(NULL){}void Reset(){if(p)p->Release();p=NULL;}};
namespace AtlasView{typedef unsigned __int64 U64;const U64 GpuCap=201326592;U64 WindowGpuBytes(){return 189006000;}struct Uniforms{float tileBounds[4];};struct Cell{Ref<Resource>texture[3],view[3];unsigned side;Cell():side(0){}void Clear(){for(unsigned i=0;i<3;++i){texture[i].Reset();view[i].Reset();}side=0;}};}
unsigned initRaster=0,initDecode=0,initSurface=0,copies=0,rasters=0,decodes=0,clears=0,errors=0;HRESULT failRaster=S_OK,failDecode=S_OK,failCopy=S_OK;bool failInit=false;
const AtlasView::U64 Other=189006000+3408256+526416;
bool Fits(AtlasView::U64 caller,AtlasView::U64 own){return caller<=AtlasView::GpuCap-own;}
namespace AtlasModels{const AtlasView::U64 PrivateBytes=1048720;struct View{};struct Packet{float origin[3];Packet(){std::memset(origin,0,sizeof(origin));}const float*SourceSceneOrigin()const{return origin;}};
struct Raster{void Clear(){++clears;}HRESULT Init(Resource*,const void*,SIZE_T,const void*,SIZE_T,AtlasView::U64 caller){++initRaster;return !failInit&&Fits(caller,PrivateBytes)?S_OK:E_FAIL;}HRESULT Render(bool,const Packet&,const View&,AtlasView::U64 caller){++rasters;return Fits(caller,PrivateBytes)?failRaster:E_FAIL;}Resource*Output(unsigned i){static Resource r[3];return r+i;}};}
namespace AtlasModelDecode{const AtlasView::U64 PrivateBytes=1048640;struct ProvenMapping{float sourceSceneOrigin[3],sceneToCacheTranslation[3];unsigned generation;bool verified;ProvenMapping():generation(0),verified(false){std::memset(sourceSceneOrigin,0,12);std::memset(sceneToCacheTranslation,0,12);}};struct Footprint{float minX,minZ,width,height;};
struct Layer{void Clear(){++clears;}HRESULT Init(bool,Resource*,const void*,SIZE_T,AtlasView::U64 caller){++initDecode;return Fits(caller,PrivateBytes)?S_OK:E_FAIL;}HRESULT Render(bool,const AtlasModels::Packet&,const AtlasModels::View&,const Footprint&,const ProvenMapping&,Resource*const inputs[6],Resource*const outputs[3],AtlasView::U64 caller){++decodes;if(!Fits(caller,PrivateBytes)||FAILED(failDecode))return E_FAIL;for(unsigned i=0;i<3;++i){if(inputs[i+3]!=outputs[i])return E_FAIL;outputs[i]->value+=10;}return S_OK;}};}
namespace AtlasModelSurface{const AtlasView::U64 PrivateBytes=1048576;struct Layer{Resource tex[3];void Clear(){++clears;}HRESULT Init(bool,Resource*,AtlasView::U64 caller){++initSurface;return Fits(caller,PrivateBytes)?S_OK:E_FAIL;}HRESULT Copy(bool,Resource*const inputs[3],AtlasView::U64 caller){++copies;if(!Fits(caller,PrivateBytes)||FAILED(failCopy))return E_FAIL;for(unsigned i=0;i<3;++i)tex[i].value=inputs[i]->value;return S_OK;}Resource*Texture(unsigned i){return tex+i;}Resource*Output(unsigned i){return tex+i;}};}
namespace AtlasWater{const AtlasView::U64 PipelinePrivateBytes=3408256;}
namespace AtlasLocalLights{const AtlasView::U64 PrivateBytes=526416;}
namespace AtlasModelRegion{struct Bounds{bool intersects;Bounds():intersects(true){}};bool Intersects(const Bounds&b,const AtlasModelDecode::Footprint&f){return b.intersects&&f.width>0&&f.height>0;}bool View(const Bounds&,const float*,const AtlasModelDecode::Footprint&,unsigned,AtlasModels::View&){return true;}}
namespace GsmModels{const unsigned Slots=64;struct Source{bool valid;AtlasModels::Packet packet;Source():valid(false){}bool Valid(unsigned g)const{return valid&&g==7;}const AtlasModels::Packet&Geometry()const{return packet;}};struct Item{Source source;AtlasModelRegion::Bounds bounds;};struct Cache{Item items[Slots];unsigned revision;Cache():revision(1){}void Clear(){for(unsigned i=0;i<Slots;++i)items[i].source.valid=false;++revision;}const Item&At(unsigned i)const{return items[i];}};}
struct Entry{AtlasView::Cell gpu;};
bool modelOption=true,modelFailed=false,modelGpuReady=false,modelCatalogReady=false;DWORD modelCatalogAt=0;LONG observedGeneration=7;unsigned modelRenders=0;
struct Manifest{bool modelVerified;Manifest():modelVerified(true){}}manifest;
GsmModels::Cache models;AtlasModels::Raster modelRaster;AtlasModelDecode::Layer modelDecode;AtlasModelSurface::Layer modelSurface;AtlasView::Cell modelCell;AtlasModelDecode::ProvenMapping modelMapping;const void*modelProofScene=NULL;
const AtlasView::U64 ModelGpuBytes=AtlasModels::PrivateBytes+AtlasModelDecode::PrivateBytes+AtlasModelSurface::PrivateBytes;
struct NativeLight{Ref<Resource>device;}nativeLight;
const unsigned char AtlasModelVertexShader[]={0},AtlasModelPixelShader[]={0},AtlasModelDecodeShader[]={0};
void ErrorLog(const std::string&){++errors;}
ACTUAL
unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){printf("FAIL line%u %s\n",__LINE__,#x);return 1;}}while(0)
void Admit(){modelOption=true;modelFailed=false;manifest.modelVerified=true;modelMapping.verified=true;modelMapping.generation=7;models.items[0].source.valid=true;}
int main(){Resource original[3];Entry e;for(unsigned i=0;i<3;++i){original[i].value=int(i+1);e.gpu.texture[i].p=e.gpu.view[i].p=original+i;}e.gpu.side=256;AtlasView::Uniforms u={};u.tileBounds[2]=u.tileBounds[3]=2304;bool applied=true;
 modelOption=false;CHECK(PrepareModelCell(e,u,applied)==S_FALSE&&!applied&&!initRaster&&!copies);
 Admit();modelMapping.verified=false;CHECK(PrepareModelCell(e,u,applied)==S_FALSE&&!applied&&!initRaster);
 Admit();models.items[0].bounds.intersects=false;CHECK(PrepareModelCell(e,u,applied)==S_FALSE&&!applied&&!copies);models.items[0].bounds.intersects=true;
 models.items[0].source.packet.origin[0]=1;CHECK(PrepareModelCell(e,u,applied)==S_FALSE&&!copies);models.items[0].source.packet.origin[0]=0;
 CHECK(PrepareModelCell(e,u,applied)==S_OK&&applied&&modelGpuReady&&initRaster==1&&initDecode==1&&initSurface==1&&copies==1&&rasters==1&&decodes==1);
 for(unsigned i=0;i<3;++i){CHECK(original[i].value==int(i+1));CHECK(modelSurface.tex[i].value==int(i+11));CHECK(modelCell.view[i].p==modelSurface.Output(i)&&modelCell.texture[i].p==modelSurface.Texture(i));}
 CHECK(PrepareModelCell(e,u,applied)==S_OK&&applied&&initRaster==1&&initDecode==1&&initSurface==1&&copies==2&&rasters==2&&decodes==2);
 for(unsigned i=0;i<3;++i)CHECK(modelSurface.tex[i].value==int(i+11)); // pristine copied each time, no additive accumulation
 models.items[0].source.valid=false;CHECK(PrepareModelCell(e,u,applied)==S_FALSE&&!applied&&copies==2);
 Admit();failRaster=E_FAIL;CHECK(FAILED(PrepareModelCell(e,u,applied))&&!applied);ModelFailure("fixture");CHECK(modelFailed&&!modelGpuReady&&!modelMapping.verified&&!modelProofScene&&!modelCatalogReady&&!modelCell.side&&errors==1);
 for(unsigned i=0;i<3;++i)CHECK(!modelCell.view[i].p&&!modelCell.texture[i].p&&modelSurface.tex[i].refs==1);
 ModelFailure("again");CHECK(errors==1);
 Admit();failRaster=S_OK;failDecode=E_FAIL;CHECK(FAILED(PrepareModelCell(e,u,applied)));ModelFailure("decode");CHECK(!modelGpuReady&&!modelCell.side);
 Admit();failDecode=S_OK;failCopy=E_FAIL;unsigned prior=rasters;CHECK(FAILED(PrepareModelCell(e,u,applied))&&rasters==prior);ModelFailure("copy");
 Admit();failCopy=S_OK;failInit=true;CHECK(FAILED(PrepareModelCell(e,u,applied))&&!modelGpuReady);ModelFailure("init");
 CHECK(Other+ModelGpuBytes==196086608&&Other+ModelGpuBytes<AtlasView::GpuCap);
 printf("Actual model caller %u checks PASS\n",checks);return 0;}
'''.replace('ACTUAL','\n'.join(body(x) for x in ('void ResetModels()', 'void ModelFailure(const char*why){', 'bool ModelsAvailable()', 'HRESULT PrepareModelCell(')))

def run(name,code,success=True):
    cpp=OUT/(name+'.cpp');cpp.write_text(code,encoding='utf-8')
    env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
    compiled=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/W3',str(cpp),f'/Fe{OUT/name}.exe',f'/Fo{OUT/name}.obj'],env=env,capture_output=True,text=True)
    if compiled.returncode:raise RuntimeError(compiled.stdout+compiled.stderr)
    result=subprocess.run([str(OUT/(name+'.exe'))],capture_output=True,text=True)
    if (result.returncode==0)!=success:raise AssertionError(name+': '+result.stdout)
    print(name,result.stdout.strip())

run('actual_model_caller',CODE)
for name,old,new in (
 ('skip_pristine_copy','HRESULT hr=modelSurface.Copy(true,pristine,','HRESULT hr=SkipCopy(true,pristine,'),
 ('ignore_origin','!std::memcmp(modelMapping.sourceSceneOrigin,m.source.Geometry().SourceSceneOrigin(),12)','true'),
 ('recreate_each_window','if(!modelGpuReady){','if(true){'),
 ('retain_proof','modelMapping=AtlasModelDecode::ProvenMapping();',''),
 ('forget_cell_clear','modelCell.Clear();',''),
):
    assert old in CODE,name
    changed=CODE.replace(old,new)
    if name=='skip_pristine_copy':changed=changed.replace('ACTUAL_UNUSED','') .replace('unsigned checks=0;', 'HRESULT SkipCopy(bool,Resource*const*,AtlasView::U64){return S_OK;}\nunsigned checks=0;')
    # Add declaration before production bodies for this deliberately bypassed call.
    if name=='skip_pristine_copy':changed=changed.replace('void ResetModels(){','HRESULT SkipCopy(bool,Resource*const*,AtlasView::U64);\nvoid ResetModels(){')
    run(name,changed,False)
print('5 compiled actual model caller mutants rejected')
