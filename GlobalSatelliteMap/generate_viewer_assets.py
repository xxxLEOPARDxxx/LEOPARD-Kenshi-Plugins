"""Generate this module's frozen native profile and shader; no probe build writes."""
from pathlib import Path
import json,re,subprocess,hashlib,sys
here=Path(__file__).resolve().parent
probe=here.parent/'tools/diagnostics/satellite-capture-probe'
out=here/'build';out.mkdir(exist_ok=True)
profile=json.loads((probe/'render_abi_profile.json').read_text())
lines=['#pragma once','// Generated from the audited native render profile; do not hand-edit.','struct GsmCodeWindow { DWORD rva; BYTE code[24]; };']
for key in ('ogre','d3d11'):
 p=profile[key];prefix='Gsm'+key.capitalize()
 lines += [f'static const char {prefix}File[]={json.dumps(p["file"])};',f'static const char {prefix}Hash[]={json.dumps(p["sha256"])};',f'static const DWORD {prefix}Timestamp=0x{p["timestamp"]};',f'static const DWORD {prefix}Size=0x{p["image_size"]};']
 checks=list(p['windows'])
 for e in p['exports']:
  if key=='ogre' and e['key'] in ('NativeFindNode','NativeNodeTexture','PrivateResourceCreator','PrivateResourceLoaded','PrivateResourceSetLoaded'):
   lines += [f'static const char Gsm{e["key"]}Name[]={json.dumps(e["name"])};',f'static const DWORD Gsm{e["key"]}Rva=0x{e["rva"]};']
  if 'code' in e:checks.append(e)
 lines += [f'static const GsmCodeWindow {prefix}Windows[]={{']
 lines += ['{0x'+e['rva']+',{'+','.join('0x'+format(b,'02x') for b in bytes.fromhex(e['code']))+'}},' for e in checks]
 lines += ['};']
 if key=='d3d11':
  lines += [f'static const DWORD GsmTextureVtable=0x{p["texture_vtable_rva"]};',f'static const DWORD GsmSrvGetter=0x{p["texture_srv_getter_rva"]};']
region=(probe/'region-catalog/RegionCatalogData.h').read_text()
sha=re.search(r'ManifestSha256\[\]="([0-9a-f]{64})"',region).group(1)
lines += [f'static const char GsmSourceManifestHash[]="{sha}";','static const char GsmEngineHash[]="504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e";']
manifest=json.loads((probe/'region-catalog/source-manifest.json').read_text())
blend=next(v['sha256'] for v in manifest['identities'] if v['name']=='blend')
lines += [f'static const char GsmBlendHash[]="{blend}";']
sys.path.insert(0,str(here/'tests'))
from test_pe_contract import PE
backend=PE(Path('D:/SteamLibrary/steamapps/common/Kenshi')/profile['d3d11']['file'])
assert hashlib.sha256(backend.data).hexdigest()==profile['d3d11']['sha256']
lines += ['static const GsmCodeWindow GsmSamplerWindows[]={']
for rva in (0x28b70,0x34632,0x34639):
 lines += ['{0x'+format(rva,'x')+',{'+','.join('0x'+format(b,'02x') for b in backend.data[backend.offset(rva):backend.offset(rva)+24])+'}},']
lines += ['};']
game_root=Path('D:/SteamLibrary/steamapps/common/Kenshi')
materials=game_root/'data/materials'
search=[materials/'common',materials/'deferred',materials/'post']
pending=[materials/'deferred/deferred.material',materials/'deferred/deferred.hlsl',materials/'post/hdr.material',materials/'post/hdrutils.hlsl',materials/'common/constants.hlsl',materials/'common/lightingFunctions.hlsl']
shader_files={}
while pending:
 path=pending.pop()
 if path in shader_files:continue
 raw=path.read_bytes();shader_files[path]=hashlib.sha256(raw).hexdigest()
 for include in re.findall(r'^\s*#\s*include\s+"([^"]+)"',raw.decode('latin1'),re.M):
  found=next((d/include for d in [path.parent]+search if (d/include).is_file()),None)
  if found is None:raise FileNotFoundError(include)
  pending.append(found)
 for source in re.findall(r'^\s*source\s+([^\s{}]+\.hlsl)',raw.decode('latin1'),re.M):
  found=next((d/source for d in [path.parent]+search if (d/source).is_file()),None)
  if found is None:raise FileNotFoundError(source)
  pending.append(found)
lines += ['struct GsmShaderInput {const char*path;const char*sha256;};','static const GsmShaderInput GsmShaderInputs[]={']
lines += ['{'+json.dumps(p.relative_to(game_root).as_posix())+','+json.dumps(h)+'},' for p,h in sorted(shader_files.items())]
lines += ['};']
local_profile=(here/'atlas-lights/NativeLocalLightProfile.h').read_text()
local_body=re.search(r'static const SourcePin SourcePins\[\]=\{(.*?)\};',local_profile,re.S).group(1)
local_pins=re.findall(r'\{"([^"]+)","([0-9a-f]{64})"\}',local_body)
assert len(local_pins)==7 and len({p for p,h in local_pins})==7
for relative,pinned_hash in local_pins:
 assert shader_files.get(game_root/relative)==pinned_hash,('local-light pin not covered by terrain worker',relative)
