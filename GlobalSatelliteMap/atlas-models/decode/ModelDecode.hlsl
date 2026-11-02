#include "GBufferDecode.hlsl"
cbuffer DecodeValues:register(b0){float4 rawBounds,cameraFar,sceneToCache,limits;};
Texture2D<float4> modelNormal:register(t1);
Texture2D<float> modelRadial:register(t2);
Texture2D<float4> oldMaterial:register(t3);
Texture2D<float4> oldNormal:register(t4);
Texture2D<float> oldHeight:register(t5);
RWTexture2D<float4> mergedMaterial:register(u0);
RWTexture2D<float4> mergedNormal:register(u1);
RWTexture2D<float> mergedHeight:register(u2);
[numthreads(8,8,1)]
void main(uint3 id:SV_DispatchThreadID){
 uint2 p=id.xy;float4 material=oldMaterial.Load(int3(p,0)),normal=oldNormal.Load(int3(p,0));float height=oldHeight.Load(int3(p,0));
 float depth=modelRadial.Load(int3(p,0));float3 n=modelNormal.Load(int3(p,0)).rgb*2-1;
 if(depth>0&&isfinite(depth)&&all(isfinite(n))&&dot(n,n)>1e-8){
  float2 xz=rawBounds.xy+(float2(p)+.5)/256.0*rawBounds.zw;
  float2 dxz=xz-cameraFar.xz;float radial=depth*cameraFar.w,horizontal=dot(dxz,dxz),q=radial*radial-horizontal;
  // Error interval includes radial-depth float storage and reconstruction
  // arithmetic. Ill-conditioned horizon/invalid roots do not invent Y=0.
  float errorQ=limits.x*(radial*radial+horizontal);
  if(isfinite(q)&&isfinite(errorQ)&&q>errorQ){
   float vertical=sqrt(q),modelHeight=cameraFar.y-vertical+sceneToCache.y;
   float errorY=errorQ/(vertical+sqrt(q-errorQ))+limits.x*(abs(cameraFar.y)+vertical+abs(sceneToCache.y));
   float4 packed=SatelliteAlbedoSample(int2(p),256);float3 rgb=SatelliteDecodeAlbedo(int2(p),256);
   if(isfinite(modelHeight)&&isfinite(errorY)&&all(isfinite(rgb))&&isfinite(packed.a)&&(normal.a<=0||!isfinite(height)||modelHeight-errorY>height)){
    material=float4(rgb,packed.a);normal=float4(normalize(n)*.5+.5,1);height=modelHeight;
   }
  }
 }
 mergedMaterial[p]=material;mergedNormal[p]=normal;mergedHeight[p]=height;
}
