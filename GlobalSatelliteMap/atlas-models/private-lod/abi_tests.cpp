#include "../NativeModel.h"
#include <OgreMeshSerializer.h>
#include <OgreDataStream.h>
#include <OgreResource.h>
#include <cstdio>
// Test-only data import replacement: base ctor/dtor merely write this table.
// Virtual dispatch below uses the real SDK-derived LOCAL object's own table.
// No native DLL is loaded or method invoked by this ABI executable.
static void*emptyBaseTable[3]={0};
extern "C" {void*LocalManualLoaderImport=emptyBaseTable;}
#pragma comment(linker,"/alternatename:__imp_??_7ManualResourceLoader@Ogre@@6B@=LocalManualLoaderImport")
struct SerializerProbe:Ogre::MeshSerializer {static size_t Listener(){return offsetof(SerializerProbe,mListener);}};
struct StreamProbe:Ogre::MemoryDataStream {static size_t Data(){return offsetof(StreamProbe,mData);}static size_t Pointer(){return offsetof(StreamProbe,mPos);}static size_t End(){return offsetof(StreamProbe,mEnd);}static size_t Free(){return offsetof(StreamProbe,mFreeOnClose);}};
struct AtomicProbe:Ogre::AtomicScalar<unsigned>{static size_t Field(){return offsetof(AtomicProbe,mField);}};
struct LoaderProbe:Ogre::ManualResourceLoader{unsigned prepare,load;LoaderProbe():prepare(0),load(0){}void prepareResource(Ogre::Resource*){++prepare;}void loadResource(Ogre::Resource*){++load;}};
int main(){std::printf("MeshSize=%x align=%u\n",unsigned(sizeof(Ogre::Mesh)),unsigned(__alignof(Ogre::Mesh)));if(offsetof(Ogre::SharedPtrInfo,useCount)+AtomicProbe::Field()!=0x20)return 1;LoaderProbe loader;typedef void(__fastcall*Fn)(void*,void*);reinterpret_cast<Fn>((*reinterpret_cast<void***>(&loader))[2])(&loader,NULL);if(loader.prepare||loader.load!=1)return 1;std::printf("MeshSerializer=%u listener=%x MemoryDataStream=%u data=%x ptr=%x end=%x free=%x SharedPtr=%u ManualLoader=%u\n",unsigned(sizeof(Ogre::MeshSerializer)),unsigned(SerializerProbe::Listener()),unsigned(sizeof(Ogre::MemoryDataStream)),unsigned(StreamProbe::Data()),unsigned(StreamProbe::Pointer()),unsigned(StreamProbe::End()),unsigned(StreamProbe::Free()),unsigned(sizeof(Ogre::DataStreamPtr)),unsigned(sizeof(Ogre::ManualResourceLoader)));return sizeof(Ogre::MeshSerializer)==120&&SerializerProbe::Listener()==0x70&&sizeof(Ogre::MemoryDataStream)==96&&StreamProbe::Data()==0x40&&StreamProbe::Pointer()==0x48&&StreamProbe::End()==0x50&&StreamProbe::Free()==0x58&&sizeof(Ogre::DataStreamPtr)==16&&sizeof(Ogre::ManualResourceLoader)==8?0:1;}
