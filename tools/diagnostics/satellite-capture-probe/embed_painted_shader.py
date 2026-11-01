"""Build a private native-derived PS; never edits installed shader files."""
from pathlib import Path
import hashlib,json,re,struct

HERE=Path(__file__).resolve().parent
GAME=Path('D:/SteamLibrary/steamapps/common/Kenshi')
SHADERS=HERE.parents[2]/'GlobalSatelliteMap/shaders'
KNOWN={
 'deferred/terrainfp4.hlsl':'050de26aa6082155fe95bf027b5783b86d87ae6cc03381d9c761b731a88327d7',
 'deferred/gbuffer.hlsl':'d6a525658f224379269369c9adaa928fc82a72285aed5cffb21d077df4c9c943',
 'common/wet.hlsl':'e9042fbf57df1463d014761576701dd3c7e8680f70c89adcb9196ffdb5580d11',
 'common/shadowcaster.hlsl':'dde7939a53c5d9c3f79c97d12bf044fb005f93f3f352175148ead7a841cd12f7',
}
def source(relative):
    path=GAME/'data/materials'/relative
    data=path.read_bytes()
    assert hashlib.sha256(data).hexdigest()==KNOWN[relative],('native shader changed',relative)
    return data.decode('utf-8-sig').replace('\r\n','\n')
def expand(relative):
    text=source(relative)
    def include(match):
        name=match.group(1)
        paths=[key for key in KNOWN if Path(key).name==name]
        assert len(paths)==1,('unreviewed include',name)
        return expand(paths[0])
    return re.sub(r'^\s*#include\s+"([^"]+)"\s*$',include,text,flags=re.M)
def replace_once(text,old,new,count=1):
    assert text.count(old)==count,('native shader contract differs',old)
    return text.replace(old,new)
def main():
    text=expand('deferred/terrainfp4.hlsl')
    text=replace_once(text,'SamplerState Nearest {','SamplerState Nearest : register(s4) {')
    text=replace_once(text,'SamplerState Linear {','SamplerState Linear : register(s2) {')
    text=replace_once(text,'SamplerState Anisotropic {','SamplerState Anisotropic : register(s0) {')
    for slot,name in enumerate(('diffuseMaps','normalMaps','overlayMap','colourMap','interiorClip')):
        pattern=rf'(Texture2D(?:Array)?\s+{name}\s*:\s*register\()s{slot}(\);)'
        text,n=re.subn(pattern,rf'\g<1>t{slot}\g<2>',text)
        assert n==1,(name,'texture slot contract differs')
    for name in ('overlayMap','colourMap'):
        text,n=re.subn(rf'Texture2D(\s+{name}\s*:)',r'Texture2DArray\1',text)
        assert n==1
    helper=(SHADERS/'PaintedMap.hlsl').read_text(encoding='ascii')
    text=helper+'\nuniform float4 paintedTileBase;\n'+text
    text=replace_once(text,'float4 map = overlayMap.Sample(Linear,mapCoords.xy);',
        'float3 painted = SatellitePaintedCoordinates(worldPos.xz,paintedTileBase.xy);\n'
        '\tfloat2 paintedDx = ddx(worldPos.xz) / 36900.0f;\n'
        '\tfloat2 paintedDy = ddy(worldPos.xz) / 36900.0f;\n'
        '\tfloat4 map = overlayMap.SampleGrad(Linear,painted,paintedDx,paintedDy);',2)
    text=replace_once(text,'float4 colour = colourMap.Sample(Linear, mapCoords.xy) * 1.2;',
        'float4 colour = colourMap.SampleGrad(Linear,painted,paintedDx,paintedDy) * 1.2;',2)
    assert '#include' not in text and text.isascii() and len(text)<32768
    out=HERE/'build';out.mkdir(exist_ok=True)
    (out/'PaintedTerrain.hlsl').write_text(text,encoding='ascii')
    (out/'PaintedShader.h').write_text('static const char PaintedShaderSource[] =\n'+
        '\n'.join(json.dumps(line+'\n') for line in text.splitlines())+';\n',encoding='ascii')
    print('Private painted PS embedded:',len(text),'ASCII bytes; native inputs checked, includes flattened')
if __name__=='__main__':main()
