#include "GsmFinePrefetch.h"
#include "GsmFinePump.h"
#include "GsmFineShadeCursor.h"
// Included inside the viewer's private namespace, after its coarse renderer.
// GPU work is admitted ONLY by the checked native Main workspace callback.
typedef WorldNativeRebind::Session<WorldNative::StageAdapter<WorldNative::NativeStage> > FineNativeSession;
typedef FineCoordinator::Controller<FineNativeSession> FineController;
struct FineEntry {Entry image;FineWindows::Key key;bool valid,opaque;FineWindows::U64 touch;FineEntry():valid(false),opaque(false),touch(0){std::memset(&key,0,sizeof(key));}};
FineEntry fineCache[FineJob::CacheSlots];
WidgetRef fineTiles[GsmFineWidgets::MaxWidgets];GsmFineWidgets::Slots fineWidgetSlots;
GsmFineIndex::Index fineIndex;
unsigned fineVisible[GsmFineIndex::MaxVisibleIndices],fineVisibleCount=0;
FineWindows::U64 fineVisibleDemand=0,fineVisibleIndex=0;
// One serialized render-thread workspace, rather than several large automatic
// arrays on the native1MiB stack. Used only on demand/pressure changes.
struct FineCacheScratch {FineWindows::Key keys[FineJob::CacheSlots];bool valid[FineJob::CacheSlots];FineWindows::U64 touch[FineJob::CacheSlots];}fineScratch;
FineController*fineOwner=NULL;FineStore::Identity fineIdentity={};FineJob::Demand fineDemand={};FineJob::Plan finePlan;
DWORD fineStatsAt=0,fineChangedAt=0;unsigned fineRetires=0,fineAhead=0,fineAheadRead=0;double fineMotionX=0,fineMotionZ=0;bool fineSpeculative=false,fineAheadMiss=false;FineJob::Plan fineAheadPlan;
MapScreen*fineMap=NULL;WidgetRef fineMapWidget;DWORD fineThread=0;unsigned finePair=0,fineRead=0,finePhase=0;bool fineMiss=false,fineBlocked=false,fineClosing=false,fineCenter=true;
__declspec(align(8)) volatile LONGLONG fineEpoch=0;FineWindows::U64 fineUseClock=0;
bool fineDeferPaint=false;GsmFineShadeCursor::State fineShadeCursor;
// BEGIN FINE PRESENT CACHE
// Copied presentation metadata only; no cached pointer is ever dereferenced.
// One rectangle per existing bounded widget pool, not per retained GPU tile.
struct FinePresentationCache {
 const void*map,*main,*image,*scroll,*owner;FineWindows::U64 demand;MyGUI::IntCoord canvas,clip,rect[GsmFineWidgets::MaxWidgets];bool current,active;
 FinePresentationCache():map(NULL),main(NULL),image(NULL),scroll(NULL),owner(NULL),demand(0),current(false),active(false){}
 void Reset(){current=false;}
 bool Same(MapScreen*m,MyGUI::Widget*o,FineWindows::U64 revision,const MyGUI::IntCoord&c,const MyGUI::IntCoord&v)const{return current&&map==m&&main==m->mainWidget&&image==m->mapImage&&scroll==m->mapScrollView&&owner==o&&demand==revision&&canvas==c&&clip==v;}
 void Remember(MapScreen*m,MyGUI::Widget*o,FineWindows::U64 revision,const MyGUI::IntCoord&c,const MyGUI::IntCoord&v){map=m;main=m->mainWidget;image=m->mapImage;scroll=m->mapScrollView;owner=o;demand=revision;canvas=c;clip=v;current=true;}
}finePresentation;
struct FinePresentationPass {FinePresentationCache&state;explicit FinePresentationPass(FinePresentationCache&s):state(s){state.active=true;}~FinePresentationPass(){state.active=false;}};
// END FINE PRESENT CACHE
bool FinePending(){return fineOwner!=NULL;}
bool FineQualified(){return ready&&manifest.fineVerified&&!fineBlocked&&!GetModuleHandleW(L"SatelliteCaptureProbe.dll")&&!GetModuleHandleW(L"PrivateSatelliteCaptureProbe.dll");}
// Coarse zoom restores ordinary base LOD after the active producer is drained.
bool FineDetailFeasible(){return FineQualified()&&FineJob::Internal::DemandValid(fineDemand)&&GsmFinePolicy::CanRefine(fineDemand.view);}
bool FineReservesBaseDetail(){return ProducerGpuBytes()!=0||(FineDetailFeasible()&&(fineOwner||FineGpuBytes()!=0));}
AtlasView::U64 FineGpuBytes(){return FineWindows::U64(fineIndex.Count())*GsmFineGpu::WindowBytes;}
AtlasView::U64 ProducerGpuBytes(){return fineOwner?fineOwner->ProducerGpuBytes():0;}
void FineDetachWidget(unsigned slot){int pool=fineWidgetSlots.Find(slot);if(pool<0)return;MyGUI::ImageBox*w=static_cast<MyGUI::ImageBox*>(fineTiles[pool].get());if(w){if(!w->_getTextureName().empty())w->setImageTexture("");if(w->getVisible())w->setVisible(false);}fineWidgetSlots.Unbind(slot);}
void FineDrop(unsigned n){FineEntry&f=fineCache[n];if(!f.valid)return;Entry&e=f.image;ID3D11Texture2D*planes[3]={e.gpu.texture[0].p,e.gpu.texture[1].p,e.gpu.texture[2].p};if(!gpuRetired.Add(planes,e.output.p,GsmFineGpu::WindowBytes)){gpuRetirementFailed=true;return;}
 FineDetachWidget(n);MyGUI::RenderManager*rm=MyGUI::RenderManager::getInstancePtr();if(rm&&!e.name.empty()){MyGUI::ITexture*t=rm->getTexture(e.name);if(t)rm->destroyTexture(t);}RetireReads(e.output.p);e.output.Reset();e.gpu.Clear();e.texture.setNull();Ogre::TextureManager*tm=Ogre::TextureManager::getSingletonPtr();if(tm&&!e.name.empty()&&!tm->getByName(e.name).isNull())tm->remove(e.name);e.name.clear();e.shaded=0;fineIndex.Erase(f.key,n);f.valid=false;
}
bool FineRetire(unsigned n){if(fineRetires>=2)return false;bool was=fineCache[n].valid;FineDrop(n);if(was&&!fineCache[n].valid)++fineRetires;return !fineCache[n].valid;}
void FineCancel(){finePresentation.Reset();if(!seamReadback.Failed()){RetireGpuCharge(seamReadback.GpuBytes());seamReadback.Clear();seamBlob.Clear();}if(fineOwner)fineOwner->Cancel();for(unsigned n=0;n<FineJob::ActiveSlots();++n){FineDrop(n);}for(unsigned p=0;p<GsmFineWidgets::MaxWidgets;++p)fineTiles[p].reset();fineWidgetSlots.Clear();fineMap=NULL;fineMapWidget.reset();finePhase=0;finePlan=FineJob::Plan();fineSpeculative=false;fineAhead=0;fineAheadPlan=FineJob::Plan();fineMotionX=fineMotionZ=0;}
int FineFind(const FineWindows::Key&key){return fineIndex.Find(key);}
// Reserve requested layers before refining. Their user settings stay unchanged
// when the remaining budget cannot admit the source bank.
AtlasView::U64 FineLayerReserve(){AtlasView::U64 bytes=ViewerGpuBytes();if((waterOption||localOption||modelOption)&&!waterPipelineReady)bytes+=AtlasWater::PipelinePrivateBytes;if(modelOption&&!loadedGpuReady)bytes+=LoadedGpuBytes;if(localOption&&!localLayerReady)bytes+=AtlasLocalLights::PrivateBytes;if(modelOption&&!modelGpuReady)bytes+=ModelGpuBytes;if(modelOption&&cachedTown.OwnedGpuBytes()<3*PrivateTownLod::MaxBytes)bytes+=3*PrivateTownLod::MaxBytes-cachedTown.OwnedGpuBytes();return bytes;}
FineCoordinator::Residency FineResidency(){FineCoordinator::Residency r;r.viewerGpu=FineLayerReserve();r.fineGpu=FineGpuBytes();r.retiredGpu=gpuRetired.Bytes();r.occupied=fineIndex.Count();return r;}
FineStore::Ledger FineCpuLedger(){FineStore::Ledger l;l.other=sizeof(finePresentation)+sizeof(cpuTiming)+seamReadback.CpuBytes()+seamBlob.OwnedBytes()+sizeof(fineShadeCursor)+sizeof(cache)+sizeof(fineCache)+sizeof(gpuRetired)+sizeof(models)+sizeof(finePlan)+sizeof(fineAheadPlan)+sizeof(manifest)+fineIndex.MetadataBytes()+sizeof(fineTiles)+sizeof(fineWidgetSlots)+sizeof(fineScratch)+sizeof(fineVisible)+manifest.windows.records.capacity()+cachedTown.CpuPayloadBytes()+sizeof(localDiagnostic)+sizeof(localRevision)+sizeof(nativeLight)+sizeof(relighter)+512*1024+(modelOption?3ULL*1024*1024:0);
 GsmAtlas::Job*jobs[2]={job,completed};for(unsigned n=0;n<2;++n)if(jobs[n]){GsmAtlas::Job&j=*jobs[n];l.other+=sizeof(j)+j.manifest.windows.records.capacity()+j.cachedTownResult.bytes.capacity();for(unsigned p=0;p<3;++p)l.other+=j.read.plane[p].capacity();}return l;}
