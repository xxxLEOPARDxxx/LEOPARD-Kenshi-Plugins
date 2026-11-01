from pathlib import Path
import hashlib,json,struct,sys
from geography_vertex_source import native_vertex,owned_vertex
here=Path(__file__).resolve().parent;out=here/'build/geography';out.mkdir(exist_ok=True)
helper=(Path(sys.argv[1]) if len(sys.argv)>1 else here/'GeographyCliff.hlsl').read_text()
fixture=(here/'geography_effects_fixture.hlsl').read_text()
native=native_vertex().replace('main_vs (','NativeMainVs (').replace('uniform ','')
owned=owned_vertex(helper).replace('main_vs (','OwnedMainVs (').replace('uniform ','')
for kind,body in (('reference',native),('owned',owned)):
    (out/('effects-'+kind+'.hlsl')).write_text(fixture.replace('// VERTEX_FUNCTION',body))
record_bytes=Path('D:/DEV/Kenshi/research/global-satellite-20261031/biome-current-36248/active-biomes.json').read_bytes()
assert hashlib.sha256(record_bytes).hexdigest()=='511a8813928114dbf6b2697d5fa46a46e858c46690907cf8493e092c84633d0f'
records=json.loads(record_bytes)
by_sid={r['sid']:r for r in records['records'] if 'sid' in r};manifest=json.loads((here/'geography-inputs.json').read_text())
data=[]
def f32(value):return struct.unpack('<f',struct.pack('<f',value))[0]
for biome in manifest['biomes']:
    f=by_sid[biome['sid']]['float'];w=f32(f['distort wavelength'])
    # Match native SSE scalarfloat division/multiplication, including amplitudes
    # such as5*.01f that differ from rounding a doubleprecision .05 literal.
    data.extend([f32(1/w) if w>0 else 0,f32(f32(f['distort amplitude'])*f32(.01)),0,0])
(out/'effects-distortion.bin').write_bytes(struct.pack('<60f',*data))
print('Prepared pinned native/owned VS fixtures,15 actual signed distortion records')
