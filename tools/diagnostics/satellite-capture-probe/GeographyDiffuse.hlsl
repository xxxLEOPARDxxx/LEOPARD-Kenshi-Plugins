// Fixed static SRVs, explicit gradients and initialized branch result.
float4 GeoDiffuse(float2 uv,float layer,float2 dx,float2 dy) {
    float4 colour=float4(0,0,0,0);
    [branch] if(layer<24.0) colour=diffuseMaps.SampleGrad(Anisotropic,float3(uv,layer),dx,dy);
    else colour=geoDiffuse1.SampleGrad(Anisotropic,float3(uv,layer-24.0),dx,dy);
    return colour;
}
