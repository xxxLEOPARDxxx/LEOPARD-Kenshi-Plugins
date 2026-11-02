#include "BlobPolicy.h"
#include "_generated/PrivateProfile.h"
#include <string>
#include <vector>
#include <stdexcept>
#include <cstdio>
#include <new>
using namespace PrivateTownLod;
static unsigned checks=0;static void Check(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}}
namespace Ogre {
 typedef std::string String;typedef unsigned __int64 ResourceHandle;
 struct Resource {void*creator;String name,group;ResourceHandle handle;bool typed,loaded;Resource():creator(NULL),handle(0),typed(true),loaded(false){}};struct Mesh:Resource{};
 struct DataStream{};enum SharedPtrFreeMethod{SPFM_DELETE,SPFM_DELETE_T,SPFM_FREE,SPFM_NONE};struct DataStreamPtr{DataStream*p;DataStreamPtr(DataStream*v,SharedPtrFreeMethod f):p(v){Check(f==SPFM_NONE,"stream smart pointer never owns stack stream");}};
 struct MaterialPtr{void setNull(){}};struct ManualResourceLoader{};
}
namespace NativeModels {
 inline bool Typed(const void*p,HMODULE,DWORD){return p&&static_cast<const Ogre::Resource*>(p)->typed;}
 template<class T>T Get(const void*,const char*);
 template<>inline bool Get<bool>(const void*p,const char*){return static_cast<const Ogre::Resource*>(p)->loaded;}
 template<>inline const Ogre::String&Get<const Ogre::String&>(const void*p,const char*){return static_cast<const Ogre::Resource*>(p)->name;}
}
#include "_generated/objects.extract.h"
struct State;static State*current=NULL;
static void*MeshCtor(void*,void*,const Ogre::String&,Ogre::ResourceHandle,const Ogre::String&,bool,Ogre::ManualResourceLoader*);static void MeshDtor(void*);static void Setter(void*,bool);static bool Shadow(const void*);static void Load(void*,bool);static void*Creator(const void*);static const Ogre::String&GroupName(const void*);static Ogre::ResourceHandle Handle(const void*);
static void*StreamCtor(void*,void*,SIZE_T,bool,bool);static void*SerializerCtor(void*);static void StreamDtor(void*);static void SerializerDtor(void*);static void Import(void*,Ogre::DataStreamPtr&,Ogre::Mesh*);
struct Functions {HMODULE Ogre()const{return reinterpret_cast<HMODULE>(1);}FARPROC operator[](PrivateFunction p)const{switch(p){case FMeshCtor:return reinterpret_cast<FARPROC>(MeshCtor);case FMeshDtor:return reinterpret_cast<FARPROC>(MeshDtor);case FStreamCtor:return reinterpret_cast<FARPROC>(StreamCtor);case FStreamDtor:return reinterpret_cast<FARPROC>(StreamDtor);case FSerializerCtor:return reinterpret_cast<FARPROC>(SerializerCtor);case FSerializerDtor:return reinterpret_cast<FARPROC>(SerializerDtor);case FImport:return reinterpret_cast<FARPROC>(Import);case FAutoEdges:return reinterpret_cast<FARPROC>(Setter);case FVertexShadow:case FIndexShadow:return reinterpret_cast<FARPROC>(Shadow);case FLoad:return reinterpret_cast<FARPROC>(Load);case FCreator:return reinterpret_cast<FARPROC>(Creator);case FGroup:return reinterpret_cast<FARPROC>(GroupName);case FHandle:return reinterpret_cast<FARPROC>(Handle);default:throw 1;}}};
struct State {
 std::vector<BYTE>bytes_;std::vector<unsigned>importOrder;Functions f_;NativeMeshStorage storage_;Ogre::Mesh*mesh_;Ogre::String name_,group_;Ogre::MaterialPtr material_;Ogre::ManualResourceLoader loader_;PrivateTownLod::Ownership owned;PrivateTownLod::BlobInfo info_;SIZE_T shadow_;bool allowed_,badOwner,badGroup,badName,badVtable,badReturn,constructed;unsigned materialCalls,constructs,loads,geometry,queries,packetClears;void*materialPass_;
 State():mesh_(NULL),name_("GSM.PrivateLod.test"),group_("GSM.PrivateLod.test.Group"),shadow_(0),allowed_(false),badOwner(false),badGroup(false),badName(false),badVtable(false),badReturn(false),constructed(false),materialCalls(0),constructs(0),loads(0),geometry(0),queries(0),packetClears(0),materialPass_(NULL){bytes_.resize(16,42);info_.vertexBytes=144;info_.indexBytes=12;current=this;}
 ~State(){if(constructed)reinterpret_cast<Ogre::Mesh*>(&storage_)->~Mesh();}
 void Material(){++materialCalls;}void Geometry(){++geometry;Check(!allowed_&&mesh_->loaded,"packet only after completed normal native load");}void ClearPacket(){++packetClears;}
#include "_generated/control.extract.h"
};
static void*MeshCtor(void*p,void*creator,const Ogre::String&name,Ogre::ResourceHandle h,const Ogre::String&group,bool manual,Ogre::ManualResourceLoader*loader){Check(p==&current->storage_&&!creator&&h==0&&name==current->name_&&group==current->group_&&name!=group&&manual&&loader==&current->loader_,"explicit own-storage NULL-creator constructor arguments");++current->constructs;Ogre::Mesh*m=new(p)Ogre::Mesh;current->constructed=true;m->creator=current->badOwner?reinterpret_cast<void*>(99):creator;m->name=current->badName?"original":name;m->group=current->badGroup?"General":group;m->handle=h;m->typed=!current->badVtable;return current->badReturn?NULL:p;}
static void MeshDtor(void*p){Check(p==current->mesh_,"nondeleting destructor owns exact inline storage");static_cast<Ogre::Mesh*>(p)->~Mesh();current->constructed=false;}
static void Setter(void*p,bool value){Check(p==current->mesh_&&!value,"auto edge lists disabled only on owned mesh");}static bool Shadow(const void*p){Check(p==current->mesh_,"CPU shadow flags queried on owned mesh");return true;}
static void Load(void*p,bool background){Check(p==current->mesh_&&current->allowed_&&!background&&!current->mesh_->creator,"normal native load has no shared-manager creator");++current->loads;current->Import(current->mesh_);current->mesh_->loaded=true;}
static void*Creator(const void*p){return static_cast<const Ogre::Resource*>(p)->creator;}static const Ogre::String&GroupName(const void*p){return static_cast<const Ogre::Resource*>(p)->group;}static Ogre::ResourceHandle Handle(const void*p){return static_cast<const Ogre::Resource*>(p)->handle;}
static void*StreamCtor(void*p,void*data,SIZE_T n,bool free,bool readOnly){Check(data==&current->bytes_[0]&&n==current->bytes_.size()&&!free&&readOnly,"native stream wraps exact owned read-only bytes without freeing");std::memset(p,0,96);current->importOrder.push_back(1);return p;}static void*SerializerCtor(void*p){std::memset(p,0,120);current->importOrder.push_back(2);return p;}static void Import(void*,Ogre::DataStreamPtr&,Ogre::Mesh*p){Check(p==current->mesh_&&current->allowed_,"serializer imports only current authorized owned mesh");current->importOrder.push_back(3);}static void StreamDtor(void*){current->importOrder.push_back(5);}static void SerializerDtor(void*){current->importOrder.push_back(4);}
struct EntryFixture {State*state_;char error_[256];explicit EntryFixture(State*s):state_(s){}void Error(const char*){}
#include "_generated/entry.extract.h"
};
int main(){try{State ok;ok.Build();Check(ok.constructs==1&&ok.loads==1&&ok.geometry==1&&ok.owned.mesh&&ok.shadow_==156,"actual production unregistered construct load packet route");const unsigned order[]={1,2,3,4,5};Check(ok.importOrder.size()==5&&!std::memcmp(&ok.importOrder[0],order,sizeof(order)),"actual serializer pointer and stream retirement order");bool forbidden=false;try{ok.Import(ok.mesh_);}catch(...){forbidden=true;}Check(forbidden&&ok.importOrder.size()==5,"manual reload outside explicit load refused before native calls");
 for(unsigned i=0;i<5;++i){State bad;if(i==0)bad.badOwner=true;if(i==1)bad.badGroup=true;if(i==2)bad.badName=true;if(i==3)bad.badVtable=true;if(i==4)bad.badReturn=true;bool failed=false;try{bad.Build();}catch(...){failed=true;}Check(failed&&bad.constructs==1&&!bad.loads&&!bad.geometry,"actual creator metadata or constructor mismatch refused before native load");}
 State changed;changed.Build();changed.mesh_->handle=1;Check(!changed.Identity(),"unregistered handle changed from zero refused");changed.mesh_->handle=0;changed.mesh_->creator=reinterpret_cast<void*>(99);Check(!changed.Identity(),"shared manager creator refused at retirement");
 State other;other.Build();Ogre::Mesh original;original.name=other.name_;original.group=other.group_;other.mesh_=&original;Check(!other.Identity(),"borrowed original address cannot be adopted or destroyed");other.mesh_=reinterpret_cast<Ogre::Mesh*>(&other.storage_);
 State entry;EntryFixture route(&entry);Check(route.Entry(false,false)==S_FALSE&&entry.packetClears==1&&!entry.queries&&!entry.materialCalls,"actual OFF entry makes no native queries");Check(route.Entry(true,false)==E_ACCESSDENIED&&entry.packetClears==2&&!entry.queries,"actual unverified entry makes no native queries");std::printf("Private extracted control PASS %u checks\n",checks);return 0;}catch(...){return 1;}}
