// Private single-biome material reference only. Not a whole-map biome renderer.
// Exact native array upload, never the cubemap-oriented Texture::_loadImages.
#include "PrivateBiomeDds.h"
typedef char PrivateBiomeImageStrideMustBe64[sizeof(Ogre::Image)==64?1:-1];
bool IsPrivateArrayTexture(const Ogre::Texture *texture)
{
    __try
    {
        const BYTE *base=reinterpret_cast<const BYTE*>(nativeApi.d3d11);
        const void *table=*reinterpret_cast<const void*const*>(texture);
        return table==base+PrivateArrayTextureVtableRva&&
            *reinterpret_cast<const void*const*>(static_cast<const BYTE*>(table)+0x2d8)==base+PrivateArrayUploadRva;
    }
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
struct PrivateBiomeReference
{
    typedef bool (__fastcall *WriteArray)(Ogre::Texture*,const Ogre::Image*,int);
    typedef bool (__fastcall *IsLoaded)(const Ogre::Resource*);
    typedef void (__fastcall *SetLoaded)(Ogre::Resource*);
    struct ImageSet
    {
        Ogre::Image images[6];std::vector<BYTE> encoded[6];std::string name;
    };
    ImageSet *loaders[2];Ogre::TexturePtr textures[2];bool uncertain;
    PrivateBiomeReference():uncertain(false){loaders[0]=loaders[1]=NULL;}
    static float Float(const GameData *data,const char *key)
    {
        const auto i=data->fdata.find(key);
        const float value=i==data->fdata.end()?0:i->second;
        if(!_finite(value))throw std::runtime_error("nonfinite private biome parameter");
        return value;
    }
    static std::string File(const GameData *data,const std::string &key)
    {
        const auto i=data->filesdata.find(key);
        return i==data->filesdata.end()?std::string():i->second;
    }
    static std::string Canonical(const std::string &path)
    {
        char resolved[MAX_PATH]={0};DWORD n=GetFullPathNameA(path.c_str(),MAX_PATH,resolved,NULL);
        if(!n||n>=MAX_PATH)throw std::runtime_error("private DDS path length refused");
        return std::string(resolved,n);
    }
    static std::string Hash(const std::vector<BYTE> &bytes)
    {
        HCRYPTPROV provider=0;HCRYPTHASH hash=0;BYTE digest[32];DWORD n=32;
        bool ok=CryptAcquireContextA(&provider,NULL,NULL,PROV_RSA_AES,CRYPT_VERIFYCONTEXT)!=FALSE;
        ok=ok&&CryptCreateHash(provider,CALG_SHA_256,0,0,&hash)!=FALSE;
        ok=ok&&CryptHashData(hash,&bytes[0],static_cast<DWORD>(bytes.size()),0)!=FALSE;
        ok=ok&&CryptGetHashParam(hash,HP_HASHVAL,digest,&n,0)!=FALSE;
        if(hash)CryptDestroyHash(hash);if(provider)CryptReleaseContext(provider,0);
        if(!ok)throw std::runtime_error("private DDS checksum failed");
        std::string value;const char *hex="0123456789abcdef";
        for(unsigned i=0;i<32;++i){value+=hex[digest[i]>>4];value+=hex[digest[i]&15];}
        return value;
    }
    static void ReadDds(const std::string &path,const std::string &gameRoot,std::vector<BYTE> &encoded,size_t &total)
    {
        // This first reference is deliberately restricted to twelve profiled DDS
        // identities. Size/hash are checked BEFORE Ogre decodes anything; future
        // general modded inputs need an independently bounded DDS parser/resolver.
        const std::string full=Canonical(gameRoot+"/"+path);
        const PrivateBiomeDds *expected=NULL;
        for(unsigned i=0;i<sizeof(PrivateBiomeDdsFiles)/sizeof(PrivateBiomeDdsFiles[0]);++i)
            if(_stricmp(full.c_str(),Canonical(gameRoot+"/"+PrivateBiomeDdsFiles[i].path).c_str())==0)
            {expected=&PrivateBiomeDdsFiles[i];break;}
        if(!expected||!SatelliteTextureArray::AddBytes(total,expected->bytes))
            throw std::runtime_error("private DDS identity/aggregate preflight refused before allocation");
        struct FileHandle { HANDLE h;FileHandle():h(INVALID_HANDLE_VALUE){}~FileHandle(){if(h!=INVALID_HANDLE_VALUE)CloseHandle(h);} } file;
        file.h=CreateFileA(full.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
        LARGE_INTEGER bytes={0};
        if(file.h==INVALID_HANDLE_VALUE||!GetFileSizeEx(file.h,&bytes)||bytes.QuadPart!=expected->bytes)
            throw std::runtime_error("private DDS exact size/path preflight refused");
        encoded.resize(expected->bytes);DWORD got=0;
        if(!ReadFile(file.h,&encoded[0],static_cast<DWORD>(encoded.size()),&got,NULL)||got!=encoded.size()||Hash(encoded)!=expected->sha256)
            throw std::runtime_error("private DDS content preflight refused before decode");
    }
    void Begin(const GameData *data,const std::string &gameRoot)
    {
        if(uncertain||loaders[0]||loaders[1]||!textures[0].isNull()||!textures[1].isNull())
            throw std::runtime_error("private biome reference still owns resources");
        if(!data||data->type!=BIOMES||data->stringID!="5611-Newwworld.mod")
            throw std::runtime_error("private biome exact reference record absent");
        static const char *keys[]={"texture base","texture slope","texture vertical","texture grass","texture dirt","texture road"};
        size_t total=0,decodedTotal=0;
        for(unsigned kind=0;kind<2;++kind)
        {
            loaders[kind]=new ImageSet;ImageSet &loader=*loaders[kind];
            loader.name=kind?"LEOPARD_PrivateBiomeReferenceNormals":"LEOPARD_PrivateBiomeReferenceDiffuse";
            if(Ogre::TextureManager::getSingleton().resourceExists(loader.name))
                throw std::runtime_error("private biome reference texture-name collision");
            const std::string suffix=kind?" normal":"";
            const std::string base=File(data,std::string(keys[0])+suffix);
            if(base.empty())throw std::runtime_error("private biome base texture missing");
            SatelliteTextureArray::Shape first={0};
            for(unsigned layer=0;layer<6;++layer)
            {
                std::string path=File(data,std::string(keys[layer])+suffix);
                // Native texture-array data helper falls back to the base layer
                // for an empty layer. A nonempty unresolved path is a hard error.
                if(path.empty())path=base;
                ReadDds(path,gameRoot,loader.encoded[layer],total);
                Ogre::DataStreamPtr stream(new Ogre::MemoryDataStream(&loader.encoded[layer][0],loader.encoded[layer].size(),false,true));
                loader.images[layer].load(stream,"dds");
                Ogre::Image &image=loader.images[layer];
                SatelliteTextureArray::Shape shape={image.getWidth(),image.getHeight(),image.getDepth(),
                    image.getNumMipmaps(),static_cast<unsigned>(image.getFormat()),image.getSize()};
                if(!SatelliteTextureArray::Accept(shape)||shape.mips<3||!SatelliteTextureArray::AddBytes(decodedTotal,shape.bytes)||
                   (layer&&!SatelliteTextureArray::Match(first,shape)))
                    throw std::runtime_error("private biome texture-array shape/format/mip/byte budget refused");
                if(!layer)first=shape;
                std::ostringstream s;s<<"private biome source kind="<<kind<<" layer="<<layer<<" file="<<path
                    <<" dimensions="<<shape.width<<'x'<<shape.height<<" mips="<<shape.mips<<" format="<<shape.format;Log(s.str());
            }
        }
        // Both CPU arrays are validated before either GPU allocation. Follow
        // native createTextureArray: sourceMips-3, usage0x15, then direct upload
        // outside Resource::load (writeTextureArray sends its own manager credit).
        for(unsigned kind=0;kind<2;++kind)
        {
            ImageSet &loader=*loaders[kind];Ogre::Image &first=loader.images[0];
            uncertain=true;
            textures[kind]=Ogre::TextureManager::getSingleton().createManual(loader.name,
                "Landscape",Ogre::TEX_TYPE_2D_ARRAY,first.getWidth(),first.getHeight(),6,
                first.getNumMipmaps()-3,first.getFormat(),0x15,NULL);
            if(textures[kind].isNull())throw std::runtime_error("private biome array constructor returned null");
            uncertain=false;
            if(!OnMain()||!RenderAbiMatches()||!IsPrivateArrayTexture(textures[kind].getPointer())||
               textures[kind]->getDepth()!=6||textures[kind]->getTextureType()!=Ogre::TEX_TYPE_2D_ARRAY||
               textures[kind]->getNumMipmaps()>first.getNumMipmaps())
                throw std::runtime_error("private biome native array type/shape unverified");
            WriteArray write=reinterpret_cast<WriteArray>(reinterpret_cast<BYTE*>(nativeApi.d3d11)+PrivateArrayUploadRva);
            IsLoaded loaded=reinterpret_cast<IsLoaded>(AbiExport(nativeApi.ogre,PrivateResourceLoadedName,PrivateResourceLoadedRva));
            SetLoaded finish=reinterpret_cast<SetLoaded>(AbiExport(nativeApi.ogre,PrivateResourceSetLoadedName,PrivateResourceSetLoadedRva));
            if(loaded(textures[kind].getPointer()))
                throw std::runtime_error("private biome array unexpectedly loaded before direct upload");
            uncertain=true;
            if(!write(textures[kind].getPointer(),loader.images,6))
                throw std::runtime_error("private biome array native upload/accounting unverified");
            // Native writer credits the manager but omits Loaded state. Without
            // this owned-only completion, destruction frees GPU memory while
            // skipping Resource::unload's matching accounting debit. This exact
            // export sets only state=Loaded; it sends no second manager credit.
            finish(textures[kind].getPointer());
            if(!loaded(textures[kind].getPointer()))
                throw std::runtime_error("private biome array loaded-state completion unverified");
            uncertain=false;
        }
        // The native method retains no images or loader callback.
        for(unsigned kind=0;kind<2;++kind){delete loaders[kind];loaders[kind]=NULL;}
        std::ostringstream s;s<<"private biome reference arrays loaded sid="<<data->stringID<<" sourceBytes="<<total<<" decodedBytes="<<decodedTotal;Log(s.str());
    }
    void Configure(const GameData *data,const Ogre::MaterialPtr &material)
    {
        if(textures[0].isNull()||textures[1].isNull())throw std::runtime_error("private biome arrays not loaded");
        Ogre::Pass *pass=material->getTechnique(0)->getPass(0);
        if(pass->getNumTextureUnitStates()!=5)throw std::runtime_error("native unblended Terrain material differs");
        pass->getTextureUnitState(0)->setTexture(textures[0]);pass->getTextureUnitState(1)->setTexture(textures[1]);
        for(unsigned i=2;i<5;++i)
        {
            Ogre::TextureUnitState *unit=pass->getTextureUnitState(i);
            unit->setContentType(Ogre::TextureUnitState::CONTENT_NAMED);
            unit->setTextureName(i==3?"white.dds":"nothing.dds");
        }
        Ogre::GpuProgramParametersSharedPtr vp=pass->getVertexProgramParameters();
        vp->setNamedConstant("biomeData",Ogre::Vector4(-147456,-147456,147456,147456));
        vp->setNamedConstant("overlayData",Ogre::Vector4(-147456,-147456,147456,147456));
        vp->setNamedConstant("distortion0",Ogre::Vector4::ZERO);vp->setNamedConstant("distortion1",Ogre::Vector4::ZERO);
        Ogre::GpuProgramParametersSharedPtr fp=pass->getFragmentProgramParameters();fp->removeAllSharedParameters();
        fp->setNamedConstant("scalesA",Ogre::Vector4(Float(data,"tiling X 1"),Float(data,"tiling Y 1"),Float(data,"tiling X 2"),Float(data,"tiling Y 2")));
        fp->setNamedConstant("scalesB",Ogre::Vector4(Float(data,"tiling X 0"),Float(data,"tiling Y 0"),Float(data,"tiling X grass"),Float(data,"tiling Y grass")));
        fp->setNamedConstant("scalesC",Ogre::Vector4(Float(data,"tiling X dirt"),Float(data,"tiling Y dirt"),Float(data,"tiling X road"),Float(data,"tiling Y road")));
        fp->setNamedConstant("slopeMin",Ogre::Vector4(Float(data,"slope min 1"),Float(data,"slope min 2"),Float(data,"slope min 3"),Float(data,"slope min grass"))*.01f);
        fp->setNamedConstant("slopeMax",Ogre::Vector4(Float(data,"slope max 1"),Float(data,"slope max 2"),Float(data,"slope max 3"),Float(data,"slope max grass"))*.01f);
        fp->setNamedConstant("slopeBlend",Ogre::Vector4(Float(data,"slope fade 1"),Float(data,"slope fade 2"),Float(data,"slope fade 3"),Float(data,"slope fade grass"))*.01f);
        fp->setNamedConstant("overlayMult",Ogre::Vector4(Float(data,"overlay mult vertical"),Float(data,"overlay mult grass"),Float(data,"overlay mult dirt"),Float(data,"overlay mult road")));
        fp->setNamedConstant("textureFade",Ogre::Vector4::ZERO); // Private orbital reference: no distance-to-distant fade.
        float absorbance[]={Float(data,"absorbance 0"),Float(data,"absorbance 1"),Float(data,"absorbance 2"),Float(data,"absorbance grass"),Float(data,"absorbance dirt"),Float(data,"absorbance road"),0,0};
        fp->setNamedConstant("absorbance",absorbance,2,4);
        fp->setNamedConstant("brightnessFix",Ogre::Vector4(Float(data,"brightness fix"),1,1,1));
        fp->setNamedConstant("waterHeightRel",-100000.0f);fp->setNamedConstant("wetness",0.0f);
        Log("private single-biome reference configured; neutral overlay/colour, no biome mixing/water/cliff distortion; not final imagery");
    }
    void Destroy()
    {
        if(uncertain)throw std::runtime_error("private biome constructor uncertain; retaining bounded arrays/loaders");
        for(unsigned kind=0;kind<2;++kind)
            if(!textures[kind].isNull())
            {
                Ogre::TextureManager::getSingleton().remove(textures[kind]->getName());
                if(nativeApi.useCount(&textures[kind])!=1)
                    throw std::runtime_error("private biome array still borrowed; retaining loader");
            }
        for(unsigned kind=0;kind<2;++kind)
        {textures[kind].setNull();delete loaders[kind];loaders[kind]=NULL;}
    }
};
PrivateBiomeReference& BiomeReference()
{
    static PrivateBiomeReference *owned=new PrivateBiomeReference;
    return *owned; // Bounded quarantine must not destruct after Ogre Root shutdown.
}