bool FineGeometry(MapScreen*map,FineJob::Demand&next){if(!map||!map->mapImage||!map->mapScrollView||!FineStore::Valid(fineIdentity))return false;GSM::Rect cells[16];if(!Geometry(map,cells)||cells[0].w<=0||cells[0].h<=0)return false;MyGUI::IntCoord v=map->mapScrollView->getAbsoluteCoord();MyGUI::IntPoint origin=map->mapImage->getAbsolutePosition();if(v.width<=0||v.height<=0||v.width>8192||v.height>8192)return false;
 next=fineDemand;next.scope.thread=fineThread;next.scope.world=reinterpret_cast<FineStore::U64>(ou);next.scope.sourceEpoch=fineIdentity.sourceEpoch;next.scope.job=fineIdentity.sourceEpoch;next.view.stamp.source=fineIdentity.sourceEpoch;next.view.stamp.material=fineIdentity.materialGeneration;next.view.apron=false;next.view.screenWidth=v.width;next.view.screenHeight=v.height;
 next.view.world.x=-147456.0+(v.left-origin.left-cells[0].x)*73728.0/cells[0].w;next.view.world.z=-147456.0+(v.top-origin.top-cells[0].y)*73728.0/cells[0].h;next.view.world.w=v.width*73728.0/cells[0].w;next.view.world.h=v.height*73728.0/cells[0].h;
 if(!next.revision)next.revision=1;return FineJob::Internal::DemandValid(next);}
