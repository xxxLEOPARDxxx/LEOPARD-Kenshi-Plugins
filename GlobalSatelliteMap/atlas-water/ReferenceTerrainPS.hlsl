#include "WaterUniforms.hlsl"
Texture2D<float4> terrain : register(t0);
Texture2D<float4> coverage : register(t1);
Texture2D<float> heightmap : register(t2);
void main(float4 pixel:SV_Position,out float4 colour:SV_Target,out float depth:SV_Depth){
 uint2 at=uint2(pixel.xy),global=at+uint2(sourceWindow.yz);
 colour=terrain.Load(int3(at,0));
 float h=heightmap.Load(int3(global,0));
 depth=saturate(.5f+(waterPlane.x-h)*.00001f);
}
