// Terrain base pass only: decoded LINEAR HDR albedo/gloss, world normal,
// coverage and world height from GST2. Native assets remain installed includes.
#include "constants.hlsl"
#include "lightingFunctions.hlsl"
#include "../shaders/NativeExposure.hlsl"
Texture2D<float4> materialPlane : register(t0);
Texture2D<float4> normalCoveragePlane : register(t1);
Texture2D<float> heightPlane : register(t2);
Texture2D<float4> ambientMap : register(t3);
TextureCube<float4> irradianceCube : register(t4);
TextureCube<float4> specularityCube : register(t5);
SamplerState ambientSampler : register(s0);
SamplerState irradianceSampler : register(s1);
SamplerState specularitySampler : register(s2);
RWTexture2D<float4> result : register(u0);
cbuffer AtlasUniforms : register(b0) {
    float4 tileBounds;       // minX,minZ,spanX,spanZ; absolute world coordinates
    float4 referenceSide;    // native SharedDeferredParams.camera.xyz, selected side
    float4 sunDirection;
    float4 worldOffset;
    float4 sunColour;
    float4 envColour;
    float4 ambientParams;
    float4 worldSize;
    float4 exposure;         // native exposure key/luminance (positive)
    float4 viewDirection;    // constant surface-to-camera direction (top-down)
    float4 dispatchWindow;   // originX,originZ,width,height; helper-owned512 blocks
};
[numthreads(8,8,1)]
void main(uint3 id : SV_DispatchThreadID) {
    uint side=(uint)referenceSide.w;
    if(id.x>=(uint)dispatchWindow.z||id.y>=(uint)dispatchWindow.w)return;
    uint2 global=id.xy+uint2(dispatchWindow.xy);
    if(global.x>=side||global.y>=side)return;
    int3 at=int3(global,0);
    float4 material=materialPlane.Load(at), encoded=normalCoveragePlane.Load(at);
    float height=heightPlane.Load(at);
    float3 n=encoded.xyz*2-1;
    if(encoded.a<=0||!all(isfinite(material))||!all(isfinite(n))||!isfinite(height)||dot(n,n)<1e-8){result[id.xy]=0;return;}
    float3 normal=normalize(n), albedo=material.rgb;
    float gloss=saturate(material.a);
    float2 xz=tileBounds.xy+(float2(global)+.5)/side*tileBounds.zw;
    float3 relativeWorld=float3(xz.x,height,xz.y)-referenceSide.xyz;
    float2 ambientUV=(relativeWorld.xz+worldOffset.xz)*worldSize.xy+worldSize.zw;
    // Explicit LOD0: cached atlas has no screen-space derivatives. The native
    // ambient lookup is spatial, not a photo/exposure tint of captured terrain.
    float4 ambient=ambientMap.SampleLevel(ambientSampler,ambientUV,0);
    float3 lightDirection=normalize(sunDirection.xyz), v=normalize(viewDirection.xyz);
    float3 lightColour=sunColour.rgb*sunColour.w*ambientParams.w*ambient.w*2;
    float3 specularColour=dielectric_spec; // verified terrain metalness=0
    LightingData direct=CalcPunctualLight(normal,lightDirection,v,gloss,specularColour,lightColour,0);
    float4 irradiance=irradianceCube.SampleLevel(irradianceSampler,normal,3);
    float3 specularDirection=GetSpecularDominantDir(normal,-reflect(v,normal),gloss);
    float4 reflected=specularityCube.SampleLevel(specularitySampler,specularDirection,(1-gloss)*7);
    LightingData environment;
    environment.diffuse=irradiance.rgb*irradiance.a*4*FresnelDiffuse(specularColour);
    environment.specular=LazarovEnvironmentBRDF(gloss,saturate(dot(v,normal)),specularColour)*reflected.rgb*reflected.a*10;
    float scale=envColour.w*clamp(lightDirection.y*5+.2,.1,1);
    environment.diffuse*=ambient.rgb*scale;
    environment.specular*=ambient.rgb*scale;
    float3 colour=direct.specular+environment.specular+(direct.diffuse+environment.diffuse)*albedo;
    colour=SatelliteNativeExposure(colour,exposure.xy);
    result[id.xy]=all(isfinite(colour))?float4(colour,encoded.a):float4(0,0,0,0);
}
