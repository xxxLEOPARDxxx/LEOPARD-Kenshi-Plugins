"""Pinned native-material fixture rows and literal source, no game calls."""
from pathlib import Path
import hashlib,json,struct
here=Path(__file__).resolve().parent;out=here/'build/geography';out.mkdir(exist_ok=True)
raw=Path('D:/DEV/Kenshi/research/global-satellite-20261031/biome-current-36248/active-biomes.json').read_bytes()
assert hashlib.sha256(raw).hexdigest()=='511a8813928114dbf6b2697d5fa46a46e858c46690907cf8493e092c84633d0f'
records={r['sid']:r for r in json.loads(raw)['records'] if 'sid' in r};manifest=json.loads((here/'geography-inputs.json').read_text())
def f32(v):return struct.unpack('<f',struct.pack('<f',v))[0]
def literal(v):
    s=format(f32(v),'.9g');return s+('' if '.' in s or 'e' in s.lower() else '.0')+'f'
lines=['struct FadeFixture {unsigned argb;float distance;float expected[4];float brightness;};','static const FadeFixture FadeFixtures[15]={']
for b in manifest['biomes']:
    r=records[b['sid']];argb=r['int']['ground colour']&0xffffffff;distance=f32(r['float']['fade distance'])
    expected=[f32(((argb>>shift)&255)*f32(1/255)) for shift in (16,8,0)]+[f32(1/distance) if distance>0 else distance]
    lines.append('{0x%08xu,%s,{%s},%s},'%(argb,literal(distance),','.join(map(literal,expected)),literal(r['float']['brightness fix'])))
lines+=['};'];(out/'GeographyFadeInputs.h').write_text('\n'.join(lines)+'\n')
def function(text,needle):
    start=text.index(needle);brace=text.index('{',start);depth=1;at=brace+1
    while depth:
        if text[at]=='{':depth+=1
        if text[at]=='}':depth-=1
        at+=1
    return text[start:at]
native_bytes=Path('D:/SteamLibrary/steamapps/common/Kenshi/data/materials/deferred/terrainfp4.hlsl').read_bytes()
assert hashlib.sha256(native_bytes).hexdigest()=='050de26aa6082155fe95bf027b5783b86d87ae6cc03381d9c761b731a88327d7'
native=native_bytes.decode().replace('\r\n','\n');owned=(here/'build/GeographyTerrain.hlsl').read_text()
assert hashlib.sha256((here/'build/GeographyTerrain.hlsl').read_bytes()).hexdigest()=='3cde7fa65e27b76092bb14fd4636172b3f979ac4db830e677c800433ea0d1d98','production shader source must remain unchanged'
wrapper=(here/'geography_fade_fixture.hlsl').read_text()
structure=function(native,'struct BiomeOutput')+';'
for kind,body in [('reference',function(native,'BiomeOutput computeBiome(')),('owned',(here/'GeographyDiffuse.hlsl').read_text()+'\n'+function(owned,'BiomeOutput computeGeoBiome('))]:
    (out/('fade-'+kind+'.hlsl')).write_text(wrapper.replace('// FUNCTIONS',structure+'\n'+body))
print('PASS15 pinned native ARGB/fade rows; scalarfloat1/255 and reciprocal fixtures')
