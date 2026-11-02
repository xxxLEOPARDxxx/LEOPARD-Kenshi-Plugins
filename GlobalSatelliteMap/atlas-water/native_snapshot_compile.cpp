#ifndef NATIVE_WATER_HEADER
#define NATIVE_WATER_HEADER "NativeWaterSnapshot.h"
#endif
#include NATIVE_WATER_HEADER
typedef char DefinitionAbi[sizeof(Ogre::GpuConstantDefinition)==48?1:-1];
typedef char FloatListAbi[sizeof(Ogre::FloatConstantList)==32?1:-1];
typedef char SharedOwnerAbi[sizeof(Ogre::GpuProgramParametersSharedPtr)==16?1:-1];
typedef char DefinitionTypeOffset[offsetof(Ogre::GpuConstantDefinition,constType)==0?1:-1];
typedef char DefinitionIndexOffset[offsetof(Ogre::GpuConstantDefinition,physicalIndex)==8?1:-1];
typedef char DefinitionElementOffset[offsetof(Ogre::GpuConstantDefinition,elementSize)==24?1:-1];
typedef char DefinitionArrayOffset[offsetof(Ogre::GpuConstantDefinition,arraySize)==32?1:-1];
typedef char TextureOwnerAbi[sizeof(Ogre::TexturePtr)==16?1:-1];
// This translation unit is compiled only. Never linked/loaded by the test runner.
HRESULT CompileNativeWaterAdapter(NativeWater::Snapshot&s,bool enabled,unsigned generation,bool verified){return s.Read(enabled,true,true,generation,verified);}
