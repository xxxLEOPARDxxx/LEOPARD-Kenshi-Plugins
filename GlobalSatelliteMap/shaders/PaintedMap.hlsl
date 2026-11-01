#ifndef LEOPARD_SATELLITE_PAINTED_MAP
#define LEOPARD_SATELLITE_PAINTED_MAP
// Four native painted tiles in x+2*z order, with the native18m UV border.
// FirstCore is the lower world-XZ corner of the primary73728m capture cell.
// The caller supplies derivatives of worldXZ/36900 to SampleGrad, avoiding the
// UV reset at an array-layer seam influencing minification derivatives.
float3 SatellitePaintedCoordinates(float2 worldXZ,float2 firstCore)
{
    float2 tile=clamp(floor((worldXZ-firstCore)/36864.0f),0.0f,1.0f);
    float2 minimum=firstCore+tile*36864.0f-18.0f;
    return float3((worldXZ-minimum)/36900.0f,tile.x+2.0f*tile.y);
}
#endif
