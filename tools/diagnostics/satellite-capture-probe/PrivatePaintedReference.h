// Owned four-pair reference only. No native ZoneMap/material callback mutation.
#include "PrivatePaintedInputs.h"
#include "PaintedShader.h"
#include "../../../GlobalSatelliteMap/PaintedMapPolicy.h"
struct PrivatePaintedReference
{
    struct Images {Ogre::Image image[4];std::vector<BYTE> encoded[4];};
    Images *images[2];Ogre::TexturePtr textures[2];Ogre::HighLevelGpuProgramPtr program;
    ProbePolicy::FixedProgramCache cache;bool uncertain;
    PrivatePaintedReference():uncertain(false){images[0]=images[1]=NULL;}
    static const char *Name(unsigned kind)
    {return kind?"LEOPARD_PrivatePaintedColour":"LEOPARD_PrivatePaintedOverlay";}
    static const char *ProgramName(){return "LEOPARD_PrivatePaintedFragment";}
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
    {
        if(!OnMain()||!owner||uncertain||images[0]||images[1]||!textures[0].isNull()||!textures[1].isNull()||!program.isNull())
            throw std::runtime_error("private painted reference context/ownership refused");
        PrepareProgram(owner);
        size_t encodedBytes=0,decodedBytes=0;
        for(unsigned kind=0;kind<2;++kind)
        {
            if(Ogre::TextureManager::getSingleton().resourceExists(Name(kind)))throw std::runtime_error("private painted texture-name collision");
            images[kind]=new Images;
            for(unsigned layer=0;layer<4;++layer)
            {
                const PrivatePaintedInput &input=PrivatePaintedInputs[kind*4+layer];
                if(input.kind!=kind||input.x!=2+layer%2||input.z!=2+layer/2)
                    throw std::runtime_error("private painted fixture order differs");
                Read(input,gameRoot,images[kind]->encoded[layer],encodedBytes);
                Ogre::DataStreamPtr stream(new Ogre::MemoryDataStream(&images[kind]->encoded[layer][0],input.bytes,false,true));
                Ogre::Image &image=images[kind]->image[layer];image.load(stream,"png");
                if(!SatellitePaintedMap::ImageShape(kind,image.getWidth(),image.getHeight(),image.getDepth(),image.getNumFaces(),
                     image.getNumMipmaps(),image.getFormat(),image.getSize())||
                   !SatelliteTextureArray::AddBytes(decodedBytes,image.getSize())||
                   (layer&&image.getFormat()!=images[kind]->image[0].getFormat()))
                    throw std::runtime_error("private painted decoded RGBA shape/format/byte budget refused");
            }
        }
        // Validate both complete CPU arrays before either GPU allocation. Each
        // mip0 layer is uploaded unchanged: no alpha stripping or pixel conversion.
        for(unsigned kind=0;kind<2;++kind)
        {
            Ogre::Image &first=images[kind]->image[0];uncertain=true;
            textures[kind]=Ogre::TextureManager::getSingleton().createManual(Name(kind),"Landscape",Ogre::TEX_TYPE_2D_ARRAY,
                first.getWidth(),first.getHeight(),4,0,first.getFormat(),0x15,NULL);
            if(textures[kind].isNull())throw std::runtime_error("private painted array constructor returned null");
            uncertain=false;
            if(!OnMain()||!RenderAbiMatches()||!IsPrivateArrayTexture(textures[kind].getPointer())||
                textures[kind]->getTextureType()!=Ogre::TEX_TYPE_2D_ARRAY||textures[kind]->getDepth()!=4||
                textures[kind]->getWidth()!=first.getWidth()||textures[kind]->getHeight()!=first.getHeight()||
                textures[kind]->getNumMipmaps()!=0||textures[kind]->getFormat()!=first.getFormat())
                throw std::runtime_error("private painted GPU array shape/format differs");
            PrivateBiomeReference::WriteArray write=reinterpret_cast<PrivateBiomeReference::WriteArray>(reinterpret_cast<BYTE*>(nativeApi.d3d11)+PrivateArrayUploadRva);
            PrivateBiomeReference::IsLoaded loaded=reinterpret_cast<PrivateBiomeReference::IsLoaded>(AbiExport(nativeApi.ogre,PrivateResourceLoadedName,PrivateResourceLoadedRva));
            PrivateBiomeReference::SetLoaded finish=reinterpret_cast<PrivateBiomeReference::SetLoaded>(AbiExport(nativeApi.ogre,PrivateResourceSetLoadedName,PrivateResourceSetLoadedRva));
            if(loaded(textures[kind].getPointer()))throw std::runtime_error("private painted array loaded before upload");
            uncertain=true;
            if(!write(textures[kind].getPointer(),images[kind]->image,4))throw std::runtime_error("private painted array upload/accounting unverified");
            finish(textures[kind].getPointer());
            if(!loaded(textures[kind].getPointer()))throw std::runtime_error("private painted Loaded state unverified");
            uncertain=false;
        }
        for(unsigned kind=0;kind<2;++kind){delete images[kind];images[kind]=NULL;}
        std::ostringstream message;message<<"private painted arrays loaded:4pairs core2,2; RGBA mip0; sourceBytes="<<encodedBytes<<" decodedBytes="<<decodedBytes;Log(message.str());
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
        fp->setNamedConstant("paintedTileBase",Ogre::Vector4(-73728,-73728,0,0));
        fp->setNamedAutoConstant("viewport",Ogre::GpuProgramParameters::ACT_VIEWPORT_SIZE);
        fp->setNamedAutoConstant("cameraPos",Ogre::GpuProgramParameters::ACT_CAMERA_POSITION);
        fp->setNamedAutoConstant("farClip",Ogre::GpuProgramParameters::ACT_FAR_CLIP_DISTANCE);
        Log("private painted pixel selection configured; four native18m-border maps, explicit SRV/sampler slots; no biome mixing/water/buildings");
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
    }
};
PrivatePaintedReference& PaintedReference()
{
    static PrivatePaintedReference *owned=new PrivatePaintedReference;
    return *owned; // Never destruct retained Ogre resources after Root shutdown.
}
