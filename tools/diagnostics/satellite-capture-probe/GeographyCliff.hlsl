// Shared production vertex heights and fragment derivatives. Fixed SID order.
void GeographyCliffHeights(float3 position,float4 distortion[15],out float4 a,out float4 b,out float4 c,out float4 d) {
    float h0=1.0-position.y/5000.0;
    float height[16];
    [unroll]for(uint sid=0;sid<15;++sid)
        height[sid]=h0+(cos(position.x*distortion[sid].x)+cos(position.z*distortion[sid].x))*distortion[sid].y;
    height[15]=h0;
    a=float4(height[0],height[1],height[2],height[3]);b=float4(height[4],height[5],height[6],height[7]);
    c=float4(height[8],height[9],height[10],height[11]);d=float4(height[12],height[13],height[14],height[15]);
}
struct GeographyCliffData {float4 value[4];float4 dx[4];float4 dy[4];};
GeographyCliffData GeographyCliffPrepare(float4 a,float4 b,float4 c,float4 d) {
    GeographyCliffData result;result.value[0]=a;result.value[1]=b;result.value[2]=c;result.value[3]=d;
    [unroll]for(uint i=0;i<4;++i){result.dx[i]=ddx(result.value[i]);result.dy[i]=ddy(result.value[i]);}
    return result;
}
float3 GeographyCliffSelect(GeographyCliffData data,uint sid) {
    float3 result=0;
    [unroll]for(uint index=0;index<15;++index)if(sid==index+1u)
        result=float3(data.value[index>>2u][index&3u],data.dx[index>>2u][index&3u],data.dy[index>>2u][index&3u]);
    return result;
}
