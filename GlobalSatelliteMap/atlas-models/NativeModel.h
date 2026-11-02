#pragma once
// Root calls only at a serialized render boundary with a resolved live Town.
// Packet stores raw scene-space matrix and SAME-snapshot origin separately.
#include "ModelAdmission.h"
#include "../atlas-water/NativeWaterSnapshot.h"
#include <kenshi/Town.h>
#include <OgreEntity.h>
#include <OgreSubEntity.h>
#include <OgreSubMesh.h>
#include <OgreMesh.h>
#include <OgreRenderOperation.h>
#include "_generated/ModelProfile.h"
#include "_generated/ModelShaders.h"
namespace NativeModels {
inline bool Typed(const void*p,HMODULE h,DWORD vt){const void*v=NULL;return h&&Copy(p,&v,sizeof(v))&&v==reinterpret_cast<const BYTE*>(h)+vt;}
// Serialized render-thread cache. Root resets at world/module lifetime boundaries.
// A failed generation is remembered, so a poisoned profile is never retried hot.
struct ProfileCache {
    FARPROC model[sizeof(ModelExportPins)/sizeof(ModelExportPins[0])];
    FARPROC water[sizeof(NativeWater::ExportPins)/sizeof(NativeWater::ExportPins[0])];
    HMODULE ogre,d3d;unsigned generation;bool attempted,ready;
    ProfileCache():ogre(NULL),d3d(NULL),generation(0),attempted(false),ready(false){std::memset(model,0,sizeof(model));std::memset(water,0,sizeof(water));}
};
inline ProfileCache&Dispatch(){static ProfileCache cache;return cache;}
inline void ResetProfile(){Dispatch()=ProfileCache();}
inline void Profile(){
    ProfileCache&c=Dispatch();if(c.ready)return;if(c.attempted)throw std::runtime_error("model native profile already refused");c.attempted=true;
    NativeWater::Profile();if(!GsmNative::Module(GetModuleHandleA(NULL),ModelExeTimestamp,ModelExeSize,ModelExeWindows,sizeof(ModelExeWindows)/sizeof(ModelExeWindows[0]))||!GsmNative::Module(GetModuleHandleA(GsmOgreFile),ModelOgreTimestamp,ModelOgreSize,ModelOgreWindows,sizeof(ModelOgreWindows)/sizeof(ModelOgreWindows[0]))||!GsmNative::Module(GetModuleHandleA(GsmD3d11File),ModelD3dTimestamp,ModelD3dSize,ModelD3dWindows,sizeof(ModelD3dWindows)/sizeof(ModelD3dWindows[0])))throw std::runtime_error("model native module/layout differs");
    for(unsigned i=0;i<sizeof(ModelExportPins)/sizeof(ModelExportPins[0]);++i){const ModelExportPin&p=ModelExportPins[i];FARPROC f=GsmNative::Export(p.name,p.rva);if(!NativeWater::CodeMatches(f,p.bytes))throw std::runtime_error("model native export bytes differ");c.model[i]=f;}
    for(unsigned i=0;i<sizeof(NativeWater::ExportPins)/sizeof(NativeWater::ExportPins[0]);++i)c.water[i]=NativeWater::Pinned(NativeWater::ExportPins[i].rva);
    c.ogre=GetModuleHandleA(GsmOgreFile);c.d3d=GetModuleHandleA(GsmD3d11File);c.ready=true;
}
inline void Profile(unsigned generation){ProfileCache&c=Dispatch();if(c.generation!=generation){ResetProfile();Dispatch().generation=generation;}Profile();}
inline FARPROC Pin(const char*prefix){Profile();ProfileCache&c=Dispatch();for(unsigned i=0;i<sizeof(ModelExportPins)/sizeof(ModelExportPins[0]);++i)if(!std::strncmp(ModelExportPins[i].name,prefix,std::strlen(prefix)))return c.model[i];throw std::runtime_error("model native export not pinned");}
inline FARPROC WaterPin(DWORD rva){Profile();for(unsigned i=0;i<sizeof(NativeWater::ExportPins)/sizeof(NativeWater::ExportPins[0]);++i)if(NativeWater::ExportPins[i].rva==rva)return Dispatch().water[i];throw std::runtime_error("model water export not pinned");}
// Same already-pinned ABI as NativeWater::Sampler, without export/code rescans.
inline void CachedSampler(Ogre::TextureUnitState*u,D3D11_SAMPLER_DESC&out){
    typedef const Ogre::TextureUnitState::UVWAddressingMode&(__fastcall*Address)(const Ogre::TextureUnitState*);
    typedef unsigned(__fastcall*Anisotropy)(const Ogre::TextureUnitState*);
    typedef const Ogre::ColourValue&(__fastcall*Border)(const Ogre::TextureUnitState*);
    typedef bool(__fastcall*Compare)(const Ogre::TextureUnitState*);
    typedef Ogre::FilterOptions(__fastcall*Filter)(const Ogre::TextureUnitState*,Ogre::FilterType);
    typedef float(__fastcall*Bias)(const Ogre::TextureUnitState*);
    const Ogre::TextureUnitState::UVWAddressingMode&a=reinterpret_cast<Address>(WaterPin(0x37d200))(u);unsigned modes[3]={unsigned(a.u),unsigned(a.v),unsigned(a.w)};
    const Ogre::ColourValue&b=reinterpret_cast<Border>(WaterPin(0x37d270))(u);float border[4]={b.r,b.g,b.b,b.a};Filter filter=reinterpret_cast<Filter>(WaterPin(0x37e390));
    if(!NativeWater::BuildSampler(filter(u,Ogre::FT_MIN),filter(u,Ogre::FT_MAG),filter(u,Ogre::FT_MIP),modes,reinterpret_cast<Anisotropy>(WaterPin(0x37e460))(u),border,reinterpret_cast<Bias>(WaterPin(0x3b0e0))(u),reinterpret_cast<Compare>(WaterPin(0x37e420))(u),out))throw std::runtime_error("model native sampler unsupported");
}
inline ID3D11ShaderResourceView*CachedSrv(Ogre::Texture*t){if(!GsmNative::TextureType(t))throw std::runtime_error("model texture identity differs");typedef ID3D11ShaderResourceView*(__fastcall*Fn)(Ogre::Texture*);ID3D11ShaderResourceView*v=reinterpret_cast<Fn>(reinterpret_cast<BYTE*>(Dispatch().d3d)+GsmSrvGetter)(t);if(!v)throw std::runtime_error("model existing texture SRV missing");return v;}
// BEGIN EXACT REFRESH DESCRIPTOR
// No native object is retained. COM identity tokens remain alive through Packet.
struct Descriptor {
    AtlasModels::RawDraw raw[16];D3D11_SAMPLER_DESC sampler[16];
    ULONG_PTR identity[16][18],device,material;unsigned count,generation;
    float matrix[16],origin[3],bounds[6];
    Descriptor(){std::memset(this,0,sizeof(*this));}
};
inline bool SameDescriptor(const Descriptor&a,const Descriptor&b){
    if(a.count!=b.count||a.generation!=b.generation||a.device!=b.device||a.material!=b.material||!a.count||a.count>16||std::memcmp(a.matrix,b.matrix,sizeof(a.matrix))||std::memcmp(a.origin,b.origin,sizeof(a.origin))||std::memcmp(a.bounds,b.bounds,sizeof(a.bounds)))return false;
    for(unsigned n=0;n<a.count;++n){const AtlasModels::Shape&x=a.raw[n].shape,&y=b.raw[n].shape;
        if(a.raw[n].cullMode!=b.raw[n].cullMode||std::memcmp(a.identity[n],b.identity[n],sizeof(a.identity[n]))||std::memcmp(&a.sampler[n],&b.sampler[n],sizeof(a.sampler[n])))return false;
        // Shape consists only of 32-bit scalar/array fields (SDK compile guards).
        if(std::memcmp(&x,&y,sizeof(x)))return false;
    }return true;
}
inline ULONG_PTR ComIdentity(IUnknown*p){if(!p)return 0;IUnknown*identity=NULL;HRESULT hr=p->QueryInterface(__uuidof(IUnknown),reinterpret_cast<void**>(&identity));if(FAILED(hr)||!identity)throw std::runtime_error("model COM identity unavailable");const ULONG_PTR value=reinterpret_cast<ULONG_PTR>(identity);identity->Release();return value;}
inline void Identify(Descriptor&d,ID3D11Device*device){
    if(!device||!d.count||d.count>16)throw std::runtime_error("model descriptor count/device invalid");d.device=ComIdentity(device);
    for(unsigned n=0;n<d.count;++n){for(unsigned k=0;k<16;++k)d.identity[n][k]=ComIdentity(d.raw[n].vertex[k]);d.identity[n][16]=ComIdentity(d.raw[n].index);d.identity[n][17]=ComIdentity(d.raw[n].diffuse);}
}
// END EXACT REFRESH DESCRIPTOR
template<class R>R Get(const void*p,const char*name){typedef R(__fastcall*Fn)(const void*);return reinterpret_cast<Fn>(Pin(name))(p);}
template<class R,class A>R Get(const void*p,const char*name,A a){typedef R(__fastcall*Fn)(const void*,A);return reinterpret_cast<Fn>(Pin(name))(p,a);}
inline ID3D11Buffer*Buffer(const void*wrapper,bool index){HMODULE h=GetModuleHandleA(GsmD3d11File);if(!Typed(wrapper,h,index?0x78ca8:0x795f8))throw std::runtime_error("model hardware buffer type differs");void*impl=NULL;if(!Copy(static_cast<const BYTE*>(wrapper)+(index?0x60:0x68),&impl,8)||!Typed(impl,h,0x78aa8))throw std::runtime_error("model hardware buffer implementation differs");typedef ID3D11Buffer*(__fastcall*Fn)(const void*);ID3D11Buffer*b=reinterpret_cast<Fn>(reinterpret_cast<const BYTE*>(h)+(index?0x243a0:0x288e0))(wrapper);if(!b)throw std::runtime_error("model native buffer is absent");return b;}
class Snapshot {
    AtlasModels::Packet packet_;AtlasView::Com<ID3D11SamplerState>samplers_[16];AtlasView::Com<ID3D11Device>device_;float localBounds_[6];Town*town_;Ogre::SceneManager*scene_;unsigned requested_;char error_[256];Descriptor cached_;bool cacheValid_,force_,lastChanged_;
    Snapshot(const Snapshot&);Snapshot&operator=(const Snapshot&);
    void Error(const char*s){size_t i=0;if(s)for(;i<255&&s[i];++i)error_[i]=s[i];error_[i]=0;}
    // BEGIN REFRESH ADOPTION
    void AdoptDescriptor(Descriptor&d){
        if(!force_&&cacheValid_&&packet_.Count()==d.count&&packet_.Generation()==d.generation&&SameDescriptor(cached_,d)){lastChanged_=false;return;}
        cacheValid_=false;packet_.Clear();
        for(unsigned n=0;n<d.count;++n){HRESULT hr=device_.p->CreateSamplerState(&d.sampler[n],&samplers_[n].p);if(FAILED(hr))throw std::runtime_error("model sampler creation failed");d.raw[n].sampler=samplers_[n].p;}
        HRESULT hr=packet_.Adopt(device_.p,d.raw,d.count,d.matrix,d.origin,d.generation,AtlasModelVertexShader,sizeof(AtlasModelVertexShader));if(FAILED(hr))throw std::runtime_error("model COM geometry admission failed");
        cached_=d;for(unsigned n=0;n<16;++n)cached_.raw[n].sampler=NULL;cacheValid_=true;lastChanged_=true;
    }
    // END REFRESH ADOPTION
    void Current(){
        Profile(requested_);if(!au||!au->render||!scene_||au->render->scene!=scene_||!town_)throw std::runtime_error("model current world/scene unavailable");
        void*raw=NULL;if(!OwnerEntity(town_,raw))throw std::runtime_error("existing DistantTown state2/3 Entity unavailable");HMODULE ogre=GetModuleHandleA(GsmOgreFile);if(!Typed(raw,ogre,ModelEntityVtable))throw std::runtime_error("model Entity type differs");
        Ogre::Entity*e=static_cast<Ogre::Entity*>(raw);if(Get<Ogre::SceneManager*>(e,"?_getManager@MovableObject")!=scene_)throw std::runtime_error("model Entity scene creator differs");
        typedef const void*(__fastcall*Collection)(const Ogre::SceneManager*,const Ogre::String&);Ogre::String type("Entity");const void*collection=reinterpret_cast<Collection>(Pin("?getMovableObjectCollection@SceneManager@Ogre@@IEB"))(scene_,type);if(!Registered(collection,e))throw std::runtime_error("model Entity is not currently registered");
        Ogre::SceneNode*node=Get<Ogre::SceneNode*>(e,"?getParentSceneNode@MovableObject");if(!Typed(node,ogre,ModelSceneNodeVtable)||Get<Ogre::SceneManager*>(node,"?getCreator@SceneNode")!=scene_)throw std::runtime_error("model parent SceneNode type/creator differs");
        const Ogre::Matrix4&m=Get<const Ogre::Matrix4&>(node,"?_getFullTransform@Node");float matrix[16];if(!Copy(&m[0][0],matrix,64)||!RawBasis(matrix))throw std::runtime_error("model cached raw-scene matrix unsupported");
        typedef void*(__fastcall*Origin)(const Ogre::SceneManager*,float*);float origin[3];if(reinterpret_cast<Origin>(Pin("?getRelativeOrigin@SceneManager"))(scene_,origin)!=origin||!AtlasModels::Finite(origin,3))throw std::runtime_error("model scene origin invalid");
        const Ogre::MeshPtr&mesh=Get<const Ogre::MeshPtr&>(e,"?getMesh@Entity");if(mesh.isNull()||!Typed(mesh.get(),ogre,ModelMeshVtable)||!Get<bool>(mesh.get(),"?isLoaded@Resource@")||Get<bool>(mesh.get(),"?hasVertexAnimation@Mesh"))throw std::runtime_error("model existing mesh is not loaded/static");void*skeleton=NULL;if(!Copy(reinterpret_cast<const BYTE*>(e)+0x300,&skeleton,8)||skeleton)throw std::runtime_error("model skeleton unsupported");
        const Ogre::AxisAlignedBox&bounds=Get<const Ogre::AxisAlignedBox&>(mesh.get(),"?getBounds@Mesh");if(Get<bool>(&bounds,"?isNull@AxisAlignedBox")||Get<bool>(&bounds,"?isInfinite@AxisAlignedBox"))throw std::runtime_error("model existing local bounds unavailable");const Ogre::Vector3&lo=Get<const Ogre::Vector3&>(&bounds,"?getMinimum@AxisAlignedBox@Ogre@@QEBA"),&hi=Get<const Ogre::Vector3&>(&bounds,"?getMaximum@AxisAlignedBox@Ogre@@QEBA");const float box[6]={lo.x,lo.y,lo.z,hi.x,hi.y,hi.z};if(!AtlasModels::Finite(box,6)||lo.x>hi.x||lo.y>hi.y||lo.z>hi.z)throw std::runtime_error("model local mesh bounds invalid");std::memcpy(localBounds_,box,sizeof(box));
        const size_t count=Get<size_t>(e,"?getNumSubEntities@Entity");if(!count||count>16)throw std::runtime_error("model subentity cap");Descriptor descriptor;descriptor.count=unsigned(count);descriptor.generation=requested_;AtlasModels::RawDraw*rawDraw=descriptor.raw;D3D11_SAMPLER_DESC*samplerDesc=descriptor.sampler;const Ogre::Material*firstMaterial=NULL;ID3D11ShaderResourceView*firstView=NULL;
        for(unsigned n=0;n<count;++n){Ogre::SubEntity*sub=Get<Ogre::SubEntity*,size_t>(e,"?getSubEntity@Entity@Ogre@@QEAAPEAVSubEntity@2@_K",n);if(!sub)throw std::runtime_error("model subentity absent");Ogre::SubMesh*sm=Get<Ogre::SubMesh*>(sub,"?getSubMesh@SubEntity");if(!sm)throw std::runtime_error("model submesh absent");
            size_t begin=0,end=0;if(!Copy(reinterpret_cast<const BYTE*>(sub)+0x88,&begin,8)||!Copy(reinterpret_cast<const BYTE*>(sub)+0x90,&end,8)||begin!=end)throw std::runtime_error("model subset overrides unsupported");
            AtlasModels::RawDraw&r=rawDraw[n];AtlasModels::Shape&s=r.shape;
            const Ogre::MaterialPtr&material=Get<const Ogre::MaterialPtr&>(sub,"?getMaterial@SubEntity");if(material.isNull()||!Typed(material.get(),ogre,ModelMaterialVtable)||!Get<bool>(material.get(),"?isLoaded@Resource@")||Get<const Ogre::String&>(material.get(),"?getName@Resource@")!="DistantTown"||Get<unsigned short>(material.get(),"?getNumTechniques@Material")!=1)throw std::runtime_error("model current material unsupported");if(!n)firstMaterial=material.get();else if(material.get()!=firstMaterial)throw std::runtime_error("model mixed material identities unsupported");Ogre::Technique*t=Get<Ogre::Technique*,unsigned short>(material.get(),"?getTechnique@Material@Ogre@@QEAAPEAVTechnique@2@G",0);if(!t||Get<unsigned short>(t,"?getNumPasses@Technique")!=1)throw std::runtime_error("model technique unsupported");Ogre::Pass*p=Get<Ogre::Pass*,unsigned short>(t,"?getPass@Technique@Ogre@@QEAAPEAVPass@2@G",0);
            if(!p||Get<const Ogre::String&>(p,"?getVertexProgramName@Pass")!="DistantTown_VP_HLSL"||Get<const Ogre::String&>(p,"?getFragmentProgramName@Pass")!="DistantTown_FP_HLSL"||Get<unsigned short>(p,"?getNumTextureUnitStates@Pass")!=1||!Get<bool>(p,"?getDepthCheckEnabled@Pass")||!Get<bool>(p,"?getDepthWriteEnabled@Pass")||Get<Ogre::CompareFunction>(p,"?getDepthFunction@Pass")!=Ogre::CMPF_LESS_EQUAL||Get<Ogre::PolygonMode>(p,"?getPolygonMode@Pass")!=Ogre::PM_SOLID||Get<Ogre::SceneBlendFactor>(p,"?getSourceBlendFactor@Pass")!=Ogre::SBF_ONE||Get<Ogre::SceneBlendFactor>(p,"?getDestBlendFactor@Pass")!=Ogre::SBF_ZERO||Get<bool>(p,"?hasSeparateSceneBlending@Pass")||Get<bool>(p,"?hasSeparateSceneBlendingOperations@Pass")||!Get<bool>(p,"?getColourWriteEnabled@Pass")||Get<bool>(p,"?isAlphaToCoverageEnabled@Pass")||Get<Ogre::CompareFunction>(p,"?getAlphaRejectFunction@Pass")!=Ogre::CMPF_ALWAYS_PASS||Get<Ogre::SceneBlendOperation>(p,"?getSceneBlendingOperation@Pass")!=Ogre::SBO_ADD||Get<float>(p,"?getDepthBiasConstant@Pass")!=0||Get<float>(p,"?getDepthBiasSlopeScale@Pass")!=0||Get<float>(p,"?getIterationDepthBias@Pass")!=0)throw std::runtime_error("model native pass state unsupported");r.cullMode=unsigned(Get<Ogre::CullingMode>(p,"?getCullingMode@Pass"));
            Ogre::TextureUnitState*u=Get<Ogre::TextureUnitState*,size_t>(p,"?getTextureUnitState@Pass@Ogre@@QEAAPEAVTextureUnitState@2@_K",0);if(!u)throw std::runtime_error("model existing diffuse unit absent");const void*slot=NULL;if(!NativeWater::PassiveFrame(u,slot))throw std::runtime_error("model existing diffuse frame unavailable");const Ogre::TexturePtr&tex=*static_cast<const Ogre::TexturePtr*>(slot);if(!GsmNative::TextureType(tex.get())||!Get<bool>(tex.get(),"?isLoaded@Resource@"))throw std::runtime_error("model existing diffuse is not loaded");r.diffuse=CachedSrv(tex.get());if(!n)firstView=r.diffuse;else if(r.diffuse!=firstView)throw std::runtime_error("model mixed diffuse identities unsupported");if(!device_.p)r.diffuse->GetDevice(&device_.p);CachedSampler(u,samplerDesc[n]);
            Ogre::RenderOperation op;typedef void(__fastcall*Operation)(Ogre::SubMesh*,Ogre::RenderOperation&,unsigned short);reinterpret_cast<Operation>(Pin("?_getRenderOperation@SubMesh"))(sm,op,0);if(op.operationType!=Ogre::RenderOperation::OT_TRIANGLE_LIST||!op.useIndexes||!op.vertexData||!op.indexData||op.indexData->indexBuffer.isNull()||!op.vertexData->vertexDeclaration||!op.vertexData->vertexBufferBinding)throw std::runtime_error("model triangle/index data unsupported");
            if(op.vertexData->vertexStart>UINT_MAX||op.vertexData->vertexCount>UINT_MAX||op.indexData->indexStart>UINT_MAX||op.indexData->indexCount>UINT_MAX)throw std::runtime_error("model range overflow");s.vertexStart=unsigned(op.vertexData->vertexStart);s.vertexCount=unsigned(op.vertexData->vertexCount);s.indexStart=unsigned(op.indexData->indexStart);s.indexCount=unsigned(op.indexData->indexCount);
            const Ogre::VertexDeclaration::VertexElementList&elements=Get<const Ogre::VertexDeclaration::VertexElementList&>(op.vertexData->vertexDeclaration,"?getElements@VertexDeclaration");if(elements.size()!=4)throw std::runtime_error("model declaration is not native DistantTown4");unsigned at=0;
            for(Ogre::VertexDeclaration::VertexElementList::const_iterator it=elements.begin();it!=elements.end();++it){const unsigned sem=unsigned(Get<Ogre::VertexElementSemantic>(&*it,"?getSemantic@VertexElement"));AtlasModels::Element&v=s.element[at++];v.semantic=sem==1?0:sem==4?1:sem==5?2:sem==7?3:4;v.slot=Get<unsigned short>(&*it,"?getSource@VertexElement");size_t offset=Get<size_t>(&*it,"?getOffset@VertexElement");if(Get<unsigned short>(&*it,"?getIndex@VertexElement")!=0||offset>UINT_MAX||v.slot>=16)throw std::runtime_error("model vertex semantic/index unsupported");v.offset=unsigned(offset);v.format=ElementFormat(unsigned(Get<Ogre::VertexElementType>(&*it,"?getType@VertexElement")),sem);}
            const Ogre::VertexBufferBinding::VertexBufferBindingMap&bindings=Get<const Ogre::VertexBufferBinding::VertexBufferBindingMap&>(op.vertexData->vertexBufferBinding,"?getBindings@VertexBufferBinding");if(bindings.empty()||bindings.size()>16)throw std::runtime_error("model vertex stream cap");
            for(Ogre::VertexBufferBinding::VertexBufferBindingMap::const_iterator it=bindings.begin();it!=bindings.end();++it){unsigned slot=it->first;if(slot>=16||it->second.isNull())throw std::runtime_error("model vertex binding unavailable");r.vertex[slot]=Buffer(it->second.get(),false);D3D11_BUFFER_DESC d;r.vertex[slot]->GetDesc(&d);s.bytes[slot]=d.ByteWidth;size_t stride=Get<size_t>(it->second.get(),"?getVertexSize@HardwareVertexBuffer");if(stride>UINT_MAX)throw std::runtime_error("model vertex stride overflow");s.stride[slot]=unsigned(stride);s.streamCount=(std::max)(s.streamCount,slot+1);}
            r.index=Buffer(op.indexData->indexBuffer.get(),true);D3D11_BUFFER_DESC ib;r.index->GetDesc(&ib);s.indexBytes=ib.ByteWidth;unsigned itype=unsigned(Get<Ogre::HardwareIndexBuffer::IndexType>(op.indexData->indexBuffer.get(),"?getType@HardwareIndexBuffer"));s.indexFormat=itype==0?DXGI_FORMAT_R16_UINT:itype==1?DXGI_FORMAT_R32_UINT:DXGI_FORMAT_UNKNOWN;if(!AtlasModels::ValidShape(s))throw std::runtime_error("model geometry bounds/declaration refused");

        }
        float after[3],matrixAfter[16];void*again=NULL;const Ogre::Matrix4&checkMatrix=Get<const Ogre::Matrix4&>(node,"?_getFullTransform@Node");if(!Copy(&checkMatrix[0][0],matrixAfter,64)||std::memcmp(matrix,matrixAfter,64)||!au||!au->render||au->render->scene!=scene_||reinterpret_cast<Origin>(Pin("?getRelativeOrigin@SceneManager"))(scene_,after)!=after||std::memcmp(origin,after,12)||!OwnerEntity(town_,again)||again!=e||!Registered(collection,e)||Get<Ogre::SceneNode*>(e,"?getParentSceneNode@MovableObject")!=node)throw std::runtime_error("model owner/matrix/origin changed during snapshot");
        descriptor.material=reinterpret_cast<ULONG_PTR>(firstMaterial);std::memcpy(descriptor.matrix,matrix,64);std::memcpy(descriptor.origin,origin,12);std::memcpy(descriptor.bounds,box,sizeof(box));Identify(descriptor,device_.p);AdoptDescriptor(descriptor);
    }
public:
    Snapshot():town_(NULL),scene_(NULL),requested_(0),cacheValid_(false),force_(false),lastChanged_(false){error_[0]=0;std::memset(localBounds_,0,sizeof(localBounds_));}
    void Clear(){cacheValid_=false;cached_=Descriptor();lastChanged_=false;force_=false;packet_.Clear();for(unsigned i=0;i<16;++i)samplers_[i].Reset();device_.Reset();std::memset(localBounds_,0,sizeof(localBounds_));town_=NULL;scene_=NULL;requested_=0;}
    static bool TryCurrent(Snapshot*self){__try{self->Current();return true;}__except(GetExceptionCode()==0xe06d7363?EXCEPTION_CONTINUE_SEARCH:EXCEPTION_EXECUTE_HANDLER){return false;}}
    HRESULT Capture(){lastChanged_=false;try{if(!TryCurrent(this))throw std::runtime_error("model native memory admission fault");for(unsigned i=0;i<16;++i)samplers_[i].Reset();device_.Reset();town_=NULL;scene_=NULL;requested_=0;return S_OK;}catch(const std::exception&e){Error(e.what());Clear();return E_FAIL;}catch(...){Error("model snapshot failed");Clear();return E_FAIL;}}
    HRESULT Read(bool enabled,Town*town,Ogre::SceneManager*scene,unsigned generation,bool verified){error_[0]=0;force_=true;town_=town;scene_=scene;requested_=generation;return ReadRoute(enabled,verified,generation,*this);}
    HRESULT Refresh(bool enabled,Town*town,Ogre::SceneManager*scene,unsigned generation,bool verified,bool*changed){
        if(changed)*changed=false;error_[0]=0;force_=false;town_=town;scene_=scene;requested_=generation;
        const HRESULT hr=ReadRoute(enabled,verified,generation,*this);if(SUCCEEDED(hr)&&hr==S_OK&&changed)*changed=lastChanged_;return hr;
    }
    const AtlasModels::Packet&Geometry()const{return packet_;}const float*LocalMeshBounds()const{return localBounds_;}bool Valid(unsigned generation)const{return packet_.Count()!=0&&packet_.Generation()==generation;}const char*LastError()const{return error_;}
};
}
