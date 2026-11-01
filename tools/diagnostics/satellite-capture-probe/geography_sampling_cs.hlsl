Texture2D geoSelector : register(t6);
Texture2D blendMap : register(t5);
SamplerState linearMap : register(s2);
#include "GeographySampling.hlsl"
StructuredBuffer<float2> points : register(t0);
struct Result {uint4 codes;float4 weights;};
RWStructuredBuffer<Result> results : register(u0);
[numthreads(64,1,1)] void main(uint3 id:SV_DispatchThreadID) {
    uint count,stride;points.GetDimensions(count,stride);if(id.x>=count)return;
    float2 world=points[id.x];
    Result r;r.codes=GeographyCodes(world);
    float2 uv=(world+147456.0)/294912.0;
    float4 weights=blendMap.SampleGrad(linearMap,uv,float2(1.0/2048.0,0),float2(0,1.0/2048.0));
    r.weights=GeographyWeights(r.codes,weights);results[id.x]=r;
}