bool FineContext(){return fineThread==GetCurrentThreadId()&&fineMap&&fineMapWidget.get()&&fineMapWidget.get()==fineMap->mainWidget&&fineMap->getVisible()&&enabled&&ready&&FineQualified()&&ou&&observedGeneration==InterlockedCompareExchange(&GSM::generation,0,0)&&fineDemand.scope.world==reinterpret_cast<FineStore::U64>(ou);}
bool FineOwnerContext(){return fineThread==GetCurrentThreadId()&&enabled&&ready&&FineQualified()&&ou&&observedGeneration==InterlockedCompareExchange(&GSM::generation,0,0)&&fineDemand.scope.world==reinterpret_cast<FineStore::U64>(ou);}
void FineVisibility(){
 if(fineVisibleDemand==fineDemand.revision&&fineVisibleIndex==fineIndex.Revision())return;
 unsigned next[GsmFineIndex::MaxVisibleIndices];const unsigned count=fineIndex.Visible(fineDemand.view,next,GsmFineIndex::MaxVisibleIndices);
 // One bounded reusable widget pool, independent of thousands of retained GPU
 // textures. Clear the old binding before assigning a different cache image.
 for(unsigned p=0;p<GsmFineWidgets::MaxWidgets;++p){const int old=fineWidgetSlots.Owner(p);if(old<0)continue;bool keep=false;for(unsigned i=0;i<count;++i)if(next[i]==unsigned(old)){keep=true;break;}if(!keep)FineDetachWidget(unsigned(old));}
 fineVisibleCount=count;std::memcpy(fineVisible,next,count*sizeof(unsigned));
 fineVisibleDemand=fineDemand.revision;fineVisibleIndex=fineIndex.Revision();
}
void FinePresent(MapScreen*map){if(fineDeferPaint)return;
 if(finePresentation.active)return;GsmCpuTiming::Scope timing(debug,cpuTiming,GsmCpuTiming::FinePresent);FinePresentationPass pass(finePresentation);
 MyGUI::Widget*owner=layer.get();if(!map||!map->mapImage||!map->mapScrollView||!owner||owner->getParent()!=map->mapImage){finePresentation.Reset();return;}
 const bool contextCurrent=map==fineMap&&FineContext();FineVisibility();
 if(!contextCurrent){finePresentation.Reset();for(unsigned at=0;at<fineVisibleCount;++at){const int pool=fineWidgetSlots.Find(fineVisible[at]);if(pool<0)continue;MyGUI::ImageBox*w=static_cast<MyGUI::ImageBox*>(fineTiles[pool].get());if(w&&w->getVisible())w->setVisible(false);}return;}
 const MyGUI::IntCoord canvas=map->mapImage->getCoord(),clip=map->mapScrollView->getAbsoluteCoord();const bool geometryChanged=!finePresentation.Same(map,owner,fineDemand.revision,canvas,clip);
 for(unsigned at=0;at<fineVisibleCount;++at){const unsigned n=fineVisible[at];FineEntry&f=fineCache[n];if(!f.valid)continue;const int previous=fineWidgetSlots.Find(n);const bool rebound=previous<0;const int pool=rebound?fineWidgetSlots.Bind(n):previous;if(pool<0)continue;bool created=false;
  if(!fineTiles[pool].get()){MyGUI::ImageBox*w=owner->createWidget<MyGUI::ImageBox>("ImageBox",MyGUI::IntCoord(),MyGUI::Align::Default,"");fineTiles[pool].set(w);w->setNeedMouseFocus(false);w->setNeedKeyFocus(false);created=true;}
  MyGUI::ImageBox*w=static_cast<MyGUI::ImageBox*>(fineTiles[pool].get());if(!w)continue;FineWindows::Rect bounds;const bool show=GsmFinePolicy::Intersects(f.key,fineDemand.view)&&FineWindows::Bounds(f.key,bounds)&&f.image.shaded;
  if(show){const bool textureChanged=w->_getTextureName()!=f.image.name;
   if(geometryChanged||rebound||created||!w->getVisible()||textureChanged){MyGUI::IntPoint a=map->worldToMapCoords(Ogre::Vector3(float(bounds.x),0,float(bounds.z))),b=map->worldToMapCoords(Ogre::Vector3(float(bounds.x+bounds.w),0,float(bounds.z+bounds.h)));finePresentation.rect[pool]=MyGUI::IntCoord(a.left,a.top,b.left-a.left,b.top-a.top);}
   if(w->getCoord()!=finePresentation.rect[pool])w->setCoord(finePresentation.rect[pool]);const int depth=f.key.spacing==1?-3:-2;if(w->getDepth()!=depth)w->setDepth(depth);if(textureChanged)w->setImageTexture(f.image.name);
  }if(w->getVisible()!=show)w->setVisible(show);
 }finePresentation.Remember(map,owner,fineDemand.revision,canvas,clip);
}
bool FineUpload(const FineStore::View&v){if(FineFind(v.key)>=0)return true;if(!AdmitGpu(GsmFineGpu::WindowBytes))return false;unsigned slot=0;while(slot<FineJob::ActiveSlots()&&fineCache[slot].valid)++slot;if(slot==FineJob::ActiveSlots())return false;FineEntry&f=fineCache[slot];if(!fineIndex.Insert(v.key,slot))return false;f.valid=true;f.key=v.key;f.touch=++fineUseClock;Entry&e=f.image;e.hasHeight=false;f.opaque=true;for(unsigned i=0;i<256*256;++i){if(v.normal[i*4+3]!=255)f.opaque=false;if(_finite(v.height[i])){if(!e.hasHeight||v.height[i]<e.minHeight)e.minHeight=v.height[i];e.hasHeight=true;}}
 try{HRESULT hr=GsmFineGpu::Upload(nativeLight.device.p,v.material,v.normal,v.height,e.gpu);if(FAILED(hr))throw std::runtime_error("fine immutable upload refused");if(!groupCreated){Ogre::ResourceGroupManager::getSingleton().createResourceGroup(Group);groupCreated=true;}
  std::vector<BYTE>blank(256*256*4,0);Ogre::Image image;image.loadDynamicImage(&blank[0],256,256,1,Ogre::PF_A8B8G8R8,false,1,0);std::ostringstream name;name<<"GlobalSatelliteMap_Fine_"<<observedGeneration<<'_'<<++serial;e.name=name.str();e.texture=Ogre::TextureManager::getSingleton().loadImage(e.name,Group,image,Ogre::TEX_TYPE_2D,0);GsmNative::CompleteManagedImage(e.texture.get());ID3D11ShaderResourceView*srv=GsmNative::Srv(e.texture.get());AtlasView::Com<ID3D11Resource>resource;srv->GetResource(&resource.p);if(!AtlasView::SameDevice(srv,nativeLight.device.p)||FAILED(resource.p->QueryInterface(__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&e.output.p))))throw std::runtime_error("fine output identity refused");return true;
 }catch(...){FineDrop(slot);throw;}}
