#include "../shaders/OrthographicPosition.hlsl"
StructuredBuffer<float4> source : register(t0);
RWStructuredBuffer<float4> destination : register(u0);
[numthreads(1,1,1)]
void main(uint3 index : SV_DispatchThreadID)
{
    float4 input = source[index.x];
    float3 position;
    bool valid = SatelliteOrthographicPosition(input.zw, input.x * input.y, position);
    destination[index.x] = float4(position, valid ? 1.0f : 0.0f);
}
[numthreads(1,1,1)]
void reference_main(uint3 index : SV_DispatchThreadID)
{
    float4 input = source[index.x];
    float3 capture = float3(-499.625f,4000,961.351807f);
    float3 reference = float3(input.w,input.w*0.25f+1000,input.w*0.5f);
    float3 relative = SatelliteReferenceRelativePosition(input.xyz,capture-reference,
        float3(1,0,0),float3(0,0,-1),float3(0,1,0));
    float4 worldSize = float4(1e-5f,-1e-5f,0.7f+reference.x*1e-5f,0.4f-reference.z*1e-5f);
    float2 uv = SatelliteAmbientUV(relative,float3(-51212.375f,0,2622.64819f),worldSize);
    destination[index.x] = float4(uv,relative.y,1);
}
