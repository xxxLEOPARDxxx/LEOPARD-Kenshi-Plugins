"""Controlled ordered slot staging of authenticated native-derived terrain math.

Expanded game shader snapshots and compiled artifacts remain private/ignored.
Only this component's output directories are written.
"""
from pathlib import Path
import hashlib,importlib.util,json,re,subprocess,sys
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
COMPACT=HERE.parent/'compact-material'
FXC=Path('C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe')
sha=lambda b:hashlib.sha256(b).hexdigest()
spec=importlib.util.spec_from_file_location('staged_source_preparation',COMPACT/'prepare_sources.py')
prep=importlib.util.module_from_spec(spec);spec.loader.exec_module(prep)
# The existing recipe is read-only; destination is THIS new component.
prep.prepare_sources(HERE)
native=(HERE/'_source/GeographyTerrain.hlsl').read_bytes()
assert sha(native)=='f9326a91f29e1f1644fc0acbb33bc126bce04590faef2cc7b89dabd1292fd9a1'
start=native.index(b'void main_fs (');end=native.index(b'void mapfeature_fs (',start)
active=native[start:end]
init=active.index(b'    BiomeOutput biome;biome.albedo=0;biome.normal=0;biome.absorbance=0;')
loop=active.index(b'    [unroll] for(uint slot=0;slot<4;++slot) {',init)
tail=active.index(b'    // Native per-SID vertex distortion;',loop)
prefix=active[:init];ordered=active[loop:tail];native_tail=active[tail:]
assert ordered.count(b'part.albedo.rgb*=satBiome[row+10].x;')==1
assert ordered.count(b'biome.albedo+=part.albedo*weight;biome.normal+=part.normal*weight;biome.absorbance+=part.absorbance*weight;')==1
extra=b'''\nTexture2D<float4> previousAlbedo : register(t8);
Texture2D<float4> previousNormalAbsorb : register(t9);
cbuffer StagedControl : register(b1) { uint StageSlot; uint3 StagePadding; };
struct StagedCarry {float4 albedo:SV_Target0;float4 normalAbsorb:SV_Target1;};
'''
loads=b'''    int2 carryPixel=(int2)fragCoord.xy;
    BiomeOutput biome;
    biome.albedo=previousAlbedo.Load(int3(carryPixel,0));
    float4 normalAbsorb=previousNormalAbsorb.Load(int3(carryPixel,0));
    biome.normal=float4(normalAbsorb.rgb,0);biome.absorbance=normalAbsorb.a;
'''
partial_loads=b'''    int2 carryPixel=(int2)fragCoord.xy;
    BiomeOutput biome;biome.albedo=0;biome.normal=0;biome.absorbance=0;
    if(StageSlot!=0) {
        biome.albedo=previousAlbedo.Load(int3(carryPixel,0));
        float4 normalAbsorb=previousNormalAbsorb.Load(int3(carryPixel,0));
        biome.normal=float4(normalAbsorb.rgb,0);biome.absorbance=normalAbsorb.a;
    }
    clip(StageSlot<4?1.0:-1.0);
'''
partial_prefix=prefix.replace(b'out GBuffer buffer',b'out StagedCarry carry')
assert partial_prefix!=prefix
partial_loop=ordered.replace(b'[unroll] for(uint slot=0;slot<4;++slot) {',b'{ uint slot=StageSlot;')
assert partial_loop.replace(b'{ uint slot=StageSlot;',b'[unroll] for(uint slot=0;slot<4;++slot) {')==ordered
partial=native[:start]+extra+partial_prefix+partial_loads+partial_loop+b'    carry.albedo=biome.albedo;carry.normalAbsorb=float4(biome.normal.rgb,biome.absorbance);\n}\n'
changes=[(b'writeAlbedo   ( buffer, biome.albedo.rgb, fragCoord.xy );',b'buffer.buf0.rgb = biome.albedo.rgb;'),(b'writeMetalness( buffer, 0.0f );',b'/* Native zero metalness does not overwrite full RGB. */')]
rgb_tail=native_tail
for a,b in changes:assert rgb_tail.count(a)==1;rgb_tail=rgb_tail.replace(a,b)
reverse=rgb_tail
for a,b in changes:reverse=reverse.replace(b,a)
assert reverse==native_tail
tail_source=native[:start]+extra+prefix+loads+rgb_tail
# Every helper before main (including computeGeoBiome/GeographyWeights) is byte exact.
assert partial[:start]==native[:start] and tail_source[:start]==native[:start]
assert partial.count(b'makeWet(biome.albedo')==0
assert tail_source.count(b'makeWet(biome.albedo')==1
assert b'computeGeoBiome(coords' not in tail_source
arrays=['#pragma once','namespace StagedMaterial {'];records=[]
native_fields=next(s['fields'] for s in json.loads((COMPACT/'_generated/contract.json').read_text())['shaders'] if s['name']=='Pixel')
for name,data in [('Partial',partial),('Tail',tail_source)]:
    path=OUT/(name+'.hlsl');path.write_bytes(data);cso=OUT/(name+'.cso')
    subprocess.run([str(FXC),'/nologo','/T','ps_4_0','/E','main_fs','/O3','/Gec','/D','DX11=1','/Fo',str(cso),str(path)],check=True)
    blob=cso.read_bytes();arrays+=['static const unsigned char '+name+'Shader[]={'+','.join(str(v) for v in blob)+'};']
    reflection=subprocess.check_output([str(FXC),'/dumpbin',str(cso)],text=True);(OUT/(name+'.reflection.txt')).write_text(reflection)
    fields=[dict(name=m[1],offset=int(m[2]),bytes=int(m[3])) for m in re.finditer(r'//\s+(?:float(?:[1-4](?:x[1-4])?)?)\s+(\w+)(?:\[\d+\])?;\s+// Offset:\s*(\d+) Size:\s*(\d+)',reflection)]
    assert fields==native_fields,'native float CB fields/offsets remain exact'
    assert re.search(r'previousAlbedo\s+texture.*\bt8\b',reflection)
    assert re.search(r'previousNormalAbsorb\s+texture.*\bt9\b',reflection)
    if name=='Partial':assert re.search(r'StagedControl\s+cbuffer.*\bcb1\b',reflection)
    records.append(dict(name=name,sourceSha256=sha(data),sha256=sha(blob),bytes=len(blob),nativeFloatFields=fields))
arrays+=['}'];(OUT/'StagedShaders.h').write_text('\n'.join(arrays)+'\n')
vertex=(HERE/'_source/GeographyVertex.hlsl').read_bytes()
assert sha(vertex)=='6ef9efa2c8027da354f812b3f59bfa055c48edcfa4a2057782bd7416f58d54e9'
(OUT/'shader-contract.json').write_text(json.dumps(dict(nativeSha256=sha(native),nativeCoreByteExact=True,nativeTailByteExact=True,nativeVertexUnchanged=True,vertexSourceSha256=sha(vertex),slots=[0,1,2,3],liveFields=['albedo.rgba','normal.rgb','absorbance'],stageBindings={'cb':'b1','previousAlbedo':'t8','previousNormalAbsorb':'t9'},carryBytes=4194304,stageConstantBytes=16,shaders=records),indent=2)+'\n')
print('PASS byte-exact native helper/ordered slot extraction; tail runs once; explicit b1/t8/t9')
