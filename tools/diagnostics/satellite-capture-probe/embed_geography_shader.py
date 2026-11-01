"""Native-derived private geography PS; no installed shader edits."""
from pathlib import Path
import re,json
here=Path(__file__).resolve().parent
text=(here/'build/PaintedTerrain.hlsl').read_text(encoding='ascii')
start=text.index('BiomeOutput computeBiome(');end=text.index('void main_fs (',start)
function=text[start:end].replace('computeBiome(Texture2DArray diffuseMaps, Texture2DArray normalMaps,','computeGeoBiome(')
function=function.replace('float4 absorbance[2], float4 distant, float distance) {',
    'float4 absorbance[2], float4 distant, float distance, float4 difA, float4 difB, float4 nmlA, float4 nmlB, float3 gradX, float3 gradY) {')
assert 'computeGeoBiome(' in function and 'float3 gradX' in function
def replace_samples(match):
    kind,uv,slot=match.groups();slot=int(slot)
    index=('dif' if kind=='diffuseMaps' else 'nml')+('A[%d]'%slot if slot<4 else 'B[%d]'%(slot-4))
    dx=uv.replace('texCoords','gradX');dy=uv.replace('texCoords','gradY')
    if kind=='diffuseMaps':return 'GeoDiffuse(%s,%s,%s,%s)'%(uv,index,dx,dy)
    return 'normalMaps.SampleGrad(Anisotropic,float3(%s,%s),%s,%s)'%(uv,index,dx,dy)
function,n=re.subn(r'(diffuseMaps|normalMaps)\.Sample\(Anisotropic, float3\(([^,]+), ([0-5])\)\)',replace_samples,function)
assert n==14
declarations='''Texture2D geoBlend : register(t5);
Texture2D geoSelector : register(t6);
Texture2DArray geoDiffuse1 : register(t7);
uniform float4 satBiome[225];
'''
declarations+=(here/'GeographyDiffuse.hlsl').read_text(encoding='ascii')
text=text[:end]+function+text[end:]
declarations+=(here/'GeographySampling.hlsl').read_text(encoding='ascii')
text=text.replace('struct BiomeOutput {',declarations+'\nstruct BiomeOutput {',1)
start=text.index('\t// Sample biome',text.index('void main_fs ('));end=text.index('\t// Wetness',start)
body='''    // World-fragment selection, independent of native patch centers/LOD.
    uint4 selected=GeographyCodes(worldPos.xz);
    clip((selected[0]&15u)!=0?1.0:-1.0);
    float2 blendUV=(worldPos.xz+147456.0)/294912.0;
    float4 geoWeights=geoBlend.SampleGrad(Linear,blendUV,ddx(worldPos.xz)/294912.0,ddy(worldPos.xz)/294912.0);
    float3 geoDx=ddx(texCoords),geoDy=ddy(texCoords);
    float4 selectedWeights=GeographyWeights(selected,geoWeights);
    BiomeOutput biome;biome.albedo=0;biome.normal=0;biome.absorbance=0;
    [unroll] for(uint slot=0;slot<4;++slot) {
        uint code=selected[slot];
        if((code&15u)!=0) {
            uint row=((code&15u)-1u)*15u;
            float4 slopeWeights=smoothstep(satBiome[row+3]-satBiome[row+5],satBiome[row+3],slope)*
                smoothstep(satBiome[row+4]+satBiome[row+5],satBiome[row+4],slope);
            float4 absorbValues[2]={satBiome[row+8],satBiome[row+9]};
            BiomeOutput part=computeGeoBiome(texCoords,cliffBlend,normal,satBiome[row],satBiome[row+1],satBiome[row+2],
                slopeWeights,map,colour,satBiome[row+6],absorbValues,satBiome[row+7],distance,
                satBiome[row+11],satBiome[row+12],satBiome[row+13],satBiome[row+14],geoDx,geoDy);
            part.albedo.rgb*=satBiome[row+10].x;
            float weight=selectedWeights[slot];
            biome.albedo+=part.albedo*weight;biome.normal+=part.normal*weight;biome.absorbance+=part.absorbance*weight;
        }
    }
    // First geography isolation preserves current zero cliff-distortion/fade.
'''
text=text[:start]+body+text[end:]
assert len(text)<32768 and text.isascii()
(here/'build/GeographyTerrain.hlsl').write_text(text,encoding='ascii')
(here/'build/GeographyShader.h').write_text('static const char GeographyShaderSource[]=\n'+'\n'.join(json.dumps(line+'\n') for line in text.splitlines())+';\n',encoding='ascii')
print('Embedded native-derived geographic PS:',len(text),'ASCII bytes; eight fixed SRVs and explicit SampleGrad')
