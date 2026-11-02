#include "WaterUniforms.hlsl"
Texture2D<float4> terrainHdr : register(t0);
Texture2D<float4> normalCoverage : register(t1);
Texture2D<float> worldHeight : register(t2);
Texture2D<float4> colourmap : register(t3);
Texture2D<float4> flowmap : register(t4);
Texture2D<float4> normalmap : register(t5);
TextureCube<float4> irradianceCube : register(t6);
TextureCube<float4> specularityCube : register(t7);
SamplerState colourSampler : register(s0);
SamplerState flowSampler : register(s1);
SamplerState normalSampler : register(s2);
SamplerState irradianceSampler : register(s3);
SamplerState specularitySampler : register(s4);
RWTexture2D<float4> result : register(u0);
#include "_generated/NativeWaterMaterial.hlsl"
[numthreads(8,8,1)]
void main(uint3 id : SV_DispatchThreadID){
 if(id.x>=(uint)dispatchWindow.x||id.y>=(uint)dispatchWindow.y)return;
 float4 base=terrainHdr.Load(int3(id.xy,0));
 if(waterPlane.y==0){result[id.xy]=base;return;}
 uint2 global=id.xy+uint2(sourceWindow.yz);
 float coverage=normalCoverage.Load(int3(global,0)).a;
 float ground=worldHeight.Load(int3(global,0));
 if(coverage<=0||!isfinite(ground)||ground>waterPlane.x){result[id.xy]=base;return;}
 float4 water=NativeWaterDistantMaterial(global,parallelView.xyz);
 // Native DISTANT alpha is1. Keep the cache's explicit valid-surface coverage;
 // it is not water opacity or native material alpha. Input/output are straight HDR.
 result[id.xy]=all(isfinite(water))?float4(water.rgb,base.a):base;
}
