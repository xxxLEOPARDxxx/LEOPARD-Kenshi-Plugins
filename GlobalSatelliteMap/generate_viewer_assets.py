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
  if key=='ogre' and e['key'] in ('NativeFindNode','NativeNodeTexture'):
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
(out/'GsmNativeProfile.h').write_text('\n'.join(lines)+'\n')
game=Path('D:/SteamLibrary/steamapps/common/Kenshi/data/materials')
fxc=Path('C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe')
subprocess.run([str(fxc),'/nologo','/T','cs_5_0','/E','main','/O3','/Gec','/D','DX11=1','/I',str(game/'common'),'/I',str(game/'post'),'/Fo'+str(out/'GsmRelight.cso'),str(here/'atlas-view/AtlasRelight.hlsl')],check=True)
code=(out/'GsmRelight.cso').read_bytes()
(out/'GsmRelightShader.h').write_text('static const unsigned char GsmRelightShader[]={\n'+',\n'.join(','.join(str(b) for b in code[i:i+24]) for i in range(0,len(code),24))+'\n};\n')
print('GSM native profile and shader generated; source manifest',sha)
