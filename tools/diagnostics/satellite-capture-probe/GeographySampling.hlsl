// Shared production selector/residual-weight rules. World coordinates only;
// native patch centers and mesh LOD never choose the biome for a whole patch.
uint4 GeographyCodes(float2 worldXZ) {
    int2 cell=(int2)floor((worldXZ+73728.0)/9216.0);
    if(any(cell<0)||any(cell>7))return uint4(0,0,0,0);
    return (uint4)round(geoSelector.Load(int3(cell,0))*255.0);
}
float4 GeographyWeights(uint4 selected,float4 blendWeights) {
    if((selected[0]&15u)==0)return float4(0,0,0,0);
    float4 weights=float4(1,0,0,0);
    [unroll] for(uint i=1;i<4;++i)if((selected[i]&15u)!=0) {
        weights[i]=blendWeights[selected[i]>>4u];weights[0]-=weights[i];
    }
    return weights;
}
