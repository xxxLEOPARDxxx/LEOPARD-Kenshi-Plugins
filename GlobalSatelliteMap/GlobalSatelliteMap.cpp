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
#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"advapi32.lib")
extern "C" IMAGE_DOS_HEADER __ImageBase;
namespace {
const char*const ID="GlobalSatelliteMap";
const char*const Group="LEOPARD_GlobalSatelliteMap_Development";
bool enabled=false,debug=false,sunlight=true,ambient=true,attempted=false,ready=false,failed=false,groupCreated=false,hookFailure=false,jobQuarantined=false;
LONG observedGeneration=-1;DWORD statusUntil=0;GsmViewer::TickClock tickClock;GsmViewer::LightClock lightClock;
unsigned serial=0,lightVersion=0,shadeCursor=0,cacheAge=0;
unsigned windowReads=0;AtlasView::U64 windowBytes=0;DWORD statsAt=0;
bool waterOption=false,waterAttempted=false,waterFailed=false,waterPipelineReady=false;
unsigned waterVersion=1;GsmViewer::LightClock waterClock;
NativeWater::Snapshot waterSnapshot;AtlasWater::Pipeline waterPipeline;
int projectedWidth=0,projectedHeight=0;
GSM::Session session;GsmAtlas::Manifest manifest;
GsmAtlas::Job*job=NULL,*completed=NULL;
WidgetRef layer,status,tiles[AtlasView::WindowSlots];
WidgetRef diskWarning;
AtlasView::Relighter relighter;GsmNative::Light nativeLight;
AtlasView::Uniforms lighting={};bool haveLighting=false;
struct Entry {Ogre::TexturePtr texture;std::string name;AtlasView::Cell gpu;AtlasView::Com<ID3D11Texture2D>output;AtlasView::WindowKey key;unsigned shaded,age,waterStamp;float minHeight;bool hasHeight,waterApplied;Entry():shaded(0),age(0),waterStamp(0),minHeight(0),hasHeight(false),waterApplied(false){}};
Entry cache[AtlasView::WindowSlots];
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
void Settings(){std::string ini=Ini();bool on=!hookFailure&&GetPrivateProfileIntA(ID,"Enabled",0,ini.c_str())!=0;bool s=GetPrivateProfileIntA(ID,"Sunlight",1,ini.c_str())!=0,a=GetPrivateProfileIntA(ID,"AmbientLight",1,ini.c_str())!=0;if(on!=enabled){enabled=on;InterlockedIncrement(&GSM::generation);}if(s!=sunlight||a!=ambient){sunlight=s;ambient=a;lightClock.Reset();haveLighting=false;waterClock.Reset();}bool w=GetPrivateProfileIntA(ID,"Water",0,ini.c_str())!=0;if(w!=waterOption){waterOption=w;waterSnapshot.Clear();waterPipeline.Clear();waterPipelineReady=false;waterClock.Reset();if(waterFailed){waterAttempted=false;manifest.waterVerified=false;}waterFailed=false;}debug=GetPrivateProfileIntA(ID,"Debug",0,ini.c_str())!=0;KLocLog::SetEnabled(debug);}
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
    if(diskWarning.get()&&diskWarning.get()->getParent()==p)return;
    DetachDiskWarning(NULL);
    MyGUI::EditBox*w=p->createWidget<MyGUI::EditBox>("Kenshi_WordWrapEmpty",
        MyGUI::IntCoord(4,2,p->getWidth()-8,p->getHeight()-4),MyGUI::Align::Stretch,"");
    diskWarning.set(w);
    w->setNeedMouseFocus(false);w->setNeedKeyFocus(false);
    w->setEditMultiLine(true);w->setEditWordWrap(true);w->setEditStatic(true);
    w->setTextAlign(MyGUI::Align::Left|MyGUI::Align::Top);
    w->setTextColour(MyGUI::Colour(1.0f,0.2f,0.2f));
    w->setCaption(MyGUI::UString(DiskWarningText()));
}
void SetStage(ID3D11DeviceContext*c,unsigned stage,unsigned slot,ID3D11ShaderResourceView*v){switch(stage){case 0:c->VSSetShaderResources(slot,1,&v);break;case 1:c->HSSetShaderResources(slot,1,&v);break;case 2:c->DSSetShaderResources(slot,1,&v);break;case 3:c->GSSetShaderResources(slot,1,&v);break;case 4:c->PSSetShaderResources(slot,1,&v);break;default:c->CSSetShaderResources(slot,1,&v);}}
void RetireReads(ID3D11Texture2D*output){if(!output)return;AtlasView::Com<ID3D11Device>d;output->GetDevice(&d.p);AtlasView::Com<ID3D11DeviceContext>c;d.p->GetImmediateContext(&c.p);for(unsigned stage=0;stage<6;++stage){ID3D11ShaderResourceView*v[128]={0};switch(stage){case 0:c.p->VSGetShaderResources(0,128,v);break;case 1:c.p->HSGetShaderResources(0,128,v);break;case 2:c.p->DSGetShaderResources(0,128,v);break;case 3:c.p->GSGetShaderResources(0,128,v);break;case 4:c.p->PSGetShaderResources(0,128,v);break;default:c.p->CSGetShaderResources(0,128,v);}for(unsigned i=0;i<128;++i)if(v[i]){if(AtlasView::SameResource(v[i],output))SetStage(c.p,stage,i,NULL);v[i]->Release();}}}
void DropEntry(int i){Entry&e=cache[i];if(!e.key.Valid())return;if(tiles[i].get()){MyGUI::ImageBox*w=static_cast<MyGUI::ImageBox*>(tiles[i].get());w->setImageTexture("");w->setVisible(false);}MyGUI::RenderManager*rm=MyGUI::RenderManager::getInstancePtr();if(rm&&!e.name.empty()){MyGUI::ITexture*t=rm->getTexture(e.name);if(t)rm->destroyTexture(t);}RetireReads(e.output.p);e.output.Reset();e.gpu.Clear();e.texture.setNull();Ogre::TextureManager*tm=Ogre::TextureManager::getSingletonPtr();if(tm&&!e.name.empty()&&!tm->getByName(e.name).isNull())tm->remove(e.name);e.name.clear();e.key=AtlasView::WindowKey();e.shaded=e.age=0;}
void Cancel(){if(job)InterlockedExchange(&job->read.cancel,1);if(completed){delete completed;completed=NULL;}}
void Clear(){Cancel();tickClock.Reset();ready=attempted=failed=false;haveLighting=false;lightClock.Reset();waterSnapshot.Clear();waterPipeline.Clear();waterPipelineReady=waterAttempted=waterFailed=false;waterClock.Reset();waterVersion=1;for(unsigned i=0;i<AtlasView::WindowSlots;++i)DropEntry(i);relighter.Clear();nativeLight.Clear();if(layer.get())MyGUI::Gui::getInstance().destroyWidget(layer.get());layer.reset();for(unsigned i=0;i<AtlasView::WindowSlots;++i)tiles[i].reset();if(status.get())MyGUI::Gui::getInstance().destroyWidget(status.get());status.reset();statusUntil=0;manifest=GsmAtlas::Manifest();}
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
    if(j->waterValidation){
        manifest.waterVerified=j->ok&&j->manifest.waterVerified;
        if(!manifest.waterVerified){waterFailed=true;ErrorLog(std::string("GlobalSatelliteMap: water disabled; terrain retained: ")+j->error);}
        else Diagnostic("native water source profile verified in bounded worker");
        delete j;return;
    }
    if(!j->ok){std::string error=j->error;delete j;Fail(map,error);return;}
    if(j->validation){manifest=j->manifest;delete j;nativeLight.Init();HRESULT hr=relighter.Init(nativeLight.device.p,GsmRelightShader,sizeof(GsmRelightShader),256,AtlasView::WindowSlots*AtlasView::CellBytes(5));if(FAILED(hr))throw std::runtime_error("private terrain relighter initialization failed");ready=true;Diagnostic("complete GST2/source profile verified; native terrain lighting viewer ready");}
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
void WaterFailure(const char*why){
    if(!waterFailed)ErrorLog(std::string("GlobalSatelliteMap: water disabled; terrain retained: ")+why);
    waterFailed=true;waterSnapshot.Clear();waterPipeline.Clear();waterPipelineReady=false;
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
HRESULT RenderWater(Entry&e,const AtlasView::Uniforms&u){
    if(!waterPipelineReady){
        HRESULT hr=waterPipeline.Init(nativeLight.device.p,AtlasTerrainHdrShader,sizeof(AtlasTerrainHdrShader),AtlasWaterMaterialShader,sizeof(AtlasWaterMaterialShader),AtlasWaterToneShader,sizeof(AtlasWaterToneShader),AtlasView::WindowGpuBytes());
        if(FAILED(hr))return hr;waterPipelineReady=true;
    }
    AtlasWater::Uniforms w=waterSnapshot.Values();std::memcpy(w.tileBounds,u.tileBounds,16);w.sourceWindow[0]=256;w.sourceWindow[1]=w.sourceWindow[2]=w.sourceWindow[3]=0;
    ID3D11ShaderResourceView*views[5];ID3D11SamplerState*samplers[5];for(unsigned i=0;i<5;++i){views[i]=waterSnapshot.Inputs().View(i);samplers[i]=waterSnapshot.Inputs().Sampler(i);}
    return waterPipeline.Render(e.gpu,u,nativeLight.views,nativeLight.samplers,w,views,samplers,e.output.p,AtlasView::WindowGpuBytes());
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
    if(slot<0||slot>=int(AtlasView::WindowSlots)||!key.Valid()||j.read.level!=5||AtlasView::WindowGpuBytes()>AtlasView::GpuCap)throw std::runtime_error("terrain window GPU resource budget refused");
    if(cache[slot].key.Valid())DropEntry(slot);
    Entry&e=cache[slot];e.key=key;e.hasHeight=j.windowHasFiniteHeight;e.minHeight=j.windowMinHeight;e.waterApplied=false;e.waterStamp=0;
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
void WindowBounds(const AtlasView::WindowKey&key,double&x,double&z,double&sx,double&sz){
    double n=AtlasView::Side(key.level)/AtlasView::WindowSide;
    sx=(manifest.maxX-manifest.minX)/4/n;sz=(manifest.maxZ-manifest.minZ)/4/n;
    x=manifest.minX+(manifest.maxX-manifest.minX)*(key.tile%4)/4+sx*key.x;
    z=manifest.minZ+(manifest.maxZ-manifest.minZ)*(key.tile/4)/4+sz*key.z;
}
MyGUI::IntCoord WindowGeometry(MapScreen*map,const AtlasView::WindowKey&key){
    double x,z,sx,sz;WindowBounds(key,x,z,sx,sz);
    MyGUI::IntPoint a=map->worldToMapCoords(Ogre::Vector3(float(x),0,float(z)));
    MyGUI::IntPoint b=map->worldToMapCoords(Ogre::Vector3(float(x+sx),0,float(z+sz)));
    return MyGUI::IntCoord(a.left,a.top,b.left-a.left,b.top-a.top);
}
// Native map resizing is immediate. Reposition resident children on a zoom
// event even between streaming ticks; this does no IO, allocation or relighting.
void RepositionZoom(MapScreen*map){
    if(!map||!map->mapImage||!layer.get())return;
    int width=map->mapImage->getWidth(),height=map->mapImage->getHeight();
    if(width==projectedWidth&&height==projectedHeight)return;
    if(width<=0||height<=0||width>1000000||height>1000000)return;
    projectedWidth=width;projectedHeight=height;layer.get()->setSize(map->mapImage->getSize());
    for(unsigned i=0;i<AtlasView::WindowSlots;++i)if(cache[i].key.Valid()&&tiles[i].get()){
        MyGUI::IntCoord rect=WindowGeometry(map,cache[i].key);
        if(tiles[i].get()->getCoord()!=rect)tiles[i].get()->setCoord(rect);
    }
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
    AtlasView::WindowPlan plan=AtlasView::PlanWindows(cells,clip);
    for(unsigned i=0;i<AtlasView::WindowSlots;++i)if(cache[i].key.Valid()&&plan.Contains(cache[i].key))Touch(cache[i]);
    if(completed){
        ++windowReads;windowBytes+=completed->read.readBytes;
        AtlasView::WindowKey key=completed->window;
        if(plan.Contains(key)&&FindWindow(key)<0){
            int slot=int(key.tile);
            if(key.level!=5){AtlasView::WindowKey keys[AtlasView::WindowSlots];unsigned age[AtlasView::WindowSlots];for(unsigned i=0;i<AtlasView::WindowSlots;++i){keys[i]=cache[i].key;age[i]=cache[i].age;}slot=AtlasView::WindowVictim(keys,age,plan);}
            if(slot>=0)Upload(slot,key,*completed);
        }
        delete completed;completed=NULL;
    }
    Lighting(now);WaterState(now);
    // Four256 windows have the same pixel budget as one former512 tile.
    unsigned shaded=0,next=shadeCursor;
    for(unsigned pass=0;pass<3&&shaded<4;++pass)for(unsigned k=0;k<AtlasView::WindowSlots&&shaded<4;++k){unsigned i=(shadeCursor+k)%AtlasView::WindowSlots;Entry&e=cache[i];
        if(!e.key.Valid()||Superseded(e.key,plan.level))continue;
        bool onScreen=AtlasView::WindowVisible(AtlasView::SubWindow(cells[e.key.tile],e.key),clip);
        bool useWater=WantsWater(e),waterDirty=e.waterApplied!=useWater||(useWater&&e.waterStamp!=waterVersion);
        if(!AtlasView::NeedsWindowShade(onScreen,plan.Contains(e.key),e.shaded,waterDirty?0:lightVersion,pass))continue;
        AtlasView::Uniforms u=lighting;double x,z,sx,sz;WindowBounds(e.key,x,z,sx,sz);
        u.referenceSide[3]=float(AtlasView::WindowSide);u.tileBounds[0]=float(x);u.tileBounds[1]=float(z);u.tileBounds[2]=float(sx);u.tileBounds[3]=float(sz);
        unsigned refusal=0;HRESULT hr=E_FAIL;
        if(useWater){hr=RenderWater(e,u);if(FAILED(hr)){std::ostringstream error;error<<"water composition HRESULT="<<std::hex<<unsigned(hr);WaterFailure(error.str().c_str());useWater=false;}}
        if(!useWater)hr=relighter.Render(e.gpu,u,nativeLight.views,nativeLight.samplers,e.output.p,&refusal);
        if(FAILED(hr)){std::ostringstream error;error<<"terrain window relight failed HRESULT="<<std::hex<<unsigned(hr)<<" stage="<<refusal;throw std::runtime_error(error.str());}
        e.shaded=lightVersion;e.waterApplied=useWater;e.waterStamp=useWater?waterVersion:0;next=(i+1)%AtlasView::WindowSlots;++shaded;
    }shadeCursor=next;
    if(!job&&!completed){
        bool requested=false;
        if(waterOption&&!waterFailed&&!waterAttempted){std::auto_ptr<GsmAtlas::Job>owned(new GsmAtlas::Job());owned->waterValidation=true;owned->game=Game();Launch(owned.release());waterAttempted=true;requested=true;}
        for(unsigned t=0;t<16&&!requested;++t){AtlasView::WindowKey key(t,5,0,0);if(plan.visible[t]&&FindWindow(key)<0){RequestWindow(key);requested=true;}}
        if(!requested){
            // Center-first refinement, bounded56 keys, no world or filesystem scan.
            int best=-1;double nearest=1e100;
            for(unsigned priority=0;priority<2&&best<0;++priority){
                unsigned begin=priority?plan.visibleCount:0,end=priority?plan.count:plan.visibleCount;
                for(unsigned i=begin;i<end;++i)if(FindWindow(plan.keys[i])<0){AtlasView::WindowRect r=AtlasView::SubWindow(cells[plan.keys[i].tile],plan.keys[i]);double dx=r.x+r.w*.5-clip.x-clip.w*.5,dz=r.y+r.h*.5-clip.y-clip.h*.5,d=dx*dx+dz*dz;if(d<nearest){nearest=d;best=int(i);}}
            }
            if(best>=0)RequestWindow(plan.keys[best]);
        }
    }
    for(unsigned i=0;i<AtlasView::WindowSlots;++i){
        MyGUI::ImageBox*w=static_cast<MyGUI::ImageBox*>(tiles[i].get());if(!w)continue;Entry&e=cache[i];
        bool show=e.key.Valid()&&e.shaded!=0&&!Superseded(e.key,plan.level)&&AtlasView::WindowVisible(AtlasView::SubWindow(cells[e.key.tile],e.key),clip);
        if(show){MyGUI::IntCoord rect=WindowGeometry(map,e.key);if(w->getCoord()!=rect)w->setCoord(rect);if(w->_getTextureName()!=e.name){w->setDepth(int(e.key.level));w->setImageTexture(e.name);}}
        if(w->getVisible()!=show)w->setVisible(show);
    }
    if(debug&&now-statsAt>=5000){
        statsAt=now;unsigned resident=0,detailReady=0;
        for(unsigned i=0;i<AtlasView::WindowSlots;++i)if(cache[i].key.Valid())++resident;
        for(unsigned i=0;i<plan.visibleCount;++i){int slot=FindWindow(plan.keys[i]);if(slot>=0&&cache[slot].shaded)++detailReady;}
        std::ostringstream message;message<<"view sourceLevel="<<plan.level<<" densityLevel="<<plan.densityLevel<<" detailReady="<<detailReady<<'/'<<plan.visibleCount<<" apron="<<plan.count-plan.visibleCount<<" resident="<<resident<<" reads="<<windowReads<<" bytes="<<windowBytes<<" gpuCapBytes="<<AtlasView::WindowGpuBytes()<<" primary="<<coords[0].w<<'x'<<coords[0].h<<" viewport="<<view.width<<'x'<<view.height;Diagnostic(message.str());
    }
}
void Tick(MapScreen*map){LONG gen=InterlockedCompareExchange(&GSM::generation,0,0);if(gen!=observedGeneration){Clear();session.Reset();observedGeneration=gen;}Poll(map);if(!map||!map->getVisible()||!enabled){tickClock.Reset();lightClock.Reset();if(layer.get())layer.get()->setVisible(false);if(status.get())status.get()->setVisible(false);if(completed){delete completed;completed=NULL;}return;}DWORD now=GetTickCount();if(status.get()&&statusUntil&&static_cast<LONG>(now-statusUntil)>=0){MyGUI::Gui::getInstance().destroyWidget(status.get());status.reset();statusUntil=0;}if(ready)RepositionZoom(map);if(!tickClock.Due(now,200))return;SaveFileSystem*fs=SaveFileSystem::getSingleton();if(!fs||fs->busy()||!ou)return;if(session.Change("",reinterpret_cast<unsigned long long>(ou))){Clear();InterlockedIncrement(&GSM::generation);observedGeneration=InterlockedCompareExchange(&GSM::generation,0,0);}if(!attempted&&!job&&!jobQuarantined)StartValidation();if(!ready)return;Draw(map,now);if(layer.get()&&!layer.get()->getVisible())layer.get()->setVisible(true);}
void UpdateHook(MapScreen*map){originalUpdate(map);try{Tick(map);}catch(const std::exception&e){Fail(map,e.what());}catch(...){Fail(map,"terrain overlay failed; normal map retained");}}
void LoadHook(SaveFileSystem*self,const std::string&path){InterlockedIncrement(&GSM::generation);originalLoad(self,path);}void NewHook(SaveFileSystem*self){InterlockedIncrement(&GSM::generation);originalNew(self);}
}
extern "C" __declspec(dllexport)void MCM_Describe(MCM_Api*api){static std::string ini=Ini();api->beginMod(api,ID,"Global Satellite Map (development)",ini.c_str(),&Settings);if(api->version>=2){std::string info=Tr("Development terrain view with current sunlight, sky lighting and optional water. Buildings and local lights are not included.");if(api->version<4){info+="\n";info+=DiskWarningText();}api->info(api,info.c_str());}if(api->version>=4&&api->customArea)api->customArea(api,&AttachDiskWarning,&DetachDiskWarning,NULL,15);api->section(api,Tr("Global map"));api->toggle(api,ID,"Enabled",Tr("Satellite view"),Tr("Show prepared satellite terrain on the global map. The minimap keeps its existing map source."),0,0);api->toggle(api,ID,"Sunlight",Tr("Sunlight"),Tr("Use the current sun direction and brightness."),1,0);api->toggle(api,ID,"AmbientLight",Tr("Sky and ambient light"),Tr("Use the current sky and ambient illumination."),1,0);api->toggle(api,ID,"Water",Tr("Water"),Tr("Show water using the current sea level and lighting. If its resources are unavailable, terrain remains visible."),0,0);api->section(api,Tr("Diagnostics"));api->toggle(api,ID,"Debug",Tr("Detailed log"),Tr("Write detailed diagnostics to RE_Kenshi_log.txt. Errors are always logged."),0,0);}
__declspec(dllexport)void startPlugin(){Settings();if(!GsmNative::Engine()){enabled=false;hookFailure=true;ErrorLog("GlobalSatelliteMap: exact engine profile unavailable; inert");return;}if(KenshiLib::SUCCESS!=KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveFileSystem::loadGame),LoadHook,&originalLoad)||KenshiLib::SUCCESS!=KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveFileSystem::newGame),NewHook,&originalNew)||KenshiLib::SUCCESS!=KenshiLib::AddHook(KenshiLib::GetRealAddress(&MapScreen::update),UpdateHook,&originalUpdate)){enabled=false;hookFailure=true;ErrorLog("GlobalSatelliteMap: required hooks unavailable; terrain overlay disabled");}}
