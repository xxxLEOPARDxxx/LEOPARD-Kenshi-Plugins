#pragma once
#include "BlobPolicy.h"
#include "../NativeModel.h"
#include <OgreMeshSerializer.h>
#include <OgreDataStream.h>
#include <OgreMaterialManager.h>
#include <OgreMeshManager.h>
#include <OgreResourceGroupManager.h>
#include "_generated/PrivateProfile.h"
namespace PrivateTownLod {
typedef char SerializerSize[sizeof(Ogre::MeshSerializer)==120?1:-1];
typedef char StreamSize[sizeof(Ogre::MemoryDataStream)==96?1:-1];
typedef char MeshSize[sizeof(Ogre::Mesh)==0x390?1:-1];
typedef char SharedSize[sizeof(Ogre::DataStreamPtr)==16?1:-1];
struct AtomicLayout:Ogre::AtomicScalar<unsigned>{using Ogre::AtomicScalar<unsigned>::mField;};
typedef char NativeInfoCountAbi[offsetof(Ogre::SharedPtrInfo,useCount)+offsetof(AtomicLayout,mField)==0x20?1:-1];
inline Gate&GlobalGate(){static Gate gate;return gate;}
class Functions {
 FARPROC fn_[FunctionCount];HMODULE ogre_;unsigned generation_;bool attempted_,ready_;
public:Functions():ogre_(NULL),generation_(0),attempted_(false),ready_(false){std::memset(fn_,0,sizeof(fn_));}
 bool Profile(unsigned g){if(attempted_&&generation_==g)return ready_;attempted_=true;generation_=g;ready_=false;NativeModels::Profile();ogre_=GetModuleHandleA(GsmOgreFile);if(!ogre_)return false;for(unsigned i=0;i<FunctionCount;++i){const PrivatePin&p=PrivatePins[i];FARPROC f=GetProcAddress(ogre_,p.name);if(!f||reinterpret_cast<const BYTE*>(f)!=reinterpret_cast<const BYTE*>(ogre_)+p.rva||!NativeWater::CodeMatches(f,p.bytes))return false;fn_[i]=f;}for(unsigned i=0;i<sizeof(NullCreatorWindows)/sizeof(NullCreatorWindows[0]);++i)if(!NativeWater::CodeMatches(reinterpret_cast<FARPROC>(reinterpret_cast<BYTE*>(ogre_)+NullCreatorWindows[i].rva),NullCreatorWindows[i].bytes))return false;HMODULE backend=GetModuleHandleA(GsmD3d11File);if(!backend)return false;for(unsigned i=0;i<sizeof(PrivateBackendWindows)/sizeof(PrivateBackendWindows[0]);++i)if(!NativeWater::CodeMatches(reinterpret_cast<FARPROC>(reinterpret_cast<BYTE*>(backend)+PrivateBackendWindows[i].rva),PrivateBackendWindows[i].bytes))return false;if(!NativeWater::CodeMatches(reinterpret_cast<FARPROC>(reinterpret_cast<BYTE*>(ogre_)+UnloadRva),UnloadCode)||*reinterpret_cast<const void*const*>(reinterpret_cast<BYTE*>(ogre_)+MeshVtable+0x98)!=reinterpret_cast<const BYTE*>(ogre_)+UnloadRva)return false;ready_=true;return true;}
 FARPROC operator[](PrivateFunction f)const{return fn_[f];}HMODULE Ogre()const{return ogre_;}
 void Unload(void*p)const{typedef void(__fastcall*Fn)(void*);reinterpret_cast<Fn>(reinterpret_cast<BYTE*>(ogre_)+UnloadRva)(p);}
};
inline Functions&NativeFunctions(){static Functions f;return f;}
struct NativeStorage {unsigned __int64 words[15];};
struct NativeStreamStorage {unsigned __int64 words[12];};
struct NativeMeshStorage {unsigned __int64 words[0x390/8];};
class NativeObject {
 void*p_;FARPROC destroy_;NativeObject(const NativeObject&);NativeObject&operator=(const NativeObject&);
public:NativeObject(void*p,FARPROC destroy):p_(p),destroy_(destroy){}~NativeObject(){typedef void(__fastcall*Fn)(void*);reinterpret_cast<Fn>(destroy_)(p_);}
};
class State;
class Loader:public Ogre::ManualResourceLoader {State*state_;public:explicit Loader(State*s):state_(s){}void loadResource(Ogre::Resource*r);};
class State {
 State(const State&);State&operator=(const State&);
 Functions&f_;std::vector<BYTE>bytes_;BlobInfo info_;Ogre::String name_,group_;NativeMeshStorage storage_;Ogre::Mesh*mesh_;Ogre::MaterialPtr material_;bool allowed_;Loader loader_;
 AtlasView::Com<ID3D11Device>device_;AtlasView::Com<ID3D11SamplerState>sampler_;AtlasModels::Packet packet_;float matrix_[16],origin_[3];unsigned generation_;AtlasView::U64 otherGpu_,gpu_,shadow_;
 friend class Loader;
 void*Singleton(PrivateFunction n){typedef void*(__fastcall*Fn)();return reinterpret_cast<Fn>(f_[n])();}
 void Material(){
  typedef void*(__fastcall*FindMat)(void*,Ogre::MaterialPtr*,const Ogre::String&,const Ogre::String&);Ogre::String name("DistantTown");void*manager=Singleton(FMaterialManager);if(!manager||reinterpret_cast<FindMat>(f_[FMaterial])(manager,&material_,name,Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME)!=&material_||material_.isNull()||!NativeModels::Typed(material_.get(),f_.Ogre(),ModelMaterialVtable)||!NativeModels::Get<bool>(material_.get(),"?isLoaded@Resource@")||NativeModels::Get<unsigned short>(material_.get(),"?getNumTechniques@Material")!=1)throw std::runtime_error("already loaded DistantTown material unavailable");
  Ogre::Technique*t=NativeModels::Get<Ogre::Technique*,unsigned short>(material_.get(),"?getTechnique@Material@Ogre@@QEAAPEAVTechnique@2@G",0);if(!t||NativeModels::Get<unsigned short>(t,"?getNumPasses@Technique")!=1)throw std::runtime_error("private LOD technique unsupported");Ogre::Pass*p=NativeModels::Get<Ogre::Pass*,unsigned short>(t,"?getPass@Technique@Ogre@@QEAAPEAVPass@2@G",0);
  if(!p||NativeModels::Get<const Ogre::String&>(p,"?getVertexProgramName@Pass")!="DistantTown_VP_HLSL"||NativeModels::Get<const Ogre::String&>(p,"?getFragmentProgramName@Pass")!="DistantTown_FP_HLSL"||NativeModels::Get<unsigned short>(p,"?getNumTextureUnitStates@Pass")!=1||!NativeModels::Get<bool>(p,"?getDepthCheckEnabled@Pass")||!NativeModels::Get<bool>(p,"?getDepthWriteEnabled@Pass")||NativeModels::Get<Ogre::CompareFunction>(p,"?getDepthFunction@Pass")!=Ogre::CMPF_LESS_EQUAL||NativeModels::Get<Ogre::PolygonMode>(p,"?getPolygonMode@Pass")!=Ogre::PM_SOLID||NativeModels::Get<Ogre::SceneBlendFactor>(p,"?getSourceBlendFactor@Pass")!=Ogre::SBF_ONE||NativeModels::Get<Ogre::SceneBlendFactor>(p,"?getDestBlendFactor@Pass")!=Ogre::SBF_ZERO||NativeModels::Get<bool>(p,"?hasSeparateSceneBlending@Pass")||NativeModels::Get<bool>(p,"?hasSeparateSceneBlendingOperations@Pass")||!NativeModels::Get<bool>(p,"?getColourWriteEnabled@Pass")||NativeModels::Get<bool>(p,"?isAlphaToCoverageEnabled@Pass")||NativeModels::Get<Ogre::CompareFunction>(p,"?getAlphaRejectFunction@Pass")!=Ogre::CMPF_ALWAYS_PASS||NativeModels::Get<Ogre::SceneBlendOperation>(p,"?getSceneBlendingOperation@Pass")!=Ogre::SBO_ADD||NativeModels::Get<float>(p,"?getDepthBiasConstant@Pass")!=0||NativeModels::Get<float>(p,"?getDepthBiasSlopeScale@Pass")!=0||NativeModels::Get<float>(p,"?getIterationDepthBias@Pass")!=0)throw std::runtime_error("private LOD native material state unsupported");
  materialPass_=p;
 }
 Ogre::Pass*materialPass_; // transient during Build only, never a published source
 void Geometry(){
  typedef unsigned short(__fastcall*Count)(const void*);typedef Ogre::SubMesh*(__fastcall*Submesh)(const void*,unsigned short);if(reinterpret_cast<Count>(f_[FSubmeshCount])(mesh_)!=1||NativeModels::Get<bool>(mesh_,"?hasVertexAnimation@Mesh"))throw std::runtime_error("private cached mesh shape unsupported");Ogre::SubMesh*sub=reinterpret_cast<Submesh>(f_[FSubmesh])(mesh_,0);if(!sub)throw std::runtime_error("private cached submesh absent");
  AtlasModels::RawDraw raw;Ogre::RenderOperation op;typedef void(__fastcall*Operation)(Ogre::SubMesh*,Ogre::RenderOperation&,unsigned short);reinterpret_cast<Operation>(NativeModels::Pin("?_getRenderOperation@SubMesh"))(sub,op,0);if(op.operationType!=Ogre::RenderOperation::OT_TRIANGLE_LIST||!op.useIndexes||!op.vertexData||!op.indexData||op.indexData->indexBuffer.isNull()||!op.vertexData->vertexDeclaration||!op.vertexData->vertexBufferBinding)throw std::runtime_error("private mesh render operation unsupported");
  AtlasModels::Shape&s=raw.shape;if(op.vertexData->vertexStart||op.indexData->indexStart||op.vertexData->vertexCount!=info_.vertices||op.indexData->indexCount!=info_.indices)throw std::runtime_error("imported private mesh ranges differ from bounded blob");s.vertexCount=info_.vertices;s.indexCount=info_.indices;
  const Ogre::VertexDeclaration::VertexElementList&e=NativeModels::Get<const Ogre::VertexDeclaration::VertexElementList&>(op.vertexData->vertexDeclaration,"?getElements@VertexDeclaration");if(e.size()!=4)throw std::runtime_error("private vertex declaration differs");unsigned i=0;for(Ogre::VertexDeclaration::VertexElementList::const_iterator it=e.begin();it!=e.end();++it){AtlasModels::Element&x=s.element[i++];unsigned sem=unsigned(NativeModels::Get<Ogre::VertexElementSemantic>(&*it,"?getSemantic@VertexElement"));x.semantic=sem==1?0:sem==4?1:sem==5?2:sem==7?3:4;x.slot=NativeModels::Get<unsigned short>(&*it,"?getSource@VertexElement");size_t offset=NativeModels::Get<size_t>(&*it,"?getOffset@VertexElement");if(x.slot||offset>UINT_MAX||NativeModels::Get<unsigned short>(&*it,"?getIndex@VertexElement"))throw std::runtime_error("private vertex source/index differs");x.offset=unsigned(offset);x.format=NativeModels::ElementFormat(unsigned(NativeModels::Get<Ogre::VertexElementType>(&*it,"?getType@VertexElement")),sem);}
  const Ogre::VertexBufferBinding::VertexBufferBindingMap&bindings=NativeModels::Get<const Ogre::VertexBufferBinding::VertexBufferBindingMap&>(op.vertexData->vertexBufferBinding,"?getBindings@VertexBufferBinding");if(bindings.size()!=1||bindings.begin()->first||bindings.begin()->second.isNull())throw std::runtime_error("private vertex binding differs");const Ogre::HardwareVertexBufferSharedPtr&vertex=bindings.begin()->second;s.streamCount=1;size_t stride=NativeModels::Get<size_t>(vertex.get(),"?getVertexSize@HardwareVertexBuffer");if(stride!=36)throw std::runtime_error("private vertex stride differs");s.stride[0]=36;raw.vertex[0]=NativeModels::Buffer(vertex.get(),false);raw.index=NativeModels::Buffer(op.indexData->indexBuffer.get(),true);D3D11_BUFFER_DESC vb,ib;raw.vertex[0]->GetDesc(&vb);raw.index->GetDesc(&ib);if(vb.ByteWidth!=info_.vertexBytes||ib.ByteWidth!=info_.indexBytes)throw std::runtime_error("actual private GPU payload differs from preflight");s.bytes[0]=vb.ByteWidth;s.indexBytes=ib.ByteWidth;unsigned type=unsigned(NativeModels::Get<Ogre::HardwareIndexBuffer::IndexType>(op.indexData->indexBuffer.get(),"?getType@HardwareIndexBuffer"));s.indexFormat=type==0?DXGI_FORMAT_R16_UINT:type==1?DXGI_FORMAT_R32_UINT:DXGI_FORMAT_UNKNOWN;
  Ogre::TextureUnitState*u=NativeModels::Get<Ogre::TextureUnitState*,size_t>(materialPass_,"?getTextureUnitState@Pass@Ogre@@QEAAPEAVTextureUnitState@2@_K",0);const void*slot=NULL;if(!u||!NativeWater::PassiveFrame(u,slot))throw std::runtime_error("existing DistantTown diffuse frame unavailable");const Ogre::TexturePtr&texture=*static_cast<const Ogre::TexturePtr*>(slot);if(!GsmNative::TextureType(texture.get())||!NativeModels::Get<bool>(texture.get(),"?isLoaded@Resource@"))throw std::runtime_error("existing DistantTown diffuse is not loaded");raw.diffuse=GsmNative::Srv(texture.get());if(!AtlasView::SameDevice(raw.diffuse,device_.p))throw std::runtime_error("private LOD material device differs");D3D11_SAMPLER_DESC sd;NativeWater::Sampler(u,sd);HRESULT hr=device_.p->CreateSamplerState(&sd,&sampler_.p);if(FAILED(hr))throw std::runtime_error("private sampler allocation failed");raw.sampler=sampler_.p;raw.cullMode=unsigned(NativeModels::Get<Ogre::CullingMode>(materialPass_,"?getCullingMode@Pass"));gpu_=(AtlasView::U64(vb.ByteWidth)+ib.ByteWidth)*3;if(otherGpu_>AtlasView::GpuCap-gpu_)throw std::runtime_error("private GPU ledger exceeded");hr=packet_.Adopt(device_.p,&raw,1,matrix_,origin_,generation_,AtlasModelVertexShader,sizeof(AtlasModelVertexShader));if(FAILED(hr))throw std::runtime_error("private geometry packet refused");
 }
public:
 Ownership owned;
 State(Functions&f,const void*bytes,SIZE_T n,const BlobInfo&info,ID3D11Device*d,unsigned town,unsigned generation,const float*world,const float*origin,AtlasView::U64 other):f_(f),info_(info),mesh_(NULL),allowed_(false),loader_(this),generation_(generation),otherGpu_(other),gpu_(info.GpuReservation()),shadow_(0),materialPass_(NULL){bytes_.assign(static_cast<const BYTE*>(bytes),static_cast<const BYTE*>(bytes)+n);std::memcpy(matrix_,world,64);std::memcpy(origin_,origin,12);char name[128];static LONG sequence=0;_snprintf_s(name,sizeof(name),_TRUNCATE,"GSM.PrivateLod.%lu.%u.%u.%lu",GetCurrentProcessId(),generation,town,static_cast<unsigned long>(InterlockedIncrement(&sequence)));name_=name;group_=name_+".Group";device_.p=d;d->AddRef();}
 void Build(){
  Material();mesh_=reinterpret_cast<Ogre::Mesh*>(&storage_);
  typedef void*(__fastcall*Construct)(void*,void*,const Ogre::String&,Ogre::ResourceHandle,const Ogre::String&,bool,Ogre::ManualResourceLoader*);
  void*result=reinterpret_cast<Construct>(f_[FMeshCtor])(mesh_,NULL,name_,0,group_,true,&loader_);owned.mesh=true;
  if(result!=mesh_||!Identity())throw std::runtime_error("unregistered private mesh constructor/ownership refused");
  typedef void(__fastcall*BoolSetter)(void*,bool);reinterpret_cast<BoolSetter>(f_[FAutoEdges])(mesh_,false);typedef bool(__fastcall*Boolean)(const void*);shadow_=(reinterpret_cast<Boolean>(f_[FVertexShadow])(mesh_)?info_.vertexBytes:0)+(reinterpret_cast<Boolean>(f_[FIndexShadow])(mesh_)?info_.indexBytes:0);
  typedef void(__fastcall*Load)(void*,bool);allowed_=true;reinterpret_cast<Load>(f_[FLoad])(mesh_,false);allowed_=false;if(!Identity()||!NativeModels::Get<bool>(mesh_,"?isLoaded@Resource@"))throw std::runtime_error("owned private mesh failed native loading");Geometry();materialPass_=NULL;material_.setNull();
 }
 void Import(Ogre::Resource*r){
  if(!allowed_||r!=mesh_||!Identity())throw std::runtime_error("private loader invoked outside authorized owned load");NativeStreamStorage stream;typedef void*(__fastcall*StreamCtor)(void*,void*,SIZE_T,bool,bool);reinterpret_cast<StreamCtor>(f_[FStreamCtor])(&stream,&bytes_[0],bytes_.size(),false,true);NativeObject streamLife(&stream,f_[FStreamDtor]);Ogre::DataStreamPtr ptr(reinterpret_cast<Ogre::DataStream*>(&stream),Ogre::SPFM_NONE);NativeStorage serializer;typedef void*(__fastcall*Ctor)(void*);reinterpret_cast<Ctor>(f_[FSerializerCtor])(&serializer);NativeObject serializerLife(&serializer,f_[FSerializerDtor]);if(*reinterpret_cast<void*const*>(reinterpret_cast<const BYTE*>(&serializer)+0x70))throw std::runtime_error("owned serializer listener is not empty");typedef void(__fastcall*Import)(void*,Ogre::DataStreamPtr&,Ogre::Mesh*);reinterpret_cast<Import>(f_[FImport])(&serializer,ptr,mesh_);
 }
 bool Identity(){
  if(!owned.mesh||mesh_!=reinterpret_cast<Ogre::Mesh*>(&storage_)||!NativeModels::Typed(mesh_,f_.Ogre(),MeshVtable))return false;
  typedef void*(__fastcall*Creator)(const void*);typedef const Ogre::String&(__fastcall*String)(const void*);typedef Ogre::ResourceHandle(__fastcall*Handle)(const void*);
  return reinterpret_cast<Creator>(f_[FCreator])(mesh_)==NULL&&NativeModels::Get<const Ogre::String&>(mesh_,"?getName@Resource@") ==name_&&reinterpret_cast<String>(f_[FGroup])(mesh_)==group_&&reinterpret_cast<Handle>(f_[FHandle])(mesh_)==0;
 }
 void ClearPacket(){packet_.Clear();sampler_.Reset();}
 void Unload(){allowed_=false;f_.Unload(mesh_);}
 void DestroyMesh(){typedef void(__fastcall*Fn)(void*);reinterpret_cast<Fn>(f_[FMeshDtor])(mesh_);mesh_=NULL;gpu_=0;}
 void DropLoader(){materialPass_=NULL;material_.setNull();allowed_=false;}
 bool Retire(){return PrivateTownLod::Retire(*this,owned);}
 const AtlasModels::Packet&Packet()const{return packet_;}AtlasView::U64 GpuBytes()const{return gpu_;}SIZE_T CpuBytes()const{return bytes_.capacity()+SIZE_T(shadow_)+info_.GpuBytes()+sizeof(State)+name_.capacity()+group_.capacity()+2;}unsigned Signature()const{return info_.signature;}
 void AbandonTransient(){allowed_=false;materialPass_=NULL;ClearPacket();}
};
inline void Loader::loadResource(Ogre::Resource*r){state_->Import(r);}
inline int Fault(DWORD code){return code==EXCEPTION_ACCESS_VIOLATION||code==EXCEPTION_IN_PAGE_ERROR||code==EXCEPTION_DATATYPE_MISALIGNMENT?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH;}
inline bool NativeStep(State*s,bool build){__try{if(build)s->Build();else return s->Retire();return true;}__except(Fault(GetExceptionCode())){return false;}}
class Snapshot {
 State*state_;AtlasModels::Packet empty_;char error_[256];Snapshot(const Snapshot&);Snapshot&operator=(const Snapshot&);
 void Error(const char*s){size_t i=0;if(s)for(;i<255&&s[i];++i)error_[i]=s[i];error_[i]=0;}
 void Poison(){if(state_)state_->AbandonTransient();GlobalGate().Poison();}
public:Snapshot():state_(NULL){error_[0]=0;}~Snapshot(){if(!Clear())state_=NULL;/* one poisoned global slot intentionally retained until process exit */}
 bool Clear(){if(!state_){GlobalGate().Release(this);return true;}if(GlobalGate().poisoned)return false;try{if(!NativeStep(state_,false)){Error("private native cleanup identity/fault; slot quarantined");Poison();return false;}delete state_;state_=NULL;GlobalGate().Release(this);return true;}catch(const std::exception&e){Error(e.what());Poison();return false;}catch(...){Error("private native cleanup exception; slot quarantined");Poison();return false;}}
 HRESULT Read(bool enabled,const void*bytes,SIZE_T count,unsigned town,unsigned generation,const float*rawWorld,const float*sourceOrigin,ID3D11Device*d,bool verified,AtlasView::U64 otherOwnedGpu){
  if(!enabled){if(state_)state_->ClearPacket();return S_FALSE;}error_[0]=0;if(state_)state_->ClearPacket();if(!verified){Error("private mesh source profile not verified");return E_ACCESSDENIED;}BlobInfo info;
  if(!generation||!d||!rawWorld||!sourceOrigin||!NativeModels::RawBasis(rawWorld)||!AtlasModels::Finite(sourceOrigin,3)||!Parser(bytes,count).Read(info)||!Budget(otherOwnedGpu,info.GpuReservation(),AtlasView::GpuCap)){Error("private mesh bytes pose device generation or budget invalid");return E_INVALIDARG;}
  if(!Clear()||!GlobalGate().Acquire(this)){Error("private mesh slot busy/quarantined");return E_PENDING;}
  try{Functions&f=NativeFunctions();if(!f.Profile(generation)){Error("private native profile differs");GlobalGate().Release(this);return E_ACCESSDENIED;}state_=new State(f,bytes,count,info,d,town,generation,rawWorld,sourceOrigin,otherOwnedGpu);if(!NativeStep(state_,true)){Error("private native import fault; slot quarantined");Poison();return E_FAIL;}return S_OK;}catch(const std::exception&e){Error(e.what());Clear();return E_FAIL;}catch(...){Error("private native import exception");Clear();return E_FAIL;}
 }
 const AtlasModels::Packet&Geometry()const{return state_?state_->Packet():empty_;}bool Valid(unsigned generation)const{return state_&&state_->Packet().Count()&&state_->Packet().Generation()==generation&&!GlobalGate().poisoned;}const char*LastError()const{return error_;}AtlasView::U64 OwnedGpuBytes()const{return state_?state_->GpuBytes():0;}SIZE_T CpuPayloadBytes()const{return state_?state_->CpuBytes():0;}unsigned Signature()const{return state_?state_->Signature():0;}
};
}
