#include "NativeModel.h"
typedef char EntityGlobalIndexAbi[offsetof(Ogre::MovableObject,mGlobalIndex)==0x160?1:-1];
typedef char TownProxyAbi[offsetof(Town,distantTown)==0x4f8?1:-1];
typedef char ShapeBudget[sizeof(AtlasModels::RawDraw)*16<65536?1:-1];
typedef char SnapshotBudget[sizeof(NativeModels::Snapshot)<65536?1:-1];
// Native VertexData ctor38EA90: id+0, mgr+8, declaration+10,binding+18,
// deleteDcl+20, start+28,count+30. The inherited IdObject occupies8 bytes.
typedef char VertexDeclarationAbi[offsetof(Ogre::VertexData,vertexDeclaration)==0x10?1:-1];
typedef char VertexBindingAbi[offsetof(Ogre::VertexData,vertexBufferBinding)==0x18?1:-1];
typedef char VertexStartAbi[offsetof(Ogre::VertexData,vertexStart)==0x28?1:-1];
typedef char VertexCountAbi[offsetof(Ogre::VertexData,vertexCount)==0x30?1:-1];
typedef char IndexStartAbi[offsetof(Ogre::IndexData,indexStart)==0x10?1:-1];
typedef char IndexCountAbi[offsetof(Ogre::IndexData,indexCount)==0x18?1:-1];
HRESULT CompileNativeModel(NativeModels::Snapshot&s,Town*t,Ogre::SceneManager*m,unsigned generation,bool sourceVerified){return s.Read(true,t,m,generation,sourceVerified);}
