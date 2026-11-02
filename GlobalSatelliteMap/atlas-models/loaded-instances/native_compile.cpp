#define NOMINMAX
#include "NativeInstances.h"
HRESULT CompileActualNativeInstanceAdapter(bool enabled,Town*town,Ogre::SceneManager*scene,LONG generation,volatile LONG*live,bool verified,ID3D11Device*device,LoadedInstances::U64 buffers,LoadedInstances::U64 textures,unsigned kinds){LoadedInstances::Snapshot snapshot;return snapshot.Read(enabled,town,scene,generation,live,verified,device,buffers,textures,kinds);}
