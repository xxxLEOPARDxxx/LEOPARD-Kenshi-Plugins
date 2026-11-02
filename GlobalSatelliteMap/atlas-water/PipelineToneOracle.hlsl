// Independent reference invocation of the actual installed native tone/gamma.
#include "constants.hlsl"
#include "../shaders/NativeExposure.hlsl"
Texture2D<float4> nativeHdr : register(t0);
RWTexture2D<float4> referenceResult : register(u0);
cbuffer OracleTone : register(b0){float4 expectedExposure;}
[numthreads(8,8,1)]
void main(uint3 id:SV_DispatchThreadID){
 if(id.x>=(uint)expectedExposure.z||id.y>=(uint)expectedExposure.z)return;
 float4 raw=nativeHdr.Load(int3(id.xy,0));
 referenceResult[id.xy]=float4(SatelliteNativeExposure(raw.rgb,expectedExposure.xy),raw.a);
}