struct FineOpaqueReady {bool operator()(const FineWindows::Key&key)const{const int n=FineFind(key);if(n<0||!fineCache[n].valid||!fineCache[n].opaque||!fineCache[n].image.shaded)return false;const int pool=fineWidgetSlots.Find(unsigned(n));if(pool<0)return false;MyGUI::ImageBox*w=static_cast<MyGUI::ImageBox*>(fineTiles[pool].get());if(!w||!w->getVisible()||w->_getTextureName()!=fineCache[n].image.name||w->getAlpha()!=1.f)return false;FineWindows::Rect bounds;if(!FineWindows::Bounds(key,bounds))return false;MyGUI::IntPoint a=fineMap->worldToMapCoords(Ogre::Vector3(float(bounds.x),0,float(bounds.z))),z=fineMap->worldToMapCoords(Ogre::Vector3(float(bounds.x+bounds.w),0,float(bounds.z+bounds.h)));return w->getCoord()==MyGUI::IntCoord(a.left,a.top,z.left-a.left,z.top-a.top);}};
bool FineCovers(MapScreen*map){GsmCpuTiming::Scope timing(debug,cpuTiming,GsmCpuTiming::FineCoverage);if(!map||map!=fineMap||!FineContext())return false;FineJob::Demand current={};if(!FineGeometry(map,current))return false;return GsmFineCoverage::Covered(finePlan,current,true,true,fineCenter,finePhase,FineOpaqueReady());}
void FineFail(const char*why);
bool FineWantsLocal(Entry&e,const AtlasView::Uniforms&u){const NativeLocalLights::NativeSnapshot&s=localDiagnostic.Values();if(!localOption||localFailed||!localMapping.Matches(s,observedGeneration)||!s.Count())return false;if(e.localChecked==localVersion)return e.localAffects;AtlasLocalLights::Region region=LocalRegion(u);e.localAffects=false;for(unsigned i=0;i<s.Count();++i){AtlasLocalLights::GpuLight light;if(AtlasLocalLights::Pack(*s.Light(i),localMapping,light)&&AtlasLocalLights::Intersects(light,region,256,256)){e.localAffects=true;break;}}e.localChecked=localVersion;return e.localAffects;}
void FineShade(){if(fineDeferPaint||!haveLighting)return;FineVisibility();unsigned done=0;fineShadeCursor.Cursor(fineVisibleCount);for(unsigned pass=0;pass<2&&done<2;++pass)for(unsigned scan=0;scan<fineVisibleCount&&done<2;++scan){const unsigned at=fineShadeCursor.Index(scan);if(at>=fineVisibleCount)break;const unsigned n=fineVisible[at];FineEntry&f=fineCache[n];if(!f.valid||(pass==0?f.image.shaded!=0:f.image.shaded==0)||!GsmFinePolicy::Intersects(f.key,fineDemand.view))continue;FineWindows::Rect b;if(!FineWindows::Bounds(f.key,b))continue;AtlasView::Uniforms u=lighting;u.referenceSide[3]=256;u.tileBounds[0]=float(b.x);u.tileBounds[1]=float(b.z);u.tileBounds[2]=float(b.w);u.tileBounds[3]=float(b.h);Entry&e=f.image;bool useWater=WantsWater(e),useLocal=FineWantsLocal(e,u);bool dirty=e.waterApplied!=useWater||(useWater&&e.waterStamp!=waterVersion)||e.localApplied!=useLocal||(useLocal&&e.localStamp!=localVersion)||(ModelsAvailable()?e.modelStamp!=models.Revision():e.modelApplied);if(e.shaded==lightVersion&&!dirty)continue;
 bool modelsApplied=false;HRESULT hr;{GsmCpuTiming::Scope timing(debug,cpuTiming,GsmCpuTiming::FineModelPrepare);hr=PrepareModelCell(e,u,modelsApplied);}if(FAILED(hr)){FineFail("fine model composition refused");return;}const AtlasView::Cell&cell=modelsApplied?modelCell:e.gpu;unsigned refusal=0;bool detailed=LoadedTouches(u);{GsmCpuTiming::Scope timing(debug,cpuTiming,GsmCpuTiming::FineLighting);hr=(useWater||useLocal||detailed)?RenderLayers(e,cell,u,useWater,useLocal):relighter.Render(cell,u,nativeLight.views,nativeLight.samplers,e.output.p,&refusal);if(FAILED(hr)&&detailed){LoadedFailure("fine HDR composition refused");detailed=false;hr=(useWater||useLocal)?RenderLayers(e,cell,u,useWater,useLocal):relighter.Render(cell,u,nativeLight.views,nativeLight.samplers,e.output.p,&refusal);}}if(FAILED(hr)){FineFail("fine illumination composition refused");return;}e.shaded=lightVersion;e.waterApplied=useWater;e.waterStamp=useWater?waterVersion:0;e.localApplied=useLocal;e.localStamp=useLocal?localVersion:0;e.modelApplied=modelsApplied||detailed;e.modelStamp=ModelsAvailable()?models.Revision():0;fineShadeCursor.Published(at);++done;++fineShadeCount;}}
