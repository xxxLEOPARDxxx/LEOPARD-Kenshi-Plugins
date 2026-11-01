Texture2DArray diffuseMaps:register(t0);
Texture2DArray normalMaps:register(t1);
Texture2DArray geoDiffuse1:register(t7);
SamplerState Anisotropic:register(s0);
cbuffer Fixture:register(b0){float4 p[15];};
// FUNCTIONS
float4 fixture_vs(uint id:SV_VertexID):SV_Position {
    return float4(id==2?3.0:-1.0,id==1?3.0:-1.0,0,1);
}
struct Output{float4 albedo:SV_Target0;float4 normal:SV_Target1;float4 absorbance:SV_Target2;};
Output fixture_ps(float4 pixel:SV_Position) {
    float3 coords=float3(pixel.xy/32.0,0.4+pixel.x*.005-pixel.y*.003);
    float3 gradX=ddx(coords),gradY=ddy(coords);
    float4 absorb[2]={p[7],p[8]};
#ifdef REFERENCE
    BiomeOutput b=computeBiome(diffuseMaps,normalMaps,coords,float2(.35,.65),float3(.3,.7,-.2),p[0],p[1],p[2],p[3],p[4],p[5],p[6],absorb,p[9],p[10].x);
#else
    BiomeOutput b=computeGeoBiome(coords,float2(.35,.65),float3(.3,.7,-.2),p[0],p[1],p[2],p[3],p[4],p[5],p[6],absorb,p[9],p[10].x,p[11],p[12],p[13],p[14],gradX,gradY);
#endif
    b.albedo.rgb*=p[10].y;
    Output o;o.albedo=b.albedo;o.normal=b.normal;o.absorbance=float4(b.absorbance,0,0,0);return o;
}
