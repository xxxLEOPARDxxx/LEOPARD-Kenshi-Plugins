#include "OrthographicPosition.hlsl"
// Use the installed game's PBR and GBuffer decoding; these assets are not redistributed.
#include "constants.hlsl"
#include "gbuffer.hlsl"
#include "lightingFunctions.hlsl"
sampler albedoBuffer : register(s0);
sampler normalBuffer : register(s1);
sampler depthBuffer : register(s2);
sampler ambientMap : register(s3);
samplerCUBE irradianceCube : register(s4);
samplerCUBE specularityCube : register(s5);
float4 captureWindow;
float3 captureRight, captureUp, captureBack, captureCameraDelta;
float3 sunDirection, worldOffset;
float4 sunColour, envColour, ambientParams, worldSize;
float4 main_ps(float4 pixel : SV_Position) : SV_Target
{
    float2 uv = pixel.xy / captureWindow.w;
    float radial = tex2D(depthBuffer, uv).r * captureWindow.z;
    float2 lateral = (uv - 0.5f) * float2(captureWindow.x, -captureWindow.y);
    float3 viewPosition;
    if (!SatelliteOrthographicPosition(lateral, radial, viewPosition))
        return float4(0,0,0,0);
    float3 relativeWorld = SatelliteReferenceRelativePosition(viewPosition, captureCameraDelta,
        captureRight, captureUp, captureBack);
    float3 albedo = decodePixel(albedoBuffer, uv,
        float4(captureWindow.ww, 1.0f/captureWindow.ww), pixel.xy);
    float2 metalGloss = tex2D(albedoBuffer, uv).ba;
    float4 encodedNormal = tex2D(normalBuffer, uv);
    float3 normal = normalize(encodedNormal.xyz * 2.0f - 1.0f);
    float translucency = encodedNormal.w < 0.5f ? encodedNormal.w * 2.0f : 0.0f;
    float emissive = max(0.0f, encodedNormal.w - 0.5f) * 6.4f;
    float2 ambientUV = SatelliteAmbientUV(relativeWorld, worldOffset, worldSize);
    float4 ambient = tex2D(ambientMap, ambientUV);
    float3 lightDirection = normalize(sunDirection);
    float3 lightColour = sunColour.rgb * sunColour.w * ambientParams.w * ambient.w * 2.0f;
    float3 specularColour = lerp(dielectric_spec, albedo, metalGloss.x);
    albedo = lerp(albedo, 0.0f, metalGloss.x);
    // Orthographic rays are parallel; the surface-to-camera vector is constant.
    float3 viewDirection = normalize(captureBack);
    LightingData direct = CalcPunctualLight(normal, lightDirection, viewDirection,
        metalGloss.y, specularColour, lightColour, translucency);
    LightingData environment = CalcEnvironmentLight(irradianceCube, specularityCube,
        normal, viewDirection, metalGloss.y, specularColour);
    float environmentScale = envColour.w * clamp(lightDirection.y * 5.0f + 0.2f, 0.1f, 1.0f);
    environment.diffuse *= ambient.rgb * environmentScale;
    environment.specular *= ambient.rgb * environmentScale;
    float3 colour = direct.specular + environment.specular +
                    (direct.diffuse + environment.diffuse) * albedo + albedo * emissive;
    return float4(colour, 1.0f);
}
