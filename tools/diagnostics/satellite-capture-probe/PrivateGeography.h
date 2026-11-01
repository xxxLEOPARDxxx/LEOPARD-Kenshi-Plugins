// Fixed primary1,1 geography trial. No shared ZoneMap callbacks or resource edits.
#include "GeographyInputs.h"
#include "GeographyShader.h"
bool ReadGeographicCell(unsigned index,void* (&ids)[5],BYTE (&tree)[2048],unsigned &bytes)
{
    __try
    {
        const BYTE* base=reinterpret_cast<const BYTE*>(GetModuleHandleA(NULL));
        for(unsigned n=0;n<sizeof(GeoNativeChecks)/sizeof(GeoNativeChecks[0]);++n)
            if(std::memcmp(base+GeoNativeChecks[n].rva,GeoNativeChecks[n].code,24))return false;
        const BYTE* world=*reinterpret_cast<const BYTE*const*>(base+0x2133960);
        const BYTE* map=*reinterpret_cast<const BYTE*const*>(world+0x168220);
        if(*reinterpret_cast<const unsigned*>(map+0x50)!=32||*reinterpret_cast<const unsigned*>(map+0x54)!=32)return false;
        const BYTE* table=*reinterpret_cast<const BYTE*const*>(map+0x58);
        const BYTE* cell=table+(((index/8+8)*32)+(index%8+8))*48;
        const BYTE* source=*reinterpret_cast<const BYTE*const*>(cell+40);
        std::memcpy(ids,cell,40);bytes=((*source&0xe0)>0x20)?2048:1;std::memcpy(tree,source,bytes);
        return world==*reinterpret_cast<const BYTE*const*>(base+0x2133960)&&map==*reinterpret_cast<const BYTE*const*>(world+0x168220)&&
            table==*reinterpret_cast<const BYTE*const*>(map+0x58)&&source==*reinterpret_cast<const BYTE*const*>(cell+40)&&!std::memcmp(ids,cell,40);
    }
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
struct PrivateGeography
{
    Ogre::TexturePtr textures[5];Ogre::HighLevelGpuProgramPtr program;ProbePolicy::FixedProgramCache cache;
    bool uncertain,active;float parameters[900];
    PrivateGeography():uncertain(false),active(false){std::memset(parameters,0,sizeof(parameters));}
    static const char* Name(unsigned bank)
    {static const char* names[]={"LEOPARD_GeographyDiffuse0","LEOPARD_GeographyDiffuse1","LEOPARD_GeographyNormal","LEOPARD_GeographyBlend","LEOPARD_GeographySelector"};return names[bank];}
    static const char* ProgramName(){return "LEOPARD_GeographyFragment";}
    static void Read(const GeoFile& f,const std::string& root,std::vector<BYTE>& bytes)
    {
        if(!f.bytes||f.bytes>SatelliteGeography::CpuLimit)throw std::runtime_error("geography source CPU bound refused");
        struct File{HANDLE h;File():h(INVALID_HANDLE_VALUE){}~File(){if(h!=INVALID_HANDLE_VALUE)CloseHandle(h);}} file;
        const std::string path=PrivateBiomeReference::Canonical(root+"/"+f.path);
        file.h=CreateFileA(path.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);LARGE_INTEGER size={};DWORD read=0;
        if(file.h==INVALID_HANDLE_VALUE||!GetFileSizeEx(file.h,&size)||size.QuadPart!=f.bytes)throw std::runtime_error("geography exact source size/path refused");
        bytes.resize(f.bytes);
        if(!ReadFile(file.h,&bytes[0],f.bytes,&read,NULL)||read!=f.bytes||PrivateBiomeReference::Hash(bytes)!=f.sha256)
            throw std::runtime_error("geography source hash changed");
        SatelliteGeography::Mip mips[9];
        if(f.format&&!SatelliteGeography::Dds(&bytes[0],bytes.size(),f.format,mips))throw std::runtime_error("geography strict DDS shape/layout refused");
    }
    void Parameters(const std::string& root)
    {
        GameData* records[15];static const char* roles[]={"base","slope","vertical","grass","dirt","road"};
        static const char* keys[10][4]={{"tiling X 1","tiling Y 1","tiling X 2","tiling Y 2"},{"tiling X 0","tiling Y 0","tiling X grass","tiling Y grass"},
            {"tiling X dirt","tiling Y dirt","tiling X road","tiling Y road"},{"slope min 1","slope min 2","slope min 3","slope min grass"},
            {"slope max 1","slope max 2","slope max 3","slope max grass"},{"slope fade 1","slope fade 2","slope fade 3","slope fade grass"},
            {"overlay mult vertical","overlay mult grass","overlay mult dirt","overlay mult road"},{NULL,NULL,NULL,NULL},
            {"absorbance 0","absorbance 1","absorbance 2","absorbance grass"},{"absorbance dirt","absorbance road",NULL,NULL}};
        std::memset(parameters,0,sizeof(parameters));
        for(unsigned b=0;b<15;++b)
        {
            GameData* data=ou->gamedata.getData(GeoBiomes[b].sid);records[b]=data;
            if(!data||data->type!=BIOMES||data->stringID!=GeoBiomes[b].sid)throw std::runtime_error("geography active SID missing");
            for(unsigned kind=0;kind<2;++kind)for(unsigned role=0;role<6;++role)
            {
                const std::string suffix=kind?" normal":"";
                std::string path=PrivateBiomeReference::File(data,"texture "+std::string(roles[role])+suffix);
                if(path.empty())path=PrivateBiomeReference::File(data,"texture base"+suffix);
                const unsigned index=kind?46+GeoBiomes[b].normal[role]:GeoBiomes[b].diffuse[role];
                if(path.empty()||_stricmp(PrivateBiomeReference::Canonical(root+"/"+path).c_str(),PrivateBiomeReference::Canonical(root+"/"+GeoFiles[index].path).c_str()))
                    throw std::runtime_error("geography active biome asset mapping differs");
            }
            for(unsigned row=0;row<10;++row)for(unsigned c=0;c<4;++c)
                if(keys[row][c])parameters[b*60+row*4+c]=PrivateBiomeReference::Float(data,keys[row][c])*((row>=3&&row<=5)?0.01f:1.0f);
            parameters[b*60+40]=PrivateBiomeReference::Float(data,"brightness fix");
            for(unsigned role=0;role<6;++role){parameters[b*60+44+role]=float(GeoBiomes[b].diffuse[role]);parameters[b*60+52+role]=float(GeoBiomes[b].normal[role]);}
        }
        for(unsigned c=0;c<64;++c)
        {
            void* ids[5];BYTE tree[2048];unsigned count=0;
            if(!ReadGeographicCell(c,ids,tree,count))throw std::runtime_error("geography guarded native cell read refused");
            for(unsigned slot=0;slot<5;++slot)
                if(ids[slot]!=(GeoSlots[c][slot]?records[GeoSlots[c][slot]-1]:NULL))throw std::runtime_error("geography active coarse SID/channel table differs");
            std::vector<BYTE> mask(tree,tree+count);
            if(PrivateBiomeReference::Hash(mask)!=GeoTreeHashes[c])throw std::runtime_error("geography active fine mask tree differs");
        }
    }
    ID3D11ShaderResourceView* View(unsigned bank)
    {
        if(!OnMain()||!RenderAbiMatches()||!IsPrivateArrayTexture(textures[bank].get())||textures[bank]->getName()!=Name(bank))
            throw std::runtime_error("geography owned D3D texture identity differs");
        typedef Ogre::ResourceManager* (__fastcall* Creator)(Ogre::Resource*);
        PrivateBiomeReference::IsLoaded loaded=reinterpret_cast<PrivateBiomeReference::IsLoaded>(AbiExport(nativeApi.ogre,PrivateResourceLoadedName,PrivateResourceLoadedRva));
        if(loaded(textures[bank].get())||reinterpret_cast<Creator>(AbiExport(nativeApi.ogre,PrivateResourceCreatorName,PrivateResourceCreatorRva))(textures[bank].get())!=
           static_cast<Ogre::ResourceManager*>(&Ogre::TextureManager::getSingleton()))throw std::runtime_error("geography pre-upload creator/Loaded refused");
        typedef ID3D11ShaderResourceView* (__fastcall* Getter)(Ogre::Texture*);
        return reinterpret_cast<Getter>(reinterpret_cast<BYTE*>(nativeApi.d3d11)+PrivateArraySrvGetterRva)(textures[bank].get());
    }
    void Notify(unsigned bank)
    {
        typedef Ogre::ResourceManager* (__fastcall* Creator)(Ogre::Resource*);
        typedef void (__fastcall* Notify)(Ogre::ResourceManager*,Ogre::Resource*);
        Ogre::Resource* resource=textures[bank].get();
        PrivateBiomeReference::IsLoaded loaded=reinterpret_cast<PrivateBiomeReference::IsLoaded>(AbiExport(nativeApi.ogre,PrivateResourceLoadedName,PrivateResourceLoadedRva));
        Ogre::ResourceManager* creator=reinterpret_cast<Creator>(AbiExport(nativeApi.ogre,PrivateResourceCreatorName,PrivateResourceCreatorRva))(resource);
        if(loaded(resource)||creator!=static_cast<Ogre::ResourceManager*>(&Ogre::TextureManager::getSingleton()))throw std::runtime_error("geography credit ownership/state refused");
        reinterpret_cast<Notify>(AbiExport(nativeApi.ogre,PrivateResourceNotifyLoadedName,PrivateResourceNotifyLoadedRva))(creator,resource);
    }
    struct BankOperations
    {
        PrivateGeography& owner;unsigned bank;const std::string& root;Ogre::Image& blend;
        const char* stage;unsigned layer;HRESULT result;SatelliteGeography::UploadDescription description;
        BankOperations(PrivateGeography& value,unsigned index,const std::string& path,Ogre::Image& image):owner(value),bank(index),root(path),blend(image),stage("availability"),layer(~0u),result(S_OK){}
        bool Available(){stage="availability";return owner.textures[bank].isNull()&&!Ogre::TextureManager::getSingleton().resourceExists(Name(bank));}
        void Create()
        {
            stage="allocation";
            if(bank<3)
            {const unsigned slices=bank==0?24:bank==1?22:46;owner.textures[bank]=Ogre::TextureManager::getSingleton().createManual(Name(bank),"Landscape",Ogre::TEX_TYPE_2D_ARRAY,2048,2048,slices,SatelliteGeography::RequestedCompressedAdditionalMips(),bank==2?Ogre::PF_DXT1:Ogre::PF_DXT5,0x15,NULL);}
            else
            {const unsigned side=bank==3?1024:8,levels=bank==3?11:1;const Ogre::PixelFormat format=bank==3?blend.getFormat():Ogre::PF_A8B8G8R8;
             owner.textures[bank]=Ogre::TextureManager::getSingleton().createManual(Name(bank),"Landscape",Ogre::TEX_TYPE_2D,side,side,1,levels-1,format,0x15,NULL);}
        }
        bool Preflight()
        {
            stage="creator/Loaded preflight";if(owner.textures[bank].isNull())return false;
            ID3D11ShaderResourceView* view=owner.View(bank);stage="descriptor snapshot";
            result=SatelliteGeography::DescribeUpload(view,description);return SUCCEEDED(result);
        }
        bool Upload()
        {
            stage="GPU upload";
            if(bank>=3)
            {const unsigned side=bank==3?1024:8,levels=bank==3?11:1;const Ogre::PixelFormat format=bank==3?blend.getFormat():Ogre::PF_A8B8G8R8;
             result=SatelliteGeography::UploadRgba2D(owner.View(bank),side,levels,format==Ogre::PF_A8R8G8B8?87:28,bank==3?blend.getData():GeoSelector,bank==3?blend.getSize():sizeof(GeoSelector));return SUCCEEDED(result);}
            unsigned uploaded=0;const unsigned slices=bank==0?24:bank==1?22:46;
            for(unsigned f=0;f<92;++f)if(GeoFiles[f].bank==bank)
            {
                layer=GeoFiles[f].layer;stage="source hash/layout";
                std::vector<BYTE> source;Read(GeoFiles[f],root,source);SatelliteGeography::Mip mips[9];
                if(!SatelliteGeography::Dds(&source[0],source.size(),GeoFiles[f].format,mips)){result=E_INVALIDARG;return false;}
                stage="GPU upload";result=SatelliteGeography::UploadCompressedLayer(owner.View(bank),slices,layer,GeoFiles[f].format,mips);
                if(FAILED(result))return false;
                ++uploaded;
            }
            return uploaded==slices;
        }
        void Notify(){stage="native notify";owner.Notify(bank);}
        void SetLoaded(){stage="native setLoaded";reinterpret_cast<PrivateBiomeReference::SetLoaded>(AbiExport(nativeApi.ogre,PrivateResourceSetLoadedName,PrivateResourceSetLoadedRva))(owner.textures[bank].get());}
        bool Loaded(){stage="Loaded verification";return reinterpret_cast<PrivateBiomeReference::IsLoaded>(AbiExport(nativeApi.ogre,PrivateResourceLoadedName,PrivateResourceLoadedRva))(owner.textures[bank].get());}
        void Failure()const
        {
            std::ostringstream s;s<<"SatelliteCaptureProbe: geography bank="<<bank<<" layer="<<layer<<" stage="<<stage<<" HRESULT=0x"<<std::hex<<static_cast<unsigned long>(result)<<std::dec
                <<" uncertain="<<owner.uncertain<<" hasView="<<description.hasView<<" hasTexture="<<description.hasTexture;
            if(description.hasView){const D3D11_SHADER_RESOURCE_VIEW_DESC& v=description.view;s<<" SRV(format,dimension,mip0,levels,slice0,slices)="<<v.Format<<','<<v.ViewDimension<<','<<v.Texture2DArray.MostDetailedMip<<','<<v.Texture2DArray.MipLevels<<','<<v.Texture2DArray.FirstArraySlice<<','<<v.Texture2DArray.ArraySize;}
            if(description.hasTexture){const D3D11_TEXTURE2D_DESC& d=description.texture;s<<" Texture(width,height,levels,slices,format,samples,quality,usage,bind,cpu,misc)="<<d.Width<<','<<d.Height<<','<<d.MipLevels<<','<<d.ArraySize<<','<<d.Format<<','<<d.SampleDesc.Count<<','<<d.SampleDesc.Quality<<','<<d.Usage<<','<<d.BindFlags<<','<<d.CPUAccessFlags<<','<<d.MiscFlags;}
            ErrorLog(s.str());
        }
    };
    void Begin(const std::string& root,Ogre::CompositorManager2* owner)
    {
        if(uncertain||active||!OnMain()||!LoadedGuard()||!RenderAbiMatches())throw std::runtime_error("geography context/quarantine refused");
        for(unsigned b=0;b<5;++b)if(!textures[b].isNull()||Ogre::TextureManager::getSingleton().resourceExists(Name(b)))throw std::runtime_error("geography name/ownership collision");
        MEMORYSTATUSEX memory={sizeof(MEMORYSTATUSEX)};
        if(!GlobalMemoryStatusEx(&memory)||memory.ullAvailPhys<2*SatelliteGeography::GpuLimit||memory.ullAvailPageFile<2*SatelliteGeography::GpuLimit||
            !SatelliteGeography::Combined(0,64ULL*1024*1024))throw std::runtime_error("geography host/combined budget refused before allocation");
        Parameters(root);
        unsigned expectedLayers[3]={0,0,0};
        for(unsigned f=0;f<92;++f)
        {
            const GeoFile& input=GeoFiles[f];
            if(input.bank>=3||input.layer!=expectedLayers[input.bank]++||input.format!=(input.bank==2?71u:77u))throw std::runtime_error("geography bank manifest order differs");
        }
        if(expectedLayers[0]!=24||expectedLayers[1]!=22||expectedLayers[2]!=46)throw std::runtime_error("geography incomplete bank manifest refused before allocation");
        for(unsigned f=0;f<92;++f){std::vector<BYTE> source;Read(GeoFiles[f],root,source);}
        std::vector<BYTE> blendBytes;Read(GeoBlend,root,blendBytes);Ogre::Image blend;
        Ogre::DataStreamPtr stream(new Ogre::MemoryDataStream(&blendBytes[0],blendBytes.size(),false,true));blend.load(stream,"png");
        if(!SatellitePaintedMap::ImageShape(0,blend.getWidth(),blend.getHeight(),blend.getDepth(),blend.getNumFaces(),blend.getNumMipmaps(),blend.getFormat(),blend.getSize()))
            throw std::runtime_error("geography blend PNG shape refused");
        const Ogre::PixelBox pixels=blend.getPixelBox();
        if(pixels.data!=blend.getData()||pixels.rowPitch!=1024||pixels.slicePitch!=1024*1024||pixels.front||pixels.back!=1)
            throw std::runtime_error("geography blend PNG tight layout refused");
        for(unsigned bank=0;bank<5;++bank)
        {
            BankOperations operations(*this,bank,root,blend);
            bool completed=false;
            try{completed=SatelliteGeography::CompleteBank(operations,uncertain);}catch(...){operations.Failure();throw;}
            if(!completed){operations.Failure();throw std::runtime_error("geography bank allocation/upload/native completion refused");}
        }
        Ogre::HighLevelGpuProgramManager& manager=Ogre::HighLevelGpuProgramManager::getSingleton();
        Ogre::HighLevelGpuProgramPtr existing;if(manager.resourceExists(ProgramName()))existing=manager.getByName(ProgramName());
        ProbePolicy::FixedProgramCache::Choice choice=reusePrivateShaders?cache.Inspect(owner,0,existing.get(),GeographyShaderSource):
            (existing.isNull()?ProbePolicy::FixedProgramCache::Create:ProbePolicy::FixedProgramCache::Refuse);
        if(choice==ProbePolicy::FixedProgramCache::Refuse)throw std::runtime_error("geography shader identity refused");
        if(choice==ProbePolicy::FixedProgramCache::Reuse)program=existing;
        else
        {
            program=manager.createProgram(ProgramName(),"Landscape","hlsl",Ogre::GPT_FRAGMENT_PROGRAM);program->setSource(GeographyShaderSource);
            program->setParameter("entry_point","main_fs");program->setParameter("target","ps_4_0");program->setParameter("enable_backwards_compatibility","true");program->setParameter("preprocessor_defines","DX11=1");program->load();
            if(program->hasCompileError()||!program->isSupported())throw std::runtime_error("geography shader compilation failed");
            if(reusePrivateShaders&&!cache.Record(owner,0,program.get(),GeographyShaderSource))throw std::runtime_error("geography shader cache refused");
        }
        active=true;Log("geography pools loaded:15SIDs 92DDS 39ordered layouts 3banks levels9; DDSgpuBytes=385874496; zero-distortion/fade isolation");
    }
    void Bind(const Ogre::MaterialPtr& material)
    {
        Ogre::Pass* pass=material->getTechnique(0)->getPass(0);
        if(!active||program.isNull()||pass->getNumTextureUnitStates()!=5)throw std::runtime_error("geography binding state refused");
        pass->setFragmentProgram(ProgramName());pass->getTextureUnitState(0)->setTexture(textures[0]);pass->getTextureUnitState(1)->setTexture(textures[2]);
        for(unsigned kind=0;kind<2;++kind)
        {
            if(PaintedReference().textures[kind].isNull())throw std::runtime_error("geography painted array absent");
            Ogre::TextureUnitState* unit=pass->getTextureUnitState(kind+2);unit->setContentType(Ogre::TextureUnitState::CONTENT_NAMED);
            unit->setTexture(PaintedReference().textures[kind]);unit->setTextureAddressingMode(Ogre::TextureUnitState::TAM_WRAP);unit->setTextureFiltering(Ogre::TFO_TRILINEAR);
        }
        pass->getTextureUnitState(0)->setTextureFiltering(Ogre::TFO_ANISOTROPIC);
        pass->getTextureUnitState(4)->setContentType(Ogre::TextureUnitState::CONTENT_NAMED);pass->getTextureUnitState(4)->setTextureName("nothing.dds");pass->getTextureUnitState(4)->setTextureFiltering(Ogre::TFO_NONE);
        for(unsigned t=5;t<8;++t){Ogre::TextureUnitState* unit=pass->createTextureUnitState();unit->setTexture(textures[t==5?3:t==6?4:1]);unit->setTextureFiltering(t==6?Ogre::TFO_NONE:Ogre::TFO_TRILINEAR);}
        Ogre::GpuProgramParametersSharedPtr vp=pass->getVertexProgramParameters();vp->setNamedConstant("biomeData",Ogre::Vector4(-147456,-147456,147456,147456));
        vp->setNamedConstant("overlayData",Ogre::Vector4(-147456,-147456,147456,147456));vp->setNamedConstant("distortion0",Ogre::Vector4::ZERO);vp->setNamedConstant("distortion1",Ogre::Vector4::ZERO);
        Ogre::GpuProgramParametersSharedPtr fp=pass->getFragmentProgramParameters();fp->removeAllSharedParameters();fp->setNamedConstant("satBiome",parameters,225,4);
        fp->setNamedAutoConstant("viewport",Ogre::GpuProgramParameters::ACT_VIEWPORT_SIZE);fp->setNamedAutoConstant("cameraPos",Ogre::GpuProgramParameters::ACT_CAMERA_POSITION);fp->setNamedAutoConstant("farClip",Ogre::GpuProgramParameters::ACT_FAR_CLIP_DISTANCE);
        fp->setNamedConstant("paintedTileBase",Ogre::Vector4(-73728,-73728,0,0));fp->setNamedConstant("waterHeightRel",-100000.0f);fp->setNamedConstant("wetness",0.0f);
        Log("geographic per-fragment coarse root/channel selection bound; 8SRVs native residual weights; no patch-center substitution");
    }
    void Destroy()
    {
        if(uncertain)throw std::runtime_error("geography native allocation/credit uncertain; bounded quarantine retained");
        for(unsigned b=0;b<5;++b)if(!textures[b].isNull())
        {Ogre::TextureManager::getSingleton().remove(Name(b));if(nativeApi.useCount(&textures[b])!=1)throw std::runtime_error("geography texture still borrowed");textures[b].setNull();}
        if(!program.isNull()){if(!cache.Owns(0,program.get()))Ogre::HighLevelGpuProgramManager::getSingleton().remove(ProgramName());program.setNull();}
        active=false;
    }
};
PrivateGeography& GeographyReference(){static PrivateGeography* owned=new PrivateGeography;return *owned;}
