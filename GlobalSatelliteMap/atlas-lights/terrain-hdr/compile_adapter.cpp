#ifndef LOCAL_LAYER_HEADER
#define LOCAL_LAYER_HEADER "AtlasLocalLights.h"
#endif
#include LOCAL_LAYER_HEADER
HRESULT CompileLocalLayer(AtlasLocalLights::Layer&layer,const NativeLocalLights::Snapshot&s,LONG g,const AtlasLocalLights::ProvenMapping&m,const AtlasLocalLights::Region&r,ID3D11ShaderResourceView*const v[4],ID3D11Texture2D*t){return layer.Render(true,s,g,m,r,v,t,0);}
