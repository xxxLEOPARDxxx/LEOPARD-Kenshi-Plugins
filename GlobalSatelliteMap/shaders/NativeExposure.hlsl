// Installed hdrutils.hlsl and constants.hlsl are game dependencies, not bundled assets.
// The caller includes constants.hlsl before this helper. No bloom or adaptive chain.
#include "hdrutils.hlsl"
float3 SatelliteNativeExposure(float3 raw, float2 keyLuminance)
{
    float exposure;
    float3 colour=ToneMap(raw,keyLuminance.y,0,exposure,keyLuminance.x);
    if(hdr_to_gamma) colour=pow(colour,1.0f/2.2f);
    if(hdr_disable) colour=lerp(raw,colour,0.0001f);
    return colour;
}
float3 SatelliteExposureOrRaw(float3 raw,float2 keyLuminance)
{
    // Raw must bypass gamma as well as ToneMap, including after an exposed cell.
    if(keyLuminance.y<=0) return raw;
    return SatelliteNativeExposure(raw,keyLuminance);
}
