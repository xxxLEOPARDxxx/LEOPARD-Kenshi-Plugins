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
    CreateScene createScene;DestroyScene destroyScene;Memory memory;
    CreateObject createObject;DestroyObject destroyObject;
    Scale scale;Patch patch;Detail detail;Limits limits;Material material;Stream stream;Info info;
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
    }
};
struct TerrainExperiment
{
    TerrainExperimentApi api;
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
    DWORD started,lastStep;
    bool active,definitionsReady,definitionsFailed;
    TerrainExperiment():root(NULL),scene(NULL),terrain(NULL),camera(NULL),workspace(NULL),definitionOwner(NULL),
        started(0),lastStep(0),active(false),definitionsReady(false),definitionsFailed(false){}
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
    void Begin(Ogre::Root *owner,int tileX,int tileZ,int altitude,const std::string &heightPath)
    {
        if(root||scene||terrain||camera||workspace||active)
            throw std::runtime_error("private terrain experiment already owns resources");
        if(!TerrainExperimentPolicy::PrimaryCell(tileX,tileZ,altitude,region))
            throw std::runtime_error("private terrain primary cell/altitude refused");
        api.Bind();root=owner;
        scene=api.createScene(root,Ogre::ST_GENERIC,1,Ogre::INSTANCING_CULLING_SINGLETHREAD,"LEOPARD_PrivateTerrainScene");
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
        material=original->clone(materialName);
        if(material->getNumTechniques()!=1||material->getTechnique(0)->getNumPasses()!=1)
            throw std::runtime_error("native distant terrain material structure differs");
        Ogre::GpuProgramParametersSharedPtr fp=material->getTechnique(0)->getPass(0)->getFragmentProgramParameters();
        fp->removeAllSharedParameters();
        fp->setNamedConstant("worldOffset",Ogre::Vector3::ZERO);
        fp->setNamedConstant("map",Ogre::Vector4(-147456,-147456,1.0f/294912,1.0f/294912));
        material->load();
        Ogre::MovableObject *object=api.createObject(scene,"Terrain",api.memory(scene,Ogre::SCENE_STATIC),NULL);
        terrain=static_cast<Terrain*>(object);
        if(!NativeTerrainCache::Matches(terrain,nativeApi.terrainPrimary,nativeApi.terrainListener))
            throw std::runtime_error("fresh private terrain type differs");
        if(!object->getParentNode())scene->getRootSceneNode(Ogre::SCENE_STATIC)->attachObject(object);
        object->setRenderQueueGroup(25);
        api.scale(terrain,294912,9800);api.patch(terrain,64);
        api.detail(terrain,8,8,0);api.limits(terrain,32,8);
        api.material(terrain,materialName);
        if(!api.stream(terrain,heightPath))throw std::runtime_error("private fullmap stream failed");
        Ogre::CompositorManager2 *manager=root->getCompositorManager2();Define(manager);
        output=Ogre::TextureManager::getSingleton().createManual("LEOPARD_PrivateTerrainOutput",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,Ogre::TEX_TYPE_2D,Side,Side,1,0,Ogre::PF_A8R8G8B8,Ogre::TU_RENDERTARGET);
        workspace=manager->addWorkspace(scene,output->getBuffer()->getRenderTarget(),camera,
            Ogre::IdString("LEOPARD_PrivateTerrainWorkspace"),false);
        const Ogre::CompositorChannel &gb=workspace->getGlobalTexture("global_gbuffer");
        if(gb.textures.size()!=3)throw std::runtime_error("private Terrain GBuffer structure differs");
        // Allocate auxiliary workspaces outside compositor iteration, including
        // failure paths, so pending registration cannot outlive their materials.
        PreparePositionPass(manager,scene,camera,gb.textures[2]);
        PrepareSunPass(manager,scene,camera,gb);
        started=GetTickCount();lastStep=started-100;warmup=TerrainExperimentPolicy::Warmup();active=true;
        Log("isolated native Terrain ready; one 73728m primary cell,512 output; no simulation zones activated");
    }
    // Return false while bounded native stream/build work remains. Root calls only at
    // the main compositor boundary; native Root advances this Terrain's own worker.
    bool Step()
    {
        DWORD now=GetTickCount();
        if(static_cast<DWORD>(now-lastStep)<100)return false;
        lastStep=now;
        workspace->_update();
        TerrainExperimentPolicy::Stats stats={0};
        if(api.info(terrain,&stats)!=&stats)throw std::runtime_error("native Terrain Info aggregate ABI differs");
        const TerrainExperimentPolicy::Result result=warmup.Observe(started,now,stats);
        std::ostringstream s;s<<"isolated terrain step="<<warmup.frames<<" memory="<<stats.memory
          <<" patches="<<stats.patches<<" visible="<<stats.visible<<" triangles="<<stats.triangles
          <<" ogre="<<stats.ogre<<" unbuilt="<<stats.unbuilt;Log(s.str());
        if(result==TerrainExperimentPolicy::Exceeded)throw std::runtime_error("isolated terrain geometry budget exceeded");
        if(result==TerrainExperimentPolicy::Expired)throw std::runtime_error("isolated terrain did not stabilize before deadline");
        return result==TerrainExperimentPolicy::Ready;
    }
    void Destroy(Ogre::Root *current)
    {
        if(!root)return;
        if(!TerrainExperimentPolicy::CanDestroy(root,current)||!RenderAbiMatches())
            throw std::runtime_error("private Terrain root ownership unknown; retaining resources");
        Ogre::CompositorManager2 *manager=root->getCompositorManager2();
        if(workspace){manager->removeWorkspace(workspace);workspace=NULL;}
        // Terrain owns a FrameListener/worker and may borrow camera: join/destroy FIRST.
        if(terrain){api.destroyObject(scene,terrain);terrain=NULL;}
        if(camera){scene->destroyCamera(camera);camera=NULL;}
        if(scene){api.destroyScene(root,scene);scene=NULL;}
        if(!material.isNull())
        {Ogre::MaterialManager::getSingleton().remove(material->getName());material.setNull();}
        if(!output.isNull())
        {Ogre::TextureManager::getSingleton().remove(output->getName());output.setNull();}
        root=NULL;active=false;Log("isolated Terrain destroyed before its camera and scene");
    }
} terrainExperiment;
