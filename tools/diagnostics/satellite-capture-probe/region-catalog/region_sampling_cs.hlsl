Texture2D geoSelector : register(t6);
cbuffer RegionConstants : register(b0) {float4 geoSelectorRegion;uint pointCount;uint3 padding;};
struct Point {float2 worldXZ;float4 blend;};
struct Result {uint4 codes;float4 weights;};
StructuredBuffer<Point> points : register(t0);
RWStructuredBuffer<Result> results : register(u0);
#include "../GeographySampling.hlsl"
[numthreads(64,1,1)]
void main(uint3 id:SV_DispatchThreadID) {
    if(id.x>=pointCount)return;
    Point p=points[id.x];Result r;
    r.codes=GeographyCodes(p.worldXZ,geoSelectorRegion);
    r.weights=GeographyWeights(r.codes,p.blend);
    results[id.x]=r;
}
