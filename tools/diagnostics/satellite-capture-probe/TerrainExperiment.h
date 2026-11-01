// Included inside the private probe namespace after its shader/readback helpers.
// No SDK Terrain fields, copying, ZoneMap callbacks or gameplay calls are used.
struct TerrainExperimentApi
{
    typedef Ogre::SceneManager *(__fastcall *CreateScene)(Ogre::Root*,Ogre::SceneTypeMask,size_t,Ogre::InstancingThreadedCullingMethod,const Ogre::String&);
    typedef void (__fastcall *DestroyScene)(Ogre::Root*,Ogre::SceneManager*);
    typedef Ogre::ObjectMemoryManager *(__fastcall *Memory)(Ogre::SceneManager*,Ogre::SceneMemoryMgrTypes);
    typedef Ogre::MovableObject *(__fastcall *CreateObject)(Ogre::SceneManager*,const Ogre::String&,Ogre::ObjectMemoryManager*,const Ogre::NameValuePairList*);
    typedef void (__fastcall *DestroyObject)(Ogre::SceneManager*,Ogre::MovableObject*);
    typedef void (__fastcall *Scale)(Terrain*,float,float);
    typedef void (__fastcall *Patch)(Terrain*,int);
    typedef void (__fastcall *Detail)(Terrain*,float,float,float);
    typedef void (__fastcall *Limits)(Terrain*,int,int);
    typedef void (__fastcall *Material)(Terrain*,const Ogre::String&);
    typedef bool (__fastcall *Stream)(Terrain*,const Ogre::String&);
    // Native member aggregate ABI: RCX=this,RDX=out,RAX=out (not a free aggregate return).
    typedef TerrainExperimentPolicy::Stats *(__fastcall *Info)(const Terrain*,TerrainExperimentPolicy::Stats*);
    typedef int (__fastcall *ViewportHeight)(const Ogre::Viewport*);
    CreateScene createScene;DestroyScene destroyScene;Memory memory;
    CreateObject createObject;DestroyObject destroyObject;
    Scale scale;Patch patch;Detail detail;Limits limits;Material material;Stream stream;Info info;
    ViewportHeight viewportHeight;
    void Bind()
    {
        createScene=reinterpret_cast<CreateScene>(AbiExport(nativeApi.ogre,CreatePrivateSceneName,CreatePrivateSceneRva));
        destroyScene=reinterpret_cast<DestroyScene>(AbiExport(nativeApi.ogre,DestroyPrivateSceneName,DestroyPrivateSceneRva));
        memory=reinterpret_cast<Memory>(AbiExport(nativeApi.ogre,PrivateMemoryManagerName,PrivateMemoryManagerRva));
        createObject=reinterpret_cast<CreateObject>(AbiExport(nativeApi.ogre,CreatePrivateMovableName,CreatePrivateMovableRva));
        destroyObject=reinterpret_cast<DestroyObject>(AbiExport(nativeApi.ogre,DestroyPrivateMovableName,DestroyPrivateMovableRva));
        scale=reinterpret_cast<Scale>(AbiExport(nativeApi.terrain,PrivateTerrainScaleName,PrivateTerrainScaleRva));
        patch=reinterpret_cast<Patch>(AbiExport(nativeApi.terrain,PrivatePatchSizeName,PrivatePatchSizeRva));
        detail=reinterpret_cast<Detail>(AbiExport(nativeApi.terrain,PrivateDetailName,PrivateDetailRva));
        limits=reinterpret_cast<Limits>(AbiExport(nativeApi.terrain,PrivateBuildLimitsName,PrivateBuildLimitsRva));
        material=reinterpret_cast<Material>(AbiExport(nativeApi.terrain,PrivateDefaultMaterialName,PrivateDefaultMaterialRva));
        stream=reinterpret_cast<Stream>(AbiExport(nativeApi.terrain,PrivateOpenStreamName,PrivateOpenStreamRva));
        info=reinterpret_cast<Info>(AbiExport(nativeApi.terrain,PrivateInfoName,PrivateInfoRva));
        viewportHeight=reinterpret_cast<ViewportHeight>(AbiExport(nativeApi.ogre,PrivateViewportHeightName,PrivateViewportHeightRva));
    }
};
struct TerrainExperiment
{
    TerrainExperimentApi api;
    NativeWorkerHandle worker;
    Ogre::Root *root;
    Ogre::SceneManager *scene;
    Terrain *terrain;
    Ogre::Camera *camera;
    Ogre::CompositorWorkspace *workspace;
    Ogre::CompositorManager2 *definitionOwner;
    Ogre::TexturePtr output;
    Ogre::MaterialPtr material;
    TerrainExperimentPolicy::Warmup warmup;
    TerrainExperimentPolicy::Region region;
    TerrainExperimentPolicy::GridBatch batch;
    unsigned __int64 peakNativeMemory;
    int primaryX,primaryZ;
    DWORD started,lastStep;
    std::string heightSource;
    bool active,geometryOnly,definitionsReady,definitionsFailed;
    TerrainExperimentPolicy::RecycleState recycle;
    TerrainExperiment():root(NULL),scene(NULL),terrain(NULL),camera(NULL),workspace(NULL),definitionOwner(NULL),
        peakNativeMemory(0),primaryX(0),primaryZ(0),started(0),lastStep(0),active(false),geometryOnly(false),definitionsReady(false),definitionsFailed(false){}
    void Define(Ogre::CompositorManager2 *manager)
    {
        if((definitionOwner&&definitionOwner!=manager)||definitionsFailed)
            throw std::runtime_error("private terrain definition ownership failed");
        if(definitionsReady)return;
        const Ogre::IdString nodeId("LEOPARD_PrivateTerrainGBuffer"),workspaceId("LEOPARD_PrivateTerrainWorkspace");
        if(manager->hasNodeDefinition(nodeId)||manager->hasWorkspaceDefinition(workspaceId))
            throw std::runtime_error("private terrain definition collision");
        definitionOwner=manager;definitionsFailed=true;
        Ogre::CompositorWorkspaceDef *def=manager->addWorkspaceDefinition(workspaceId);
        Ogre::TextureDefinitionBase::TextureDefinition *texture=def->addTextureDefinition("global_gbuffer");
        texture->width=Side;texture->height=Side;texture->fsaa=false;
        texture->formatList.push_back(Ogre::PF_A8R8G8B8);
        texture->formatList.push_back(Ogre::PF_A8R8G8B8);
        texture->formatList.push_back(Ogre::PF_FLOAT32_R);
        Ogre::CompositorNodeDef *node=manager->addNodeDefinition("LEOPARD_PrivateTerrainGBuffer");
        node->addTextureSourceName("rt_output",0,Ogre::TextureDefinitionBase::TEXTURE_INPUT);
        node->setNumTargetPass(1);
        Ogre::CompositorTargetDef *target=node->addTargetPass("global_gbuffer");
        target->setNumPasses(2);
        Ogre::CompositorPassClearDef *clear=static_cast<Ogre::CompositorPassClearDef*>(target->addPass(Ogre::PASS_CLEAR));
        clear->mColourValue=Ogre::ColourValue(0,.5f,0,1);
        Ogre::CompositorPassSceneDef *pass=static_cast<Ogre::CompositorPassSceneDef*>(target->addPass(Ogre::PASS_SCENE));
        pass->mFirstRQ=20;pass->mLastRQ=80;pass->mIncludeOverlays=false;
        def->connectOutput(nodeId,0);
        definitionsReady=true;definitionsFailed=false;
    }
    void CreateNativeTerrain()
    {
        if(terrain||!root||!scene||!camera||material.isNull())
            throw std::runtime_error("private native Terrain factory context incomplete");
        Ogre::MovableObject *object=api.createObject(scene,"Terrain",api.memory(scene,Ogre::SCENE_STATIC),NULL);
        if(!object||object==au->terrain)throw std::runtime_error("fresh Terrain aliases main Terrain");
        terrain=static_cast<Terrain*>(object);
        if(!NativeTerrainCache::Matches(terrain,nativeApi.terrainPrimary,nativeApi.terrainListener))
            throw std::runtime_error("fresh private terrain type differs");
        if(!object->getParentNode())scene->getRootSceneNode(Ogre::SCENE_STATIC)->attachObject(object);
        object->setRenderQueueGroup(25);
        api.scale(terrain,294912,9800);api.patch(terrain,64);
        api.detail(terrain,diagnosticRaster.detail,diagnosticRaster.detail,0);api.limits(terrain,32,8);
        api.material(terrain,material->getName());
        if(!api.stream(terrain,heightSource))throw std::runtime_error("private fullmap stream failed");
    }
    void DestroyNativeTerrain()
    {
        try
        {
        // Terrain owns a FrameListener/worker and may borrow camera: join/destroy FIRST.
        if(terrain)
        {
            NativeTerrainCache::WorkerState before,after;
            if(!ReadOwnedTerrainWorker(terrain,before))throw std::runtime_error("private worker ownership unavailable");
            const bool hasWorker=before.handle!=NULL;
            if(hasWorker)
            {
                if(!worker.Adopt(before.handle,before.id)||!ReadOwnedTerrainWorker(terrain,after)||
                   before.landscape!=after.landscape||before.handle!=after.handle||before.id!=after.id||before.request!=after.request)
                    throw std::runtime_error("private worker kernel identity changed; retaining resources");
                // Native running byte starts at0 BEFORE scheduling; a kernel join
                // is necessary even during partial-construction error cleanup.
                if(!worker.StopAndJoin(before.request,5000))
                    throw std::runtime_error("private worker did not terminate; retaining resources");
            }
            api.destroyObject(scene,terrain);terrain=NULL;
            if(hasWorker)
            {
                bool closed=false;
                if(!worker.Complete(closed))throw std::runtime_error("private native worker handle cleanup unverified");
                Log(closed?"private native worker kernel-joined; omitted HANDLE close repaired":"private native worker kernel-joined; native HANDLE already released");
            }
        }
        }
        catch(...)
        {
            // A failed native join/destruction/handle completion is not permission
            // to create another Terrain or free its borrowed camera/workspaces.
            recycle.Abort();restorationSafe=false;active=false;throw;
        }
    }
    void RecycleNativeTerrain(Ogre::Root *current)
    {
        if(!active||!recycle.enabled||!recycle.pending||!terrain||!scene||!camera||
           !TerrainExperimentPolicy::CanDestroy(root,current)||!RenderAbiMatches()||
           !au||!au->render||scene==au->render->scene||camera==au->render->camera)
            throw std::runtime_error("private Terrain recycle ownership incomplete");
        if(batch.Expired(GetTickCount()))throw std::runtime_error("private primary-grid batch total deadline exceeded");
        typedef Ogre::RenderQueue *(__fastcall *Queue)(Ogre::SceneManager*);
        typedef Ogre::RenderQueueGroup *(__fastcall *Group)(Ogre::RenderQueue*,unsigned char);
        typedef void (__fastcall *Clear)(Ogre::RenderQueueGroup*,bool);
        Queue queue=reinterpret_cast<Queue>(AbiExport(nativeApi.ogre,PrivateRenderQueueName,PrivateRenderQueueRva));
        Group group=reinterpret_cast<Group>(AbiExport(nativeApi.ogre,PrivateQueueGroupName,PrivateQueueGroupRva));
        Clear clear=reinterpret_cast<Clear>(AbiExport(nativeApi.ogre,PrivateGroupClearName,PrivateGroupClearRva));
        Ogre::RenderQueue *owned=queue(scene),*main=queue(au->render->scene);
        if(!owned||owned==main)throw std::runtime_error("private render queue aliases main scene");
        Ogre::RenderQueueGroup *geometry=group(owned,25);
        if(!geometry)throw std::runtime_error("private geometry queue missing");
        // Native RenderQueue::clear clears ALL scenes. Only this group's pointer
        // collections are cleared here, while private material/Pass remain alive.
        clear(geometry,false);
        DestroyNativeTerrain();
        camera->setPosition(float(region.x),float(region.altitude),float(region.z));
        CreateNativeTerrain();
        started=GetTickCount();lastStep=started-100;warmup=TerrainExperimentPolicy::Warmup();
        recycle.Ready();
        std::ostringstream message;
        message<<"private native Terrain recycled x="<<primaryX<<" z="<<primaryZ
               <<" batchIndex="<<batch.index<<"; Scene/camera/render surfaces retained";
        Log(message.str());
    }
    void Begin(Ogre::Root *owner,int tileX,int tileZ,int altitude,const std::string &heightPath,bool geometryOnlyRequest,bool gridRequest,bool recycleRequest)
    {
        if(root||scene||terrain||camera||workspace||active)
            throw std::runtime_error("private terrain experiment already owns resources");
        if(diagnosticRaster.resolutionOnly)
        {
            MEMORYSTATUSEX memory={sizeof(MEMORYSTATUSEX)};
            if(!GlobalMemoryStatusEx(&memory)||!RasterPolicy::HostFits(diagnosticRaster,memory.ullAvailPhys,memory.ullAvailPageFile))
                throw std::runtime_error("high raster control host memory preflight refused; no scene allocated");
            std::ostringstream budget;
            budget<<"high raster host preflight available physical="<<memory.ullAvailPhys<<" commit="<<memory.ullAvailPageFile
                  <<" required="<<RasterPolicy::RequiredHostBytes(diagnosticRaster)<<"; GPU availability requires separate measurement";Log(budget.str());
        }
        batch.Begin(gridRequest,GetTickCount());
        heightSource=heightPath;recycle.Start(recycleRequest,gridRequest);
        if(batch.enabled&&!batch.Cell(tileX,tileZ))throw std::runtime_error("private grid cursor refused");
        if(!TerrainExperimentPolicy::PrimaryCell(tileX,tileZ,altitude,region))
            throw std::runtime_error("private terrain primary cell/altitude refused");
        primaryX=tileX;primaryZ=tileZ;peakNativeMemory=0;
        api.Bind();root=owner;
        Ogre::SceneManager *fresh=api.createScene(root,Ogre::ST_GENERIC,1,Ogre::INSTANCING_CULLING_SINGLETHREAD,"LEOPARD_PrivateTerrainScene");
        if(!fresh||fresh==au->render->scene)throw std::runtime_error("private scene aliases main scene");
        scene=fresh;
        camera=scene->createCamera("LEOPARD_PrivateTerrainCamera");
        Ogre::SceneNode *cameraNode=scene->getRootSceneNode()->createChildSceneNode();
        if(!ProbeOwnership::AttachOwnedCamera(camera,au->render->camera,cameraNode))
            throw std::runtime_error("private terrain camera attachment refused");
        camera->setPosition(float(region.x),float(region.altitude),float(region.z));
        camera->setOrientation(Ogre::Quaternion(Ogre::Radian(-Ogre::Math::HALF_PI),Ogre::Vector3::UNIT_X));
        camera->setProjectionType(Ogre::PT_ORTHOGRAPHIC);
        camera->setOrthoWindow(float(region.span),float(region.span));
        camera->setNearClipDistance(1);camera->setFarClipDistance(120000);camera->setAutoAspectRatio(false);
        camera->setUseRenderingDistance(false);nativeApi.setMin(camera,false);
        // Fresh scene coordinates are absolute world coordinates; private material removes
        // the live world's floating-origin offset without modifying shared parameters.
        Ogre::MaterialManager &materials=Ogre::MaterialManager::getSingleton();
        const char *materialName="LEOPARD_PrivateTerrainMaterial";
        if(materials.resourceExists(materialName))throw std::runtime_error("private terrain material collision");
        Ogre::MaterialPtr original=materials.getByName("DistantTerrain");
        if(original.isNull())throw std::runtime_error("native distant terrain material absent");
        material=materials.create(materialName,Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        CopyPrivateTechniqueConfiguration(original,material);
        if(material->getNumTechniques()!=1||material->getTechnique(0)->getNumPasses()!=1)
            throw std::runtime_error("native distant terrain material structure differs");
        Ogre::GpuProgramParametersSharedPtr fp=material->getTechnique(0)->getPass(0)->getFragmentProgramParameters();
        fp->removeAllSharedParameters();
        fp->setNamedConstant("worldOffset",Ogre::Vector3::ZERO);
        fp->setNamedConstant("map",Ogre::Vector4(-147456,-147456,1.0f/294912,1.0f/294912));
        material->load();
        CreateNativeTerrain();
        Ogre::CompositorManager2 *manager=root->getCompositorManager2();Define(manager);
        rtvDebt.UncertainAllocation();
        output=Ogre::TextureManager::getSingleton().createManual("LEOPARD_PrivateTerrainOutput",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,Ogre::TEX_TYPE_2D,Side,Side,1,0,Ogre::PF_A8R8G8B8,Ogre::TU_RENDERTARGET);
        TrackPrivateTarget(output);rtvDebt.AllocationConfirmed();
        rtvDebt.UncertainAllocation(); // Native constructor may allocate internal GBuffer targets before throwing.
        workspace=manager->addWorkspace(scene,output->getBuffer()->getRenderTarget(),camera,
            Ogre::IdString("LEOPARD_PrivateTerrainWorkspace"),false);
        const Ogre::CompositorChannel &gb=workspace->getGlobalTexture("global_gbuffer");
        if(gb.textures.size()!=3)throw std::runtime_error("private Terrain GBuffer structure differs");
        const Ogre::CompositorChannel &mainGb=au->render->workspace->getGlobalTexture("global_gbuffer");
        if(mainGb.textures.size()!=3)throw std::runtime_error("main GBuffer structure differs");
        for(unsigned i=0;i<3;++i)
            if(mainGb.textures[i]==gb.textures[i])throw std::runtime_error("private Terrain GBuffer aliases main scene");
        for(unsigned i=0;i<3;++i)TrackPrivateTarget(gb.textures[i]);
        rtvDebt.AllocationConfirmed();
        // Allocate auxiliary workspaces outside compositor iteration, including
        // failure paths, so pending registration cannot outlive their materials.
        geometryOnly=geometryOnlyRequest;
        if(!geometryOnly)
        {
            PreparePositionPass(manager,scene,camera,gb.textures[2]);
            PrepareSunPass(manager,scene,camera,gb);
        }
        started=GetTickCount();lastStep=started-100;warmup=TerrainExperimentPolicy::Warmup();active=true;
        std::ostringstream dimensions;
        dimensions<<"isolated native Terrain ready; one 73728m primary cell,"<<Side<<" output; no simulation zones activated";
        Log(dimensions.str());
        Log(geometryOnly?"private Terrain A/B mode: GBuffer only; no Position/Sun programs or readback":"private Terrain A/B mode: full Position/Sun diagnostics");
        if(batch.enabled)Log(recycle.enabled?"private primary-grid batch started:16cells,Terrain recycled between cells,oneScene,120s total deadline":"private primary-grid batch started:16cells,oneTerrain,120s total deadline");
    }
    // Return false while bounded native stream/build work remains. Root calls only at
    // the main compositor boundary; native Root advances this Terrain's own worker.
    bool Step()
    {
        DWORD now=GetTickCount();
        if(batch.Expired(now))throw std::runtime_error("private primary-grid batch total deadline exceeded");
        if(recycle.pending)return false;
        if(static_cast<DWORD>(now-lastStep)<100)return false;
        lastStep=now;
        workspace->_update();
        if(!warmup.frames)
        {
            Ogre::Viewport *last=nativeApi.getLast(camera);
            if(!last||!RasterPolicy::ViewportMatches(diagnosticRaster,api.viewportHeight(last)))
                throw std::runtime_error("private Terrain actual viewport height differs from raster control");
            std::ostringstream lod;
            lod<<"private Terrain LOD viewport height="<<api.viewportHeight(last)<<" detail="<<diagnosticRaster.detail
               <<" resolutionOnly="<<diagnosticRaster.resolutionOnly;Log(lod.str());
        }
        TerrainExperimentPolicy::Stats stats={0};
        if(api.info(terrain,&stats)!=&stats)throw std::runtime_error("native Terrain Info aggregate ABI differs");
        peakNativeMemory=(std::max)(peakNativeMemory,stats.memory);
        const TerrainExperimentPolicy::Result result=warmup.Observe(started,now,stats);
        std::ostringstream s;s<<"isolated terrain step="<<warmup.frames<<" memory="<<stats.memory
          <<" patches="<<stats.patches<<" visible="<<stats.visible<<" triangles="<<stats.triangles
          <<" ogre="<<stats.ogre<<" unbuilt="<<stats.unbuilt;Log(s.str());
        if(result==TerrainExperimentPolicy::Exceeded)throw std::runtime_error("isolated terrain geometry budget exceeded");
        if(result==TerrainExperimentPolicy::Expired)throw std::runtime_error("isolated terrain did not stabilize before deadline");
        if(result==TerrainExperimentPolicy::Ready)
        {
            std::ostringstream ready;ready<<"private primary cell ready x="<<primaryX<<" z="<<primaryZ
              <<" center="<<region.x<<','<<region.z<<" span="<<region.span<<" altitude="<<region.altitude
              <<" batchIndex="<<batch.index<<" nativePeak="<<peakNativeMemory;Log(ready.str());
        }
        return result==TerrainExperimentPolicy::Ready;
    }
    bool AdvancePrimaryCell()
    {
        if(!batch.Advance())return false;
        if(!batch.Cell(primaryX,primaryZ)||!TerrainExperimentPolicy::PrimaryCell(primaryX,primaryZ,int(region.altitude),region))
            throw std::runtime_error("private primary-grid cursor invalid");
        // Normal native camera notification/FrameListener updates drive selection.
        // No Landscape cache writes, shared ZoneMap callback or game camera movement.
        if(!recycle.Next())camera->setPosition(float(region.x),float(region.altitude),float(region.z));
        started=GetTickCount();lastStep=started-100;warmup=TerrainExperimentPolicy::Warmup();
        std::ostringstream next;next<<"private primary-grid advanced x="<<primaryX<<" z="<<primaryZ
          <<" batchIndex="<<batch.index<<" nativePeak="<<peakNativeMemory;Log(next.str());return true;
    }
    void Destroy(Ogre::Root *current)
    {
        if(!root)return;
        if(!TerrainExperimentPolicy::CanDestroy(root,current)||!RenderAbiMatches())
            throw std::runtime_error("private Terrain root ownership unknown; retaining resources");
        Ogre::CompositorManager2 *manager=root->getCompositorManager2();
        if(workspace){manager->removeWorkspace(workspace);workspace=NULL;}
        DestroyNativeTerrain();
        if(camera){scene->destroyCamera(camera);camera=NULL;}
        if(scene){api.destroyScene(root,scene);scene=NULL;}
        if(!material.isNull())
        {Ogre::MaterialManager::getSingleton().remove(material->getName());material.setNull();}
        if(!output.isNull())
        {Ogre::TextureManager::getSingleton().remove(output->getName());output.setNull();}
        root=NULL;active=false;recycle.Abort();Log("isolated Terrain destroyed before its camera and scene");
    }
} terrainExperiment;
