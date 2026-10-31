#include "OrthographicPosition.hlsl"
Texture2D<float> capturedDepth : register(t0);
float4 captureWindow;
void main_vs(float4 vertex : POSITION, out float4 screen : SV_Position)
{
    screen = float4(sign(vertex.xy), 0, 1);
}
float4 main_ps(float4 pixel : SV_Position) : SV_Target
{
    float2 lateral = (pixel.xy / captureWindow.w - 0.5f) *
                     float2(captureWindow.x, -captureWindow.y);
    float radial = capturedDepth.Load(int3(int2(pixel.xy), 0)).r * captureWindow.z;
    float3 position;
    bool valid = SatelliteOrthographicPosition(lateral, radial, position);
    return float4(position, valid ? 1.0f : 0.0f);
}
