#include "constants.hlsl"
#include "../shaders/NativeExposure.hlsl"
Texture2D<float4> hdrInput : register(t0);
RWTexture2D<float4> result : register(u0);
cbuffer WaterTone : register(b0){float4 toneExposure;}
[numthreads(8,8,1)]
void main(uint3 id:SV_DispatchThreadID){
 if(id.x>=(uint)toneExposure.z||id.y>=(uint)toneExposure.z)return;
 float4 raw=hdrInput.Load(int3(id.xy,0));
 float3 colour=SatelliteNativeExposure(raw.rgb,toneExposure.xy);
 result[id.xy]=all(isfinite(colour))&&isfinite(raw.a)?float4(colour,raw.a):float4(0,0,0,0);
}
