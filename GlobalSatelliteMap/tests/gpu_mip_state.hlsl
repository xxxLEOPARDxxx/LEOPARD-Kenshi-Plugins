float4 vs(uint vertex : SV_VertexID) : SV_Position { return float4(vertex,0,0,1); }
float4 ps() : SV_Target { return float4(1,0,1,1); }
[numthreads(1,1,1)] void cs(uint3 id : SV_DispatchThreadID) {}
