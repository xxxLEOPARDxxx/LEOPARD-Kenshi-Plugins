// Raw native object RGB/gloss is distinct from packed terrain albedo. Never
// pass native normal.w as coverage: it encodes translucency/emissive instead.
#include "constants.hlsl"
#include "lightingFunctions.hlsl"
Texture2D<float4> rawMaterial:register(t0);
Texture2D<float4> rawNormal:register(t1);
Texture2D<float> rawRadial:register(t2);
Texture2D<float4> oldMaterial:register(t3);
Texture2D<float4> oldNormal:register(t4);
Texture2D<float> oldHeight:register(t5);
Texture2D<float4> pristineHdr:register(t6);
Texture2D<float4> ambientMap:register(t7);
TextureCube<float4> irradianceCube:register(t8);
TextureCube<float4> specularityCube:register(t9);
SamplerState ambientSampler:register(s0);
SamplerState irradianceSampler:register(s1);
SamplerState specularitySampler:register(s2);
RWTexture2D<float4> acceptedMaterial:register(u0);
RWTexture2D<float4> acceptedNormal:register(u1);
RWTexture2D<float> acceptedHeight:register(u2);
RWTexture2D<float4> acceptedHdr:register(u3);
cbuffer TownValues:register(b0){
 float4 tileBounds,referenceSide,sunDirection,worldOffset,sunColour,envColour;
 float4 ambientParams,worldSize,exposure,viewDirection,dispatchWindow;
 float4 cameraFar,sceneToCache,limits;
};
// Exact current native SunProbe closure, with already-linear RGB (verified
// black metal), explicit ambient LOD0 and no exposure/tone mapping. Native
// normal.w is decoded BEFORE the independent accepted coverage is written.
float3 TownLight(float4 material,float4 encoded,float3 absoluteWorld){
 float3 albedo=material.rgb,normal=normalize(encoded.xyz*2-1);
 float translucency=encoded.w<.5?encoded.w*2:0;
 float emissive=max(0,encoded.w-.5)*6.4;
 float gloss=saturate(material.a),specularColour=dielectric_spec;
 float3 relativeWorld=absoluteWorld-referenceSide.xyz;
 float2 ambientUV=(relativeWorld.xz+worldOffset.xz)*worldSize.xy+worldSize.zw;
 float4 ambient=ambientMap.SampleLevel(ambientSampler,ambientUV,0);
 float3 lightDirection=normalize(sunDirection.xyz),v=normalize(viewDirection.xyz);
 float3 lightColour=sunColour.rgb*sunColour.w*ambientParams.w*ambient.w*2;
 LightingData direct=CalcPunctualLight(normal,lightDirection,v,gloss,specularColour,lightColour,translucency);
 float4 irradiance=irradianceCube.SampleLevel(irradianceSampler,normal,3);
 float3 specularDirection=GetSpecularDominantDir(normal,-reflect(v,normal),gloss);
 float4 reflected=specularityCube.SampleLevel(specularitySampler,specularDirection,(1-gloss)*7);
 LightingData environment;
 environment.diffuse=irradiance.rgb*irradiance.a*4*FresnelDiffuse(specularColour);
 environment.specular=LazarovEnvironmentBRDF(gloss,saturate(dot(v,normal)),specularColour)*reflected.rgb*reflected.a*10;
 float scale=envColour.w*clamp(lightDirection.y*5+.2,.1,1);
 environment.diffuse*=ambient.rgb*scale;environment.specular*=ambient.rgb*scale;
 return direct.specular+environment.specular+(direct.diffuse+environment.diffuse)*albedo+albedo*emissive;
}
[numthreads(8,8,1)]void main(uint3 id:SV_DispatchThreadID){
 uint2 p=id.xy;if(any(p>=256))return;
 int3 at=int3(p,0);float4 material=oldMaterial.Load(at),normal=oldNormal.Load(at),hdr=pristineHdr.Load(at);float height=oldHeight.Load(at);
 float4 raw=rawMaterial.Load(at),encoded=rawNormal.Load(at);float depth=rawRadial.Load(at);float3 n=encoded.xyz*2-1;
 // Native depth is radial/far, not clip-spaceZ; corner pixels CAN exceed1.
 if(depth>0&&isfinite(depth)&&all(isfinite(raw))&&all(isfinite(encoded))&&dot(n,n)>1e-8){
  float2 absoluteXZ=tileBounds.xy+(float2(p)+.5)/256*tileBounds.zw;
  float2 dxz=absoluteXZ-sceneToCache.xz-cameraFar.xz;
  float radial=depth*cameraFar.w,horizontal=dot(dxz,dxz),q=radial*radial-horizontal;
  float errorQ=limits.x*(radial*radial+horizontal);
  if(isfinite(q)&&isfinite(errorQ)&&q>errorQ){
   float vertical=sqrt(q),y=cameraFar.y-vertical+sceneToCache.y;
   float errorY=errorQ/(vertical+sqrt(q-errorQ))+limits.x*(abs(cameraFar.y)+vertical+abs(sceneToCache.y));
   if(isfinite(y)&&isfinite(errorY)&&(normal.a<=0||!isfinite(height)||y-errorY>height)){
    float3 colour=TownLight(raw,encoded,float3(absoluteXZ.x,y,absoluteXZ.y));
    if(all(isfinite(colour))&&all(abs(colour)<=65504)){
     material=raw;normal=float4(normalize(n)*.5+.5,1);height=y;hdr=float4(colour,1);
    }
   }
  }
 }
 acceptedMaterial[p]=material;acceptedNormal[p]=normal;acceptedHeight[p]=height;acceptedHdr[p]=hdr;
}
