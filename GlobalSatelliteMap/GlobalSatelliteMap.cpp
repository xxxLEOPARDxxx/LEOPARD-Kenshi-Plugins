// Development-only global-map capture viewer. Does not capture/render the world.
// All owned UI is a child overlay; native mapImage texture/input remain untouched.
#define NOMINMAX
#define KLOC_DOMAIN "global_satellite_map"
#include <Localization.h>
#include <ModConfigMenu.h>
#include <WidgetRef.h>
#include <IniDefaults.h>
#include "CaptureValidation.h"
#include <Debug.h>
#include <core/Functions.h>
#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_ImageBox.h>
#include <mygui/MyGUI_TextBox.h>
#include <mygui/MyGUI_EditBox.h>
#include <mygui/MyGUI_RenderManager.h>
#include <mygui/MyGUI_ScrollView.h>
#include <OgreTextureManager.h>
#include <OgreImage.h>
#include <OgreResourceGroupManager.h>
#include <kenshi/Globals.h>
#include <kenshi/SaveFileSystem.h>
#include <kenshi/Town.h>
#define protected public
#define private public
#include <kenshi/gui/MapScreen.h>
#undef private
#undef protected
#include <cmath>
#include <sstream>
#include <memory>
#include "atlas-view/AtlasWindowPolicy.h"
#include "GsmAtlasJob.h"
#include "GsmNativeLight.h"
#include "GsmViewerClock.h"
#include "build/GsmRelightShader.h"
#include "atlas-water/NativeWaterSnapshot.h"
#include "atlas-water/WaterPipeline.h"
#include "atlas-water/_generated/pipeline/WaterPipelineShaders.h"
#include "atlas-lights/NativeLocalLightSnapshot.h"
#include "GsmLocalDiagnostic.h"
#include "atlas-lights/OriginProof.h"
#include "GsmTownDiagnostic.h"
#include "GsmLocalMapping.h"
#include "GsmLocalRevision.h"
#include "atlas-lights/terrain-hdr/AtlasLocalLights.h"
#include "build/GsmLocalShader.h"
#include "GsmModelDiagnostic.h"
#include "GsmTownValues.h"
#include "GsmModelCache.h"
#include "GsmCachedTownState.h"
#include "atlas-models/surface/AtlasModelSurface.h"
#include "atlas-models/decode/_generated/ModelDecodeShader.h"
#include "GsmZoomPolicy.h"
#include "GsmZoomStep.h"
#include "build/GsmZoomProfile.h"
#include "GsmFineGpu.h"
#include "GsmFinePolicy.h"
#include "atlas-terrain/world-native/NativeProvider.h"
#include "atlas-terrain/fine-coordinator/FineCoordinator.h"
#include "atlas-terrain/native-region-material/NativeAdapter.h"
#include "atlas-terrain/compact-material/_generated/Shaders.h"
#include "atlas-terrain/staged-material/_generated/StagedShaders.h"
#include <Compositor/OgreCompositorWorkspaceListener.h>
#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"advapi32.lib")
#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"windowscodecs.lib")
extern "C" IMAGE_DOS_HEADER __ImageBase;
namespace {
const char*const ID="GlobalSatelliteMap";
const char*const Group="LEOPARD_GlobalSatelliteMap_Development";
int zoomMultiplier=1;GsmZoom::Lease zoomLease;WidgetRef zoomOwner;bool zoomFailed=false;
void(*originalZoomIn)(MapScreen*,MyGUI::Widget*)=NULL;
void(*originalZoomOut)(MapScreen*,MyGUI::Widget*)=NULL;
void(*originalZoomWheel)(MapScreen*,MyGUI::Widget*,int)=NULL;
bool zoomStepHooksReady=false;unsigned zoomStepDepth=0;
bool enabled=false,debug=false,sunlight=true,ambient=true,attempted=false,ready=false,failed=false,groupCreated=false,hookFailure=false,jobQuarantined=false;
LONG observedGeneration=-1;DWORD statusUntil=0;GsmViewer::TickClock tickClock;GsmViewer::LightClock lightClock;
unsigned serial=0,lightVersion=0,shadeCursor=0,cacheAge=0;
unsigned windowReads=0;AtlasView::U64 windowBytes=0;DWORD statsAt=0;
bool waterOption=false,waterAttempted=false,waterFailed=false,waterPipelineReady=false;
unsigned waterVersion=1;GsmViewer::LightClock waterClock;
NativeWater::Snapshot waterSnapshot;AtlasWater::Pipeline waterPipeline;
GsmLocalDiagnostic::Bridge<NativeLocalLights::NativeSnapshot> localDiagnostic;
DWORD localDiagnosticAt=0;bool localDiagnosticLogged=false;
NativeOriginProof::Reader originReader;DWORD originDiagnosticAt=0;bool originDiagnosticLogged=false;
bool localOption=false,localFailed=false,localLayerReady=false;
unsigned localVersion=1;
AtlasLocalLights::Layer localLayer;AtlasLocalLights::ProvenMapping localMapping;
const void*localProofScene=NULL;
GsmLocalRevision::State<NativeLocalLights::LightValue,NativeLocalLights::Limit> localRevision;
GsmModelDiagnostic::Once distantModelDiagnostic,loadedModelDiagnostic;
bool townValuesLogged=false;
bool modelOption=false,modelAttempted=false,modelFailed=false,modelGpuReady=false;
DWORD modelCatalogAt=0;bool modelCatalogReady=false;
GsmModels::Cache models;AtlasModels::Raster modelRaster;AtlasModelDecode::Layer modelDecode;AtlasModelSurface::Layer modelSurface;AtlasView::Cell modelCell;
AtlasModelDecode::ProvenMapping modelMapping;const void*modelProofScene=NULL;
unsigned modelRenders=0;
void ModelFailure(const char*why);
const AtlasView::U64 ModelGpuBytes=AtlasModels::PrivateBytes+AtlasModelDecode::PrivateBytes+AtlasModelSurface::PrivateBytes;
GsmCachedTownState::State cachedTown;std::string cachedSelectionError;
void CachedTownCompletion(const GsmAtlas::Job&);
void RetireGpuCharge(AtlasView::U64);void FineCancel();void FineObserve(MapScreen*);void FinePresent(MapScreen*);bool FineQualified();bool FinePending();AtlasView::U64 FineGpuBytes();AtlasView::U64 ProducerGpuBytes();
void ResetModels(){RetireGpuCharge((modelGpuReady?ModelGpuBytes:0)+cachedTown.OwnedGpuBytes());cachedSelectionError.clear();if(!cachedTown.Clear())ErrorLog("GlobalSatelliteMap: cached town cleanup refused; private slot retained");modelCell.Clear();modelSurface.Clear();modelDecode.Clear();modelRaster.Clear();models.Clear();modelGpuReady=false;modelMapping=AtlasModelDecode::ProvenMapping();modelProofScene=NULL;modelCatalogReady=false;modelCatalogAt=0;}
unsigned localRenders=0,localBatches=0;
void ResetLocal(){RetireGpuCharge(localLayerReady?AtlasLocalLights::PrivateBytes:0);localLayer.Clear();localLayerReady=localFailed=false;localMapping=AtlasLocalLights::ProvenMapping();localProofScene=NULL;localRevision.Reset();if(++localVersion==0)localVersion=1;}
int projectedWidth=0,projectedHeight=0;
MyGUI::IntCoord projectedClip;bool presentationCurrent=false;
GSM::Session session;GsmAtlas::Manifest manifest;std::string verifiedGameRoot;
GsmAtlas::Job*job=NULL,*completed=NULL;
WidgetRef layer,status,tiles[AtlasView::WindowSlots];
WidgetRef diskWarning;
AtlasView::Relighter relighter;GsmNative::Light nativeLight;
AtlasView::Uniforms lighting={};bool haveLighting=false;
struct Entry {Ogre::TexturePtr texture;std::string name;AtlasView::Cell gpu;AtlasView::Com<ID3D11Texture2D>output;AtlasView::WindowKey key;unsigned shaded,age,waterStamp,localStamp,localChecked,modelStamp;float minHeight;bool hasHeight,waterApplied,localApplied,localAffects,modelApplied;Entry():shaded(0),age(0),waterStamp(0),localStamp(0),localChecked(0),modelStamp(0),minHeight(0),hasHeight(false),waterApplied(false),localApplied(false),localAffects(false),modelApplied(false){}};
Entry cache[AtlasView::WindowSlots];
GsmFineGpu::Retirements gpuRetired;bool gpuRetirementFailed=false;
AtlasView::Com<ID3D11Device> gpuRetirementDevice;
void RetireGpuCharge(AtlasView::U64 bytes){if(bytes&&!gpuRetired.Charge(bytes)){gpuRetirementFailed=true;ErrorLog("GlobalSatelliteMap: GPU retirement ledger full; further allocation blocked");}}
AtlasView::U64 BaseGpuBytes(){AtlasView::U64 bytes=0;for(unsigned i=0;i<AtlasView::WindowSlots;++i)if(cache[i].key.Valid())bytes+=GsmFineGpu::WindowBytes;return bytes;}
AtlasView::U64 ViewerGpuBytes(){return BaseGpuBytes()+(ready?GsmFineGpu::RelightBytes:0)+(waterPipelineReady?AtlasWater::PipelinePrivateBytes:0)+(localLayerReady?AtlasLocalLights::PrivateBytes:0)+(modelGpuReady?ModelGpuBytes:0)+cachedTown.OwnedGpuBytes();}
AtlasView::U64 AggregateGpuBytes(){return ViewerGpuBytes()+FineGpuBytes()+gpuRetired.Bytes()+ProducerGpuBytes();}
AtlasView::U64 OtherGpuBytes(AtlasView::U64 excluded){AtlasView::U64 total=AggregateGpuBytes();return total>=excluded?total-excluded:AtlasView::GpuCap;}
bool AdmitGpu(AtlasView::U64 bytes){return !gpuRetirementFailed&&!gpuRetired.Failed()&&GsmFineGpu::Fits(ViewerGpuBytes()+FineGpuBytes(),gpuRetired.Bytes(),ProducerGpuBytes(),bytes);}
unsigned BaseDetailCapacity(){return FineQualified()?4:AtlasView::DetailSlots;}
void(*originalUpdate)(MapScreen*)=NULL;void(*originalLoad)(SaveFileSystem*,const std::string&)=NULL;void(*originalNew)(SaveFileSystem*)=NULL;
std::string Folder(){wchar_t p[MAX_PATH]={0};DWORD n=GetModuleFileNameW(reinterpret_cast<HMODULE>(&__ImageBase),p,MAX_PATH);if(!n||n>=MAX_PATH)return std::string();std::wstring w(p,n);size_t at=w.find_last_of(L"/\\");return at==w.npos?std::string():GsmAtlas::Utf8(w.substr(0,at+1));}
std::string Exe(){wchar_t p[MAX_PATH]={0};DWORD n=GetModuleFileNameW(NULL,p,MAX_PATH);return !n||n>=MAX_PATH?std::string():GsmAtlas::Utf8(std::wstring(p,n));}
std::string Game()
{
    std::string p = Exe();
    size_t at = p.find_last_of("/\\");
    if (at == p.npos)
        return std::string();
    p.erase(at + 1);
    if (GetFileAttributesW(GsmAtlas::Wide(p+"data/mods.cfg").c_str()) != INVALID_FILE_ATTRIBUTES)
        return p;
    at = p.find_last_of("/\\", p.size() - 2);
    if (at == p.npos)
        return std::string();
    p.erase(at + 1);
    return GetFileAttributesW(GsmAtlas::Wide(p+"data/mods.cfg").c_str()) != INVALID_FILE_ATTRIBUTES
               ? p
               : std::string();
}

// Shared IniDefaults/MCM use Windows ACP. Source/capture paths above are UTF8/W.
std::string Ini(){char p[MAX_PATH]={0};DWORD n=GetModuleFileNameA(reinterpret_cast<HMODULE>(&__ImageBase),p,MAX_PATH);if(!n||n>=MAX_PATH)throw std::runtime_error("plugin INI path unavailable");std::string ini(p,n);ini.erase(ini.find_last_of("/\\")+1);return IniDefaults::Prepare(ini+"GlobalSatelliteMap.ini");}
void Settings(){
    std::string ini=Ini();bool on=!hookFailure&&GetPrivateProfileIntA(ID,"Enabled",0,ini.c_str())!=0;
    bool s=GetPrivateProfileIntA(ID,"Sunlight",1,ini.c_str())!=0,a=GetPrivateProfileIntA(ID,"AmbientLight",1,ini.c_str())!=0;
    if(on!=enabled){enabled=on;InterlockedIncrement(&GSM::generation);zoomLease.AllowRequest();zoomFailed=false;}
    int factor=GetPrivateProfileIntA(ID,"ZoomMultiplier",1,ini.c_str());factor=(std::max)(1,(std::min)(8,factor));
    if(factor!=zoomMultiplier){zoomMultiplier=factor;zoomLease.AllowRequest();zoomFailed=false;}
    if(s!=sunlight||a!=ambient){sunlight=s;ambient=a;lightClock.Reset();haveLighting=false;waterClock.Reset();}
    bool w=GetPrivateProfileIntA(ID,"Water",0,ini.c_str())!=0;
    if(w!=waterOption){waterOption=w;RetireGpuCharge(waterPipelineReady?AtlasWater::PipelinePrivateBytes:0);waterSnapshot.Clear();waterPipeline.Clear();waterPipelineReady=false;waterClock.Reset();if(waterFailed){waterAttempted=false;manifest.waterVerified=false;}waterFailed=false;}
    bool l=GetPrivateProfileIntA(ID,"LocalLights",0,ini.c_str())!=0;
    if(l!=localOption){localOption=l;ResetLocal();localDiagnostic.Reset();originDiagnosticLogged=false;}
    bool m=GetPrivateProfileIntA(ID,"CityModels",0,ini.c_str())!=0;
    if(m!=modelOption){modelOption=m;ResetModels();modelAttempted=modelFailed=false;manifest.modelVerified=false;originDiagnosticLogged=false;}
    debug=GetPrivateProfileIntA(ID,"Debug",0,ini.c_str())!=0;KLocLog::SetEnabled(debug);
}
void Diagnostic(const std::string&s){if(debug)DebugLog("GlobalSatelliteMap: "+s);}
const char* DiskWarningText(){return Tr("WARNING: Before rendering 4x4 tiles at 8192x8192, allow at least 24 GB free.\nUncompressed terrain data: about 23 GB.");}
void __cdecl DetachDiskWarning(void*)
{
    if(diskWarning.get())MyGUI::Gui::getInstance().destroyWidget(diskWarning.get());
    diskWarning.reset();
}
void __cdecl AttachDiskWarning(void*,void*parent)
{
    MyGUI::Widget*p=static_cast<MyGUI::Widget*>(parent);
    if(!p||p->getWidth()<=8||p->getHeight()<=4)return;
    // MCM refreshes custom areas periodically. Keep one child, with native
    // stretch layout; no repeated allocations, translation or disk polling.
    const int inset=(std::max)(4,int(p->getWidth()*.03f));
    const int top=(std::min)(10,(p->getHeight()-2)/2);
    const MyGUI::IntCoord position(inset,top,p->getWidth()-inset*2,p->getHeight()-top-2);
    if(diskWarning.get()&&diskWarning.get()->getParent()==p){if(diskWarning.get()->getCoord()!=position)diskWarning.get()->setCoord(position);return;}
    DetachDiskWarning(NULL);
    MyGUI::EditBox*w=p->createWidget<MyGUI::EditBox>("Kenshi_WordWrapEmpty",
        position,MyGUI::Align::Stretch,"");
    diskWarning.set(w);
    w->setNeedMouseFocus(false);w->setNeedKeyFocus(false);
    w->setEditMultiLine(true);w->setEditWordWrap(true);w->setEditStatic(true);
    w->setTextAlign(MyGUI::Align::Left|MyGUI::Align::Top);
    w->setTextColour(MyGUI::Colour(1.0f,0.2f,0.2f));
    w->setCaption(MyGUI::UString(DiskWarningText()));
}
void SetStage(ID3D11DeviceContext*c,unsigned stage,unsigned slot,ID3D11ShaderResourceView*v){switch(stage){case 0:c->VSSetShaderResources(slot,1,&v);break;case 1:c->HSSetShaderResources(slot,1,&v);break;case 2:c->DSSetShaderResources(slot,1,&v);break;case 3:c->GSSetShaderResources(slot,1,&v);break;case 4:c->PSSetShaderResources(slot,1,&v);break;default:c->CSSetShaderResources(slot,1,&v);}}
void RetireReads(ID3D11Texture2D*output){if(!output)return;AtlasView::Com<ID3D11Device>d;output->GetDevice(&d.p);AtlasView::Com<ID3D11DeviceContext>c;d.p->GetImmediateContext(&c.p);for(unsigned stage=0;stage<6;++stage){ID3D11ShaderResourceView*v[128]={0};switch(stage){case 0:c.p->VSGetShaderResources(0,128,v);break;case 1:c.p->HSGetShaderResources(0,128,v);break;case 2:c.p->DSGetShaderResources(0,128,v);break;case 3:c.p->GSGetShaderResources(0,128,v);break;case 4:c.p->PSGetShaderResources(0,128,v);break;default:c.p->CSGetShaderResources(0,128,v);}for(unsigned i=0;i<128;++i)if(v[i]){if(AtlasView::SameResource(v[i],output))SetStage(c.p,stage,i,NULL);v[i]->Release();}}}
void DropEntry(int i){Entry&e=cache[i];if(!e.key.Valid())return;ID3D11Texture2D*planes[3]={e.gpu.texture[0].p,e.gpu.texture[1].p,e.gpu.texture[2].p};if(!gpuRetired.Add(planes,e.output.p,GsmFineGpu::WindowBytes)){gpuRetirementFailed=true;return;}if(tiles[i].get()){MyGUI::ImageBox*w=static_cast<MyGUI::ImageBox*>(tiles[i].get());w->setImageTexture("");w->setVisible(false);}MyGUI::RenderManager*rm=MyGUI::RenderManager::getInstancePtr();if(rm&&!e.name.empty()){MyGUI::ITexture*t=rm->getTexture(e.name);if(t)rm->destroyTexture(t);}RetireReads(e.output.p);e.output.Reset();e.gpu.Clear();e.texture.setNull();Ogre::TextureManager*tm=Ogre::TextureManager::getSingletonPtr();if(tm&&!e.name.empty()&&!tm->getByName(e.name).isNull())tm->remove(e.name);e.name.clear();e.key=AtlasView::WindowKey();e.shaded=e.age=0;}
void Cancel(){if(job)InterlockedExchange(&job->read.cancel,1);if(completed){delete completed;completed=NULL;}}
void Clear(){presentationCurrent=false;FineCancel();Cancel();ResetModels();modelAttempted=modelFailed=false;modelRenders=0;townValuesLogged=false;distantModelDiagnostic.Reset();loadedModelDiagnostic.Reset();localRenders=localBatches=0;ResetLocal();localDiagnostic.Reset();localDiagnosticLogged=originDiagnosticLogged=false;tickClock.Reset();RetireGpuCharge((ready?GsmFineGpu::RelightBytes:0)+(waterPipelineReady?AtlasWater::PipelinePrivateBytes:0));ready=attempted=failed=false;haveLighting=false;lightClock.Reset();waterSnapshot.Clear();waterPipeline.Clear();waterPipelineReady=waterAttempted=waterFailed=false;waterClock.Reset();waterVersion=1;for(unsigned i=0;i<AtlasView::WindowSlots;++i)DropEntry(i);relighter.Clear();nativeLight.Clear();if(layer.get())MyGUI::Gui::getInstance().destroyWidget(layer.get());layer.reset();for(unsigned i=0;i<AtlasView::WindowSlots;++i)tiles[i].reset();if(status.get())MyGUI::Gui::getInstance().destroyWidget(status.get());status.reset();statusUntil=0;manifest=GsmAtlas::Manifest();}
void Notice(MapScreen*map,bool drawFailure){if(!map||status.get()||!map->mainWidget)return;MyGUI::Widget*p=map->mainWidget;MyGUI::TextBox*w=p->createWidget<MyGUI::TextBox>("Kenshi_TextboxStandardText",MyGUI::IntCoord(12,12,p->getWidth()>24?p->getWidth()-24:300,56),MyGUI::Align::Default,"");status.set(w);w->setNeedMouseFocus(false);w->setNeedKeyFocus(false);w->setCaption(MyGUI::UString(drawFailure?Tr("Satellite terrain could not be drawn. Prepared data is retained. The normal map remains visible."):Tr("Satellite terrain is unavailable. Prepared terrain data is missing or does not match this game. The normal map remains visible.")));statusUntil=GetTickCount()+12000;}
void Fail(MapScreen*map,const std::string&why){bool drawFailure=ready;Clear();attempted=failed=true;ErrorLog("GlobalSatelliteMap: "+why);Notice(map,drawFailure);}
std::string ModulePath(const char*name){wchar_t path[MAX_PATH]={0};HMODULE h=GetModuleHandleA(name);DWORD n=h?GetModuleFileNameW(h,path,MAX_PATH):0;if(!n||n>=MAX_PATH)throw std::runtime_error("native renderer module path missing");return GsmAtlas::Utf8(std::wstring(path,n));}
void Launch(GsmAtlas::Job*j){if(job||jobQuarantined){delete j;throw std::runtime_error("GST2 worker still owned");}j->generation=InterlockedCompareExchange(&GSM::generation,0,0);j->thread=CreateThread(NULL,0,GsmAtlas::Run,j,0,NULL);if(!j->thread){delete j;throw std::runtime_error("GST2 bounded worker creation failed");}job=j;}
void StartValidation(){attempted=true;std::auto_ptr<GsmAtlas::Job>owned(new GsmAtlas::Job());GsmAtlas::Job*j=owned.get();j->validation=true;j->folder=Folder()+"captures/";j->game=Game();j->engine=Exe();j->module[0]=ModulePath(GsmOgreFile);j->module[1]=ModulePath(GsmD3d11File);if(j->game.empty()||j->engine.empty())throw std::runtime_error("game source directory unavailable");Launch(owned.release());Diagnostic("validating complete terrain atlas and immutable world/mod source profile in bounded file worker");}
void Poll(MapScreen*map){
    if(!job||jobQuarantined)return;
    DWORD wait=WaitForSingleObject(job->thread,0);if(wait==WAIT_TIMEOUT)return;
    if(wait!=WAIT_OBJECT_0||!CloseHandle(job->thread)){jobQuarantined=true;throw std::runtime_error("GST2 worker handle state uncertain; retained without retry");}
    job->thread=NULL;GsmAtlas::Job*j=job;job=NULL;
    bool current=j->generation==InterlockedCompareExchange(&GSM::generation,0,0)&&!InterlockedCompareExchange(&j->read.cancel,0,0);
    if(!current){delete j;return;}
    if(j->cachedTown){std::auto_ptr<GsmAtlas::Job>owned(j);CachedTownCompletion(*owned);return;}
    if(j->modelValidation){manifest.modelVerified=j->ok&&j->manifest.modelVerified;if(!manifest.modelVerified){modelFailed=true;ErrorLog(std::string("GlobalSatelliteMap: city models disabled; terrain retained: ")+j->error);}else Diagnostic("native city model sources verified in bounded worker");delete j;return;}
    if(j->waterValidation){
        manifest.waterVerified=j->ok&&j->manifest.waterVerified;
        if(!manifest.waterVerified){waterFailed=true;ErrorLog(std::string("GlobalSatelliteMap: water disabled; terrain retained: ")+j->error);}
        else Diagnostic("native water source profile verified in bounded worker");
        delete j;return;
    }
    if(!j->ok){std::string error=j->error;delete j;Fail(map,error);return;}
    if(j->validation){manifest=j->manifest;verifiedGameRoot=j->game;delete j;nativeLight.Init();if(!gpuRetirementDevice.p){gpuRetirementDevice.p=nativeLight.device.p;gpuRetirementDevice.p->AddRef();}HRESULT hr=relighter.Init(nativeLight.device.p,GsmRelightShader,sizeof(GsmRelightShader),256,OtherGpuBytes(0));if(FAILED(hr))throw std::runtime_error("private terrain relighter initialization failed");ready=true;Diagnostic("complete GST2/source profile verified; native terrain lighting viewer ready");}
    else{if(completed)delete completed;completed=j;}
}
bool Geometry(MapScreen *map, GSM::Rect out[16])
{
    if (!map->mapImage || !map->mapScrollView) return false;
    int w=map->mapImage->getWidth(),h=map->mapImage->getHeight();
    if(w<=0||h<=0||w>1000000||h>1000000)return false;
    MyGUI::IntPoint center=map->worldToMapCoords(Ogre::Vector3(0,0,0));
    int xs[5],zs[5];
    for(int i=0;i<5;++i){
        double x=manifest.minX+(manifest.maxX-manifest.minX)*i/4;
        double z=manifest.minZ+(manifest.maxZ-manifest.minZ)*i/4;
        if(!GSM::Finite(x)||!GSM::Finite(z))return false;
        MyGUI::IntPoint px=map->worldToMapCoords(Ogre::Vector3(float(x),0,0));
        MyGUI::IntPoint pz=map->worldToMapCoords(Ogre::Vector3(0,0,float(z)));
        if(px.top!=center.top||pz.left!=center.left||px.left<-1000000||px.left>1000000||pz.top<-1000000||pz.top>1000000)return false;
        xs[i]=px.left;zs[i]=pz.top;
        if(i&&(xs[i]<xs[i-1]||zs[i]<zs[i-1]))return false;
    }
    if(xs[4]<=xs[0]||zs[4]<=zs[0])return false;
    for(int z=0;z<4;++z)for(int x=0;x<4;++x)out[z*4+x]=GSM::Rect(xs[x],zs[z],xs[x+1]-xs[x],zs[z+1]-zs[z]);
    return true;
}
void EnsureLayer(MapScreen *map)
{
    MyGUI::Widget *image = map->mapImage;
    if (layer.get() && layer.get()->getParent() != image)
    {
        MyGUI::Gui::getInstance().destroyWidget(layer.get());
        layer.reset();
    }
    if (!layer.get())
    {
        presentationCurrent=false;
        MyGUI::Widget *w = image->createWidget<MyGUI::Widget>(
            "PanelEmpty", MyGUI::IntCoord(0, 0, image->getWidth(), image->getHeight()),
            MyGUI::Align::Default, "");
        layer.set(w);
        w->setDepth(1000000);
        w->setNeedMouseFocus(false);
        w->setNeedKeyFocus(false);
        for (unsigned i = 0; i < AtlasView::WindowSlots; ++i)
        {
            MyGUI::ImageBox *t = w->createWidget<MyGUI::ImageBox>("ImageBox", MyGUI::IntCoord(),
                                                                  MyGUI::Align::Default, "");
            tiles[i].set(t);
            t->setNeedMouseFocus(false);
            t->setNeedKeyFocus(false);
            t->setVisible(false);
        }
    }
    else if (layer.get()->getSize() != image->getSize())
        layer.get()->setSize(image->getSize());
}

bool SameLighting(const AtlasView::Uniforms&a,const AtlasView::Uniforms&b){if(std::memcmp(a.sunDirection,b.sunDirection,16)||std::memcmp(a.sunColour,b.sunColour,16)||std::memcmp(a.envColour,b.envColour,16)||std::memcmp(a.ambientParams,b.ambientParams,16)||std::memcmp(a.worldSize,b.worldSize,16)||std::memcmp(a.exposure,b.exposure,16))return false;for(int i=0;i<3;++i)if(std::fabs((a.worldOffset[i]-a.referenceSide[i])-(b.worldOffset[i]-b.referenceSide[i]))>.001f)return false;return true;}
void Lighting(DWORD now){if(!lightClock.Due(now,1000))return;AtlasView::Uniforms fresh=nativeLight.Snapshot(sunlight,ambient);if(!haveLighting||!SameLighting(fresh,lighting)){lighting=fresh;haveLighting=true;if(++lightVersion==0){lightVersion=1;for(unsigned i=0;i<AtlasView::WindowSlots;++i)cache[i].shaded=0;}}}
bool DiagnosticScene(const void*&scene,const void*&camera){
    scene=camera=NULL;
    __try{if(!au||!au->render)return false;scene=au->render->scene;camera=au->render->camera;return scene&&camera;}
    __except(NativeLocalLights::NativeFault(GetExceptionCode())){scene=camera=NULL;return false;}
}
Town*DiagnosticTown(const hand&value){
    __try{TownBase*t=value.getTown();return t?t->isTown():NULL;}
    __except(NativeLocalLights::NativeFault(GetExceptionCode())){return NULL;}
}
bool DiagnosticTownPosition(Town*town,float*out){
    __try{if(!town)return false;std::memcpy(out,reinterpret_cast<const BYTE*>(town)+0x48,12);return _finite(out[0])&&_finite(out[2]);}
    __except(NativeLocalLights::NativeFault(GetExceptionCode())){return false;}
}
void TownValueDiagnostic(){
    if(!debug||!ready||townValuesLogged)return;
    townValuesLogged=true;
    GsmTownValues::Result values;
    HRESULT hr=GsmTownValues::Read(true,ready,shou,GetModuleHandleA(NULL),observedGeneration,&GSM::generation,values);
    if(InterlockedCompareExchange(&GSM::generation,0,0)!=observedGeneration)return;
    std::ostringstream status;status<<"town values HRESULT="<<std::hex<<unsigned(hr)<<std::dec<<" total="<<values.total<<" copied="<<values.count<<" null="<<values.nullEntries<<" other="<<values.otherTypes;Diagnostic(status.str());
    if(hr!=S_OK)return;
    for(unsigned i=0;i<values.count;++i){const GsmTownValues::TownValue&v=values.towns[i];std::ostringstream row;row.precision(9);row<<"town value id="<<v.id<<" xyz="<<v.storedXYZ[0]<<','<<v.storedXYZ[1]<<','<<v.storedXYZ[2]<<" proxy="<<v.hasProxy<<" state="<<v.proxyState;Diagnostic(row.str());}
}
void ModelDiagnostic(GsmModelDiagnostic::Once&once,Town*town,const void*scene,const char*label){
    bool fresh=false;once.Capture(debug,town,static_cast<Ogre::SceneManager*>(const_cast<void*>(scene)),observedGeneration,&GSM::generation,ready,&fresh);
    if(!fresh)return;const GsmModelDiagnostic::Report&r=once.Values();const NativeLoadedCensus::Result&v=r.loaded;
    std::ostringstream s;s<<"model diagnostic "<<label<<" model="<<std::hex<<unsigned(r.modelHr)<<" census="<<unsigned(r.censusHr)<<std::dec<<" packets="<<r.packets<<" valid="<<r.modelValid<<" shaderVerified="<<r.modelShadersVerified<<" reason="<<r.modelError<<" loadedEntities="<<v.selected<<" materials="<<v.materialCount;Diagnostic(s.str());
    if(r.modelValid){std::ostringstream geometry;geometry.precision(9);geometry<<"model geometry "<<label<<" world=";for(unsigned i=0;i<16;++i)geometry<<(i?",":"")<<r.rawSceneWorld[i];geometry<<" origin=";for(unsigned i=0;i<3;++i)geometry<<(i?",":"")<<r.sourceSceneOrigin[i];geometry<<" bounds=";for(unsigned i=0;i<6;++i)geometry<<(i?",":"")<<r.localBounds[i];Diagnostic(geometry.str());}
    for(unsigned i=0;i<v.materialCount;++i){const NativeLoadedCensus::Material&m=v.materials[i];std::ostringstream line;line<<"loaded material "<<i<<" name="<<m.name<<" vs="<<m.vertex<<" ps="<<m.pixel<<" textures="<<m.textures<<" vertices="<<m.vertices<<" indices="<<m.indices<<" elements="<<m.elements;Diagnostic(line.str());}
}
void LocalDiagnostic(DWORD now){
    // The existing terrain source-validation job checks the same seven pinned
    // local-light shader files. The build verifies this set inclusion.
    const void*scene=NULL,*camera=NULL;if(debug||localOption)DiagnosticScene(scene,camera);
    if(localProofScene&&localProofScene!=scene){localMapping=AtlasLocalLights::ProvenMapping();localProofScene=NULL;originDiagnosticLogged=false;}
    HRESULT hr=localDiagnostic.Tick(debug||(localOption&&!localFailed),ready,observedGeneration,scene,&GSM::generation,now);
    const NativeLocalLights::NativeSnapshot&values=localDiagnostic.Values();
    if(FAILED(hr)){ErrorLog(std::string("GlobalSatelliteMap: local-light read refused; terrain retained: ")+values.LastError());localFailed=true;RetireGpuCharge(localLayerReady?AtlasLocalLights::PrivateBytes:0);localLayer.Clear();localLayerReady=false;localMapping=AtlasLocalLights::ProvenMapping();return;}
    if(localOption&&hr==S_OK&&localRevision.Observe(values,observedGeneration)){if(++localVersion==0){localVersion=1;for(unsigned i=0;i<AtlasView::WindowSlots;++i)cache[i].localStamp=cache[i].localChecked=0;}}
    if(!debug){localDiagnosticLogged=false;return;}
    if(!values.Valid(observedGeneration)||hr!=S_OK||(localDiagnosticLogged&&DWORD(now-localDiagnosticAt)<5000))return;
    localDiagnosticLogged=true;localDiagnosticAt=now;
    const NativeLocalLights::Information&info=values.Info();std::ostringstream s;
    s<<"local snapshot registered="<<info.registered<<" active="<<values.Count()<<" hidden="<<info.hidden<<" daySuppressed="<<info.daySuppressed<<" gain="<<info.rendererGain<<" time="<<info.clock.time<<" origin=";
    for(unsigned i=0;i<3;++i)s<<(i?",":"")<<info.SourceSceneOrigin[i];
    s<<" sharedCamera=";for(unsigned i=0;i<3;++i)s<<(i?",":"")<<lighting.referenceSide[i];
    s<<" sharedOffset=";for(unsigned i=0;i<3;++i)s<<(i?",":"")<<lighting.worldOffset[i];
    for(unsigned i=0;i<values.Count()&&i<4;++i){const NativeLocalLights::LightValue&v=*values.Light(i);s<<" rawLight["<<i<<"]="<<v.worldPosition[0]<<','<<v.worldPosition[1]<<','<<v.worldPosition[2]<<" radius="<<v.range<<" power="<<v.effectivePower;}
    Diagnostic(s.str());
}
void OriginDiagnostic(MapScreen*map,DWORD now){
    if(!debug&&(!localOption||localFailed)&&(!modelOption||modelFailed)){originDiagnosticLogged=false;return;}
    if(originDiagnosticLogged&&DWORD(now-originDiagnosticAt)<5000)return;
    originDiagnosticLogged=true;originDiagnosticAt=now;
    const void*scene=NULL,*camera=NULL;if(!DiagnosticScene(scene,camera))return;
    if(!map||map->mapMarkersTowns.size()>4096)return;
    Town*town=NULL;
    for(ogre_unordered_map<hand,MapScreen::MapMarkerTown*>::type::const_iterator i=map->mapMarkersTowns.begin();i!=map->mapMarkersTowns.end();++i){
        Town*candidate=DiagnosticTown(i->first);
        if(candidate&&NativeOriginProof::HasExistingProxy(candidate)){town=candidate;break;}
    }
    if(!town){
        GsmTownDiagnostic::Stats census;const void*candidate=NULL;
        HRESULT censusHr=GsmTownDiagnostic::Read(true,shou,GetModuleHandleA(NULL),ready,census,candidate);
        std::ostringstream s;s<<"town proxy census HRESULT="<<std::hex<<unsigned(censusHr)<<std::dec<<" total="<<census.total<<" primary="<<census.primary<<" other="<<census.otherType<<" missing="<<census.missingProxy<<" mismatch="<<census.ownerMismatch<<" states=";
        for(unsigned i=0;i<4;++i)s<<(i?",":"")<<census.state[i];s<<" unexpected="<<census.otherState<<" nullEntity="<<census.nullEntity<<" candidates="<<census.candidates;Diagnostic(s.str());
        if(censusHr!=S_OK||!candidate)return;
        town=static_cast<Town*>(const_cast<void*>(candidate));
    }
    NativeOriginProof::Values value;HRESULT hr=originReader.Read(true,observedGeneration,scene,camera,town,ready,&GSM::generation,value);
    if(hr!=S_OK){if(FAILED(hr)){if(localOption&&!localFailed){ErrorLog(std::string("GlobalSatelliteMap: local light coordinate proof refused: ")+value.error);localFailed=true;}else Diagnostic(std::string("origin paired read unavailable: ")+value.error);if(modelOption&&!modelFailed)ModelFailure(value.error);}return;}
    if(localOption&&!localFailed){
        if(!GsmLocalMapping::Adopt(value,observedGeneration,localMapping)){ErrorLog("GlobalSatelliteMap: inconsistent native town/origin pair; local lights retained OFF");localFailed=true;if(modelOption&&!modelFailed)ModelFailure("coordinate proof refused");return;}
        localProofScene=scene;
    }
    if(modelOption&&!modelFailed){AtlasModelDecode::ProvenMapping next;if(!GsmLocalMapping::Adopt(value,observedGeneration,next)){ModelFailure("coordinate proof refused");return;}if(modelProofScene&&modelProofScene!=scene)ResetModels();if(!modelMapping.verified||modelMapping.generation!=next.generation||modelProofScene!=scene||std::memcmp(modelMapping.sourceSceneOrigin,next.sourceSceneOrigin,12)||std::memcmp(modelMapping.sceneToCacheTranslation,next.sceneToCacheTranslation,12))models.Invalidate();modelMapping=next;modelProofScene=scene;}
    const float*rows[]={value.townStored,value.proxyNodeLocal,value.proxyNodeDerived,value.sceneOrigin,value.cameraDerived,value.sharedCamera,value.sharedWorldOffset};
    const char*names[]={"town","nodeLocal","nodeDerived","origin","camera","sharedCamera","sharedOffset"};std::ostringstream s;s<<"origin proof";
    for(unsigned r=0;r<7;++r){s<<' '<<names[r]<<'=';for(unsigned c=0;c<3;++c)s<<(c?",":"")<<rows[r][c];}
    s<<" townYWouldRefresh="<<value.townYWouldRefresh;Diagnostic(s.str());
    if(debug){
        ModelDiagnostic(distantModelDiagnostic,town,scene,"distant");
        Town*nearest=NULL;double best=10000.0*10000.0;
        for(ogre_unordered_map<hand,MapScreen::MapMarkerTown*>::type::const_iterator i=map->mapMarkersTowns.begin();i!=map->mapMarkersTowns.end();++i){Town*t=DiagnosticTown(i->first);float p[3];if(!DiagnosticTownPosition(t,p))continue;double dx=double(p[0])-(value.cameraDerived[0]+double(value.sceneOrigin[0])),dz=double(p[2])-(value.cameraDerived[2]+double(value.sceneOrigin[2]));double d=dx*dx+dz*dz;if(d<best){best=d;nearest=t;}}
        if(nearest)ModelDiagnostic(loadedModelDiagnostic,nearest,scene,"near camera");
    }
}
void WaterFailure(const char*why){
    if(!waterFailed)ErrorLog(std::string("GlobalSatelliteMap: water disabled; terrain retained: ")+why);
    waterFailed=true;RetireGpuCharge(waterPipelineReady?AtlasWater::PipelinePrivateBytes:0);waterSnapshot.Clear();waterPipeline.Clear();waterPipelineReady=false;
}
void WaterState(DWORD now){
    if(!waterOption||waterFailed||!manifest.waterVerified||!waterClock.Due(now,200))return;
    const unsigned generation=unsigned(observedGeneration);
    bool previous=waterSnapshot.Valid(generation);AtlasWater::Uniforms old={};
    ID3D11ShaderResourceView*before[5]={0};ID3D11SamplerState*beforeSampler[5]={0};
    if(previous){old=waterSnapshot.Values();for(unsigned i=0;i<5;++i){before[i]=waterSnapshot.Inputs().View(i);beforeSampler[i]=waterSnapshot.Inputs().Sampler(i);}}
    HRESULT hr=waterSnapshot.Read(true,sunlight,ambient,generation,true);
    if(FAILED(hr)){WaterFailure(waterSnapshot.LastError());return;}
    bool changed=!previous||std::memcmp(&old,&waterSnapshot.Values(),sizeof(old))!=0;
    for(unsigned i=0;i<5;++i)if(before[i]!=waterSnapshot.Inputs().View(i)||beforeSampler[i]!=waterSnapshot.Inputs().Sampler(i))changed=true;
    if(changed&&++waterVersion==0){waterVersion=1;for(unsigned i=0;i<AtlasView::WindowSlots;++i)cache[i].waterStamp=0;}
    if(!previous&&debug){const NativeWater::Resources&r=waterSnapshot.Inputs();std::ostringstream s;s<<"water snapshot height="<<waterSnapshot.Values().waterPlane[0]<<" time="<<waterSnapshot.Values().waterTime[0]<<" mips=";for(unsigned i=0;i<5;++i)s<<(i?",":"")<<r.Mips(i);s<<" depthBias="<<waterSnapshot.DepthBiasConstant()<<','<<waterSnapshot.DepthBiasSlope();Diagnostic(s.str());}
}
bool WantsWater(const Entry&e){return waterOption&&!waterFailed&&waterSnapshot.Valid(unsigned(observedGeneration))&&e.hasHeight&&e.minHeight<=waterSnapshot.Values().waterPlane[0];}
void WindowBounds(const AtlasView::WindowKey&key,double&x,double&z,double&sx,double&sz){
    double n=AtlasView::Side(key.level)/AtlasView::WindowSide;
    sx=(manifest.maxX-manifest.minX)/4/n;sz=(manifest.maxZ-manifest.minZ)/4/n;
    x=manifest.minX+(manifest.maxX-manifest.minX)*(key.tile%4)/4+sx*key.x;
    z=manifest.minZ+(manifest.maxZ-manifest.minZ)*(key.tile/4)/4+sz*key.z;
}
AtlasLocalLights::Region LocalRegion(const AtlasView::Uniforms&u){AtlasLocalLights::Region r;r.minX=u.tileBounds[0];r.minZ=u.tileBounds[1];r.spanX=u.tileBounds[2];r.spanZ=u.tileBounds[3];r.sourceSide=256;return r;}
bool WantsLocal(Entry&e){
    const NativeLocalLights::NativeSnapshot&s=localDiagnostic.Values();
    if(!localOption||localFailed||!localMapping.Matches(s,observedGeneration)||!s.Count())return false;
    if(e.localChecked==localVersion)return e.localAffects;
    double x,z,sx,sz;WindowBounds(e.key,x,z,sx,sz);AtlasView::Uniforms u={};u.tileBounds[0]=float(x);u.tileBounds[1]=float(z);u.tileBounds[2]=float(sx);u.tileBounds[3]=float(sz);
    AtlasLocalLights::Region r=LocalRegion(u);e.localAffects=false;
    for(unsigned i=0;i<s.Count();++i){AtlasLocalLights::GpuLight light;if(AtlasLocalLights::Pack(*s.Light(i),localMapping,light)&&AtlasLocalLights::Intersects(light,r,256,256)){e.localAffects=true;break;}}
    e.localChecked=localVersion;return e.localAffects;
}
void ModelFailure(const char*why){if(!modelFailed)ErrorLog(std::string("GlobalSatelliteMap: city models disabled; terrain retained: ")+why);ResetModels();modelFailed=true;}
void ModelSourceError(unsigned id,const char*why){std::ostringstream s;s<<"GlobalSatelliteMap: existing town model "<<id<<" unavailable: "<<(why&&*why?why:"invalid model bounds");ErrorLog(s.str());}
void ModelState(DWORD now){
    if(!modelOption||modelFailed||!manifest.modelVerified||!modelMapping.verified)return;
    const void*scene=NULL,*camera=NULL;if(!DiagnosticScene(scene,camera)||scene!=modelProofScene){ModelFailure("scene identity changed");return;}
    try{
        if(!modelCatalogReady||DWORD(now-modelCatalogAt)>=1000){
            GsmTownValues::Result values;HRESULT hr=GsmTownValues::Read(true,ready,shou,GetModuleHandleA(NULL),observedGeneration,&GSM::generation,values);
            if(hr!=S_OK||!models.Catalog(values)){ModelFailure("coherent town catalog or fixed model capacity refused");return;}
            const unsigned before=cachedTown.Revision();
            HRESULT selected=cachedTown.Select(values,lighting.referenceSide,modelMapping.sourceSceneOrigin,scene,observedGeneration,&GSM::generation,shou,GetModuleHandleA(NULL));
            if(FAILED(selected)&&selected!=E_ABORT){
                const char*why=cachedTown.LastError();if(cachedSelectionError!=why){cachedSelectionError=why;ErrorLog(std::string("GlobalSatelliteMap: cached town selection unavailable: ")+why);}
            }else cachedSelectionError.clear();
            if(cachedTown.Revision()!=before)models.Invalidate();
            modelCatalogReady=true;modelCatalogAt=now;
        }
        if(!models.Refresh(shou,GetModuleHandleA(NULL),static_cast<Ogre::SceneManager*>(const_cast<void*>(scene)),observedGeneration,&GSM::generation,&ModelSourceError))ModelFailure("world changed or global retained model budget exceeded");
    }catch(const std::exception&e){ModelFailure(e.what());}catch(...){ModelFailure("model admission exception");}
}
bool ModelsAvailable(){return modelOption&&!modelFailed&&manifest.modelVerified&&modelMapping.verified&&modelMapping.generation==unsigned(observedGeneration);}
void CachedTownCompletion(const GsmAtlas::Job&j){
    if(!enabled||!ready||!ModelsAvailable())return;
    const void*scene=NULL,*camera=NULL;
    if(!DiagnosticScene(scene,camera)||scene!=modelProofScene)return;
    const unsigned before=cachedTown.Revision();
    const AtlasView::U64 other=OtherGpuBytes(cachedTown.OwnedGpuBytes());
    HRESULT hr=cachedTown.Adopt(j.cachedTownRequest,j.cachedTownResult,observedGeneration,modelMapping.sourceSceneOrigin,scene,nativeLight.device.p,manifest.modelVerified,other);
    if(cachedTown.Revision()!=before)models.Invalidate();
    if(FAILED(hr))ErrorLog(std::string("GlobalSatelliteMap: cached town unavailable; terrain retained: ")+cachedTown.LastError());
    else if(hr==S_OK){std::ostringstream row;row<<"cached town adopted id="<<cachedTown.TownId()<<" ownedGpuBytes="<<cachedTown.OwnedGpuBytes()<<" cpuPayloadBytes="<<cachedTown.CpuPayloadBytes();Diagnostic(row.str());}
}
HRESULT PrepareModelCell(const Entry&e,const AtlasView::Uniforms&u,bool&applied){
    applied=false;if(!ModelsAvailable())return S_FALSE;
    AtlasModelDecode::Footprint footprint;footprint.minX=u.tileBounds[0];footprint.minZ=u.tileBounds[1];footprint.width=u.tileBounds[2];footprint.height=u.tileBounds[3];
    bool intersects=false;for(unsigned i=0;i<GsmModels::Slots;++i){const GsmModels::Item&m=models.At(i);if(m.source.Valid(unsigned(observedGeneration))&&!std::memcmp(modelMapping.sourceSceneOrigin,m.source.Geometry().SourceSceneOrigin(),12)&&AtlasModelRegion::Intersects(m.bounds,footprint)){intersects=true;break;}}
    const bool cachedValid=cachedTown.Valid(unsigned(observedGeneration))&&!std::memcmp(modelMapping.sourceSceneOrigin,cachedTown.Geometry().SourceSceneOrigin(),12);
    if(cachedValid&&AtlasModelRegion::Intersects(cachedTown.Bounds(),footprint))intersects=true;
    if(!intersects)return S_FALSE;
    const AtlasView::U64 other=OtherGpuBytes(modelGpuReady?ModelGpuBytes:0);
    if(!modelGpuReady){
        HRESULT hr=modelRaster.Init(nativeLight.device.p,AtlasModelVertexShader,sizeof(AtlasModelVertexShader),AtlasModelPixelShader,sizeof(AtlasModelPixelShader),other+AtlasModelDecode::PrivateBytes+AtlasModelSurface::PrivateBytes);
        if(SUCCEEDED(hr))hr=modelDecode.Init(true,nativeLight.device.p,AtlasModelDecodeShader,sizeof(AtlasModelDecodeShader),other+AtlasModels::PrivateBytes+AtlasModelSurface::PrivateBytes);
        if(SUCCEEDED(hr))hr=modelSurface.Init(true,nativeLight.device.p,other+AtlasModels::PrivateBytes+AtlasModelDecode::PrivateBytes);
        if(FAILED(hr))return hr;
        for(unsigned p=0;p<3;++p){modelCell.texture[p].p=modelSurface.Texture(p);modelCell.texture[p].p->AddRef();modelCell.view[p].p=modelSurface.Output(p);modelCell.view[p].p->AddRef();}modelCell.side=256;modelGpuReady=true;
    }
    ID3D11ShaderResourceView*pristine[3]={e.gpu.view[0].p,e.gpu.view[1].p,e.gpu.view[2].p};HRESULT hr=modelSurface.Copy(true,pristine,other+AtlasModels::PrivateBytes+AtlasModelDecode::PrivateBytes);if(FAILED(hr))return hr;
    // The optional last packet is borrowed from our one owned cached mesh;
    // it remains alive throughout this synchronous raster/decode loop.
    for(unsigned i=0;i<=GsmModels::Slots;++i){
        const AtlasModels::Packet*p=NULL;const AtlasModelRegion::Bounds*bounds=NULL;
        if(i<GsmModels::Slots){const GsmModels::Item&m=models.At(i);if(!m.source.Valid(unsigned(observedGeneration)))continue;p=&m.source.Geometry();bounds=&m.bounds;}
        else{if(!cachedValid)continue;p=&cachedTown.Geometry();bounds=&cachedTown.Bounds();}
        if(std::memcmp(modelMapping.sourceSceneOrigin,p->SourceSceneOrigin(),12)||!AtlasModelRegion::Intersects(*bounds,footprint))continue;
        const AtlasModels::Packet&packet=*p;AtlasModels::View view;if(!AtlasModelRegion::View(*bounds,p->SourceSceneOrigin(),footprint,unsigned(observedGeneration),view))return E_INVALIDARG;
        hr=modelRaster.Render(true,packet,view,other+AtlasModelDecode::PrivateBytes+AtlasModelSurface::PrivateBytes);if(FAILED(hr))return hr;
        ID3D11ShaderResourceView*inputs[6]={modelRaster.Output(0),modelRaster.Output(1),modelRaster.Output(2),modelSurface.Output(0),modelSurface.Output(1),modelSurface.Output(2)};
        ID3D11Texture2D*outputs[3]={modelSurface.Texture(0),modelSurface.Texture(1),modelSurface.Texture(2)};
        hr=modelDecode.Render(true,packet,view,footprint,modelMapping,inputs,outputs,other+AtlasModels::PrivateBytes+AtlasModelSurface::PrivateBytes);if(FAILED(hr))return hr;
        ++modelRenders;applied=true;
    }return S_OK;
}
HRESULT ApplyLocal(void*,const AtlasView::Cell&cell,const AtlasView::Uniforms&u,ID3D11ShaderResourceView*hdrView,ID3D11Texture2D*hdr){
    ID3D11ShaderResourceView*source[4]={hdrView,cell.view[0].p,cell.view[1].p,cell.view[2].p};
    HRESULT hr=localLayer.Render(true,localDiagnostic.Values(),observedGeneration,localMapping,LocalRegion(u),source,hdr,OtherGpuBytes(localLayerReady?AtlasLocalLights::PrivateBytes:0));
    if(SUCCEEDED(hr)){++localRenders;localBatches+=localLayer.LastBatches();}return hr;
}
HRESULT RenderLayers(Entry&e,const AtlasView::Cell&cell,const AtlasView::Uniforms&u,bool useWater,bool useLocal){
    if(useLocal&&!localLayerReady){
        HRESULT hr=localLayer.Init(nativeLight.device.p,GsmLocalShader,sizeof(GsmLocalShader),OtherGpuBytes(localLayerReady?AtlasLocalLights::PrivateBytes:0));
        if(FAILED(hr))return hr;localLayerReady=true;
    }
    AtlasView::U64 caller=OtherGpuBytes(waterPipelineReady?AtlasWater::PipelinePrivateBytes:0);
    if(!waterPipelineReady){
        HRESULT hr=waterPipeline.Init(nativeLight.device.p,AtlasTerrainHdrShader,sizeof(AtlasTerrainHdrShader),AtlasWaterMaterialShader,sizeof(AtlasWaterMaterialShader),AtlasWaterToneShader,sizeof(AtlasWaterToneShader),caller);
        if(FAILED(hr))return hr;waterPipelineReady=true;
    }
    AtlasWater::HdrStage stage={NULL,&ApplyLocal};
    if(!useWater)return waterPipeline.RenderTerrainWithStage(cell,u,nativeLight.views,nativeLight.samplers,e.output.p,caller,useLocal?&stage:NULL);
    AtlasWater::Uniforms w=waterSnapshot.Values();std::memcpy(w.tileBounds,u.tileBounds,16);w.sourceWindow[0]=256;w.sourceWindow[1]=w.sourceWindow[2]=w.sourceWindow[3]=0;
    ID3D11ShaderResourceView*views[5];ID3D11SamplerState*samplers[5];for(unsigned i=0;i<5;++i){views[i]=waterSnapshot.Inputs().View(i);samplers[i]=waterSnapshot.Inputs().Sampler(i);}
    return waterPipeline.RenderWithStage(cell,u,nativeLight.views,nativeLight.samplers,w,views,samplers,e.output.p,caller,useLocal?&stage:NULL);
}
int FindWindow(const AtlasView::WindowKey&key){for(unsigned i=0;i<AtlasView::WindowSlots;++i)if(cache[i].key==key)return int(i);return -1;}
bool Superseded(const AtlasView::WindowKey&key,unsigned desired){
    if(key.level>=desired)return false;
    unsigned shift=desired-key.level;
    int ancestor=FindWindow(AtlasView::WindowKey(key.tile,desired,key.x>>shift,key.z>>shift));
    return ancestor>=0&&cache[ancestor].shaded!=0;
}
void Touch(Entry&e){if(++cacheAge==0){cacheAge=1;for(unsigned i=0;i<AtlasView::WindowSlots;++i)cache[i].age=0;}e.age=cacheAge;}
void Upload(int slot,const AtlasView::WindowKey&key,GsmAtlas::Job&j){
    if(slot<0||slot>=int(AtlasView::WindowSlots)||!key.Valid()||j.read.level!=5||!AdmitGpu(GsmFineGpu::WindowBytes))throw std::runtime_error("terrain window GPU resource budget refused");
    if(cache[slot].key.Valid())DropEntry(slot);if(cache[slot].key.Valid()||!AdmitGpu(GsmFineGpu::WindowBytes))return;
    Entry&e=cache[slot];e.key=key;e.hasHeight=j.windowHasFiniteHeight;e.minHeight=j.windowMinHeight;e.waterApplied=e.localApplied=e.localAffects=e.modelApplied=false;e.waterStamp=e.localStamp=e.localChecked=e.modelStamp=0;
    try{
        HRESULT hr=e.gpu.Upload(nativeLight.device.p,j.read);
        if(FAILED(hr))throw std::runtime_error("terrain window GPU upload failed");
        j.read.Clear();
        if(!groupCreated){Ogre::ResourceGroupManager::getSingleton().createResourceGroup(Group);groupCreated=true;}
        const unsigned side=AtlasView::WindowSide;std::vector<unsigned char>blank(side*side*4,0);
        Ogre::Image image;image.loadDynamicImage(&blank[0],side,side,1,Ogre::PF_A8B8G8R8,false,1,0);
        std::ostringstream name;name<<"GlobalSatelliteMap_GST2_"<<observedGeneration<<'_'<<++serial;e.name=name.str();
        e.texture=Ogre::TextureManager::getSingleton().loadImage(e.name,Group,image,Ogre::TEX_TYPE_2D,0);
        GsmNative::CompleteManagedImage(e.texture.get());
        if(e.texture.isNull()||e.texture->getFormat()!=Ogre::PF_A8B8G8R8||e.texture->getNumMipmaps()!=0)throw std::runtime_error("owned terrain window output format differs");
        ID3D11ShaderResourceView*v=GsmNative::Srv(e.texture.get());AtlasView::Com<ID3D11Resource>resource;v->GetResource(&resource.p);
        if(!AtlasView::SameDevice(v,nativeLight.device.p)||FAILED(resource.p->QueryInterface(__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&e.output.p))))throw std::runtime_error("owned terrain window output D3D identity differs");
        Touch(e);
    }catch(...){DropEntry(slot);throw;}
}

