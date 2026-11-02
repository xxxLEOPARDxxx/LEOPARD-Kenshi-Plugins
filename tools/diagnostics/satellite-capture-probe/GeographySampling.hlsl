// Shared production selector/residual-weight rules. World coordinates only;
// native patch centers and mesh LOD never choose the biome for a whole patch.
uint4 GeographyCodes(float2 worldXZ,float4 selectorRegion) {
    int2 cell=(int2)floor((worldXZ-selectorRegion.xy)/9216.0);
    if(any(cell<0)||any(cell>=(int2)selectorRegion.zw))return uint4(0,0,0,0);
    return (uint4)round(geoSelector.Load(int3(cell,0))*255.0);
}
// Historical primary1,1 fixture entry point; production binds explicit bounds.
uint4 GeographyCodes(float2 worldXZ) {
    return GeographyCodes(worldXZ,float4(-73728.0,-73728.0,8.0,8.0));
}
float4 GeographyWeights(uint4 selected,float4 blendWeights) {
    if((selected[0]&15u)==0)return float4(0,0,0,0);
    float4 weights=float4(1,0,0,0);
    [unroll] for(uint i=1;i<4;++i)if((selected[i]&15u)!=0) {
        weights[i]=blendWeights[selected[i]>>4u];weights[0]-=weights[i];
    }
    return weights;
}
