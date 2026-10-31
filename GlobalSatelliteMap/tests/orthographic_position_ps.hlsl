#include "../shaders/OrthographicPosition.hlsl"
cbuffer TestParameters : register(b0) { float radialDistance; };
float4 main(float4 screen : SV_Position, float2 lateral : TEXCOORD0) : SV_Target
{
    float3 position;
    bool valid = SatelliteOrthographicPosition(lateral, radialDistance, position);
    return float4(position, valid ? 1.0f : 0.0f);
}
