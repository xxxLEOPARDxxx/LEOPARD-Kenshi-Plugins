#ifndef LEOPARD_SATELLITE_GBUFFER_DECODE
#define LEOPARD_SATELLITE_GBUFFER_DECODE
Texture2D<float4> albedoBuffer : register(t0);
float4 SatelliteAlbedoSample(int2 pixel, int side)
{
    return albedoBuffer.Load(int3(clamp(pixel, int2(0,0), int2(side-1,side-1)),0));
}
float3 SatelliteDecodeAlbedo(int2 pixel, int side)
{
    float4 center = SatelliteAlbedoSample(pixel,side);
    float2 a = SatelliteAlbedoSample(pixel+int2(1,0),side).rg;
    float2 b = SatelliteAlbedoSample(pixel-int2(1,0),side).rg;
    float2 c = SatelliteAlbedoSample(pixel+int2(0,1),side).rg;
    float2 d = SatelliteAlbedoSample(pixel-int2(0,1),side).rg;
    float4 weights = 1.0f-step(30.0f/255.0f,abs(float4(a.x,b.x,c.x,d.x)-center.x));
    float sum = dot(weights,1.0f);
    if (sum == 0.0f) { weights=float4(1,0,0,0);sum=1; }
    float other = dot(weights,float4(a.y,b.y,c.y,d.y))/sum;
    bool sameParity = (pixel.x & 1) == (pixel.y & 1);
    float co = (sameParity ? other : center.g)-0.5f;
    float cg = (sameParity ? center.g : other)-0.5f;
    return float3(center.r+co-cg,center.r+cg,center.r-co-cg);
}
#endif
