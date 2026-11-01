"""Native-derived private geography PS; no installed shader edits."""
from pathlib import Path
import re,json
from geography_vertex_source import owned_vertex
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
cliff=(here/'GeographyCliff.hlsl').read_text(encoding='ascii')
declarations+=cliff
text=text.replace('struct BiomeOutput {',declarations+'\nstruct BiomeOutput {',1)
signature='float4 texCoordsV    : TEXCOORD5,'
assert text.count(signature)==1
text=text.replace(signature,signature+'\n    float4 geoCliff1 : TEXCOORD6,\n    float4 geoCliff2 : TEXCOORD7,\n    float4 geoCliff3 : TEXCOORD8,')
start=text.index('\t// Sample biome',text.index('void main_fs ('));end=text.index('\t// Wetness',start)
body='''    // World-fragment selection, independent of native patch centers/LOD.
    float3 geoDx=ddx(texCoords),geoDy=ddy(texCoords);
    GeographyCliffData geoCliff=GeographyCliffPrepare(texCoordsV,geoCliff1,geoCliff2,geoCliff3);
    uint4 selected=GeographyCodes(worldPos.xz);
    clip((selected[0]&15u)!=0?1.0:-1.0);
    float2 blendUV=(worldPos.xz+147456.0)/294912.0;
    float4 geoWeights=geoBlend.SampleGrad(Linear,blendUV,ddx(worldPos.xz)/294912.0,ddy(worldPos.xz)/294912.0);
    float4 selectedWeights=GeographyWeights(selected,geoWeights);
    BiomeOutput biome;biome.albedo=0;biome.normal=0;biome.absorbance=0;
    [unroll] for(uint slot=0;slot<4;++slot) {
        uint code=selected[slot];
        if((code&15u)!=0) {
            uint row=((code&15u)-1u)*15u;
            float4 slopeWeights=smoothstep(satBiome[row+3]-satBiome[row+5],satBiome[row+3],slope)*
                smoothstep(satBiome[row+4]+satBiome[row+5],satBiome[row+4],slope);
            float4 absorbValues[2]={satBiome[row+8],satBiome[row+9]};
            float3 cliff=GeographyCliffSelect(geoCliff,code&15u);
            float3 coords=texCoords,gradX=geoDx,gradY=geoDy;coords.z=cliff.x;gradX.z=cliff.y;gradY.z=cliff.z;
            BiomeOutput part=computeGeoBiome(coords,cliffBlend,normal,satBiome[row],satBiome[row+1],satBiome[row+2],
                slopeWeights,map,colour,satBiome[row+6],absorbValues,satBiome[row+7],distance,
                satBiome[row+11],satBiome[row+12],satBiome[row+13],satBiome[row+14],gradX,gradY);
            part.albedo.rgb*=satBiome[row+10].x;
            float weight=selectedWeights[slot];
            biome.albedo+=part.albedo*weight;biome.normal+=part.normal*weight;biome.absorbance+=part.absorbance*weight;
        }
    }
    // Native per-SID vertex distortion; this trial still has zero distance fade.
'''
text=text[:start]+body+text[end:]
assert len(text)<32768 and text.isascii()
(here/'build/GeographyTerrain.hlsl').write_text(text,encoding='ascii')
vertex=owned_vertex(cliff)
(here/'build/GeographyVertex.hlsl').write_text(vertex,encoding='ascii')
embedded='static const char GeographyShaderSource[]=\n'+'\n'.join(json.dumps(line+'\n') for line in text.splitlines())+';\n'
embedded+='static const char GeographyVertexSource[]=\n'+'\n'.join(json.dumps(line+'\n') for line in vertex.splitlines())+';\n'
(here/'build/GeographyShader.h').write_text(embedded,encoding='ascii')
print('Embedded native-derived geographic PS:',len(text),'ASCII bytes; eight fixed SRVs and explicit SampleGrad')
