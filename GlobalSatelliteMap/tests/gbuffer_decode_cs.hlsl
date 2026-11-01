#include "../shaders/GBufferDecode.hlsl"
RWStructuredBuffer<float4> destination : register(u0);
[numthreads(1,1,1)]
void main(uint3 index : SV_DispatchThreadID)
{
    int2 pixel = int2(index.x % 8,index.x / 8);
    destination[index.x] = float4(SatelliteDecodeAlbedo(pixel,8),1);
}
