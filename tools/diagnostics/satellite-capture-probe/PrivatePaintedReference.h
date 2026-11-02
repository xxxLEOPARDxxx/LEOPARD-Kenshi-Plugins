// Owned four-pair reference only. No native ZoneMap/material callback mutation.
#include "PrivatePaintedInputs.h"
#include "PaintedShader.h"
#include "../../../GlobalSatelliteMap/PaintedMapPolicy.h"
#include "PaintedMipPolicy.h"
#include "region-catalog/RegionCatalogData.h"
struct PrivatePaintedReference
{
    struct Images {Ogre::Image image[4];std::vector<BYTE> encoded[4];SatelliteGpuMip::Layer layers[4];};
    Images *images[2];Ogre::TexturePtr textures[2];Ogre::HighLevelGpuProgramPtr program;
    ProbePolicy::FixedProgramCache cache;bool uncertain;int primaryX,primaryZ;
    PrivatePaintedReference():uncertain(false),primaryX(1),primaryZ(1){images[0]=images[1]=NULL;}
    static const char *Name(unsigned kind)
    {return kind?"LEOPARD_PrivatePaintedColour":"LEOPARD_PrivatePaintedOverlay";}
    static const char *ProgramName(){return "LEOPARD_PrivatePaintedFragment";}
    struct UploadOperations
    {
        typedef ID3D11ShaderResourceView* (__fastcall *BorrowedSrv)(Ogre::Texture*);
        typedef Ogre::ResourceManager* (__fastcall *Creator)(Ogre::Resource*);
        typedef void (__fastcall *NotifyLoaded)(Ogre::ResourceManager*,Ogre::Resource*);
        Ogre::Texture *texture;Images &sources;unsigned kind;PaintedMipPolicy::Plan plan;
        ID3D11ShaderResourceView *view;Ogre::ResourceManager *creator;
        Creator getCreator;NotifyLoaded notify;PrivateBiomeReference::IsLoaded loaded;
        PrivateBiomeReference::SetLoaded finish;
        UploadOperations(Ogre::Texture *owned,Images &input,unsigned index,const PaintedMipPolicy::Plan &shape):
            texture(owned),sources(input),kind(index),plan(shape),view(NULL),creator(NULL)
        {
            getCreator=reinterpret_cast<Creator>(AbiExport(nativeApi.ogre,PrivateResourceCreatorName,PrivateResourceCreatorRva));
            notify=reinterpret_cast<NotifyLoaded>(AbiExport(nativeApi.ogre,PrivateResourceNotifyLoadedName,PrivateResourceNotifyLoadedRva));
            loaded=reinterpret_cast<PrivateBiomeReference::IsLoaded>(AbiExport(nativeApi.ogre,PrivateResourceLoadedName,PrivateResourceLoadedRva));
            finish=reinterpret_cast<PrivateBiomeReference::SetLoaded>(AbiExport(nativeApi.ogre,PrivateResourceSetLoadedName,PrivateResourceSetLoadedRva));
        }
        bool Preflight()
        {
            Ogre::Image &first=sources.image[0];
            if(!OnMain()||!RenderAbiMatches()||!texture||!IsPrivateArrayTexture(texture)||
                texture->getName()!=Name(kind)||texture->getTextureType()!=Ogre::TEX_TYPE_2D_ARRAY||
                texture->getDepth()!=4||texture->getWidth()!=first.getWidth()||texture->getHeight()!=first.getHeight()||
                texture->getNumMipmaps()!=plan.additionalMips||texture->getFormat()!=first.getFormat()||loaded(texture))return false;
            creator=getCreator(texture);
            if(creator!=static_cast<Ogre::ResourceManager*>(&Ogre::TextureManager::getSingleton()))return false;
            // Guarded nonvirtual native getter; never use the incompatible SDK
            // D3D11Texture layout or Texture::getCustomAttribute (native no-op).
            BorrowedSrv getter=reinterpret_cast<BorrowedSrv>(reinterpret_cast<BYTE*>(nativeApi.d3d11)+PrivateArraySrvGetterRva);
            view=getter(texture);
            return view!=NULL; // Borrowed only, scoped to this operations object.
        }
        bool Upload()
        {
            return SUCCEEDED(SatelliteGpuMip::UploadFourLayerMipChain(view,
                static_cast<UINT>(sources.image[0].getWidth()),static_cast<UINT>(sources.image[0].getHeight()),
                plan.levels,static_cast<DXGI_FORMAT>(plan.dxgiFormat),sources.layers));
        }
        void Notify(){notify(creator,texture);}
        void SetLoaded(){finish(texture);}
        bool Loaded(){return loaded(texture);}
    };
    static void Read(const PrivatePaintedInput &input,const std::string &gameRoot,std::vector<BYTE> &encoded,size_t &total)
    {
        if(!SatelliteTextureArray::AddBytes(total,input.bytes))throw std::runtime_error("private painted input byte budget refused");
        const std::string path=PrivateBiomeReference::Canonical(gameRoot+"/"+input.path);
        struct File {HANDLE h;File():h(INVALID_HANDLE_VALUE){}~File(){if(h!=INVALID_HANDLE_VALUE)CloseHandle(h);}} file;
        file.h=CreateFileA(path.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
        LARGE_INTEGER size={0};
        if(file.h==INVALID_HANDLE_VALUE||!GetFileSizeEx(file.h,&size)||size.QuadPart!=input.bytes)
            throw std::runtime_error("private painted exact file size/path refused before decode");
        encoded.resize(input.bytes);DWORD got=0;
        if(!ReadFile(file.h,&encoded[0],input.bytes,&got,NULL)||got!=input.bytes||PrivateBiomeReference::Hash(encoded)!=input.sha256)
            throw std::runtime_error("private painted checksum refused before decode");
    }
    void PrepareProgram(Ogre::CompositorManager2 *owner)
    {
        Ogre::HighLevelGpuProgramManager &manager=Ogre::HighLevelGpuProgramManager::getSingleton();
        Ogre::HighLevelGpuProgramPtr existing;
        if(manager.resourceExists(ProgramName()))existing=manager.getByName(ProgramName());
        const ProbePolicy::FixedProgramCache::Choice choice=reusePrivateShaders?
            cache.Inspect(owner,0,existing.get(),PaintedShaderSource):
            (existing.isNull()?ProbePolicy::FixedProgramCache::Create:ProbePolicy::FixedProgramCache::Refuse);
        if(choice==ProbePolicy::FixedProgramCache::Refuse)throw std::runtime_error("private painted shader identity changed");
        if(choice==ProbePolicy::FixedProgramCache::Reuse)program=existing;
        else
        {
            program=manager.createProgram(ProgramName(),Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,"hlsl",Ogre::GPT_FRAGMENT_PROGRAM);
            program->setSource(PaintedShaderSource);program->setParameter("entry_point","main_fs");program->setParameter("target","ps_4_0");
            program->setParameter("enable_backwards_compatibility","true");program->setParameter("preprocessor_defines","DX11=1");
            program->load();
            if(!program->isSupported()||program->hasCompileError())throw std::runtime_error("private painted shader unsupported");
            if(reusePrivateShaders&&!cache.Record(owner,0,program.get(),PaintedShaderSource))throw std::runtime_error("private painted shader cache refused");
        }
        if(program.isNull()||!program->isSupported()||program->hasCompileError())throw std::runtime_error("private painted cached shader unsupported");
    }
    void Begin(const std::string &gameRoot,Ogre::CompositorManager2 *owner)
    {BeginInputs(gameRoot,owner,PrivatePaintedInputs,1,1);}
    static bool InputsForPrimary(int x,int z,PrivatePaintedInput (&inputs)[8])
    {
        if(x<0||x>3||z<0||z>3||!SatelliteRegion::Validate(SatelliteRegion::Data))return false;
        for(unsigned kind=0;kind<2;++kind)for(unsigned layer=0;layer<4;++layer)
        {
            const SatelliteRegion::Painted& p=SatelliteRegion::Data.painted[kind*64+(z*2+layer/2)*8+x*2+layer%2];
            const PrivatePaintedInput value={p.kind,p.x,p.z,p.side,p.bytes,p.path,p.sha256};inputs[kind*4+layer]=value;
        }
        return true;
    }
    void BeginPrimary(const std::string &gameRoot,Ogre::CompositorManager2 *owner,int x,int z)
    {
        PrivatePaintedInput inputs[8];
        if(!InputsForPrimary(x,z,inputs))throw std::runtime_error("private painted primary catalog refused");
        BeginInputs(gameRoot,owner,inputs,x,z);
    }
    void BeginInputs(const std::string &gameRoot,Ogre::CompositorManager2 *owner,const PrivatePaintedInput (&inputs)[8],int x,int z)
    {
        if(x<0||x>3||z<0||z>3)throw std::runtime_error("private painted primary bounds refused");
        if(!OnMain()||!owner||uncertain||images[0]||images[1]||!textures[0].isNull()||!textures[1].isNull()||!program.isNull())
            throw std::runtime_error("private painted reference context/ownership refused");
        primaryX=x;primaryZ=z;
        PrepareProgram(owner);
        size_t encodedBytes=0,decodedBytes=0,gpuBytes=0;PaintedMipPolicy::Plan plans[2];
        for(unsigned kind=0;kind<2;++kind)
        {
            if(Ogre::TextureManager::getSingleton().resourceExists(Name(kind)))throw std::runtime_error("private painted texture-name collision");
            images[kind]=new Images;
            for(unsigned layer=0;layer<4;++layer)
            {
                const PrivatePaintedInput &input=inputs[kind*4+layer];
                if(input.kind!=kind||input.x!=unsigned(x*2)+layer%2||input.z!=unsigned(z*2)+layer/2)
                    throw std::runtime_error("private painted fixture order differs");
                Read(input,gameRoot,images[kind]->encoded[layer],encodedBytes);
                Ogre::DataStreamPtr stream(new Ogre::MemoryDataStream(&images[kind]->encoded[layer][0],input.bytes,false,true));
                Ogre::Image &image=images[kind]->image[layer];image.load(stream,"png");
                if(!SatellitePaintedMap::ImageShape(kind,image.getWidth(),image.getHeight(),image.getDepth(),image.getNumFaces(),
                     image.getNumMipmaps(),image.getFormat(),image.getSize())||
                   !SatelliteTextureArray::AddBytes(decodedBytes,image.getSize())||
                   (layer&&image.getFormat()!=images[kind]->image[0].getFormat()))
                    throw std::runtime_error("private painted decoded RGBA shape/format/byte budget refused");
                const Ogre::PixelBox pixels=image.getPixelBox(0,0);
                if(!pixels.data||pixels.data!=image.getData()||pixels.left||pixels.top||pixels.front||
                   pixels.right!=image.getWidth()||pixels.bottom!=image.getHeight()||pixels.back!=1||
                   pixels.rowPitch!=image.getWidth()||pixels.slicePitch!=image.getWidth()*image.getHeight()||
                   pixels.format!=image.getFormat())
                    throw std::runtime_error("private painted mip0 contiguous pixel layout refused");
                images[kind]->layers[layer].data=pixels.data;images[kind]->layers[layer].bytes=image.getSize();
            }
            const Ogre::Image &first=images[kind]->image[0];
            if(!PaintedMipPolicy::Select(kind,first.getWidth(),first.getHeight(),first.getFormat(),plans[kind]))
                throw std::runtime_error("private painted full-mip plan refused");
        }
        size_t combinedBytes=encodedBytes;
        if(!SatelliteTextureArray::AddBytes(combinedBytes,decodedBytes))
            throw std::runtime_error("private painted combined source payload budget refused before allocation");
        if(!PaintedMipPolicy::Budget(plans,gpuBytes))throw std::runtime_error("private painted full-chain GPU budget refused before allocation");
        {std::ostringstream message;message<<"private painted full-mip preflight: levels=11/12 layers=4/4 gpuTexelBytes="<<gpuBytes
            <<" cap="<<PaintedMipPolicy::GpuLimit<<"; manager size is base-level accounting only";Log(message.str());}
        // Both complete CPU arrays and the explicit full-chain GPU budget are
        // validated before either GPU array allocation. Upload mip0 unchanged;
        // GenerateMips once per full-array SRV, then one credit + Loaded state.
        for(unsigned kind=0;kind<2;++kind)
        {
            Ogre::Image &first=images[kind]->image[0];uncertain=true;
            textures[kind]=Ogre::TextureManager::getSingleton().createManual(Name(kind),"Landscape",Ogre::TEX_TYPE_2D_ARRAY,
                first.getWidth(),first.getHeight(),4,plans[kind].additionalMips,first.getFormat(),0x15,NULL);
            if(textures[kind].isNull())throw std::runtime_error("private painted array constructor returned null");
            uncertain=false;
            UploadOperations operations(textures[kind].getPointer(),*images[kind],kind,plans[kind]);
            if(!PaintedMipPolicy::CompleteUpload(operations,uncertain))
                throw std::runtime_error("private painted full-mip upload/native completion unverified");
            std::ostringstream message;message<<"private painted full-mip submitted: kind="<<kind<<" levels="<<plans[kind].levels
                <<" DXGI="<<plans[kind].dxgiFormat<<" gpuTexelBytes="<<plans[kind].gpuBytes
                <<" creator=TextureManager notify=1 Loaded=1; native pixel equivalence pending";Log(message.str());
        }
        for(unsigned kind=0;kind<2;++kind){delete images[kind];images[kind]=NULL;}
        std::ostringstream message;message<<"private painted arrays loaded:4pairs core"<<primaryX*2<<','<<primaryZ*2<<"; RGBA full-chain; sourceBytes="<<encodedBytes
            <<" decodedBytes="<<decodedBytes<<" gpuTexelBytes="<<gpuBytes;Log(message.str());
    }
    void Program(const Ogre::MaterialPtr &material)
    {
        if(program.isNull()||material.isNull())throw std::runtime_error("private painted program/material missing");
        material->getTechnique(0)->getPass(0)->setFragmentProgram(ProgramName());
    }
    void Bind(const Ogre::MaterialPtr &material)
    {
        if(textures[0].isNull()||textures[1].isNull())throw std::runtime_error("private painted texture pair missing");
        Ogre::Pass *pass=material->getTechnique(0)->getPass(0);
        if(pass->getNumTextureUnitStates()!=5)throw std::runtime_error("private painted pass texture layout differs");
        for(unsigned kind=0;kind<2;++kind)
        {
            Ogre::TextureUnitState *unit=pass->getTextureUnitState(kind+2);
            unit->setContentType(Ogre::TextureUnitState::CONTENT_NAMED);unit->setTexture(textures[kind]);
            unit->setTextureAddressingMode(Ogre::TextureUnitState::TAM_WRAP);unit->setTextureFiltering(Ogre::TFO_TRILINEAR);
        }
        pass->getTextureUnitState(0)->setTextureFiltering(Ogre::TFO_ANISOTROPIC);
        pass->getTextureUnitState(4)->setTextureFiltering(Ogre::TFO_NONE);
        Ogre::GpuProgramParametersSharedPtr fp=pass->getFragmentProgramParameters();
        BindPrimaryBase(material);
        fp->setNamedAutoConstant("viewport",Ogre::GpuProgramParameters::ACT_VIEWPORT_SIZE);
        fp->setNamedAutoConstant("cameraPos",Ogre::GpuProgramParameters::ACT_CAMERA_POSITION);
        fp->setNamedAutoConstant("farClip",Ogre::GpuProgramParameters::ACT_FAR_CLIP_DISTANCE);
        Log("private painted pixel selection configured; four native18m-border maps, explicit SRV/sampler slots; no biome mixing/water/buildings");
    }
    bool MatchesPrimary(unsigned x,unsigned z)const
    {return !textures[0].isNull()&&!textures[1].isNull()&&primaryX==int(x)&&primaryZ==int(z);}
    void BindPrimaryBase(const Ogre::MaterialPtr& material)
    {
        if(primaryX<0||primaryX>3||primaryZ<0||primaryZ>3||material.isNull()||textures[0].isNull()||textures[1].isNull())
            throw std::runtime_error("private painted primary base unavailable");
        material->getTechnique(0)->getPass(0)->getFragmentProgramParameters()->setNamedConstant("paintedTileBase",
            Ogre::Vector4(float(-147456+primaryX*73728),float(-147456+primaryZ*73728),0,0));
    }
    void Destroy()
    {
        if(uncertain)throw std::runtime_error("private painted array constructor/upload uncertain; retaining bounded inputs");
        for(unsigned kind=0;kind<2;++kind)if(!textures[kind].isNull())
        {
            Ogre::TextureManager::getSingleton().remove(Name(kind));
            if(nativeApi.useCount(&textures[kind])!=1)throw std::runtime_error("private painted array still borrowed");
        }
        for(unsigned kind=0;kind<2;++kind){textures[kind].setNull();delete images[kind];images[kind]=NULL;}
        if(!program.isNull())
        {
            if(!cache.Owns(0,program.get()))Ogre::HighLevelGpuProgramManager::getSingleton().remove(ProgramName());
            program.setNull();
        }
        primaryX=primaryZ=1;
    }
};
PrivatePaintedReference& PaintedReference()
{
    static PrivatePaintedReference *owned=new PrivatePaintedReference;
    return *owned; // Never destruct retained Ogre resources after Root shutdown.
}
