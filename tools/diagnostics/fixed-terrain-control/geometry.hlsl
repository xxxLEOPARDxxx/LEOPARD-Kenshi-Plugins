// Diagnostic only: explicit pinned native morph0 declaration, no game ABI call.
cbuffer View : register(b0) { float4 bounds; } // centerX,centerZ,span,unused
struct Input { float4 position:POSITION; float3 normal:NORMAL; float3 target:BINORMAL; };
struct Output { float4 position:SV_Position; float3 normal:TEXCOORD0; float height:TEXCOORD1; };
Output vertex(Input v) {
    Output o;
    o.position=float4((v.position.x-bounds.x)*2/bounds.z,
                     -(v.position.z-bounds.y)*2/bounds.z,
                     (10000-v.position.y)/10000,1);
    o.normal=normalize(v.normal.zxy);
    o.height=v.position.y;
    return o;
}
float4 pixel(Output v):SV_Target {return float4(normalize(v.normal),v.height);}
