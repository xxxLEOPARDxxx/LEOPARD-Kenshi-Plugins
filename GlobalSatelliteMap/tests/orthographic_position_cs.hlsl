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
