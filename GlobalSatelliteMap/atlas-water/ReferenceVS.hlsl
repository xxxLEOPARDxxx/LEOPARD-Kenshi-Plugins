#include "WaterUniforms.hlsl"
struct Out {
 float4 position:SV_Position;
 float4 tex:TEXCOORD0;
 float4 refTex:TEXCOORD1;
 float3 sun:TEXCOORD2;
 float3 cam:TEXCOORD3;
 float4 map:TEXCOORD4;
 float2 speedMult:TEXCOORD5;
};
Out main(uint id:SV_VertexID){
 float2 pos=id==0?float2(-1,-1):id==1?float2(-1,3):float2(3,-1);
 float2 uv=pos*float2(.5,-.5)+.5;
 float2 pixel=sourceWindow.yz+uv*dispatchWindow.xy;
 float2 xz=tileBounds.xy+pixel/sourceWindow.x*tileBounds.zw;
 Out o;o.position=float4(pos,.5,1);o.tex=xz.xyxy*normalScale;
 o.refTex=float4(0,0,0,1);o.sun=waterSunDirection.xyz;
 // Reference only: original pixel shader's distant LOD selector must pass.
 // It uses normalize(cam) for BRDF; production supplies the same parallel view.
 o.cam=normalize(parallelView.xyz)*5000;
 o.map=float4((xz-nativeMapBounds.xy)/(nativeMapBounds.zw-nativeMapBounds.xy),xz*.01);
 o.speedMult=normalScale.xy*5000;return o;
}
