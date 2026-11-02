#pragma once
#include "AtlasModel.h"
namespace NativeModels {
// No native function/query is reachable on disabled or unverified paths.
template<class Ops>HRESULT ReadRoute(bool enabled,bool verified,unsigned generation,Ops&ops){if(!enabled){ops.Clear();return S_FALSE;}if(!verified||!generation){ops.Clear();return E_ACCESSDENIED;}return ops.Capture();}
inline bool Copy(const void*source,void*dest,size_t bytes){__try{if(!source||!dest)return false;std::memcpy(dest,source,bytes);return true;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
inline bool OwnerEntity(const void*town,void*&entity){entity=NULL;__try{if(!town)return false;const BYTE*proxy=*reinterpret_cast<const BYTE*const*>(static_cast<const BYTE*>(town)+0x4f8);if(!proxy||*reinterpret_cast<const void*const*>(proxy)!=town||*reinterpret_cast<const unsigned*>(proxy+8)!=3)return false;entity=*reinterpret_cast<void*const*>(proxy+16);return entity!=NULL;}__except(EXCEPTION_EXECUTE_HANDLER){entity=NULL;return false;}}
inline bool Registered(const void*collection,const void*entity){__try{if(!collection||!entity)return false;const ULONG_PTR*range=static_cast<const ULONG_PTR*>(collection);const ULONG_PTR begin=range[0],end=range[1];if(!begin||end<begin||(end-begin)%8||(end-begin)/8>131072)return false;const size_t index=*reinterpret_cast<const size_t*>(static_cast<const BYTE*>(entity)+0x160);return index<(end-begin)/8&&reinterpret_cast<const void*const*>(begin)[index]==entity;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
inline DXGI_FORMAT ElementFormat(unsigned type,unsigned semantic){
    if(semantic==1||semantic==4)return type==2?DXGI_FORMAT_R32G32B32_FLOAT:type==3?DXGI_FORMAT_R32G32B32A32_FLOAT:DXGI_FORMAT_UNKNOWN;
    if(semantic==5)return type==3?DXGI_FORMAT_R32G32B32A32_FLOAT:(type==4||type==10||type==11)?DXGI_FORMAT_R8G8B8A8_UNORM:DXGI_FORMAT_UNKNOWN;
    return semantic==7&&type==1?DXGI_FORMAT_R32G32_FLOAT:DXGI_FORMAT_UNKNOWN;
}
inline bool RawBasis(const float*m){if(!AtlasModels::Finite(m,16)||m[12]!=0||m[13]!=0||m[14]!=0||m[15]!=1)return false;const double det=double(m[0])*(double(m[5])*m[10]-double(m[6])*m[9])-double(m[1])*(double(m[4])*m[10]-double(m[6])*m[8])+double(m[2])*(double(m[4])*m[9]-double(m[5])*m[8]);return _finite(det)&&det>1e-10;}
}