water_pins_raw=(here/'atlas-water/NativeWaterSnapshot.h').read_text()
water_pin_body=re.search(r'static const SourcePin SourcePins\[\]=\{(.*?)\};',water_pins_raw,re.S).group(1)
water_pins=re.findall(r'\{"([^"]+)","([0-9a-f]{64})"\}',water_pin_body)
assert len(water_pins)==5 and len({p for p,h in water_pins})==5
lines += ['static const GsmShaderInput GsmWaterShaderInputs[]={']
for relative,pinned_hash in water_pins:
 assert hashlib.sha256((game_root/relative).read_bytes()).hexdigest()==pinned_hash,relative
 lines += ['{'+json.dumps(relative)+','+json.dumps(pinned_hash)+'},']
lines += ['};']
lines += ['static const GsmShaderInput GsmModelShaderInputs[]={']
for relative, expected in (
 ('data/materials/deferred/distant_town.hlsl','ea54dc4c8361e0b300ce34cefe1a26d927ebb5082942642ca769824ddd75c066'),
 ('data/materials/deferred/distant_town.material','eaa612b91dad500c9f77dc253019363d2dffea5653a43ed660bd0cc5e4450259'),
 ('data/materials/deferred/gbuffer.hlsl','d6a525658f224379269369c9adaa928fc82a72285aed5cffb21d077df4c9c943')):
 assert hashlib.sha256((game_root/relative).read_bytes()).hexdigest()==expected,relative
 lines += ['{'+json.dumps(relative)+','+json.dumps(expected)+'},']
lines += ['};']
(out/'GsmNativeProfile.h').write_text('\n'.join(lines)+'\n')
engine=PE(game_root/'RE_Kenshi/Kenshi_x64.exe')
assert hashlib.sha256(engine.data).hexdigest()=='504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e'
zoom=['#pragma once','static const DWORD GsmZoomInRva=0x48bb50;','static const DWORD GsmZoomOutRva=0x48bc90;','static const DWORD GsmZoomWheelRva=0x48b9e0;','static const GsmCodeWindow GsmZoomWindows[]={']
for rva in (0x490328,0x490332,0x490347,0x490367,0x49037f,0x48bb50,0x48bb65,0x48bc90,0x48b9e0,0x48bcaa,0x48baa5,0x48ba7c,0x4907e0,0x49080c,0x4908fc,0x48d246):
 code=engine.data[engine.offset(rva):engine.offset(rva)+24]
 zoom+=['{0x'+format(rva,'x')+',{'+','.join('0x'+format(b,'02x') for b in code)+'}},']
zoom+=['};']
(out/'GsmZoomProfile.h').write_text('\n'.join(zoom)+'\n')
game=Path('D:/SteamLibrary/steamapps/common/Kenshi/data/materials')
fxc=Path('C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe')
subprocess.run([str(fxc),'/nologo','/T','cs_5_0','/E','main','/O3','/Gec','/D','DX11=1','/I',str(game/'common'),'/I',str(game/'post'),'/Fo'+str(out/'GsmRelight.cso'),str(here/'atlas-view/AtlasRelight.hlsl')],check=True)
code=(out/'GsmRelight.cso').read_bytes()
(out/'GsmRelightShader.h').write_text('static const unsigned char GsmRelightShader[]={\n'+',\n'.join(','.join(str(b) for b in code[i:i+24]) for i in range(0,len(code),24))+'\n};\n')
print('GSM native profile and shader generated; source manifest',sha)
sys.path.insert(0,str(here/'atlas-water'))
from generate_pipeline import generate as generate_water_pipeline
generate_water_pipeline()
local=here/'atlas-lights/terrain-hdr'
subprocess.run([sys.executable,str(local/'generate_shader.py')],check=True)
subprocess.run([str(fxc),'/nologo','/T','cs_5_0','/E','main','/O3','/Gec','/Fo'+str(out/'GsmLocal.cso'),str(local/'_generated/LocalLights.generated.hlsl')],check=True)
code=(out/'GsmLocal.cso').read_bytes()
(out/'GsmLocalShader.h').write_text('static const unsigned char GsmLocalShader[]={\n'+',\n'.join(','.join(str(b) for b in code[i:i+24]) for i in range(0,len(code),24))+'\n};\n')

subprocess.run([sys.executable,str(here/"atlas-models/generate_models.py")],check=True)
subprocess.run([sys.executable,str(here/"atlas-models/loaded-census/generate_profile.py")],check=True)
