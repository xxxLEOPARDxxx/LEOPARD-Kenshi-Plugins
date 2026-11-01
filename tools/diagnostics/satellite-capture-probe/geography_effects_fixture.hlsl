cbuffer Fixture : register(b0) {
    row_major float4x4 fixtureWvp;
    row_major float4x4 fixtureWorld;
    float4 fixtureMorph;
    float4 fixtureDistortion[15];
    float4 fixtureOptions;
};
struct Input {float4 position:POSITION;float3 normal:NORMAL;float3 normal1:BINORMAL;};
struct Output {float4 position:SV_Position;float3 normal:TEXCOORD0;float3 world:TEXCOORD1;float3 coord:TEXCOORD2;float4 map:TEXCOORD3;float2 blend:TEXCOORD4;float4 a:TEXCOORD5;float4 b:TEXCOORD6;float4 c:TEXCOORD7;float4 d:TEXCOORD8;};
// VERTEX_FUNCTION
Output fixture_vs(Input v) {
    Output o;
#ifdef REFERENCE
    float4 unused;
    NativeMainVs(v.position,v.normal,v.normal1,o.position,o.normal,o.world,o.coord,o.map,o.blend,unused,fixtureWvp,fixtureWorld,fixtureMorph.x,0,0,float4(-147456,-147456,147456,147456),float4(-147456,-147456,147456,147456),0,0);
    float heights[16];
    [unroll]for(uint sid=0;sid<15;++sid) {
        float4 clip,map,vertical;float3 normal,world,coord;float2 blend;float4 d0=0,d1=0;uint slot=(uint)fixtureOptions.y;
        if(slot==0)d0.xy=fixtureDistortion[sid].xy;
        if(slot==1)d0.zw=fixtureDistortion[sid].xy;
        if(slot==2)d1.xy=fixtureDistortion[sid].xy;
        if(slot==3)d1.zw=fixtureDistortion[sid].xy;
        NativeMainVs(v.position,v.normal,v.normal1,clip,normal,world,coord,map,blend,vertical,fixtureWvp,fixtureWorld,fixtureMorph.x,d0,d1,float4(-147456,-147456,147456,147456),float4(-147456,-147456,147456,147456),0,0);
        heights[sid]=slot==0?coord.z:vertical[slot-1];
    }
    heights[15]=o.coord.z;
    o.a=float4(heights[0],heights[1],heights[2],heights[3]);o.b=float4(heights[4],heights[5],heights[6],heights[7]);
    o.c=float4(heights[8],heights[9],heights[10],heights[11]);o.d=float4(heights[12],heights[13],heights[14],heights[15]);
#else
    OwnedMainVs(v.position,v.normal,v.normal1,o.position,o.normal,o.world,o.coord,o.map,o.blend,o.a,o.b,o.c,o.d,fixtureWvp,fixtureWorld,fixtureMorph.x,fixtureDistortion,float4(-147456,-147456,147456,147456),float4(-147456,-147456,147456,147456),0,0);
#endif
    return o;
}
struct Result {float4 cliff:SV_Target0;float4 normal:SV_Target1;float4 world:SV_Target2;float4 coord:SV_Target3;float4 map:SV_Target4;};
Result fixture_ps(Output i) {
    uint sid=(uint)fixtureOptions.x;
    if(sid==15)sid=((uint)i.position.x&1u)?9u:1u;
#ifdef REFERENCE
    float4 values[4]={i.a,i.b,i.c,i.d};float4 dx[4]={ddx(i.a),ddx(i.b),ddx(i.c),ddx(i.d)};float4 dy[4]={ddy(i.a),ddy(i.b),ddy(i.c),ddy(i.d)};
    float3 cliff=0;[unroll]for(uint n=0;n<15;++n)if(sid==n)cliff=float3(values[n>>2][n&3],dx[n>>2][n&3],dy[n>>2][n&3]);
#else
    GeographyCliffData prepared=GeographyCliffPrepare(i.a,i.b,i.c,i.d);
    float3 cliff=GeographyCliffSelect(prepared,sid+1u);
#endif
    Result o;o.cliff=float4(cliff,i.d.w);o.normal=float4(i.normal,i.blend.x);o.world=float4(i.world,i.blend.y);o.coord=float4(i.coord,i.position.z);o.map=i.map;return o;
}
