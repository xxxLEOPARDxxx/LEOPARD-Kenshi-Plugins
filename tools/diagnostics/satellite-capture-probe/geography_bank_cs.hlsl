Texture2DArray diffuseMaps : register(t0);
Texture2DArray normalMaps : register(t1);
Texture2DArray geoDiffuse1 : register(t7);
SamplerState Anisotropic : register(s0);
#include "GeographyDiffuse.hlsl"
StructuredBuffer<uint2> indexes : register(t2);
struct Result {float4 diffuse;float4 normal;};
RWStructuredBuffer<Result> results : register(u0);
[numthreads(64,1,1)]void main(uint3 id:SV_DispatchThreadID) {
    uint count,stride;indexes.GetDimensions(count,stride);if(id.x>=count)return;
    uint2 index=indexes[id.x];Result r;
    r.diffuse=GeoDiffuse(float2(.5,.5),index.x,float2(0,0),float2(0,0));
    r.normal=normalMaps.SampleGrad(Anisotropic,float3(.5,.5,index.y),float2(0,0),float2(0,0));
    results[id.x]=r;
}
