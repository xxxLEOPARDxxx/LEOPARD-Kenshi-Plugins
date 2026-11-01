#pragma once
#include <process.h>
#include <memory>
#include <kenshi/SaveFileSystem.h>
#include <kenshi/SaveManager.h>
#include <kenshi/gui/LoadSaveWindow.h>
#include <mygui/MyGUI_MultiListBox.h>
#include <mygui/MyGUI_ListBox.h>
#include <OgreRenderWindow.h>
#include <OgrePixelFormat.h>
#include <OgrePixelBox.h>
#include <OgreHardwarePixelBuffer.h>
#include "SavePreviewCodec.h"
#include "SavePreviewGpu.h"
#include "SavePreviewAlpha.h"
#include <stddef.h>

// Verified against installed KenshiLib 0.5 and live Steam 1.0.65 native code.
static_assert(sizeof(SaveInfo)==0x108,"SaveInfo ABI changed");
static_assert(offsetof(LoadSaveWindow,list)==0xa0&&offsetof(LoadSaveWindow,games)==0xa8,"load-window ABI changed");
static_assert(offsetof(SaveFileSystem,state)==0x1e0&&offsetof(SaveFileSystem,failedToCopyError)==0x1e8,"save completion ABI changed");

namespace UITSavePreviews
{
    class PreviewWidgetRef : public MyGUI::IUnlinkWidget
    {
        MyGUI::Widget* value;MyGUI::WidgetManager* owner;
    public:
        PreviewWidgetRef():value(NULL),owner(NULL){}
        MyGUI::Widget* get()const{return value;}
        void reset(){value=NULL;}
        void set(MyGUI::Widget* widget)
        {
            MyGUI::WidgetManager* manager=MyGUI::WidgetManager::getInstancePtr();
            if(!manager){value=NULL;return;}
            if(owner!=manager){owner=manager;owner->registerUnlinker(this);}
            value=widget;
        }
        void release()
        {
            if(owner&&owner==MyGUI::WidgetManager::getInstancePtr())owner->unregisterUnlinker(this);
            owner=NULL;value=NULL;
        }
        virtual void _unlinkWidget(MyGUI::Widget* widget){if(widget==value)value=NULL;}
    };
    struct Writer
    {
        CRITICAL_SECTION gate;volatile LONG cancelled;HANDLE thread;bool success;
        std::string path,error;SavePreviewCodec::Header header;std::vector<unsigned char> pixels;
        Writer():cancelled(0),thread(NULL),success(false){InitializeCriticalSection(&gate);memset(&header,0,sizeof(header));}
        ~Writer(){DeleteCriticalSection(&gate);}
        struct Gate
        {
            Writer& owner;explicit Gate(Writer& value):owner(value){EnterCriticalSection(&owner.gate);}
            ~Gate(){LeaveCriticalSection(&owner.gate);}
        private:Gate(const Gate&);Gate& operator=(const Gate&);
        };
    };
    static bool StopWriter(Writer& job,DWORD timeout)
    {
        InterlockedExchange(&job.cancelled,1);
        if(!job.thread)return true;
        if(WaitForSingleObject(job.thread,timeout)!=WAIT_OBJECT_0)return false;
        CloseHandle(job.thread);job.thread=NULL;return true;
    }
    static bool enabled=true,debug=false,installed=false,initialized=false;
    static DWORD mainThread=0,started=0;static unsigned serial=0;
    static SavePreviewPolicy::Pending pending;
    static GameWorld* world=NULL;static Renderer* renderer=NULL;
    static std::vector<unsigned char> thumbnail;
    static unsigned thumbW=0,thumbH=0;
    static Writer* writer=NULL;
    static PreviewWidgetRef root,panel,image,saveList,sizedRoots[2];
    static SavePreviewPolicy::DialogHeightState dialogHeights[2];
    static int dialogHeightPercent=120;
    static MyGUI::ITexture* texture=NULL;
    static std::string selectedPath;
    static SavePreviewCodec::Header displayedRevision={};
    static bool displayed=false;
    static bool refreshSelection=false;
    static bool selectionActive=false;
    __declspec(thread) static bool inCapture=false;
    typedef SavePreviewAlpha<MyGUI::Widget,PreviewWidgetRef> AlphaMask;
    __declspec(thread) static AlphaMask* activeAlpha=NULL;
    __declspec(thread) static MyGUI::Widget* settingAlpha=NULL;
    static void (*originalAlpha)(MyGUI::Widget*)=NULL;
    struct SettingAlpha
    {
        MyGUI::Widget* previous;explicit SettingAlpha(MyGUI::Widget* w):previous(settingAlpha){settingAlpha=w;}
        ~SettingAlpha(){settingAlpha=previous;}
    };
    static void AlphaUpdate(MyGUI::Widget* self)
    {
        if(activeAlpha&&self!=settingAlpha)
        {
            activeAlpha->Remember(self);
            if(self->getAlpha()!=0){SettingAlpha scope(self);self->setAlpha(0);return;}
        }
        originalAlpha(self);
    }

