#include "_generated/NativeLocal.hlsl"
struct GpuLight {float4 positionRange,directionType,diffusePower,spot;};
cbuffer LocalValues : register(b0) {
 float4 tileBounds,sourceWindow,dispatchWindow,sceneToCache,parallelView;
 GpuLight lights[32];
};
Texture2D<float4> terrainHdr : register(t0);
Texture2D<float4> material : register(t1);
Texture2D<float4> normalCoverage : register(t2);
Texture2D<float> absoluteHeight : register(t3);
RWTexture2D<float4> result : register(u0);
float3 Shade(float3 worldPos,float3 normal,float3 albedo,float gloss,GpuLight light){
 float3 position=light.positionRange.xyz,diffuseColour=light.diffusePower.rgb;
 float power=light.diffusePower.a;float4 falloff=float4(0,0,0,light.positionRange.w);
 float3 direction=light.directionType.xyz,spot=light.spot.xyz;
 float3 lightDir=position-worldPos;float distance=length(lightDir);
 if(distance<=1e-6||distance>falloff.w)return 0;
 lightDir/=distance;if(dot(normal,lightDir)<.01)return 0;
 float metalness=0;
 // BEGIN NATIVE LOCAL TAIL
 return NativeLocalTail_PLACEHOLDER;
 // END NATIVE LOCAL TAIL
}
[numthreads(8,8,1)]
void main(uint3 id:SV_DispatchThreadID){
 uint width=(uint)dispatchWindow.x,height=(uint)dispatchWindow.y;if(id.x>=width||id.y>=height)return;
 float4 base=terrainHdr.Load(int3(id.xy,0));uint2 source=id.xy+(uint2)sourceWindow.yz;
 float4 m=material.Load(int3(source,0)),n=normalCoverage.Load(int3(source,0));float h=absoluteHeight.Load(int3(source,0));float3 normal=n.rgb*2-1;
 if(n.a<=0||!all(isfinite(m))||!all(isfinite(normal))||!isfinite(h)||dot(normal,normal)<1e-8){result[id.xy]=base;return;}
 normal=normalize(normal);float2 xz=tileBounds.xy+(float2(source)+.5)/sourceWindow.x*tileBounds.zw;float3 worldPos=float3(xz.x,h,xz.y),sum=0;
 for(uint i=0;i<(uint)dispatchWindow.z;++i)sum+=Shade(worldPos,normal,m.rgb,saturate(m.a),lights[i]);
 float3 rgb=base.rgb+sum;result[id.xy]=all(isfinite(rgb))?float4(rgb,base.a):base;
}
