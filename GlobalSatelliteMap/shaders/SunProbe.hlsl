#include "OrthographicPosition.hlsl"
// Use the installed game's PBR and GBuffer decoding; these assets are not redistributed.
#include "constants.hlsl"
#include "GBufferDecode.hlsl"
#include "lightingFunctions.hlsl"
Texture2D<float4> normalBuffer : register(t1);
Texture2D<float> depthBuffer : register(t2);
Texture2D<float4> ambientMap : register(t3);
TextureCube<float4> irradianceCube : register(t4);
TextureCube<float4> specularityCube : register(t5);
SamplerState ambientSampler : register(s3);
SamplerState irradianceSampler : register(s4);
SamplerState specularitySampler : register(s5);
float4 captureWindow;
float3 captureRight, captureUp, captureBack, captureCameraDelta;
float3 sunDirection, worldOffset;
float4 sunColour, envColour, ambientParams, worldSize;
float4 main_ps(float4 pixel : SV_Position) : SV_Target
{
    float2 uv = pixel.xy / captureWindow.w;
    int2 pixelCoord = int2(pixel.xy);
    float radial = depthBuffer.Load(int3(pixelCoord,0)).r * captureWindow.z;
    float2 lateral = (uv - 0.5f) * float2(captureWindow.x, -captureWindow.y);
    float3 viewPosition;
    if (!SatelliteOrthographicPosition(lateral, radial, viewPosition))
        return float4(0,0,0,0);
    float3 relativeWorld = SatelliteReferenceRelativePosition(viewPosition, captureCameraDelta,
        captureRight, captureUp, captureBack);
    float3 albedo = SatelliteDecodeAlbedo(pixelCoord,int(captureWindow.w));
    float2 metalGloss = albedoBuffer.Load(int3(pixelCoord,0)).ba;
    float4 encodedNormal = normalBuffer.Load(int3(pixelCoord,0));
    float3 normal = normalize(encodedNormal.xyz * 2.0f - 1.0f);
    float translucency = encodedNormal.w < 0.5f ? encodedNormal.w * 2.0f : 0.0f;
    float emissive = max(0.0f, encodedNormal.w - 0.5f) * 6.4f;
    float2 ambientUV = SatelliteAmbientUV(relativeWorld, worldOffset, worldSize);
    float4 ambient = ambientMap.Sample(ambientSampler,ambientUV);
    float3 lightDirection = normalize(sunDirection);
    float3 lightColour = sunColour.rgb * sunColour.w * ambientParams.w * ambient.w * 2.0f;
    float3 specularColour = lerp(dielectric_spec, albedo, metalGloss.x);
    albedo = lerp(albedo, 0.0f, metalGloss.x);
    // Orthographic rays are parallel; the surface-to-camera vector is constant.
    float3 viewDirection = normalize(captureBack);
    LightingData direct = CalcPunctualLight(normal, lightDirection, viewDirection,
        metalGloss.y, specularColour, lightColour, translucency);
    float4 irradiance = irradianceCube.SampleLevel(irradianceSampler,normal,3.0f);
    float3 specularDirection = GetSpecularDominantDir(normal,-reflect(viewDirection,normal),metalGloss.y);
    float4 reflected = specularityCube.SampleLevel(specularitySampler,specularDirection,(1.0f-metalGloss.y)*7.0f);
    LightingData environment;
    environment.diffuse = irradiance.rgb*irradiance.a*4.0f*FresnelDiffuse(specularColour);
    environment.specular = LazarovEnvironmentBRDF(metalGloss.y,saturate(dot(viewDirection,normal)),specularColour)*reflected.rgb*reflected.a*10.0f;
    float environmentScale = envColour.w * clamp(lightDirection.y * 5.0f + 0.2f, 0.1f, 1.0f);
    environment.diffuse *= ambient.rgb * environmentScale;
    environment.specular *= ambient.rgb * environmentScale;
    float3 colour = direct.specular + environment.specular +
                    (direct.diffuse + environment.diffuse) * albedo + albedo * emissive;
    return float4(colour, 1.0f);
}
