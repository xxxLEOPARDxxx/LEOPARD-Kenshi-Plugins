// Test oracle: actual native SunProbe body. The fixture supplies exposure.y=0
// to preserve raw HDR, and explicit LOD0 for compute ambient sampling.
#define Sample(s,u) SampleLevel(s,u,0)
#include "../shaders/SunProbe.hlsl"
RWTexture2D<float4> referenceResult : register(u0);
[numthreads(8,8,1)]
void main(uint3 id:SV_DispatchThreadID){
 if(id.x<(uint)captureWindow.w&&id.y<(uint)captureWindow.w)
  referenceResult[id.xy]=main_ps(float4(float2(id.xy)+.5,0,1));
}
