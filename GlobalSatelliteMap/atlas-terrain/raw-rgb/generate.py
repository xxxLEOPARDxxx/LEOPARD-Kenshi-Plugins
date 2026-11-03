"""Only the ACTIVE native material writers change. Native math stays literal."""
from pathlib import Path
import hashlib,json,subprocess,re,importlib.util,sys
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True);COMPACT=HERE.parent/'compact-material'
FXC=Path('C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe')
sha=lambda b:hashlib.sha256(b).hexdigest()
spec=importlib.util.spec_from_file_location('compact_source_preparation',COMPACT/'prepare_sources.py');preparation=importlib.util.module_from_spec(spec);spec.loader.exec_module(preparation);preparation.prepare_sources(COMPACT)
native=(COMPACT/'_source/GeographyTerrain.hlsl').read_bytes();assert sha(native)=='f9326a91f29e1f1644fc0acbb33bc126bce04590faef2cc7b89dabd1292fd9a1'
start=native.index(b'void main_fs (');end=native.index(b'void mapfeature_fs (',start);active=native[start:end]
changes=[(b'writeAlbedo   ( buffer, biome.albedo.rgb, fragCoord.xy );',b'buffer.buf0.rgb = biome.albedo.rgb;'),(b'writeMetalness( buffer, 0.0f );',b'/* Native zero metalness is not written over full RGB. */')]
rgb_active=active
for a,b in changes:assert rgb_active.count(a)==1;rgb_active=rgb_active.replace(a,b)
rgb=native[:start]+rgb_active+native[end:]
reverse=rgb
for a,b in changes:assert reverse.count(b)==1;reverse=reverse.replace(b,a)
assert reverse==native and rgb[end+len(rgb_active)-len(active):]==native[end:],'inactive mapfeature unchanged'
# Independent observer: original pre-write biome.albedo before ANY GBuffer
# material writer; it does not use the RGB substitutions or packed decoder.
mark=active.index(b'\t// Write to GBuffer')
observer=active[:mark]+b'\tINITIALISE_OUTPUT(buffer);\n\tbuffer.buf0 = biome.albedo;\n}\n\n'
oracle=native[:start]+observer+native[end:]
arrays=['#pragma once','namespace TerrainRaw {'];records=[]
native_fields=next(s['fields']for s in json.loads((OUT/'fixture/contract.json').read_text())['shaders']if s['name']=='Pixel')
native_bindings=re.findall(r'^//\s+(\w+)\s+(texture|sampler)\s+\S+\s+(\S+)\s+([ts]\d+)\s+(\d+)\s*$',(OUT/'fixture/Pixel.reflection.txt').read_text(),re.M)
for name,data in [('Rgb',rgb),('Prewrite',oracle)]:
 path=OUT/(name+'.hlsl');path.write_bytes(data);cso=OUT/(name+'.cso')
 subprocess.run([str(FXC),'/nologo','/T','ps_4_0','/E','main_fs','/O3','/Gec','/D','DX11=1','/Fo',str(cso),str(path)],check=True)
 compiled=cso.read_bytes();arrays+=['static const unsigned char '+name+'Shader[]={'+','.join(str(v)for v in compiled)+'};']
 reflection=subprocess.check_output([str(FXC),'/dumpbin',str(cso)],text=True);(OUT/(name+'.reflection.txt')).write_text(reflection)
 assert 'float4 satBiome[225]' in reflection and 'Offset:  240 Size:  3600' in reflection
 fields=[dict(name=m[1],offset=int(m[2]),bytes=int(m[3]))for m in re.finditer(r'//\s+(?:float(?:[1-4](?:x[1-4])?)?)\s+(\w+)(?:\[\d+\])?;\s+// Offset:\s*(\d+) Size:\s*(\d+)',reflection)]
 assert fields==native_fields,'native constant ABI unchanged'
 bindings=re.findall(r'^//\s+(\w+)\s+(texture|sampler)\s+\S+\s+(\S+)\s+([ts]\d+)\s+(\d+)\s*$',reflection,re.M)
 expected=native_bindings if name=='Rgb' else [b for b in native_bindings if b[0]!='normalMaps']
 assert bindings==expected,'production native slots unchanged; observer omits only unused normal-map RGB'
 records.append(dict(name=name,sourceSha256=sha(data),sha256=sha(compiled),bytes=len(compiled),fields=fields,bindings=bindings))
arrays+=['}'];(OUT/'RawShaders.h').write_text('\n'.join(arrays)+'\n')
(OUT/'shader-contract.json').write_text(json.dumps(dict(nativeSha256=sha(native),activeChanges=[dict(before=a.decode(),after=b.decode())for a,b in changes],inactiveMapfeatureUntouched=True,observer='original biome.albedo before first active GBuffer writer; RGBA32F',shaders=records,vertexUnchangedSha256='dbd13838450fa683426bf3df92728fc0c3dbdf2b798c31c6c1004d0a3e3ed238'),indent=2)+'\n')
print('PASS active two-writer reversal; native core/mapfeature/VS unchanged; independent prewrite observer')
