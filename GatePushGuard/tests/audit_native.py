"""Read-only native binding/disassembly audit; never opens a game process."""
import argparse, hashlib, importlib.util, json, pathlib, sys
sys.dont_write_bytecode=True
parser=argparse.ArgumentParser();parser.add_argument('--output',type=pathlib.Path,required=True);args=parser.parse_args()
sys.path.insert(0,r'D:\DEV\Kenshi\ours\SkillLevelUpInfo\tests\_tools')
import capstone,pefile
spec=importlib.util.spec_from_file_location('slots',r'D:\DEV\Kenshi\tools\slots.py');slots=importlib.util.module_from_spec(spec);spec.loader.exec_module(slots)
image=pathlib.Path(r'D:\SteamLibrary\steamapps\common\Kenshi\RE_Kenshi\Kenshi_x64.exe')
data=image.read_bytes();sha=hashlib.sha256(data).hexdigest()
assert sha=='504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e',sha
pe=pefile.PE(data=data,fast_load=True);md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_IMPORT']])
imports={i.address-pe.OPTIONAL_HEADER.ImageBase:(d.dll.decode(),i.name.decode() if i.name else '') for d in pe.DIRECTORY_ENTRY_IMPORT for i in d.imports}
assert imports[0x22461e0][1]=='??4Vector3@Ogre@@QEAAAEAV01@AEBV01@@Z',imports.get(0x22461e0)
# Native 5c9bfd sets rcx=this+48, rdx=argument then calls Ogre Vector3::operator=.
# Movement receives the SAME argument at5c9c18; this is absolute, not displacement.
assert data[pe.get_offset_from_rva(0x5c9bfd):pe.get_offset_from_rva(0x5c9c0a)]==bytes.fromhex('4883c148488bdaff15d6c5c701')
source=(pathlib.Path(__file__).resolve().parents[1]/'GatePushGuard.cpp').read_text(encoding='utf-8')
assert 'c->teleport(target);' in source,'native movement adapter must pass the observed absolute outside target'
import re
for label,rva in [('teleportPrefix',0x5c9bf0),('posGetter',0x332810),('outGetter',0x2969f0),('inGetter',0x338630),('movementGetter',0x2ae030)]:
 body=re.search(r'\b'+label+r'\[\]=\{([^}]+)\}',source).group(1)
 prefix=bytes(int(v.strip(),0) for v in body.split(','));at=pe.get_offset_from_rva(rva)
 assert data[at:at+len(prefix)]==prefix,label
assert 'if(!SupportedNative())' in source
assert 'ou->getCharactersWithinSphere(' not in source,'the native max argument does not bound this query'
assert 'g_events.Drain(g_batch)' in source and 'INSTALL(&CharMovement::_NV_update,Movement,g_origMovement)' in source
patterns=['?teleport@Character','getDoorPos','getActorClosedPosition','?_NV_update@DoorStuff','setupPhysicalUT@DoorStuff','?_NV_onBuildingLoaded@DoorStuff','?setDoorState@DoorStuff','getCharactersWithinSphere','?_NV_getFaction@DoorStuff','?_NV_needsUpdate@DoorStuff','?getBuilding@hand','?_NV_setHandle@DoorStuff','?_NV_setHandle@Building','?_CONSTRUCTOR@DoorStuff','?createBuilding@RootObjectFactory','?_NV_update@CharMovement','?getCharacter@CharMovement']
rows=[]
boundaries=sorted({r for name,target in slots.stubs().items() for r in [slots.table()[(target-slots.base_of_table())//8]] if r})
for pattern in patterns:
 for name,rva,slot in slots.resolve(pattern):
  end=next((r for r in boundaries if r>rva),rva+8192)
  offset=pe.get_offset_from_rva(rva);code=data[offset:offset+min(end-rva,16384)]
  instructions=[]
  for ins in md.disasm(code,rva):
   instructions.append(f'{ins.address:08x} {ins.mnemonic} {ins.op_str}')
   if ins.mnemonic=='ret' or len(instructions)>=2000:break
  rows.append(dict(name=name,rva=hex(rva),slot=slot,first32=code[:32].hex(),instructions=instructions))
args.output.mkdir(parents=True,exist_ok=True)
plugin=pathlib.Path(r'D:\DEV\Kenshi\build\GatePushGuard\GatePushGuard.dll')
built=pefile.PE(str(plugin));exports={name for name,address in slots.exports(slots.DLL)[0]}
plugin_exports={name for name,address in slots.exports(str(plugin))[0]}
assert 'MCM_Describe' in plugin_exports and '?startPlugin@@YAXXZ' in plugin_exports,plugin_exports
native_imports=[i.name.decode() for d in built.DIRECTORY_ENTRY_IMPORT if d.dll.lower()==b'kenshilib.dll' for i in d.imports if i.name]
missing=sorted(set(native_imports)-exports);assert not missing,missing
bindings=[]
for name in native_imports:
 matches=[(n,r,s) for n,r,s in slots.resolve(name) if n==name]
 if matches:
  assert len(matches)==1 and matches[0][1],matches
  bindings.append(dict(name=name,rva=hex(matches[0][1]),slot=matches[0][2]))
(args.output/'native-audit.json').write_text(json.dumps(dict(image=str(image),sha256=sha,kenshilib_sha256=hashlib.sha256(pathlib.Path(slots.DLL).read_bytes()).hexdigest(),plugin_sha256=hashlib.sha256(plugin.read_bytes()).hexdigest(),mcm_export_verified=True,plugin_exports=sorted(plugin_exports),native_import_count=len(native_imports),imports_missing=missing,bindings=bindings,functions=rows),indent=2),encoding='utf-8')
(args.output/'native-disassembly.txt').write_text('\n\n'.join(r['name']+' '+r['rva']+'\n'+'\n'.join(r['instructions']) for r in rows),encoding='utf-8')
print(f'Pinned Steam executable {sha}; {len(rows)} mapped native entries; {len(native_imports)} actual KenshiLib imports resolved; read-only audit written to {args.output}')