MyGUI::IntCoord WindowGeometry(MapScreen*map,const AtlasView::WindowKey&key){
    double x,z,sx,sz;WindowBounds(key,x,z,sx,sz);
    MyGUI::IntPoint a=map->worldToMapCoords(Ogre::Vector3(float(x),0,float(z)));
    MyGUI::IntPoint b=map->worldToMapCoords(Ogre::Vector3(float(x+sx),0,float(z+sz)));
    return MyGUI::IntCoord(a.left,a.top,b.left-a.left,b.top-a.top);
}
// Share ready-texture presentation with Draw. A formerly offscreen resident
// may still be hidden or unbound; resizing its rectangle alone leaves a hole.
void PresentWindows(MapScreen*map,const AtlasView::WindowRect cells[16],const AtlasView::WindowRect&clip,const AtlasView::WindowPlan&plan){
    if(layer.get()->getSize()!=map->mapImage->getSize())layer.get()->setSize(map->mapImage->getSize());
    for(unsigned i=0;i<AtlasView::WindowSlots;++i){
        MyGUI::ImageBox*w=static_cast<MyGUI::ImageBox*>(tiles[i].get());if(!w)continue;Entry&e=cache[i];
        bool show=e.key.Valid()&&e.shaded!=0&&!Superseded(e.key,plan.level)&&AtlasView::WindowVisible(AtlasView::SubWindow(cells[e.key.tile],e.key),clip);
        if(show){MyGUI::IntCoord rect=WindowGeometry(map,e.key);if(w->getCoord()!=rect)w->setCoord(rect);if(w->_getTextureName()!=e.name){w->setDepth(int(e.key.level));w->setImageTexture(e.name);}}
        if(w->getVisible()!=show)w->setVisible(show);
    }
    FinePresent(map);projectedWidth=map->mapImage->getWidth();projectedHeight=map->mapImage->getHeight();
    projectedClip=MyGUI::IntCoord(int(clip.x),int(clip.y),int(clip.w),int(clip.h));presentationCurrent=true;
}
// Native resize/pan has completed before Tick. Refresh only presentation on
// geometry/clip changes, independently of the streaming cadence and save/load
// busy gate. The unchanged frame path reads scalar geometry, without scanning
// windows; this path never reads source files, uploads or relights textures.
void RepositionZoom(MapScreen*map){
    if(!map||!map->mapImage||!map->mapScrollView||!layer.get()||layer.get()->getParent()!=map->mapImage){presentationCurrent=false;return;}
    int width=map->mapImage->getWidth(),height=map->mapImage->getHeight();
    if(width<=0||height<=0||width>1000000||height>1000000)return;
    MyGUI::IntCoord view=map->mapScrollView->getAbsoluteCoord();MyGUI::IntPoint origin=map->mapImage->getAbsolutePosition();
    MyGUI::IntCoord clip(view.left-origin.left,view.top-origin.top,view.width,view.height);
    if(presentationCurrent&&width==projectedWidth&&height==projectedHeight&&!(clip!=projectedClip))return;
    GSM::Rect coords[16];if(!Geometry(map,coords))return;
    AtlasView::WindowRect cells[16];for(unsigned i=0;i<16;++i)cells[i]=AtlasView::WindowRect(coords[i].x,coords[i].y,coords[i].w,coords[i].h);
    AtlasView::WindowRect windowClip(clip.left,clip.top,clip.width,clip.height);
    AtlasView::WindowPlan plan=AtlasView::PlanWindows(cells,windowClip,BaseDetailCapacity());
    PresentWindows(map,cells,windowClip,plan);
}
void RequestWindow(const AtlasView::WindowKey&key){
    std::auto_ptr<GsmAtlas::Job>owned(new GsmAtlas::Job());GsmAtlas::Job*j=owned.get();j->window=key;
    j->read.tileX=key.tile%4;j->read.tileZ=key.tile/4;j->read.level=5;
    std::memcpy(j->read.expectedHeader,manifest.header[key.tile],32);
    if(!manifest.windows.Hashes(key,j->windowHashes))throw std::runtime_error("terrain window checksum missing");
    std::ostringstream file;file<<Folder()<<"captures/tile_"<<key.tile%4<<'_'<<key.tile/4<<".gst";j->read.path=GsmAtlas::Wide(file.str());
    Launch(owned.release());
}
void Draw(MapScreen*map,DWORD now){
    GSM::Rect coords[16];if(!Geometry(map,coords))throw std::runtime_error("native map projection not ready/unsupported");EnsureLayer(map);
    MyGUI::IntCoord view=map->mapScrollView->getAbsoluteCoord();MyGUI::IntPoint origin=map->mapImage->getAbsolutePosition();
    AtlasView::WindowRect cells[16],clip(view.left-origin.left,view.top-origin.top,view.width,view.height);
    for(unsigned i=0;i<16;++i)cells[i]=AtlasView::WindowRect(coords[i].x,coords[i].y,coords[i].w,coords[i].h);
    AtlasView::WindowPlan plan=AtlasView::PlanWindows(cells,clip,BaseDetailCapacity());
    for(unsigned i=0;i<AtlasView::WindowSlots;++i)if(cache[i].key.Valid()&&plan.Contains(cache[i].key))Touch(cache[i]);
    if(completed){
        bool deferUpload=false;
        ++windowReads;windowBytes+=completed->read.readBytes;
        AtlasView::WindowKey key=completed->window;
        if(plan.Contains(key)&&FindWindow(key)<0){
            int slot=int(key.tile);
            if(key.level!=5){AtlasView::WindowKey keys[AtlasView::WindowSlots];unsigned age[AtlasView::WindowSlots];for(unsigned i=0;i<AtlasView::WindowSlots;++i){keys[i]=cache[i].key;age[i]=cache[i].age;}slot=AtlasView::WindowVictim(keys,age,plan);}
            if(slot>=0&&!AdmitGpu(GsmFineGpu::WindowBytes)&&cache[slot].key.Valid())DropEntry(slot);
            if(slot>=0&&AdmitGpu(GsmFineGpu::WindowBytes))Upload(slot,key,*completed);else deferUpload=true;
        }
        if(!deferUpload){delete completed;completed=NULL;}
    }
    TownValueDiagnostic();Lighting(now);WaterState(now);LocalDiagnostic(now);OriginDiagnostic(map,now);ModelState(now);
    // Four256 windows have the same pixel budget as one former512 tile.
    unsigned shaded=0,next=shadeCursor;
    for(unsigned pass=0;pass<3&&shaded<4;++pass)for(unsigned k=0;k<AtlasView::WindowSlots&&shaded<4;++k){unsigned i=(shadeCursor+k)%AtlasView::WindowSlots;Entry&e=cache[i];
        if(!e.key.Valid()||Superseded(e.key,plan.level))continue;
        bool onScreen=AtlasView::WindowVisible(AtlasView::SubWindow(cells[e.key.tile],e.key),clip);
        bool useWater=WantsWater(e),useLocal=WantsLocal(e);
        bool waterDirty=e.waterApplied!=useWater||(useWater&&e.waterStamp!=waterVersion),localDirty=e.localApplied!=useLocal||(useLocal&&e.localStamp!=localVersion);
        bool modelDirty=ModelsAvailable()?e.modelStamp!=models.Revision():e.modelApplied;
        if(!AtlasView::NeedsWindowShade(onScreen,plan.Contains(e.key),e.shaded,(waterDirty||localDirty||modelDirty)?0:lightVersion,pass))continue;
        AtlasView::Uniforms u=lighting;double x,z,sx,sz;WindowBounds(e.key,x,z,sx,sz);
        u.referenceSide[3]=float(AtlasView::WindowSide);u.tileBounds[0]=float(x);u.tileBounds[1]=float(z);u.tileBounds[2]=float(sx);u.tileBounds[3]=float(sz);
        unsigned refusal=0;HRESULT hr=E_FAIL;bool useModels=false;
        HRESULT modelHr=PrepareModelCell(e,u,useModels);if(FAILED(modelHr)){ModelFailure("native model raster/decode refused");useModels=false;}
        const AtlasView::Cell&cell=useModels?modelCell:e.gpu;
        if(useWater||useLocal){
            hr=RenderLayers(e,cell,u,useWater,useLocal);
            if(FAILED(hr)&&useLocal){ErrorLog("GlobalSatelliteMap: local light composition refused; terrain/water retained");localFailed=true;RetireGpuCharge(localLayerReady?AtlasLocalLights::PrivateBytes:0);localLayer.Clear();localLayerReady=false;useLocal=false;if(useWater)hr=RenderLayers(e,cell,u,true,false);}
            if(FAILED(hr)&&useWater){std::ostringstream error;error<<"water composition HRESULT="<<std::hex<<unsigned(hr);WaterFailure(error.str().c_str());useWater=false;}
        }
        if(!useWater&&!useLocal)hr=relighter.Render(cell,u,nativeLight.views,nativeLight.samplers,e.output.p,&refusal);
        if(FAILED(hr)){std::ostringstream error;error<<"terrain window relight failed HRESULT="<<std::hex<<unsigned(hr)<<" stage="<<refusal;throw std::runtime_error(error.str());}
        e.shaded=lightVersion;e.waterApplied=useWater;e.waterStamp=useWater?waterVersion:0;e.localApplied=useLocal;e.localStamp=useLocal?localVersion:0;e.modelApplied=useModels;e.modelStamp=ModelsAvailable()?models.Revision():0;next=(i+1)%AtlasView::WindowSlots;++shaded;
    }shadeCursor=next;
    if(!job&&!completed){
        bool requested=false;
        if(modelOption&&!modelFailed&&!modelAttempted){std::auto_ptr<GsmAtlas::Job>owned(new GsmAtlas::Job());owned->modelValidation=true;owned->game=Game();Launch(owned.release());modelAttempted=true;requested=true;}
        if(!requested&&waterOption&&!waterFailed&&!waterAttempted){std::auto_ptr<GsmAtlas::Job>owned(new GsmAtlas::Job());owned->waterValidation=true;owned->game=Game();Launch(owned.release());waterAttempted=true;requested=true;}
        for(unsigned t=0;t<16&&!requested;++t){AtlasView::WindowKey key(t,5,0,0);if(plan.visible[t]&&FindWindow(key)<0){RequestWindow(key);requested=true;}}
        if(!requested){
            // Center-first refinement, bounded56 keys, no world or filesystem scan.
            int best=-1;double nearest=1e100;
            for(unsigned priority=0;priority<2&&best<0;++priority){
                if(priority&&cachedTown.WantsRequest()){
                    std::auto_ptr<GsmAtlas::Job>owned(new GsmAtlas::Job());owned->cachedTown=true;owned->cachedTownRequest=cachedTown.RequestValue();owned->game=Game();Launch(owned.release());cachedTown.MarkLaunched();requested=true;break;
                }
                unsigned begin=priority?plan.visibleCount:0,end=priority?plan.count:plan.visibleCount;
                for(unsigned i=begin;i<end;++i)if(FindWindow(plan.keys[i])<0){AtlasView::WindowRect r=AtlasView::SubWindow(cells[plan.keys[i].tile],plan.keys[i]);double dx=r.x+r.w*.5-clip.x-clip.w*.5,dz=r.y+r.h*.5-clip.y-clip.h*.5,d=dx*dx+dz*dz;if(d<nearest){nearest=d;best=int(i);}}
            }
            if(best>=0)RequestWindow(plan.keys[best]);
        }
    }
    PresentWindows(map,cells,clip,plan);
    if(debug&&now-statsAt>=5000){
        statsAt=now;unsigned resident=0,detailReady=0;
        for(unsigned i=0;i<AtlasView::WindowSlots;++i)if(cache[i].key.Valid())++resident;
        for(unsigned i=0;i<plan.visibleCount;++i){int slot=FindWindow(plan.keys[i]);if(slot>=0&&cache[slot].shaded)++detailReady;}
        unsigned coarseValid=0,coarseShaded=0,coarseBound=0,coarseVisible=0;
        for(unsigned i=0;i<AtlasView::FallbackSlots;++i)if(cache[i].key==AtlasView::WindowKey(i,5,0,0)){
            coarseValid|=1u<<i;if(cache[i].shaded)coarseShaded|=1u<<i;
            MyGUI::ImageBox*w=static_cast<MyGUI::ImageBox*>(tiles[i].get());if(w){if(w->_getTextureName()==cache[i].name)coarseBound|=1u<<i;if(w->getVisible())coarseVisible|=1u<<i;}
        }
        std::ostringstream message;message<<"view sourceLevel="<<plan.level<<" densityLevel="<<plan.densityLevel<<" detailReady="<<detailReady<<'/'<<plan.visibleCount<<" apron="<<plan.count-plan.visibleCount<<" resident="<<resident<<" reads="<<windowReads<<" bytes="<<windowBytes<<" localRenders="<<localRenders<<" localBatches="<<localBatches<<" localReady="<<localLayerReady<<" localProof="<<localMapping.verified<<" modelReady="<<modelGpuReady<<" modelCount="<<models.Valid(unsigned(observedGeneration))<<" modelRenders="<<modelRenders<<" modelBufferBytes="<<models.BorrowedBuffers()<<" modelTextureBytes="<<models.BorrowedTextures()<<" ownedGpuBytes="<<AggregateGpuBytes()<<" gpuCapBytes="<<AtlasView::GpuCap<<" primary="<<coords[0].w<<'x'<<coords[0].h<<" viewport="<<view.width<<'x'<<view.height;
        message<<" cachedTown="<<(cachedTown.Valid(unsigned(observedGeneration))?cachedTown.TownId():0)<<" cachedTownGpu="<<cachedTown.OwnedGpuBytes();
        message<<" coarseValid=0x"<<std::hex<<coarseValid<<" coarseShaded=0x"<<coarseShaded<<" coarseBound=0x"<<coarseBound<<" coarseVisible=0x"<<coarseVisible<<std::dec;Diagnostic(message.str());
    }
}
#include "GsmFineViewer.h"
void Tick(MapScreen*map){FineObserve(map);LONG gen=InterlockedCompareExchange(&GSM::generation,0,0);if(gen!=observedGeneration){Clear();session.Reset();observedGeneration=gen;}Poll(map);if(!map||!map->getVisible()||!enabled){localDiagnostic.Reset();localDiagnosticLogged=originDiagnosticLogged=false;tickClock.Reset();lightClock.Reset();if(layer.get())layer.get()->setVisible(false);if(status.get())status.get()->setVisible(false);if(completed){delete completed;completed=NULL;}return;}DWORD now=GetTickCount();if(status.get()&&statusUntil&&static_cast<LONG>(now-statusUntil)>=0){MyGUI::Gui::getInstance().destroyWidget(status.get());status.reset();statusUntil=0;}if(ready)RepositionZoom(map);if(!tickClock.Due(now,200))return;SaveFileSystem*fs=SaveFileSystem::getSingleton();if(!fs||fs->busy()||!ou)return;if(session.Change("",reinterpret_cast<unsigned long long>(ou))){Clear();InterlockedIncrement(&GSM::generation);observedGeneration=InterlockedCompareExchange(&GSM::generation,0,0);}if(!attempted&&!job&&!jobQuarantined&&!FinePending())StartValidation();if(!ready)return;Draw(map,now);if(layer.get()&&!layer.get()->getVisible())layer.get()->setVisible(true);}
bool RelativeZoomEnabled(MapScreen*map){return zoomStepHooksReady&&enabled&&zoomMultiplier>1&&!zoomFailed&&!zoomStepDepth&&zoomLease.Owned()&&map&&zoomOwner.get()==map->mainWidget&&GsmZoom::StepValues(map->zoomValueBase,map->zoomValueCurrent);}
void ZoomInHook(MapScreen*map,MyGUI::Widget*sender){
    if(!RelativeZoomEnabled(map)){originalZoomIn(map,sender);return;}
    GsmZoom::DepthScope depth(zoomStepDepth);GsmZoom::StepScope step(map->zoomValueBase,GsmZoom::ButtonStep(map->zoomValueBase,map->zoomValueCurrent));originalZoomIn(map,sender);
}
void ZoomOutHook(MapScreen*map,MyGUI::Widget*sender){
    if(!RelativeZoomEnabled(map)){originalZoomOut(map,sender);return;}
    GsmZoom::DepthScope depth(zoomStepDepth);GsmZoom::StepScope step(map->zoomValueBase,GsmZoom::ButtonStep(map->zoomValueBase,map->zoomValueCurrent));originalZoomOut(map,sender);
}
void ZoomWheelHook(MapScreen*map,MyGUI::Widget*sender,int relative){
    if(!RelativeZoomEnabled(map)){originalZoomWheel(map,sender,relative);return;}
    GsmZoom::DepthScope depth(zoomStepDepth);originalZoomWheel(map,sender,GsmZoom::WheelStep(relative,map->zoomValueBase,map->zoomValueCurrent));
}
bool ZoomNativeProfile(){
    static int checked=0;if(checked)return checked>0;checked=-1;
    if(!GsmNative::Engine())return false;
    __try{const BYTE*engine=reinterpret_cast<const BYTE*>(GetModuleHandleA(NULL));for(unsigned i=0;i<sizeof(GsmZoomWindows)/sizeof(GsmZoomWindows[0]);++i)if(std::memcmp(engine+GsmZoomWindows[i].rva,GsmZoomWindows[i].code,24))return false;checked=1;return true;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
typedef char ZoomFieldLayout[(offsetof(MapScreen,zoomValueBase)==0x3c&&offsetof(MapScreen,zoomValueMax)==0x40&&offsetof(MapScreen,zoomValueCurrent)==0x44&&offsetof(MapScreen,zoomLevels)==0x80)?1:-1];
void InstallZoomSteps(){
    // Validate before patching the native entry windows. Partial installation
    // remains a native pass-through until every returned trampoline is ready.
    if(!ZoomNativeProfile()){ErrorLog("GlobalSatelliteMap: relative zoom profile unavailable; original input retained");return;}
    BYTE*engine=reinterpret_cast<BYTE*>(GetModuleHandleA(NULL));
    KenshiLib::HookStatus a=KenshiLib::AddHook(engine+GsmZoomInRva,ZoomInHook,&originalZoomIn);
    KenshiLib::HookStatus b=KenshiLib::AddHook(engine+GsmZoomOutRva,ZoomOutHook,&originalZoomOut);
    KenshiLib::HookStatus c=KenshiLib::AddHook(engine+GsmZoomWheelRva,ZoomWheelHook,&originalZoomWheel);
    zoomStepHooksReady=a==KenshiLib::SUCCESS&&b==KenshiLib::SUCCESS&&c==KenshiLib::SUCCESS&&originalZoomIn&&originalZoomOut&&originalZoomWheel;
    if(!zoomStepHooksReady)ErrorLog("GlobalSatelliteMap: relative zoom hooks incomplete; original input retained");
}
void ApplyZoomLimit(MapScreen*map){
    if((!enabled||zoomMultiplier==1)&&!zoomLease.Owned())return;
    if(!map||!map->mainWidget||!map->mapImage||!map->mapScrollView||!map->getVisible())return;
    // Native clear/zoom input updates scalar zoom before resizing the widget.
    // Let originalUpdate consume that dirty state before inferring canvas size.
    if(map->zoomUpdated)return;
    if(zoomOwner.get()!=map->mainWidget){zoomOwner.set(map->mainWidget);zoomLease.Reset();zoomFailed=false;if(zoomOwner.get()!=map->mainWidget)return;}
    if(zoomFailed)return;
    if(!ZoomNativeProfile()){zoomFailed=true;ErrorLog("GlobalSatelliteMap: map zoom native profile differs; extension inactive");return;}
    const GsmZoom::Lease before=zoomLease;
    GsmZoom::Action action;GsmZoom::Result result=zoomLease.Plan(enabled,zoomMultiplier,map->zoomValueBase,map->zoomLevels[2],map->zoomValueMax,map->zoomValueCurrent,map->mapImage->getWidth(),map->mapImage->getHeight(),action);
    if(result==GsmZoom::Refused){zoomFailed=true;ErrorLog("GlobalSatelliteMap: map zoom geometry refused; native values retained");return;}
    if(result==GsmZoom::ForeignChange){Diagnostic("map zoom maximum changed by another owner; further override suspended until settings change");return;}
    if(result!=GsmZoom::Apply)return;
    const float oldMaximum=map->zoomValueMax,oldCurrent=map->zoomValueCurrent;const Ogre::Vector4 oldCenter=map->zoomCenterOffset;const bool oldDirty=map->zoomUpdated;
    map->zoomValueMax=action.maximum;
    // This audited native method ignores sender, clamps through zoomValueMax
    // and rebuilds the viewport-centre anchor. It does not inject input.
    try{if(action.clamp){if(originalZoomIn)originalZoomIn(map,NULL);else{typedef void(__fastcall*Clamp)(MapScreen*,MyGUI::Widget*);reinterpret_cast<Clamp>(reinterpret_cast<BYTE*>(GetModuleHandleA(NULL))+GsmZoomInRva)(map,NULL);}}}
    catch(...){map->zoomValueMax=oldMaximum;map->zoomValueCurrent=oldCurrent;map->zoomCenterOffset=oldCenter;map->zoomUpdated=oldDirty;zoomLease=before;throw;}
    std::ostringstream s;s<<"map maximum zoom="<<action.maximum<<" multiplier="<<(enabled?zoomMultiplier:1);Diagnostic(s.str());
}
void UpdateHook(MapScreen*map){try{ApplyZoomLimit(map);}catch(...){zoomFailed=true;ErrorLog("GlobalSatelliteMap: zoom extension failed; further changes suspended");}originalUpdate(map);try{Tick(map);}catch(const std::exception&e){Fail(map,e.what());}catch(...){Fail(map,"terrain overlay failed; normal map retained");}}
void LoadHook(SaveFileSystem*self,const std::string&path){InterlockedIncrement(&GSM::generation);originalLoad(self,path);}void NewHook(SaveFileSystem*self){InterlockedIncrement(&GSM::generation);originalNew(self);}
}
extern "C" __declspec(dllexport)void MCM_Describe(MCM_Api*api){static std::string ini=Ini();api->beginMod(api,ID,"Global Satellite Map (development)",ini.c_str(),&Settings);if(api->version>=2){std::string info=Tr("Development terrain view with current sunlight, sky lighting, optional water and lights in loaded areas. City models are experimental.");if(api->version<4){info+="\n";info+=DiskWarningText();}api->info(api,info.c_str());}if(api->version>=4&&api->customArea)api->customArea(api,&AttachDiskWarning,&DetachDiskWarning,NULL,15);api->section(api,Tr("Global map"));api->toggle(api,ID,"Enabled",Tr("Satellite view"),Tr("Show prepared satellite terrain on the global map. The minimap keeps its existing map source."),0,0);api->toggle(api,ID,"Sunlight",Tr("Sunlight"),Tr("Use the current sun direction and brightness."),1,0);api->toggle(api,ID,"AmbientLight",Tr("Sky and ambient light"),Tr("Use the current sky and ambient illumination."),1,0);api->toggle(api,ID,"Water",Tr("Water"),Tr("Show water using the current sea level and lighting. If its resources are unavailable, terrain remains visible."),0,0);api->toggle(api,ID,"LocalLights",Tr("Torches and lamps"),Tr("Light terrain with existing point and spot lights in loaded areas. Unloaded areas do not gain invented lights."),0,0);api->toggle(api,ID,"CityModels",Tr("Existing city models"),Tr("Show existing city LOD models and an available cached model for a loaded town. Full building geometry and recent changes missing from that cache are not included."),0,0);api->integer(api,ID,"ZoomMultiplier",Tr("Maximum map zoom, multiplier"),Tr("Increase the maximum zoom while satellite view is enabled. 1 restores the normal limit. Larger zoom does not add detail beyond the captured terrain resolution."),1,1,8,0);api->section(api,Tr("Diagnostics"));api->toggle(api,ID,"Debug",Tr("Detailed log"),Tr("Write detailed diagnostics to RE_Kenshi_log.txt. Errors are always logged."),0,0);}
__declspec(dllexport)void startPlugin(){Settings();if(!GsmNative::Engine()){enabled=false;hookFailure=true;ErrorLog("GlobalSatelliteMap: exact engine profile unavailable; inert");return;}if(KenshiLib::SUCCESS!=KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveFileSystem::loadGame),LoadHook,&originalLoad)||KenshiLib::SUCCESS!=KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveFileSystem::newGame),NewHook,&originalNew)||KenshiLib::SUCCESS!=KenshiLib::AddHook(KenshiLib::GetRealAddress(&MapScreen::update),UpdateHook,&originalUpdate)){enabled=false;hookFailure=true;ErrorLog("GlobalSatelliteMap: required hooks unavailable; terrain overlay disabled");}if(!hookFailure)InstallZoomSteps();}
