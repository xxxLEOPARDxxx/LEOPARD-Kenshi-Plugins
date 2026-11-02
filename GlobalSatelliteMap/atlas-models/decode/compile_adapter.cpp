#include "AtlasModelDecode.h"
HRESULT CompileModelDecode(AtlasModelDecode::Layer&l,const AtlasModels::Packet&p,const AtlasModels::View&v,const AtlasModelDecode::Footprint&f,const AtlasModelDecode::ProvenMapping&m,ID3D11ShaderResourceView*const s[6],ID3D11Texture2D*const o[3]){return l.Render(true,p,v,f,m,s,o,0);}
