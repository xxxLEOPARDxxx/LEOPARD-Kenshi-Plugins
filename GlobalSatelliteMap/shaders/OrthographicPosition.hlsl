#ifndef LEOPARD_SATELLITE_POSITION
#define LEOPARD_SATELLITE_POSITION
bool SatelliteOrthographicPosition(float2 lateral, float radialDistance, out float3 viewPosition)
{
    viewPosition = float3(0, 0, 0);
    if (!all(isfinite(float3(lateral, radialDistance))) || radialDistance <= 0.0f)
        return false;
    float radialSquared = radialDistance * radialDistance;
    float lateralSquared = dot(lateral, lateral);
    if (!isfinite(radialSquared) || !isfinite(lateralSquared))
        return false;
    float forwardSquared = radialSquared - lateralSquared;
    // A relative float32 rounding allowance at the squared-distance scale permits grazing
    // samples, while materially impossible radial/lateral combinations fail closed.
    float tolerance = 4.76837158203125e-7f * max(radialSquared, lateralSquared);
    if (forwardSquared < -tolerance)
        return false;
    viewPosition = float3(lateral, -sqrt(max(0.0f, forwardSquared)));
    return true;
}
float3 SatelliteReferenceRelativePosition(float3 viewPosition, float3 captureCameraDelta,
                                        float3 right, float3 up, float3 back)
{
    return captureCameraDelta + right * viewPosition.x + up * viewPosition.y + back * viewPosition.z;
}
float2 SatelliteAmbientUV(float3 relativeWorld, float3 worldOffset, float4 worldSize)
{
    return (relativeWorld.xz + worldOffset.xz) * worldSize.xy + worldSize.zw;
}
#endif
