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
#include "GsmAtlasJob.h"
#include "GsmNativeLight.h"
#include "GsmViewerClock.h"
#include "build/GsmRelightShader.h"
#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"advapi32.lib")
extern "C" IMAGE_DOS_HEADER __ImageBase;
namespace {
const char*const ID="GlobalSatelliteMap";
const char*const Group="LEOPARD_GlobalSatelliteMap_Development";
bool enabled=false,debug=false,sunlight=true,ambient=true,attempted=false,ready=false,failed=false,groupCreated=false,hookFailure=false,jobQuarantined=false;
LONG observedGeneration=-1;DWORD statusUntil=0;GsmViewer::TickClock tickClock;GsmViewer::LightClock lightClock;
unsigned serial=0,lightVersion=0,shadeCursor=0;int desiredLevel=4;
GSM::Session session;GsmAtlas::Manifest manifest;
GsmAtlas::Job*job=NULL,*completed=NULL;
WidgetRef layer,status,tiles[16];
AtlasView::Budget budget;AtlasView::Relighter relighter;GsmNative::Light nativeLight;
AtlasView::Uniforms lighting={};bool haveLighting=false;
struct Entry {Ogre::TexturePtr texture;std::string name;AtlasView::Cell gpu;AtlasView::Com<ID3D11Texture2D>output;int level;unsigned shaded;Entry():level(-1),shaded(0){}};
Entry cache[16];
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
void Settings(){std::string ini=Ini();bool on=!hookFailure&&GetPrivateProfileIntA(ID,"Enabled",0,ini.c_str())!=0;bool s=GetPrivateProfileIntA(ID,"Sunlight",1,ini.c_str())!=0,a=GetPrivateProfileIntA(ID,"AmbientLight",1,ini.c_str())!=0;if(on!=enabled){enabled=on;InterlockedIncrement(&GSM::generation);}if(s!=sunlight||a!=ambient){sunlight=s;ambient=a;lightClock.Reset();haveLighting=false;}debug=GetPrivateProfileIntA(ID,"Debug",0,ini.c_str())!=0;KLocLog::SetEnabled(debug);}
void Diagnostic(const std::string&s){if(debug)DebugLog("GlobalSatelliteMap: "+s);}
void SetStage(ID3D11DeviceContext*c,unsigned stage,unsigned slot,ID3D11ShaderResourceView*v){switch(stage){case 0:c->VSSetShaderResources(slot,1,&v);break;case 1:c->HSSetShaderResources(slot,1,&v);break;case 2:c->DSSetShaderResources(slot,1,&v);break;case 3:c->GSSetShaderResources(slot,1,&v);break;case 4:c->PSSetShaderResources(slot,1,&v);break;default:c->CSSetShaderResources(slot,1,&v);}}
void RetireReads(ID3D11Texture2D*output){if(!output)return;AtlasView::Com<ID3D11Device>d;output->GetDevice(&d.p);AtlasView::Com<ID3D11DeviceContext>c;d.p->GetImmediateContext(&c.p);for(unsigned stage=0;stage<6;++stage){ID3D11ShaderResourceView*v[128]={0};switch(stage){case 0:c.p->VSGetShaderResources(0,128,v);break;case 1:c.p->HSGetShaderResources(0,128,v);break;case 2:c.p->DSGetShaderResources(0,128,v);break;case 3:c.p->GSGetShaderResources(0,128,v);break;case 4:c.p->PSGetShaderResources(0,128,v);break;default:c.p->CSGetShaderResources(0,128,v);}for(unsigned i=0;i<128;++i)if(v[i]){if(AtlasView::SameResource(v[i],output))SetStage(c.p,stage,i,NULL);v[i]->Release();}}}
void DropEntry(int i){Entry&e=cache[i];if(e.level<0)return;if(tiles[i].get()){MyGUI::ImageBox*w=static_cast<MyGUI::ImageBox*>(tiles[i].get());w->setImageTexture("");w->setVisible(false);}MyGUI::RenderManager*rm=MyGUI::RenderManager::getInstancePtr();if(rm&&!e.name.empty()){MyGUI::ITexture*t=rm->getTexture(e.name);if(t)rm->destroyTexture(t);}RetireReads(e.output.p);e.output.Reset();e.gpu.Clear();e.texture.setNull();Ogre::TextureManager*tm=Ogre::TextureManager::getSingletonPtr();if(tm&&!e.name.empty()&&!tm->getByName(e.name).isNull())tm->remove(e.name);e.name.clear();e.level=-1;e.shaded=0;budget.Remove(i);}
void Cancel(){if(job)InterlockedExchange(&job->read.cancel,1);if(completed){delete completed;completed=NULL;}}
void Clear(){Cancel();tickClock.Reset();ready=attempted=failed=false;haveLighting=false;lightClock.Reset();for(int i=0;i<16;++i)DropEntry(i);relighter.Clear();nativeLight.Clear();if(layer.get())MyGUI::Gui::getInstance().destroyWidget(layer.get());layer.reset();for(int i=0;i<16;++i)tiles[i].reset();if(status.get())MyGUI::Gui::getInstance().destroyWidget(status.get());status.reset();statusUntil=0;manifest=GsmAtlas::Manifest();}
void Notice(MapScreen*map){if(!map||status.get()||!map->mainWidget)return;MyGUI::Widget*p=map->mainWidget;MyGUI::TextBox*w=p->createWidget<MyGUI::TextBox>("Kenshi_TextboxStandardText",MyGUI::IntCoord(12,12,p->getWidth()>24?p->getWidth()-24:300,56),MyGUI::Align::Default,"");status.set(w);w->setNeedMouseFocus(false);w->setNeedKeyFocus(false);w->setCaption(MyGUI::UString(Tr("Satellite terrain is unavailable. Prepared terrain data is missing or does not match this game. The normal map remains visible.")));statusUntil=GetTickCount()+12000;}
void Fail(MapScreen*map,const std::string&why){Clear();attempted=failed=true;ErrorLog("GlobalSatelliteMap: "+why);Notice(map);}
std::string ModulePath(const char*name){wchar_t path[MAX_PATH]={0};HMODULE h=GetModuleHandleA(name);DWORD n=h?GetModuleFileNameW(h,path,MAX_PATH):0;if(!n||n>=MAX_PATH)throw std::runtime_error("native renderer module path missing");return GsmAtlas::Utf8(std::wstring(path,n));}
void Launch(GsmAtlas::Job*j){if(job||jobQuarantined){delete j;throw std::runtime_error("GST2 worker still owned");}j->generation=InterlockedCompareExchange(&GSM::generation,0,0);j->thread=CreateThread(NULL,0,GsmAtlas::Run,j,0,NULL);if(!j->thread){delete j;throw std::runtime_error("GST2 bounded worker creation failed");}job=j;}
void StartValidation(){attempted=true;std::auto_ptr<GsmAtlas::Job>owned(new GsmAtlas::Job());GsmAtlas::Job*j=owned.get();j->validation=true;j->folder=Folder()+"captures/";j->game=Game();j->engine=Exe();j->module[0]=ModulePath(GsmOgreFile);j->module[1]=ModulePath(GsmD3d11File);if(j->game.empty()||j->engine.empty())throw std::runtime_error("game source directory unavailable");Launch(owned.release());Diagnostic("validating complete terrain atlas and immutable world/mod source profile in bounded file worker");}
void Poll(MapScreen*map){if(!job||jobQuarantined)return;DWORD wait=WaitForSingleObject(job->thread,0);if(wait==WAIT_TIMEOUT)return;if(wait!=WAIT_OBJECT_0||!CloseHandle(job->thread)){jobQuarantined=true;throw std::runtime_error("GST2 worker handle state uncertain; retained without retry");}job->thread=NULL;GsmAtlas::Job*j=job;job=NULL;bool current=j->generation==InterlockedCompareExchange(&GSM::generation,0,0)&&!InterlockedCompareExchange(&j->read.cancel,0,0);if(!current){delete j;return;}if(!j->ok){std::string error=j->error;delete j;Fail(map,error);return;}if(j->validation){manifest=j->manifest;delete j;nativeLight.Init();HRESULT hr=relighter.Init(nativeLight.device.p,GsmRelightShader,sizeof(GsmRelightShader),2048,0);if(FAILED(hr))throw std::runtime_error("private terrain relighter initialization failed");ready=true;Diagnostic("complete GST2/source profile verified; terrain-only native lighting viewer ready");}else{if(completed)delete completed;completed=j;}}
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
        for (int i = 0; i < 16; ++i)
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
void Lighting(DWORD now){if(!lightClock.Due(now,1000))return;AtlasView::Uniforms fresh=nativeLight.Snapshot(sunlight,ambient);if(!haveLighting||!SameLighting(fresh,lighting)){lighting=fresh;haveLighting=true;if(++lightVersion==0){lightVersion=1;for(int i=0;i<16;++i)cache[i].shaded=0;}}}
void Upload(int tile,int level,GsmAtlas::Job&j){if(cache[tile].level>=0)DropEntry(tile);if(!budget.Set(tile,level))throw std::runtime_error("terrain GPU resource budget refused");Entry&e=cache[tile];e.level=level;try{HRESULT hr=e.gpu.Upload(nativeLight.device.p,j.read);if(FAILED(hr))throw std::runtime_error("terrain material/normal/height GPU upload failed");j.read.Clear();if(!groupCreated){Ogre::ResourceGroupManager::getSingleton().createResourceGroup(Group);groupCreated=true;}unsigned side=AtlasView::Side(level);std::vector<unsigned char>blank(side*side*4,0);Ogre::Image image;image.loadDynamicImage(&blank[0],side,side,1,Ogre::PF_A8B8G8R8,false,1,0);std::ostringstream name;name<<"GlobalSatelliteMap_GST2_"<<observedGeneration<<'_'<<++serial;e.name=name.str();e.texture=Ogre::TextureManager::getSingleton().loadImage(e.name,Group,image,Ogre::TEX_TYPE_2D,0);if(e.texture.isNull()||e.texture->getFormat()!=Ogre::PF_A8B8G8R8||e.texture->getNumMipmaps()!=0)throw std::runtime_error("owned terrain output format differs");ID3D11ShaderResourceView*v=GsmNative::Srv(e.texture.get());AtlasView::Com<ID3D11Resource>resource;v->GetResource(&resource.p);if(!AtlasView::SameDevice(v,nativeLight.device.p)||FAILED(resource.p->QueryInterface(__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&e.output.p))))throw std::runtime_error("owned terrain output D3D identity differs");}catch(...){DropEntry(tile);throw;}}
void Draw(MapScreen*map,DWORD now){GSM::Rect coords[16];if(!Geometry(map,coords))throw std::runtime_error("native map projection not ready/unsupported");EnsureLayer(map);MyGUI::IntCoord view=map->mapScrollView->getAbsoluteCoord();MyGUI::IntPoint origin=map->mapImage->getAbsolutePosition();GSM::Rect clip(view.left-origin.left,view.top-origin.top,view.width,view.height);bool visible[16]={false};unsigned count=0,display=1;for(int i=0;i<16;++i){visible[i]=GSM::Intersects(coords[i],clip);if(visible[i]){++count;display=std::max(display,unsigned(coords[i].w>coords[i].h?coords[i].w:coords[i].h));}}if(!count)return;desiredLevel=GsmAtlas::Level(count,display);for(int i=0;i<16;++i)if(cache[i].level>=0&&(!visible[i]||cache[i].level!=desiredLevel))DropEntry(i);
    if(completed){int tile=int(completed->read.tileZ*4+completed->read.tileX);if(tile>=0&&tile<16&&visible[tile]&&completed->read.level==unsigned(desiredLevel))Upload(tile,desiredLevel,*completed);delete completed;completed=NULL;}
    Lighting(now);
    // Fair cursor prevents continuous1Hz light changes starving later cells.
    for(unsigned k=0;k<16;++k){unsigned i=(shadeCursor+k)%16;if(visible[i]&&cache[i].level==desiredLevel&&cache[i].shaded!=lightVersion){AtlasView::Uniforms u=lighting;u.referenceSide[3]=float(AtlasView::Side(desiredLevel));u.tileBounds[0]=float(manifest.minX+(manifest.maxX-manifest.minX)*(i%4)/4);u.tileBounds[1]=float(manifest.minZ+(manifest.maxZ-manifest.minZ)*(i/4)/4);u.tileBounds[2]=float((manifest.maxX-manifest.minX)/4);u.tileBounds[3]=float((manifest.maxZ-manifest.minZ)/4);HRESULT hr=relighter.Render(cache[i].gpu,u,nativeLight.views,nativeLight.samplers,cache[i].output.p);if(FAILED(hr)){std::ostringstream e;e<<"terrain relight failed HRESULT="<<std::hex<<unsigned(hr);throw std::runtime_error(e.str());}cache[i].shaded=lightVersion;shadeCursor=(i+1)%16;break;}}
    if(!job&&!completed)for(int i=0;i<16;++i)if(visible[i]&&cache[i].level<0){std::auto_ptr<GsmAtlas::Job>owned(new GsmAtlas::Job());GsmAtlas::Job*j=owned.get();j->read.tileX=i%4;j->read.tileZ=i/4;j->read.level=desiredLevel;std::memcpy(j->read.expectedHeader,manifest.header[i],32);std::ostringstream file;file<<Folder()<<"captures/tile_"<<i%4<<'_'<<i/4<<".gst";j->read.path=GsmAtlas::Wide(file.str());Launch(owned.release());break;}
    for(int i=0;i<16;++i){MyGUI::ImageBox*w=static_cast<MyGUI::ImageBox*>(tiles[i].get());if(!w)continue;bool show=visible[i]&&cache[i].level==desiredLevel&&cache[i].shaded!=0;if(show){MyGUI::IntCoord rect(coords[i].x,coords[i].y,coords[i].w,coords[i].h);if(w->getCoord()!=rect)w->setCoord(rect);if(w->_getTextureName()!=cache[i].name)w->setImageTexture(cache[i].name);}if(w->getVisible()!=show)w->setVisible(show);}
}
void Tick(MapScreen*map){LONG gen=InterlockedCompareExchange(&GSM::generation,0,0);if(gen!=observedGeneration){Clear();session.Reset();observedGeneration=gen;}Poll(map);if(!map||!map->getVisible()||!enabled){tickClock.Reset();lightClock.Reset();if(layer.get())layer.get()->setVisible(false);if(status.get())status.get()->setVisible(false);if(completed){delete completed;completed=NULL;}return;}DWORD now=GetTickCount();if(status.get()&&statusUntil&&static_cast<LONG>(now-statusUntil)>=0){MyGUI::Gui::getInstance().destroyWidget(status.get());status.reset();statusUntil=0;}if(!tickClock.Due(now,200))return;SaveFileSystem*fs=SaveFileSystem::getSingleton();if(!fs||fs->busy()||!ou)return;if(session.Change("",reinterpret_cast<unsigned long long>(ou))){Clear();InterlockedIncrement(&GSM::generation);observedGeneration=InterlockedCompareExchange(&GSM::generation,0,0);}if(!attempted&&!job&&!jobQuarantined)StartValidation();if(!ready)return;Draw(map,now);if(layer.get()&&!layer.get()->getVisible())layer.get()->setVisible(true);}
void UpdateHook(MapScreen*map){originalUpdate(map);try{Tick(map);}catch(const std::exception&e){Fail(map,e.what());}catch(...){Fail(map,"terrain overlay failed; normal map retained");}}
void LoadHook(SaveFileSystem*self,const std::string&path){InterlockedIncrement(&GSM::generation);originalLoad(self,path);}void NewHook(SaveFileSystem*self){InterlockedIncrement(&GSM::generation);originalNew(self);}
}
extern "C" __declspec(dllexport)void MCM_Describe(MCM_Api*api){static std::string ini=Ini();api->beginMod(api,ID,"Global Satellite Map (development)",ini.c_str(),&Settings);if(api->version>=2)api->info(api,Tr("Development terrain view with current sunlight and sky lighting. Buildings, water and local lights are not included."));api->section(api,Tr("Global map"));api->toggle(api,ID,"Enabled",Tr("Satellite view"),Tr("Show prepared satellite terrain on the global map. The minimap keeps its existing map source."),0,0);api->toggle(api,ID,"Sunlight",Tr("Sunlight"),Tr("Use the current sun direction and brightness."),1,0);api->toggle(api,ID,"AmbientLight",Tr("Sky and ambient light"),Tr("Use the current sky and ambient illumination."),1,0);api->section(api,Tr("Diagnostics"));api->toggle(api,ID,"Debug",Tr("Detailed log"),Tr("Write detailed diagnostics to RE_Kenshi_log.txt. Errors are always logged."),0,0);}
__declspec(dllexport)void startPlugin(){Settings();if(!GsmNative::Engine()){enabled=false;hookFailure=true;ErrorLog("GlobalSatelliteMap: exact engine profile unavailable; inert");return;}if(KenshiLib::SUCCESS!=KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveFileSystem::loadGame),LoadHook,&originalLoad)||KenshiLib::SUCCESS!=KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveFileSystem::newGame),NewHook,&originalNew)||KenshiLib::SUCCESS!=KenshiLib::AddHook(KenshiLib::GetRealAddress(&MapScreen::update),UpdateHook,&originalUpdate)){enabled=false;hookFailure=true;ErrorLog("GlobalSatelliteMap: required hooks unavailable; terrain overlay disabled");}}