void FineFail(const char*why){bool first=!fineBlocked;fineBlocked=true;if(fineOwner)fineOwner->Cancel();if(first)ErrorLog(std::string("GlobalSatelliteMap: fine refinement stopped; coarse view retained: ")+why);}
bool FineQueueRefused(const char*why,bool required){if(required)FineFail(why);else Diagnostic(std::string("fine cache-ahead skipped: ")+why);return false;}
bool FineQueue(const FineJob::Plan&plan,unsigned pair,bool required=true){
  NativeRegionMaterial::Snapshot material={};unsigned region=plan.pairs[pair].region;const std::string nativeRoot=GsmFinePolicy::NativeRoot(verifiedGameRoot);
  if(!NativeRegionMaterial::Capture(fineDemand.scope,region,nativeRoot.c_str(),fineIdentity.materialGeneration,false,material)){NativeRegionMaterial::Internal::NativeAccess access(fineDemand.scope);std::ostringstream why;why<<"regional capture refused region="<<region<<" root="<<NativeRegionMaterial::Internal::NativeRoot(nativeRoot.c_str())<<" engine="<<NativeRegionMaterial::Internal::EngineProfile()<<" scope="<<access.Current(fineDemand.scope)<<" world="<<fineDemand.scope.world<<" source="<<fineDemand.scope.sourceEpoch<<" thread="<<fineDemand.scope.thread<<" rootPath="<<nativeRoot;return FineQueueRefused(why.str().c_str(),required);}
  if(std::memcmp(material.parameterSha,manifest.regionParameters[region],32)){std::ostringstream why;why<<"regional parameter SHA mismatch region="<<region;return FineQueueRefused(why.str().c_str(),required);}
  if(std::memcmp(material.distortionSha,manifest.regionDistortion[region],32)){std::ostringstream why;why<<"regional distortion SHA mismatch region="<<region;return FineQueueRefused(why.str().c_str(),required);}
  FineJob::Snapshot snapshot={fineDemand.view.stamp,material.material};FineJob::Settings settings={16000,40000,0,0};WorldRaster::Shaders shaders={{CompactMaterial::VertexShader,sizeof(CompactMaterial::VertexShader)},{StagedMaterial::PartialShader,sizeof(StagedMaterial::PartialShader)},{StagedMaterial::TailShader,sizeof(StagedMaterial::TailShader)}};WorldCohort::Job nativeJob={};unsigned pairSpan=1;FineJob::Budget batchBudget;if(required&&!fineCenter&&FineJob::CanBatchNext(plan,pair)&&FineFind(plan.pairs[pair+1].key[0])<0&&fineOwner->MakeBudget(FineResidency(),FineCpuLedger(),batchBudget)&&FineJob::CanRender(plan,fineDemand,batchBudget,4))pairSpan=2;std::wstring height=GsmAtlas::Wide(verifiedGameRoot+"data/newland/land/fullmap.tif");if(!FineJob::MakeJob(plan,pair,fineDemand,snapshot,settings,shaders,height.c_str(),nativeJob,pairSpan))return FineQueueRefused("native fine placement refused",required);if(!fineOwner->Queue(plan,pair,fineDemand,fineIdentity,nativeJob,FineResidency(),FineCpuLedger())){if(fineOwner->RetryPending())return false;return FineQueueRefused("native fine source admission refused",required);}return true;
}
void FineFinishOwner(HRESULT completion){
 if(!fineOwner||fineOwner->Active())return;
 const HRESULT operation=fineOwner->Error();delete fineOwner;fineOwner=NULL;fineClosing=false;fineIdentity=FineStore::Identity();fineDemand=FineJob::Demand();finePlan=FineJob::Plan();finePhase=0;
 const HRESULT failure=FAILED(operation)?operation:completion;
 if(FAILED(failure)&&failure!=E_ABORT&&failure!=HRESULT_FROM_WIN32(ERROR_CANCELLED)){std::ostringstream why;why<<"fine source operation stopped hr="<<std::hex<<failure;FineFail(why.str().c_str());}
}
void FineRenderTick(){
 if(!gpuRetirementDevice.p&&!fineOwner)return;HRESULT retirement=gpuRetirementDevice.p?gpuRetired.Tick(gpuRetirementDevice.p):S_OK;if(FAILED(retirement))FineFail("GPU retirement completion unknown");
 bool admitted=FineContext();if(debug&&GetTickCount()-fineStatsAt>=5000){fineStatsAt=GetTickCount();std::ostringstream row;row<<"fine state admitted="<<admitted<<" phase="<<finePhase<<" center="<<fineCenter<<" revision="<<fineDemand.revision<<" spacing="<<finePlan.spacing<<" pair="<<finePair<<'/'<<finePlan.pairCount<<" read="<<fineRead<<" ahead="<<fineAhead<<" speculative="<<fineSpeculative<<" sourceGpu="<<ProducerGpuBytes()<<" viewerGpu="<<ViewerGpuBytes()<<" fineGpu="<<FineGpuBytes()<<" retiredGpu="<<gpuRetired.Bytes()<<" externalCpu="<<FineCpuLedger().other<<" stored="<<(fineOwner?fineOwner->StoredWindows():0)<<" heightAuthentications="<<(fineOwner?fineOwner->HeightAuthentications():0)<<" gpuCap="<<GsmCacheProfile::GpuBudgetBytes()<<" ramCap="<<GsmCacheProfile::RamBudgetBytes()<<" fineSlots="<<FineJob::ActiveSlots();if(fineOwner){FineWrite::RamStats memory=fineOwner->MemoryStats();row<<" ramBytes="<<memory.bytes<<" ramEntries="<<memory.entries<<" ramHits="<<memory.hits<<" ramMisses="<<memory.misses;}Diagnostic(row.str());}if(fineOwner){int result;{GsmCpuTiming::Scope timing(debug,cpuTiming,GsmCpuTiming::FineSourceTick);result=fineOwner->Tick(FineOwnerContext(),nativeLight.device.p,fineDemand,fineIdentity,FineResidency(),FineCpuLedger());}if(fineOwner->Quarantined()){std::ostringstream why;why<<"fine owner quarantine hr="<<std::hex<<fineOwner->Error();FineFail(why.str().c_str());return;}if(fineOwner->Terminated()){if(!fineClosing){HRESULT hr=fineOwner->RequestShutdown();if(FAILED(hr)){std::ostringstream why;why<<"fine source shutdown request hr="<<std::hex<<hr;FineFail(why.str().c_str());return;}fineClosing=true;}HRESULT hr=fineOwner->PollShutdown();if(hr==S_OK||(FAILED(hr)&&!fineOwner->Active())){FineFinishOwner(hr);}else if(FAILED(hr)){std::ostringstream why;why<<"fine source shutdown completion hr="<<std::hex<<hr;FineFail(why.str().c_str());}return;}if(!admitted){if(result==1){fineOwner->Drain();finePhase=2;}else if(result==2){if(fineSpeculative){fineSpeculative=false;++fineAhead;}finePhase=3;fineRead=0;}if(fineOwner->ReadReady())fineOwner->ReleaseRead();return;}
  if(result==1){fineOwner->Drain();finePhase=2;}else if(result==2){if(fineSpeculative){fineSpeculative=false;++fineAhead;}finePhase=3;fineRead=0;}else if(result==3||fineOwner->ReadReady()){if(fineSpeculative){fineOwner->ReleaseRead();++fineAheadRead;}else{FineStore::View view={};if(fineOwner->PeekRead(true,fineDemand,fineIdentity,FineResidency(),FineCpuLedger(),view)){try{if(!FineUpload(view)){fineOwner->ReleaseRead();return;}fineOwner->ReleaseRead();++fineRead;}catch(...){fineOwner->ReleaseRead();throw;}}}}else if(result==4){if(fineSpeculative){fineAheadMiss=true;++fineAheadRead;}else{fineMiss=true;++fineRead;}}
 }
 if(!admitted)return;
 if(!fineOwner){FineCoordinator::Residency initial=FineResidency();if(!FineDetailFeasible()||!FineCoordinator::StartupFits(initial))return;SourceReceipts::Scope scope=fineDemand.scope;FineStore::Config config;if(!GsmFinePolicy::CacheConfig(GsmAtlas::Wide(Folder()),GetCurrentProcessId(),fineIdentity.sourceEpoch,config)){FineFail("cache directory configuration refused");return;}fineOwner=new FineController;if(!fineOwner->Begin(scope,fineIdentity,config,FineCpuLedger())){if(!fineOwner->Active()){delete fineOwner;fineOwner=NULL;}FineFail("fine owner startup refused");return;}return;}
 if(!fineOwner->Ready()){FineShade();FinePresent(fineMap);return;}
 FineJob::Budget budget;if(!fineOwner->MakeBudget(FineResidency(),FineCpuLedger(),budget)){FineShade();FinePresent(fineMap);return;}
 if(finePlan.state!=FineJob::Detail||!FineJob::Internal::SameDemand(finePlan.demand,fineDemand)){FineWindows::Key*resident=fineScratch.keys;unsigned occupied=0;for(unsigned n=0;n<FineJob::ActiveSlots();++n)if(fineCache[n].valid)resident[occupied++]=fineCache[n].key;finePlan=FineJob::MakePlan(fineDemand,budget,fineCenter?FineJob::CenterPair:FineJob::VisibleSet,resident,occupied);finePair=fineRead=0;finePhase=3;fineMiss=false;if(finePlan.state!=FineJob::Detail){if(finePlan.state==FineJob::Coarse){FineWindows::Key*held=fineScratch.keys;bool*valid=fineScratch.valid;FineWindows::U64*touches=fineScratch.touch;for(unsigned n=0;n<FineJob::ActiveSlots();++n){held[n]=fineCache[n].key;valid[n]=fineCache[n].valid;touches[n]=fineCache[n].touch;}int victim=GsmFinePolicy::PlanningVictim(held,valid,FineJob::ActiveSlots(),fineDemand.view,touches);if(victim>=0)FineRetire(unsigned(victim));}FineShade();FinePresent(fineMap);return;}std::ostringstream row;row<<"fine plan spacing="<<finePlan.spacing<<" count="<<finePlan.count<<" pairs="<<finePlan.pairCount<<" center="<<fineCenter<<" cpuPeak="<<finePlan.cpuPeak<<" producerPeak="<<finePlan.producerPeak;Diagnostic(row.str());}
 // Bounded cache-ahead only after every visible pair was produced. It never
 // evicts a visible GPU image or delays publication when actual headroom fails.
 if(fineSpeculative){
  if(fineAheadRead<2){if(fineOwner->RequestRead(fineAheadPlan,fineAheadPlan.keys[fineAheadRead],fineDemand,fineIdentity,FineResidency(),FineCpuLedger())){FineShade();FinePresent(fineMap);return;}}
  else if(fineAheadMiss){if(FineJob::CanRender(fineAheadPlan,fineDemand,budget)){if(!FineQueue(fineAheadPlan,0,false)){if(fineOwner->RetryPending())return;fineSpeculative=false;fineAhead=GsmFinePrefetch::PairLimit();finePhase=3;fineRead=0;FineShade();FinePresent(fineMap);return;}finePhase=2;FineShade();FinePresent(fineMap);return;}fineSpeculative=false;fineAhead=GsmFinePrefetch::PairLimit();}
  else{fineSpeculative=false;++fineAhead;}
  FineShade();FinePresent(fineMap);return;
 }
 const bool produced=finePair>=finePlan.pairCount&&((finePhase==3&&fineRead==0)||finePhase==4);
 if(GsmFinePrefetch::Eligible(GetTickCount(),fineChangedAt,produced,fineCenter,fineAhead,fineOwner->Ready(),finePlan,fineDemand,budget,fineMotionX,fineMotionZ)){
  fineAheadPlan=GsmFinePrefetch::Make(finePlan,fineDemand,budget,fineMotionX,fineMotionZ,fineAhead);fineAheadRead=0;fineAheadMiss=false;fineSpeculative=true;FineShade();FinePresent(fineMap);return;
 }
 if(finePhase==3){unsigned count=finePair<finePlan.pairCount?2:finePlan.count;if(fineRead<count){FineWindows::Key key=finePair<finePlan.pairCount?finePlan.pairs[finePair].key[fineRead]:finePlan.keys[fineRead];bool wanted=finePair>=finePlan.pairCount||finePlan.pairs[finePair].wanted[fineRead];if(!wanted||FineFind(key)>=0){++fineRead;return;}if(fineOwner->RequestRead(finePlan,key,fineDemand,fineIdentity,FineResidency(),FineCpuLedger()))return;}
  else if(finePair>=finePlan.pairCount){if(fineCenter){fineCenter=false;finePlan=FineJob::Plan();finePhase=0;}else finePhase=4;}
  else if(!fineMiss){++finePair;fineRead=0;}else finePhase=1;
 }
 if(finePhase==1&&finePair<finePlan.pairCount){
  // Completed pairs are already visible. Retire only enough distant images
  // for the next real source bank, then wait for our owned retirement fence.
  if(!FineJob::CanRender(finePlan,fineDemand,budget)){FineWindows::Key*keys=fineScratch.keys;bool*valid=fineScratch.valid;FineWindows::U64*touches=fineScratch.touch;for(unsigned n=0;n<FineJob::ActiveSlots();++n){keys[n]=fineCache[n].key;valid[n]=fineCache[n].valid;touches[n]=fineCache[n].touch;}
   while(FineLayerReserve()+FineGpuBytes()>AtlasView::GpuCap-WorldCohort::GpuReservation(2)){int victim=GsmFinePolicy::Lru(keys,valid,touches,FineJob::ActiveSlots(),fineDemand.view,finePlan.pairs[finePair].key,2);if(victim<0)break;if(!FineRetire(unsigned(victim)))break;valid[victim]=false;}
   FineShade();FinePresent(fineMap);return;}
  if(!FineQueue(finePlan,finePair))return;fineMiss=false;finePhase=2;
 }
 FineShade();FinePresent(fineMap);
}
#include "GsmFineSeamDiagnostic.h"
// Explicit progress proves another ready transition is useful. Waiting on a
// worker or a GPU fence ends this frame's batch; never poll in a busy loop.
struct FineBatchProgress {
 FineController*owner;FineCoordinator::Progress native;
 FineWindows::U64 demand,index,planRevision;unsigned planSpacing,phase,pair,read,ahead,aheadRead,flags,planState,planCount,planPairs;
 FineBatchProgress():owner(fineOwner),demand(fineDemand.revision),index(fineIndex.Revision()),planRevision(finePlan.demand.revision),planSpacing(finePlan.spacing),phase(finePhase),pair(finePair),read(fineRead),ahead(fineAhead),aheadRead(fineAheadRead),flags((fineMiss?1U:0U)|(fineBlocked?2U:0U)|(fineClosing?4U:0U)|(fineCenter?8U:0U)|(fineSpeculative?16U:0U)|(fineAheadMiss?32U:0U)),planState(unsigned(finePlan.state)),planCount(finePlan.count),planPairs(finePlan.pairCount){if(owner)native=owner->ObserveProgress();}
 bool Same(const FineBatchProgress&b)const{return owner==b.owner&&FineCoordinator::SameProgress(native,b.native)&&demand==b.demand&&index==b.index&&planRevision==b.planRevision&&planSpacing==b.planSpacing&&phase==b.phase&&pair==b.pair&&read==b.read&&ahead==b.ahead&&aheadRead==b.aheadRead&&flags==b.flags&&planState==b.planState&&planCount==b.planCount&&planPairs==b.planPairs;}
};
struct FinePaintDeferral {bool previous;FinePaintDeferral():previous(fineDeferPaint){fineDeferPaint=true;}~FinePaintDeferral(){fineDeferPaint=previous;}};
void FineRenderBatch(){
 if(fineDeferPaint)return;
 LARGE_INTEGER frequency,started,now;
 if(!QueryPerformanceFrequency(&frequency)||frequency.QuadPart<=0||!QueryPerformanceCounter(&started)||started.QuadPart<0){FineRenderTick();return;}
 {
  FinePaintDeferral defer;
  GsmFinePump::Budget budget(GsmFinePump::U64(started.QuadPart),GsmFinePump::U64(frequency.QuadPart));now=started;
  while(budget.CanStep(GsmFinePump::U64(now.QuadPart))){
   FineBatchProgress before;FineRenderTick();FineBatchProgress after;
   if(!QueryPerformanceCounter(&now)||now.QuadPart<0||!budget.Finished(GsmFinePump::U64(now.QuadPart),!before.Same(after)))break;
  }
 }
 // One shading budget and one UI pass per workspace frame, not per transition.
 if(FineContext()){FineShade();FinePresent(fineMap);}
 FineSeamTick();
}
struct FineListener:Ogre::CompositorWorkspaceListener {Ogre::CompositorWorkspace*workspace;Ogre::CompositorWorkspaceListener*previous;const void*renderer;bool active;FineListener():workspace(NULL),previous(NULL),renderer(NULL),active(false){}
 virtual void workspacePreUpdate(Ogre::CompositorWorkspace*w){if(previous)previous->workspacePreUpdate(w);if(!active||w!=workspace||fineThread!=GetCurrentThreadId()||!au||au->render!=renderer||au->render->workspace!=workspace||workspace->getListener()!=this)return;try{FineRenderBatch();}catch(const std::exception&e){FineFail(e.what());}catch(...){FineFail("native fine callback failed");}}
 virtual void passPreExecute(Ogre::CompositorPass*p){if(previous)previous->passPreExecute(p);}
}fineListener;
void FineObserve(MapScreen*map){
 if(!fineThread)fineThread=GetCurrentThreadId();if(GetCurrentThreadId()!=fineThread)return;fineRetires=0;
 if(au&&au->render&&au->render->workspace){Ogre::CompositorWorkspace*w=au->render->workspace;if(!fineListener.workspace){fineListener.workspace=w;fineListener.renderer=au->render;fineListener.previous=w->getListener();w->setListener(&fineListener);fineListener.active=true;}else if(fineListener.workspace!=w||fineListener.renderer!=au->render){fineListener.active=false;FineFail("native workspace identity changed");}}
 if(!enabled||!ready||!FineQualified()||(fineDemand.revision&&!FineOwnerContext())){if(fineOwner)fineOwner->Cancel();return;}if(!map||!map->getVisible())return;fineMap=map;fineMapWidget.set(map->mainWidget);
 if(!FineStore::Valid(fineIdentity)||fineDemand.scope.world!=reinterpret_cast<FineStore::U64>(ou)||std::memcmp(fineIdentity.baseSha,manifest.captureMetadata,32)){
  if(fineOwner){fineOwner->Cancel();return;}
  // A new worker identity must never plan with old-stamp GPU keys. Retiring
  // their owners keeps their bytes charged until the real GPU fence completes.
  for(unsigned n=0;n<FineJob::ActiveSlots();++n){FineDrop(n);if(fineCache[n].valid)return;}
  std::memset(&fineIdentity,0,sizeof(fineIdentity));fineIdentity.sourceEpoch=FineStore::U64(InterlockedIncrement64(&fineEpoch));fineIdentity.materialGeneration=fineIdentity.sourceEpoch;std::memcpy(fineIdentity.baseSha,manifest.captureMetadata,32);std::memcpy(fineIdentity.materialSha,manifest.materialSnapshots,32);if(!GsmAtlas::Hex(SatelliteRegion::ManifestSha256,fineIdentity.sourceCatalogSha)){FineFail("source catalog identity unavailable");return;}std::memset(&fineDemand,0,sizeof(fineDemand));fineMotionX=fineMotionZ=0;fineCenter=true;
 }
 FineJob::Demand next={};if(!FineGeometry(map,next))return;if(!fineDemand.revision||!FineJob::Internal::SameDemand(next,fineDemand)){if(fineOwner&&fineOwner->ReadReady())fineOwner->ReleaseRead();if(fineDemand.revision){++next.revision;double dx=next.view.world.x+next.view.world.w*.5-fineDemand.view.world.x-fineDemand.view.world.w*.5,dz=next.view.world.z+next.view.world.h*.5-fineDemand.view.world.z-fineDemand.view.world.h*.5;if(std::abs(dx)+std::abs(dz)>=.01){fineMotionX=dx;fineMotionZ=dz;}}fineChangedAt=GetTickCount();fineDemand=next;fineCenter=true;finePlan=FineJob::Plan();finePhase=0;fineSpeculative=false;fineAhead=0;fineAheadPlan=FineJob::Plan();++fineUseClock;for(unsigned n=0;n<FineJob::ActiveSlots();++n)if(fineCache[n].valid&&GsmFinePolicy::Intersects(fineCache[n].key,fineDemand.view))fineCache[n].touch=fineUseClock;}
 // Offscreen same-scope images stay warm until real physical/budget pressure.
 // Fixed overview cells are persistent coverage, including offscreen cells.
 // Only detail slots may retire for fine admission. If the complete overview,
 // requested layers and retained center cannot fit, the checked producer waits.
 unsigned retained=0;for(unsigned n=AtlasView::FallbackSlots;n<AtlasView::WindowSlots;++n)if(cache[n].key.Valid()&&++retained>BaseDetailCapacity())DropEntry(n);
}
