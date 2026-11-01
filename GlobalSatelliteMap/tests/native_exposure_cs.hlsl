#include "constants.hlsl"
#include "../shaders/NativeExposure.hlsl"
StructuredBuffer<float4> inputs : register(t0);
RWStructuredBuffer<float4> outputs : register(u0);
[numthreads(1,1,1)]
void main(uint3 id : SV_DispatchThreadID)
{
    float4 v=inputs[id.x];
    float lum=(id.x%4==0)?0.0001f:((id.x%4==1)?0.9f:((id.x%4==2)?10.0f:1e10f));
    if(id.x>=24) lum=0;
    outputs[id.x]=float4(SatelliteExposureOrRaw(v.rgb,float2(v.a,lum)),1);
}
