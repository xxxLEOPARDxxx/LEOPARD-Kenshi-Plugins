#include "../NativeModel.h"
#include "../decode/AtlasModelDecode.h"
#include "AtlasModelSurface.h"
HRESULT SurfaceAdapter(ID3D11Device*d,ID3D11ShaderResourceView*const*v,AtlasView::U64 bytes){
 AtlasModelSurface::Layer s;HRESULT h=s.Init(true,d,bytes);if(SUCCEEDED(h))h=s.Copy(true,v,bytes);s.Clear();return h;
}
