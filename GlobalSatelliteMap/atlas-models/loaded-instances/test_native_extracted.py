"""Execute actual passive bodies at native boundaries; never load game DLLs."""
from pathlib import Path
import argparse,os,subprocess
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
parser=argparse.ArgumentParser();parser.add_argument('--header',type=Path);parser.add_argument('--label',default='native-extracted');args=parser.parse_args()
assert args.label.replace('-','').isalnum();source=(args.header or HERE/'NativeInstances.h').read_text()
def body(signature):
 a=source.index(signature);b=source.index('{',a)+1;depth=1
 while depth:depth+=(source[b]=='{')-(source[b]=='}');b+=1
 return source[a:b]
functions='\n'.join(body(s) for s in ['inline void SharedParameters(','inline bool InstanceActive(','inline const Ogre::String&StringAt(','inline int MaterialFamily(','inline bool CopyMaterialName(','inline const char*TextureName(','inline const void*Program(','inline bool Vector(','inline bool Same(const PointerVector&','inline bool PhysicalArray(','inline bool Same(const PhysicalHeader&'])
profile=body('inline void NativeProfile(').replace('GetModuleHandleA(', 'Module(') # module lookup boundary only
members='\n'.join(body(s) for s in ['bool SharedCurrent()','bool Current()const','bool PhysicalSame(','unsigned PhysicalFor(','bool ManagerCurrent()const','bool MemberOf(','static bool SameMaterial('])
end=body('bool End()');tail=end[end.index('stage_="instance end origin/world identity";'):-1]
members+='\n bool Finish(){'+tail+'}' # exact production prepublication tail
snapshot=body('class Snapshot ')
prefix=r'''
#define NOMINMAX
#include "InstanceAdmission.h"
#include "../loaded-census/CensusRoute.h"
#include "_generated/InstanceSource.h"
#include "_generated/InstanceRuntimeProfile.h"
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
using namespace LoadedInstances;
namespace Ogre{typedef std::string String;struct Pass{BYTE bytes[0x200];};struct GpuSharedParametersPtr{const void*p;GpuSharedParametersPtr():p(NULL){}void setNull(){p=NULL;}bool isNull()const{return p==NULL;}const void*get()const{return p;}};}
static const char*GsmD3d11File="fake.dll",*GsmOgreFile="fake-ogre.dll";static DWORD CensusTownVtable=1;
static std::vector<BYTE>module[3];static int missingModule=-1;static bool censusProfile=true;static unsigned modelProfiles=0;
static HMODULE Module(const char*n){unsigned i=n==NULL?0:std::strcmp(n,GsmOgreFile)==0?1:2;return int(i)==missingModule?NULL:reinterpret_cast<HMODULE>(&module[i][0]);}
namespace NativeModels{inline void Profile(unsigned){++modelProfiles;}}namespace NativeLoadedCensus{inline bool Profile(){return censusProfile;}}
static BYTE program[0x400],usage[0x40],townData[0x100],sceneData[0x600],entityData[0x200];static bool typed=true,loaded=true;
static const void*resolvedTown=townData;static float nativeOrigin[3]={1000,0,0};namespace GsmModels{inline const void*Resolve(const void*,const void*,unsigned,LONG g,volatile LONG*live){return live&&InterlockedCompareExchange(live,0,0)==g?resolvedTown:NULL;}}
static unsigned checks=0;static void C(bool b,const char*s){++checks;if(!b){std::printf("FAIL %s\n",s);throw 1;}}
inline const void*Pointer(const void*p){return *static_cast<const void*const*>(p);}inline unsigned Number(const void*p){return *static_cast<const unsigned*>(p);}
inline bool Typed(const void*,HMODULE,DWORD){return typed;}template<class T>T Get(const void*,const char*);template<>inline bool Get<bool>(const void*,const char*){return loaded;}
inline bool Copy(const void*a,void*b,size_t n){std::memcpy(b,a,n);return true;}
static const void*currentShared=program;static bool managerAvailable=true,sharedReturn=true;static void*__fastcall FakeSingleton(){return managerAvailable?program:NULL;}static Ogre::GpuSharedParametersPtr*__fastcall FakeShared(const void*,Ogre::GpuSharedParametersPtr*out,const Ogre::String&){out->p=currentShared;return sharedReturn?out:NULL;}
static bool active=true,visible=true;static bool __fastcall FakeActive(const void*){return active;}static bool __fastcall FakeVisible(const void*){return visible;}inline FARPROC Leaf(DWORD rva){if(rva==0xdddc0)return reinterpret_cast<FARPROC>(FakeSingleton);if(rva==0xdebf0)return reinterpret_cast<FARPROC>(FakeShared);return reinterpret_cast<FARPROC>(rva==0x143c80?FakeActive:FakeVisible);}
struct PointerVector{const void*const*data;size_t count,capacity;};struct PhysicalHeader{const void*const*data;unsigned count,capacity;};
struct Render{const void*scene;};struct AU{Render*render;};static Render renderer;static AU auValue;static AU*au;static void*shou;
struct PhysicalEntity{BYTE a[8];const void*entity;BYTE b[8];signed char floorNumber;BYTE c[23];bool instanced,isShell,isEmissive;};
'''
fixture=r'''
class OwnerFixture{public:
 struct Params{const void*p;const void*get()const{return p;}};
 struct Material{const void*identity,*pass,*program[2];char name[1025];Params parameters[2];struct Raw{unsigned family;Kind kind;unsigned cullMode;Surface surface;const void*texture[7];}raw;D3D11_SAMPLER_DESC sampler;Material(){std::memset(this,0,sizeof(*this));}};
 struct Physical{const void*const*slot;const BYTE*base;PhysicalEntity*part;const void*entity;int floor;bool instanced,shell,emissive;};
 struct Key{unsigned firstPhysical,physicalCount;};struct Batch{PointerVector members;unsigned firstMember;};struct Member{const void*entity;};
 Ogre::String sharedName_;Ogre::GpuSharedParametersPtr shared_,sharedAfter_;float origin_[3];const char*stage_;void Origin(float*out)const{std::memcpy(out,nativeOrigin,12);}LONG generation_;volatile LONG*live_;const void*town_,*scene_,*king_;unsigned townId_;Physical physical_[4];Key keys_[2];Batch batches_[2];Member members_[4];PointerVector managers_;const void*managerValues_[4];
'''+members+r'''
};
namespace LoadedInstances {
struct Packet{unsigned clears;Packet():clears(0){}void Clear(){++clears;}};
static unsigned captures=0,destroyed=0,queries=0;static unsigned mode=0;
struct NativeCapture{NativeCapture(Packet&,Stats&,void*,void*,LONG,volatile LONG*,ID3D11Device*,U64,U64,unsigned){++captures;}~NativeCapture(){++destroyed;}void Clear(){}const char*LastError()const{return "fixture source refused";}};
inline HRESULT Protected(NativeCapture&,LONG,volatile LONG*,Stats&){++queries;if(mode==1)throw std::runtime_error("native refused");return mode==2?E_ABORT:S_OK;}
typedef void Town;namespace OgreDummy{}
}
typedef void Town;
namespace Ogre{typedef void SceneManager;}
namespace LoadedInstances{
'''+snapshot+r''';
}
static bool ProgramRefuses(Ogre::Pass&p,Kind k,bool vertex){try{Program(&p,k,vertex);return false;}catch(...){return true;}}
static void SetString(unsigned off,const char*s){new(program+off) Ogre::String(s);}
int main(){try{
 Ogre::GpuSharedParametersPtr sharedValue;SharedParameters("SharedWaterParams",sharedValue);C(sharedValue.get()==program,"actual shared getter hidden output ABI");managerAvailable=false;{bool bad=false;try{SharedParameters("SharedWaterParams",sharedValue);}catch(...){bad=true;}C(bad&&sharedValue.isNull(),"missing shared manager refuses without old owner");}managerAvailable=true;sharedReturn=false;{bool bad=false;try{SharedParameters("SharedWaterParams",sharedValue);}catch(...){bad=true;}C(bad,"wrong shared hidden return refuses");}sharedReturn=true;currentShared=NULL;{bool bad=false;try{SharedParameters("SharedWaterParams",sharedValue);}catch(...){bad=true;}C(bad,"missing current shared owner refuses");}currentShared=program;
 char copiedName[1025];C(CopyMaterialName("actual-resource:id",copiedName)&&std::strcmp(copiedName,"actual-resource:id")==0,"copied full material identity");C(!CopyMaterialName(Ogre::String(1025,'x'),copiedName)&&!CopyMaterialName(Ogre::String("a\0b",3),copiedName)&&!CopyMaterialName("a\nb",copiedName),"material identity length NUL and controls refuse");
 C(InstanceActive(entityData),"actual active visible nonskeletal tuple");active=false;C(!InstanceActive(entityData),"unused instance refuses");active=true;visible=false;C(!InstanceActive(entityData),"manual hidden instance refuses");visible=true;*reinterpret_cast<const void**>(entityData+0x188)=program;C(!InstanceActive(entityData),"skeletal instance refuses");*reinterpret_cast<const void**>(entityData+0x188)=NULL;
 for(unsigned i=0;i<3;++i){DWORD maxRva=0;for(unsigned j=0;j<sizeof(InstancePins)/sizeof(InstancePins[0]);++j)if(InstancePins[j].module==i)maxRva=(std::max)(maxRva,InstancePins[j].rva);module[i].resize(maxRva+24);}
 for(unsigned j=0;j<sizeof(InstancePins)/sizeof(InstancePins[0]);++j){const InstanceCodePin&p=InstancePins[j];std::memcpy(&module[p.module][p.rva],p.bytes,24);}NativeProfile(7);C(modelProfiles==1,"actual native profile validates existing profiles first");
 for(unsigned j=0;j<sizeof(InstancePins)/sizeof(InstancePins[0]);++j){const InstanceCodePin&p=InstancePins[j];module[p.module][p.rva]^=1;bool bad=false;try{NativeProfile(7);}catch(...){bad=true;}C(bad,"each actual native code24 window refuses mismatch");module[p.module][p.rva]^=1;}missingModule=1;{bool bad=false;try{NativeProfile(7);}catch(...){bad=true;}C(bad,"missing native module refuses");}missingModule=-1;censusProfile=false;{bool bad=false;try{NativeProfile(7);}catch(...){bad=true;}C(bad,"census profile refusal propagates");}censusProfile=true;
 C(MaterialFamily("StaticObject_c8:Wall4_Diff.ddsWall4_NM.dds:mod-id")==2,"actual wall identity");C(MaterialFamily("StaticObject_c3:Plaster-Concrete_DIF.ddsPlaster-Concrete_NML.ddsPlaster-Walls_DIF.dds:id")==0,"actual plaster identity");C(MaterialFamily("StaticObject_c8:Wall4_Diff.ddsWall4_NM.dds:")==-1&&MaterialFamily("StaticObject_c9:Wall4_Diff.ddsWall4_NM.dds:id")==-1,"unknown and empty owner identity refuse");C(std::strcmp(TextureName(0,5),"black.dds")==0&&TextureName(5,0)==NULL&&TextureName(0,7)==NULL,"actual dual black role and role limits");
 const void*items[4]={program,usage,townData,sceneData};const void*header[]={items,items+4,items+4};PointerVector v;C(Vector(header,v)&&v.count==4,"actual bounded native pointer vector");header[1]=items+3;header[2]=items+2;C(!Vector(header,v),"reversed vector capacity refuses");header[1]=reinterpret_cast<const void*>(reinterpret_cast<ULONG_PTR>(items)+1);header[2]=items+4;C(!Vector(header,v),"misaligned vector end refuses");header[1]=items+4097;header[2]=items+4097;C(!Vector(header,v),"native vector4097 cap");
 BYTE physicalHeader[24]={};*reinterpret_cast<unsigned*>(physicalHeader+8)=4;*reinterpret_cast<unsigned*>(physicalHeader+12)=4;*reinterpret_cast<const void*const**>(physicalHeader+16)=items;PhysicalHeader h;C(PhysicalArray(physicalHeader,h)&&h.count==4,"actual native lektor physical array");*reinterpret_cast<unsigned*>(physicalHeader+8)=5;C(!PhysicalArray(physicalHeader,h),"physical count beyond capacity refuses");
 Ogre::Pass pass={};*reinterpret_cast<const void**>(pass.bytes+0x138)=usage;*reinterpret_cast<const void**>(pass.bytes+0x150)=usage;*reinterpret_cast<const void**>(usage+0x18)=program;SetString(0x60,"Object_VP_83");SetString(0x170,"objects.hlsl");SetString(0x198,"");*reinterpret_cast<Ogre::String*>(program+0x198)=Ogre::String(reinterpret_cast<const char*>(InstanceSource),sizeof(InstanceSource));SetString(0x2d0,"vs_4_0");SetString(0x2f8,"main_vs");SetString(0x320,"COLOURING,INSTANCED");program[0x348]=1;
 C(Program(&pass,Dual43,true)==program,"actual cached instanced HLSL program");typed=false;C(ProgramRefuses(pass,Dual43,true),"wrong native program type refuses");typed=true;loaded=false;C(ProgramRefuses(pass,Dual43,true),"unloaded native program refuses");loaded=true;program[0x349]=1;C(ProgramRefuses(pass,Dual43,true),"backwards compile flag refuses");program[0x349]=0;program[0x34a]=1;C(ProgramRefuses(pass,Dual43,true),"native compile error flag refuses");program[0x34a]=0;*reinterpret_cast<Ogre::String*>(program+0x320)="COLOURING";C(ProgramRefuses(pass,Dual43,true),"missing instancing define refuses");*reinterpret_cast<Ogre::String*>(program+0x320)="COLOURING,INSTANCED";(*reinterpret_cast<Ogre::String*>(program+0x198))[0]^=1;C(ProgramRefuses(pass,Dual43,true),"cached source hash identity differs");(*reinterpret_cast<Ogre::String*>(program+0x198))[0]^=1;
 *reinterpret_cast<Ogre::String*>(program+0x60)="Object_FP_48";*reinterpret_cast<Ogre::String*>(program+0x2d0)="ps_4_0";*reinterpret_cast<Ogre::String*>(program+0x2f8)="main_ps";*reinterpret_cast<Ogre::String*>(program+0x320)="DX11,DUST,TRANSPARENCY";C(Program(&pass,Alpha48,false)==program,"actual native alpha program");C(ProgramRefuses(pass,Dual43,false),"cross material shader cannot substitute");
 OwnerFixture owner;volatile LONG generation=1;owner.generation_=1;owner.live_=&generation;owner.town_=townData;owner.scene_=sceneData;owner.king_=program;owner.townId_=5;*reinterpret_cast<unsigned*>(townData+0x64)=5;shou=program;renderer.scene=sceneData;auValue.render=&renderer;au=&auValue;C(owner.Current(),"current native owner world");generation=2;C(!owner.Current(),"world generation invalidates source");generation=1;shou=usage;C(!owner.Current(),"new shared world invalidates source");shou=program;renderer.scene=usage;C(!owner.Current(),"new scene invalidates source");renderer.scene=sceneData;*reinterpret_cast<unsigned*>(townData+0x64)=6;C(!owner.Current(),"reused town ID invalidates source");*reinterpret_cast<unsigned*>(townData+0x64)=5;
 owner.sharedName_="SharedWaterParams";owner.shared_.p=program;C(owner.SharedCurrent(),"authoritative current shared owner matches");currentShared=usage;C(!owner.SharedCurrent(),"authoritative shared owner replacement refuses old retained values");currentShared=program;std::memcpy(owner.origin_,nativeOrigin,12);C(owner.Finish(),"actual End tail accepts coherent source");currentShared=usage;C(!owner.Finish(),"actual End tail rejects authoritative shared owner replacement");currentShared=program;nativeOrigin[0]+=1;C(!owner.Finish(),"actual End tail rejects shifted origin");nativeOrigin[0]-=1;resolvedTown=NULL;C(!owner.Finish(),"actual End tail rejects current ID resolution change");resolvedTown=townData;generation=2;C(!owner.Finish(),"actual End tail rejects world generation change");generation=1;
 OwnerFixture::Material ma,mb;strcpy_s(ma.name,"actual-material:id1");mb=ma;C(OwnerFixture::SameMaterial(ma,mb),"actual material reread identity");strcpy_s(mb.name,"actual-material:id2");C(!OwnerFixture::SameMaterial(ma,mb),"same prefix material owner suffix change refuses");mb=ma;mb.parameters[0].p=program;C(!OwnerFixture::SameMaterial(ma,mb),"native parameter owner replacement refuses");mb=ma;mb.raw.surface.wetness=.2f;C(!OwnerFixture::SameMaterial(ma,mb),"shared weather revision refuses mixed capture");mb=ma;mb.raw.texture[6]=program;C(!OwnerFixture::SameMaterial(ma,mb),"native role view replacement refuses");mb=ma;mb.sampler.MipLODBias=-.5f;C(!OwnerFixture::SameMaterial(ma,mb),"sampler revision refuses mixed capture");mb=ma;mb.program[0]=program;C(!OwnerFixture::SameMaterial(ma,mb),"cached shader replacement refuses mixed capture");
 PhysicalEntity part={};part.entity=program;part.instanced=true;const void*slot=&part;OwnerFixture::Physical&p=owner.physical_[0];p.slot=&slot;p.base=reinterpret_cast<BYTE*>(&part);p.part=&part;p.entity=program;p.instanced=true;p.floor=0;C(owner.PhysicalSame(p),"actual physical owner record");part.isShell=true;C(!owner.PhysicalSame(p),"physical shell mutation refuses");part.isShell=false;part.entity=usage;C(!owner.PhysicalSame(p),"physical entity owner mutation refuses");part.entity=program;slot=NULL;C(!owner.PhysicalSame(p),"physical collection slot mutation refuses");slot=&part;owner.keys_[0].firstPhysical=0;owner.keys_[0].physicalCount=1;C(owner.PhysicalFor(0,program)==0,"actual physical lookup");owner.physical_[1]=p;owner.keys_[0].physicalCount=2;C(owner.PhysicalFor(0,program)==4096,"ambiguous physical owners refuse");
 const void*registered[2]={program,usage};const void*registry[]={registered,registered+2,registered+2};std::memcpy(sceneData+0x578,registry,sizeof(registry));owner.managers_.data=registered;owner.managers_.count=owner.managers_.capacity=2;owner.managerValues_[0]=program;owner.managerValues_[1]=usage;C(owner.ManagerCurrent(),"actual current instance manager registry");registered[1]=townData;C(!owner.ManagerCurrent(),"same count registry owner replacement refuses");registered[1]=usage;
 owner.batches_[0].firstMember=0;owner.batches_[0].members.count=2;owner.members_[0].entity=program;owner.members_[1].entity=usage;C(owner.MemberOf(0,program),"actual unique batch membership");owner.members_[1].entity=program;C(!owner.MemberOf(0,program),"ambiguous batch membership refuses");
 {LoadedInstances::Snapshot s;C(s.Read(false,NULL,NULL,1,&generation,true,NULL)==S_FALSE&&!captures&&!queries,"actual Snapshot OFF makes zero allocations queries");C(s.Read(true,NULL,NULL,1,&generation,false,NULL)==E_ACCESSDENIED&&!captures,"actual Snapshot unverified before allocation");generation=2;C(s.Read(true,NULL,NULL,1,&generation,true,NULL)==E_ABORT&&!captures,"actual Snapshot stale generation before allocation");generation=1;C(s.Read(true,NULL,NULL,1,&generation,true,NULL,0,0,0)==E_INVALIDARG&&!captures,"empty native kind mask refuses before allocation");C(s.Read(true,NULL,NULL,1,&generation,true,NULL,0,0,4)==E_INVALIDARG&&!captures,"unknown native kind mask refuses before allocation");C(s.Read(true,NULL,NULL,1,&generation,true,NULL)==S_OK&&captures==1&&destroyed==1,"actual Snapshot destroys all transient owner state");mode=1;C(FAILED(s.Read(true,NULL,NULL,1,&generation,true,NULL))&&destroyed==2&&*s.LastError(),"actual Snapshot exception cleans transient sources");mode=2;C(s.Read(true,NULL,NULL,1,&generation,true,NULL)==E_ABORT&&destroyed==3,"actual Snapshot source refusal cleanup");}
 std::printf("PASS %u actual extracted native instance controls\n",checks);return 0;}catch(...){return 1;}}
'''
text=prefix+profile+functions+fixture
path=OUT/(args.label+'.cpp');path.write_text(text)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=os.environ.copy();env['INCLUDE']=f'{vc}/include;{sdk}/Include;{HERE}';env['LIB']=f'{vc}/lib/amd64;{sdk}/Lib/x64'
exe=OUT/(args.label+'.exe');r=subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT',str(path),'/Fo'+str(OUT/(args.label+'.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib'],env=env,capture_output=True,text=True);(OUT/(args.label+'-build.log')).write_text(r.stdout+r.stderr)
if r.returncode:print(r.stdout+r.stderr);raise SystemExit(r.returncode)
r=subprocess.run([str(exe)],capture_output=True,text=True);print(r.stdout+r.stderr,end='');(OUT/(args.label+'-run.log')).write_text(r.stdout+r.stderr);raise SystemExit(r.returncode)