    static bool OnMain()
    {
        if(!mainThread&&au&&au->render&&au->render->window)
        {
            try
            {
                HWND window=NULL;au->render->window->getCustomAttribute("WINDOW",&window);
                DWORD process=0;DWORD thread=window?GetWindowThreadProcessId(window,&process):0;
                if(thread&&process==GetCurrentProcessId())mainThread=thread;
            }
            catch(...){return false;}
        }
        return mainThread&&GetCurrentThreadId()==mainThread;
    }

    static std::string FullPath(const std::string& value)
    {
        if(value.empty())return std::string();
        wchar_t path[MAX_PATH]={};DWORD n=GetFullPathNameW(SavePreviewCodec::Wide(value).c_str(),MAX_PATH,path,NULL);
        if(!n||n>=MAX_PATH)return std::string();
        std::string result=SavePreviewCodec::Utf8(std::wstring(path,n));while(!result.empty()&&(result[result.size()-1]=='/'||result[result.size()-1]=='\\'))result.resize(result.size()-1);
        return result;
    }
    static void Cancel()
    {
        pending.Reset();std::vector<unsigned char>().swap(thumbnail);world=NULL;renderer=NULL;
        if(writer){EnterCriticalSection(&writer->gate);InterlockedExchange(&writer->cancelled,1);LeaveCriticalSection(&writer->gate);}
    }
    static unsigned __stdcall WriteThread(void* arg)
    {
        Writer* job=static_cast<Writer*>(arg);
        try
        {
            std::vector<unsigned char> jpeg;
            if(InterlockedCompareExchange(&job->cancelled,0,0))return 0;
            if(!SavePreviewCodec::Encode(job->pixels,job->header.width,job->header.height,jpeg))
            {job->error="PNG encoding failed";return 0;}
            job->header.encodedBytes=(DWORD)jpeg.size();
            const std::string temp=job->path+"/uitweaks-preview.tmp",dest=job->path+"/uitweaks-preview.bin";
            HANDLE file=CreateFileW(SavePreviewCodec::Wide(temp).c_str(),GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
            if(file==INVALID_HANDLE_VALUE){job->error="cannot create preview temporary file";return 0;}
            DWORD n=0;bool ok=WriteFile(file,&job->header,sizeof(job->header),&n,NULL)&&n==sizeof(job->header)&&
                WriteFile(file,&jpeg[0],(DWORD)jpeg.size(),&n,NULL)&&n==jpeg.size()&&FlushFileBuffers(file);
            CloseHandle(file);
            {
                Writer::Gate guard(*job);
                SavePreviewCodec::Header current={};
                if(ok&&!job->cancelled&&SavePreviewCodec::Stamp(job->path,current)&&SavePreviewCodec::SameRevision(job->header,current))
                {
                    job->success=MoveFileExW(SavePreviewCodec::Wide(temp).c_str(),SavePreviewCodec::Wide(dest).c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=FALSE;
                    if(!job->success)job->error="cannot replace preview file";
                }
                else if(!ok)job->error="cannot write preview file";
            }
            if(!job->success)DeleteFileW(SavePreviewCodec::Wide(temp).c_str());
        }
        catch(...){job->error="preview worker exception";}
        return 0;
    }
    static void ReapWriter()
    {
        if(!writer||WaitForSingleObject(writer->thread,0)!=WAIT_OBJECT_0)return;
        CloseHandle(writer->thread);writer->thread=NULL;
        if(!writer->error.empty())ErrorLog("UITweaks: "+writer->error+" ("+writer->path+")");
        else if(debug&&writer->success)DebugLog("UITweaks: save preview committed: "+writer->path);
        if(writer->success&&writer->path==selectedPath)refreshSelection=true;
        delete writer;writer=NULL;
    }
    static void Submit()
    {
        if(!pending.Ready())return;
        ReapWriter();
        if(writer){ErrorLog("UITweaks: save preview writer is busy; screenshot skipped");pending.Reset();thumbnail.clear();return;}
        std::auto_ptr<Writer> owned(new Writer);Writer* job=owned.get();
        job->path=pending.path;memcpy(job->header.magic,"UITPREV1",8);job->header.version=1;
        job->header.width=thumbW;job->header.height=thumbH;
        if(!SavePreviewCodec::Stamp(job->path,job->header)){Cancel();ErrorLog("UITweaks: saved game revision unavailable for preview");return;}
        job->pixels.swap(thumbnail);pending.Reset();world=NULL;renderer=NULL;
        uintptr_t handle=_beginthreadex(NULL,0,WriteThread,job,0,NULL);
        if(!handle){ErrorLog("UITweaks: cannot start save preview writer");return;}
        job->thread=reinterpret_cast<HANDLE>(handle);writer=owned.release();
    }
    static void ClearTexture()
    {
        if(image.get())image.get()->castType<MyGUI::ImageBox>()->setImageTexture("");
        if(texture){MyGUI::RenderManager* manager=MyGUI::RenderManager::getInstancePtr();if(manager)manager->destroyTexture(texture);texture=NULL;}
    }
    static void Hide()
    {
        if(panel.get())panel.get()->setVisible(false);
        selectedPath.clear();displayed=selectionActive=false;ClearTexture();
    }
    static void Configure(bool value,bool diagnostic,int heightPercent=120)
    {dialogHeightPercent=(std::max)(100,(std::min)(160,heightPercent));debug=diagnostic;value=value&&(!initialized||installed);if(enabled&&!value)Cancel();if(!enabled&&value)refreshSelection=true;enabled=value;if(!enabled)Hide();}
    static void LoadPreview(const std::string& path)
    {
        SavePreviewCodec::Header revision={};
        if(path==selectedPath&&displayed&&SavePreviewCodec::Stamp(path,revision)&&SavePreviewCodec::SameRevision(revision,displayedRevision))return;
        ClearTexture();selectedPath=path;
        displayed=false;
        if(panel.get())panel.get()->setVisible(false);
        if(!image.get())return;
        if(path.empty())return;
        SavePreviewCodec::Header header={};std::vector<unsigned char> rgba;std::string error;
        if(!SavePreviewCodec::Read(path,header,rgba,&error)){if(!error.empty())ErrorLog("UITweaks: "+error+" ("+path+")");return;}
        try
        {
            MyGUI::RenderManager& manager=MyGUI::RenderManager::getInstance();
            texture=manager.createTexture("UITweaks_SavePreview");
            if(!texture){ErrorLog("UITweaks: save preview texture unavailable");return;}
            texture->createManual(header.width,header.height,MyGUI::TextureUsage::Static|MyGUI::TextureUsage::Write,MyGUI::PixelFormat::R8G8B8A8);
            // ITexture::lock exposes no row pitch. Let the native Ogre pixel
            // buffer handle driver padding, including portrait thumbnail widths.
            Ogre::TexturePtr native=Ogre::TextureManager::getSingleton().getByName(texture->getName());
            if(native.isNull()||native->getWidth()!=header.width||native->getHeight()!=header.height)
            {ClearTexture();ErrorLog("UITweaks: native save preview texture identity differs");return;}
            const Ogre::PixelBox pixels(header.width,header.height,1,Ogre::PF_BYTE_RGBA,&rgba[0]);
            native->getBuffer()->blitFromMemory(pixels);
            image.get()->castType<MyGUI::ImageBox>()->setImageTexture(texture->getName());
            displayedRevision=header;displayed=true;
        }
        catch(...){ClearTexture();ErrorLog("UITweaks: cannot create save preview texture");}
    }
    static void EnsurePanel(LoadSaveWindow* menu)
    {
        if(!menu||!menu->mMainWidget)return;
        if(root.get()!=menu->mMainWidget)
        {
            Hide();if(panel.get())MyGUI::Gui::getInstance().destroyWidget(panel.get());
            panel.reset();image.reset();root.set(menu->mMainWidget);
        }
        if(panel.get())return;
        MyGUI::Widget* p=MyGUI::Gui::getInstance().createWidget<MyGUI::Widget>("Kenshi_FloatingPanelSkin",MyGUI::IntCoord(0,0,1,1),MyGUI::Align::Default,"Window","UITweaks_SavePreviewPanel");
        panel.set(p);p->setNeedMouseFocus(false);p->setNeedKeyFocus(false);
        MyGUI::ImageBox* i=p->createWidget<MyGUI::ImageBox>("ImageBox",MyGUI::IntCoord(0,0,1,1),MyGUI::Align::Default);image.set(i);i->setNeedMouseFocus(false);
        p->setVisible(false);
    }
    // Installed native RTTI verifies both derived menu bases are at offset 0.
    static LoadSaveWindow* CurrentMenu()
    {
        SaveManager* sm=SaveManager::getSingleton();if(!sm||!root.get())return NULL;
        LoadSaveWindow* load=reinterpret_cast<LoadSaveWindow*>(sm->loadMenu);
        LoadSaveWindow* save=reinterpret_cast<LoadSaveWindow*>(sm->saveMenu);
        if(load&&load->mMainWidget==root.get())return load;
        if(save&&save->mMainWidget==root.get())return save;
        return NULL;
    }
    static void AnchorMenu(LoadSaveWindow* menu)
    {
        if(!menu||!menu->mMainWidget)return;
        const MyGUI::IntSize view=MyGUI::RenderManager::getInstance().getViewSize();
        const MyGUI::IntCoord r=menu->mMainWidget->getAbsoluteCoord();
        const SavePreviewPolicy::Point anchor=SavePreviewPolicy::AnchorPreviewDialog(r.left,r.top,r.width,r.height,view.width,view.height,enabled);
        if(anchor.y!=r.top)menu->mMainWidget->setPosition(menu->mMainWidget->getLeft(),menu->mMainWidget->getTop()+anchor.y-r.top);
    }
    static void SizeMenu(LoadSaveWindow* menu,bool saveDialog=false)
    {
        if(!menu||!menu->mMainWidget)return;
        MyGUI::Widget* widget=menu->mMainWidget;
        const unsigned slot=saveDialog?1:0;
        SavePreviewPolicy::DialogHeightState& dialogHeight=dialogHeights[slot];
        const MyGUI::IntSize view=MyGUI::RenderManager::getInstance().getViewSize();
        const int height=dialogHeight.Next(widget->getHeight(),sizedRoots[slot].get()==widget,
            dialogHeightPercent,widget->getWidth(),view.width,view.height,enabled);
        sizedRoots[slot].set(widget);
        if(height==widget->getHeight())return;
        // Verified vanilla and Dark layouts: the direct GamesList child uses
        // vertical stretch, so native resize expands its actual row viewport.
        if(!menu->list||menu->list->getParent()!=widget||!menu->list->getAlign().isVStretch())
        {dialogHeight.last=widget->getHeight();ErrorLog("UITweaks: save list cannot safely follow the configured dialog height");return;}
        widget->setSize(widget->getWidth(),height);
    }
    static void Tick()
    {
        if(!OnMain())return;
        ReapWriter();
        if(pending.active&&GetTickCount()-started>60000){Cancel();ErrorLog("UITweaks: save preview timed out");}
        if(pending.active&&(!enabled||ou!=world||!au||au->render!=renderer))Cancel();
        if(!enabled||!root.get()||!root.get()->getVisible()||!root.get()->getInheritedVisible())
        {if(panel.get()&&panel.get()->getVisible())Hide();return;}
        if(!panel.get())return;
        if(!selectionActive){if(panel.get()->getVisible())Hide();return;}
        if(refreshSelection)
        {
            refreshSelection=false;LoadSaveWindow* menu=CurrentMenu();
            if(menu&&menu->mMainWidget==root.get()&&menu->list)
            {
                const size_t index=menu->list->getIndexSelected();
                if(index<menu->games.size())LoadPreview(FullPath(menu->games[index].location+"/"+menu->games[index].name));
            }
        }
        const MyGUI::IntSize view=MyGUI::RenderManager::getInstance().getViewSize();
        const MyGUI::IntCoord r=root.get()->getAbsoluteCoord();
        const SavePreviewPolicy::Placement position=SavePreviewPolicy::PlacePreview(r.left,r.top,r.width,r.height,view.width,view.height,
            texture?texture->getWidth():0,texture?texture->getHeight():0,texture!=NULL);
        const SavePreviewPolicy::Rect& box=position.panel;
        if(box.height<32){panel.get()->setVisible(false);return;}
        const MyGUI::IntCoord panelCoord(box.x,box.y,box.width,box.height);
        if(panel.get()->getCoord()!=panelCoord)panel.get()->setCoord(panelCoord);
        if(!panel.get()->getVisible())panel.get()->setVisible(true);
        if(texture&&image.get())
        {
            const MyGUI::IntCoord imageCoord(position.image.x,position.image.y,position.image.width,position.image.height);
            if(image.get()->getCoord()!=imageCoord)image.get()->setCoord(imageCoord);
        }
        Submit();
    }
    static bool MainTarget(void* target)
    {
        MyGUI::IRenderTarget* main=dynamic_cast<MyGUI::IRenderTarget*>(MyGUI::RenderManager::getInstancePtr());
        return main&&target==main;
    }
    static void RequireCapture(unsigned ticket)
    {
        if(!pending.active||pending.serial!=ticket||!renderer||!renderer->window||ou!=world||!au||au->render!=renderer||au->titleScreen)
            throw std::runtime_error("save preview capture invalidated");
    }
    static void CapturePixels(unsigned ticket)
    {
            RequireCapture(ticket);
            unsigned w=renderer->window->getWidth(),h=renderer->window->getHeight();
            if(!SavePreviewPolicy::AllowedReadback(w,h))throw std::runtime_error("save preview readback exceeds pixel budget");
            std::vector<unsigned char> full(size_t(w)*h*4);
            Ogre::PixelBox box(w,h,1,Ogre::PF_BYTE_BGRA,&full[0]);
            renderer->window->copyContentsToMemory(box,Ogre::RenderTarget::FB_BACK);
            SavePreviewPolicy::Size size=SavePreviewPolicy::Fit(w,h,SavePreviewPolicy::MaxWidth,SavePreviewPolicy::MaxHeight);
            thumbW=size.width;thumbH=size.height;
            if(!SavePreviewCodec::ResizeBgra(full,w,h,thumbW,thumbH,thumbnail))throw std::runtime_error("save preview filtering failed");
    }
    static void SafePreGui(void* target)
    {
        try{if(!inCapture&&OnMain()&&MainTarget(target))Tick();}
        catch(...){Cancel();enabled=false;ErrorLog("UITweaks: save preview interface update failed; previews disabled");}
    }
    static void AddModal(std::vector<MyGUI::Widget*>& roots,MyGUI::Widget* w)
    {
        if(!w||!w->getVisible()||!w->getInheritedVisible())return; // Alpha-independent children can remain visible at root alpha0.
        if(std::find(roots.begin(),roots.end(),w)==roots.end())roots.push_back(w);
        if(roots.size()>8)throw std::runtime_error("save preview modal root budget exceeded");
    }
    static void FindPauseRoot(MyGUI::Widget* w,MyGUI::Widget* top,std::vector<MyGUI::Widget*>& roots,unsigned& visited,unsigned depth)
    {
        if(++visited>4096||depth>64)throw std::runtime_error("save preview modal search budget exceeded");
        const std::string& name=w->getName();static const std::string suffix="MainMenuPopupPanel";
        if(name.size()>=suffix.size()&&name.compare(name.size()-suffix.size(),suffix.size(),suffix)==0)AddModal(roots,top);
        const size_t count=w->getChildCount();for(size_t i=0;i<count;++i)FindPauseRoot(w->getChildAt(i),top,roots,visited,depth+1);
    }
    typedef void (*DrawFunction)(void*,void*,bool);
    static void DrawGui(void* self,void* target,bool update,DrawFunction original)
    {
        if(inCapture||!enabled||!pending.active||pending.captured||!OnMain()||!MainTarget(target)){original(self,target,update);return;}
        if(ou!=world||!au||au->render!=renderer||au->titleScreen||!renderer||!renderer->window){Cancel();original(self,target,update);return;}
        struct CaptureScope{CaptureScope(){inCapture=true;}~CaptureScope(){inCapture=false;}} captureScope;
        const unsigned ticket=pending.serial;
        bool presented=false,normalCompleted=false;
        const char* stage="modal discovery";
        try
        {
            std::vector<MyGUI::Widget*> roots;roots.reserve(8);SaveManager* sm=SaveManager::getSingleton();
            if(sm)
            {
                LoadSaveWindow* menus[]={reinterpret_cast<LoadSaveWindow*>(sm->saveMenu),reinterpret_cast<LoadSaveWindow*>(sm->loadMenu),sm->importMenu};
                for(unsigned i=0;i<3;++i)if(menus[i])AddModal(roots,menus[i]->mMainWidget);
            }
            AddModal(roots,panel.get());unsigned visited=0;
            MyGUI::EnumeratorWidgetPtr widgets=MyGUI::Gui::getInstance().getEnumerator();
            while(widgets.next())if(widgets.current()->getVisible()&&widgets.current()->getInheritedVisible())FindPauseRoot(widgets.current(),widgets.current(),roots,visited,0);
            if(roots.empty())
            {stage="normal GUI";presented=true;original(self,target,update);normalCompleted=true;stage="HUD readback";CapturePixels(ticket);}
            else
            {
                struct ModalRefs
                {
                    struct Entry{PreviewWidgetRef ref;~Entry(){ref.release();}}entries[8];
                    explicit ModalRefs(const std::vector<MyGUI::Widget*>& roots)
                    {for(size_t i=0;i<roots.size();++i)entries[i].ref.set(roots[i]);}
                } liveRoots(roots);
                SavePreviewGpu::SceneBackup backup;ID3D11Texture2D* borrowed=NULL;
                stage="GPU scene backup";
                renderer->window->getCustomAttribute("ID3D11Texture2D",&borrowed);
                if(!backup.Begin(borrowed))throw std::runtime_error("save preview main scene backup refused");
                RequireCapture(ticket);
                AlphaMask mask;
                stage="modal alpha preparation";
                struct ActiveScope
                {
                    AlphaMask* previous;explicit ActiveScope(AlphaMask* m):previous(activeAlpha){activeAlpha=m;}
                    ~ActiveScope(){activeAlpha=previous;}
                };
                {ActiveScope active(&mask);for(size_t i=0;i<roots.size();++i){MyGUI::Widget* w=liveRoots.entries[i].ref.get();if(w)w->setInheritsAlpha(w->getInheritsAlpha());}}
                stage="isolated HUD draw";original(self,target,true); // only modal draw alpha masked
                stage="HUD readback";
                CapturePixels(ticket);
                stage="scene restoration";
                const bool alphaRestored=mask.Restore(),sceneRestored=backup.Restore();
                if(!alphaRestored||!sceneRestored)throw std::runtime_error("save preview scene restoration failed");
                stage="normal GUI";presented=true;original(self,target,true);normalCompleted=true; // no masked frame is presented
            }
            stage="writer submission";RequireCapture(ticket);pending.Captured();if(debug)DebugLog("UITweaks: HUD save preview captured without save/load/pause menus");Submit();
        }
        catch(...)
        {
            const bool cancelled=!pending.active||pending.serial!=ticket;
            if(pending.serial==ticket)Cancel();else thumbnail.clear();
            if(!cancelled)ErrorLog(std::string("UITweaks: HUD save preview failed at ")+stage+"; optional image skipped");
            else if(debug)DebugLog("UITweaks: HUD save preview cancelled");
            if(presented&&!normalCompleted)throw; // preserve genuine normal GUI failures
            if(!presented){presented=true;original(self,target,true);}
        }
    }

    static bool (*originalSave)(SaveFileSystem*,const std::string&)=NULL;
    static void (*originalSync)(SaveFileSystem*)=NULL;
    static void (*originalLoad)(SaveFileSystem*,const std::string&)=NULL;
    static void (*originalNew)(SaveFileSystem*)=NULL;
    static void (*originalShow)(SaveManager*)=NULL;
    static void (*originalShowSave)(SaveManager*)=NULL;
    static void (*originalInfo)(LoadSaveWindow*,size_t)=NULL;
    static void (*originalShutdown)(void*)=NULL;
    static void (*originalGuiShutdown)(ForgottenGUI*)=NULL;
    static bool Save(SaveFileSystem* self,const std::string& path)
    {
        const bool wanted=enabled&&OnMain();
        if(wanted)Cancel(); // A refused/new save must never complete an older pending ticket.
        const bool accepted=originalSave(self,path);
        if(!wanted||!accepted)return accepted;
        try
        {
            const std::string full=FullPath(path);
            if(full.empty()||!ou||!au||!au->render||au->titleScreen)return accepted;
            pending.Begin(full,++serial);world=ou;renderer=au->render;started=GetTickCount();
        }
        catch(...){Cancel();ErrorLog("UITweaks: cannot queue save preview");}
        return accepted;
    }
    static void Sync(SaveFileSystem* self)
    {
        const bool completed=OnMain()&&pending.active&&self->state==SaveFileSystem::COMPLETE;
        const bool success=completed&&self->failedToCopyError.empty();
        originalSync(self);
        try
        {
            if(completed)
            {
                const bool same=success&&_stricmp(FullPath(self->currentSave).c_str(),pending.path.c_str())==0;
                pending.Complete(same);
                if(!same){thumbnail.clear();ErrorLog("UITweaks: save copy failed or changed; preview unchanged");}else Submit();
            }
        }
        catch(...){Cancel();ErrorLog("UITweaks: cannot finalize save preview");}
    }
    static void Load(SaveFileSystem* self,const std::string& path){Cancel();Hide();originalLoad(self,path);}
    static void New(SaveFileSystem* self){Cancel();Hide();originalNew(self);}
    static void ActivateSelection()
    {
        if(!enabled||!root.get()||!saveList.get()||!root.get()->getVisible()||!root.get()->getInheritedVisible())return;
        LoadSaveWindow* menu=CurrentMenu();
        if(!menu||menu->mMainWidget!=root.get()||menu->list!=saveList.get())return;
        const size_t index=menu->list->getIndexSelected();
        if(index>=menu->games.size()){Hide();return;}
        selectionActive=true;const SaveInfo& info=menu->games[index];
        LoadPreview(FullPath(info.location+"/"+info.name));Tick();
    }
    static void ListChanged(MyGUI::MultiListBox*,size_t){try{ActivateSelection();}catch(...){Hide();ErrorLog("UITweaks: save preview selection failed");}}
    static void MouseActivated(MyGUI::ListBox*,size_t){try{ActivateSelection();}catch(...){Hide();ErrorLog("UITweaks: save preview activation failed");}}
    static void ListKey(MyGUI::Widget*,MyGUI::KeyCode key,MyGUI::Char)
    {
        if(key==MyGUI::KeyCode::ArrowUp||key==MyGUI::KeyCode::ArrowDown||key==MyGUI::KeyCode::Home||key==MyGUI::KeyCode::End||
           key==MyGUI::KeyCode::PageUp||key==MyGUI::KeyCode::PageDown)
            try{ActivateSelection();}catch(...){Hide();ErrorLog("UITweaks: save preview keyboard selection failed");}
    }
    static void SubscribeColumns(MyGUI::Widget* node,unsigned& visited,unsigned depth)
    {
        if(++visited>512||depth>32)throw std::runtime_error("save preview list subtree budget exceeded");
        MyGUI::ListBox* list=node->castType<MyGUI::ListBox>(false);
        if(list){list->eventListMouseItemActivate+=MyGUI::newDelegate(&MouseActivated);list->eventKeyButtonPressed+=MyGUI::newDelegate(&ListKey);}
        const size_t count=node->getChildCount();for(size_t i=0;i<count;++i)SubscribeColumns(node->getChildAt(i),visited,depth+1);
    }
    static void Show(SaveManager* self)
    {
        Hide();
        originalShow(self);
        try
        {
            LoadSaveWindow* menu=reinterpret_cast<LoadSaveWindow*>(self->loadMenu);SizeMenu(menu);AnchorMenu(menu);if(!enabled)return;EnsurePanel(menu);
            if(menu&&menu->list)
            {
                saveList.set(menu->list);menu->list->eventListChangePosition+=MyGUI::newDelegate(&ListChanged);
                unsigned visited=0;SubscribeColumns(menu->list,visited,0);
            }
            Tick();
        }
        catch(...){enabled=false;Cancel();ErrorLog("UITweaks: cannot create load-game preview; previews disabled");}
    }
    static void ShowSave(SaveManager* self)
    {
        Hide();originalShowSave(self);
        try
        {
            LoadSaveWindow* menu=reinterpret_cast<LoadSaveWindow*>(self->saveMenu);SizeMenu(menu,true);AnchorMenu(menu);if(!enabled)return;EnsurePanel(menu);
            if(menu&&menu->list)
            {
                saveList.set(menu->list);menu->list->eventListChangePosition+=MyGUI::newDelegate(&ListChanged);
                unsigned visited=0;SubscribeColumns(menu->list,visited,0);
            }
            Tick();
        }
        catch(...){enabled=false;Cancel();ErrorLog("UITweaks: cannot create save-game preview; previews disabled");}
    }
    static void Info(LoadSaveWindow* self,size_t index)
    {
        originalInfo(self,index);
        SaveManager* sm=SaveManager::getSingleton();
        if(!enabled||!selectionActive||!sm||CurrentMenu()!=self)return;
        try
        {
            EnsurePanel(self);
            if(index>=self->games.size()){Hide();return;}
            const SaveInfo& info=self->games[index];
            LoadPreview(FullPath(info.location+"/"+info.name));Tick();
        }
        catch(...){enabled=false;Cancel();ErrorLog("UITweaks: cannot display selected save preview; previews disabled");}
    }
    static void Shutdown(void* self)
    {
        // No MyGUI/GPU access: the platform may already have destroyed its
        // texture wrappers. The engine owns those; this worker owns CPU data only.
        enabled=installed=false;pending.Reset();thumbnail.clear();world=NULL;renderer=NULL;
        if(writer)
        {
            if(StopWriter(*writer,5000)){delete writer;writer=NULL;}
            else ErrorLog("UITweaks: preview writer shutdown timed out; one isolated cancelled job retained until process exit");
        }
        texture=NULL;root.reset();panel.reset();image.reset();saveList.reset();sizedRoots[0].reset();sizedRoots[1].reset();selectionActive=false;
        originalShutdown(self);
    }
    static void GuiShutdown(ForgottenGUI* self)
    {
        Cancel();
        // Native ForgottenGUI::shutDown destroys its MyGUI manager. Release
        // our children, texture and unlink registrations before that happens.
        ClearTexture();
        if(panel.get())MyGUI::Gui::getInstance().destroyWidget(panel.get());
        root.release();panel.release();image.release();saveList.release();sizedRoots[0].release();sizedRoots[1].release();
        selectedPath.clear();displayed=false;
        originalGuiShutdown(self);
    }
    static void Initialize(bool drawingHook)
    {
        // Plugin loading can run before the game window exists. Bind to its
        // real owner thread on the first native UI callback, not the loader.
        mainThread=0;
        initialized=true;
        if(!drawingHook){enabled=false;ErrorLog("UITweaks: interface capture hook unavailable; save previews disabled");return;}
        HMODULE ogre=GetModuleHandleA("OgreMain_x64.dll");
        HMODULE mygui=GetModuleHandleA("MyGUIEngine_x64.dll");
        void* alpha=mygui?reinterpret_cast<void*>(GetProcAddress(mygui,"?_updateAlpha@Widget@MyGUI@@AEAAXXZ")):NULL;
        void* shutdown=ogre?reinterpret_cast<void*>(GetProcAddress(ogre,"?shutdown@Root@Ogre@@QEAAXXZ")):NULL;
        installed=alpha&&KenshiLib::AddHook(alpha,AlphaUpdate,reinterpret_cast<void**>(&originalAlpha))==KenshiLib::SUCCESS&&
            shutdown&&KenshiLib::AddHook(shutdown,Shutdown,reinterpret_cast<void**>(&originalShutdown))==KenshiLib::SUCCESS&&
            KenshiLib::AddHook(KenshiLib::GetRealAddress(&ForgottenGUI::shutDown),GuiShutdown,&originalGuiShutdown)==KenshiLib::SUCCESS&&
            KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveFileSystem::saveGame),Save,&originalSave)==KenshiLib::SUCCESS&&
            KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveFileSystem::sync),Sync,&originalSync)==KenshiLib::SUCCESS&&
            KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveFileSystem::loadGame),Load,&originalLoad)==KenshiLib::SUCCESS&&
            KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveFileSystem::newGame),New,&originalNew)==KenshiLib::SUCCESS&&
            KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveManager::showLoad),Show,&originalShow)==KenshiLib::SUCCESS&&
            KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveManager::showSave),ShowSave,&originalShowSave)==KenshiLib::SUCCESS&&
            KenshiLib::AddHook(KenshiLib::GetRealAddress(&LoadSaveWindow::updateInfo),Info,&originalInfo)==KenshiLib::SUCCESS;
        if(!installed){enabled=false;ErrorLog("UITweaks: save preview hooks unavailable; previews disabled");}
    }
}
