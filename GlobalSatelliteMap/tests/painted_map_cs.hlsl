#include "../shaders/PaintedMap.hlsl"
StructuredBuffer<float4> source : register(t0);
RWStructuredBuffer<float4> destination : register(u0);
[numthreads(1,1,1)]
void main(uint3 index : SV_DispatchThreadID)
{
    float4 input=source[index.x];
    destination[index.x]=float4(SatellitePaintedCoordinates(input.xy,input.zw),1);
}
