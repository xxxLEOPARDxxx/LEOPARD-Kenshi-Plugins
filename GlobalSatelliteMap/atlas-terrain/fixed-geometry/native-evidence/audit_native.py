"""Reproduce file-only native proof. Never loads a DLL or opens a process."""
from pathlib import Path
import hashlib,json,struct,sys
sys.path.insert(0,'D:/DEV/Kenshi/ours/SkillLevelUpInfo/tests/_tools')
import pefile
from capstone import Cs,CS_ARCH_X86,CS_MODE_64
HERE=Path(__file__).resolve().parent
GAME=Path('D:/SteamLibrary/steamapps/common/Kenshi')
profiles={
 'terrain':('Plugin_Terrain_x64.dll','53afd1e4faf35020556d33f57007d51c2a9d2330b630e39a67161ee850b7a239'),
 'ogre':('OgreMain_x64.dll','7d3888d21398647519f10a89b1f300d8606f761ad159e6021a797b1b9b3df5f6'),
 'exe':('RE_Kenshi/Kenshi_x64.exe','504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e')}
windows={
 'terrain':{'patch-vertices':(0x47a0,0x5a60),'base-indices':(0x5aa0,0x5c74),'native-height':(0x14430,0x14642),
 'world-height-scale':(0x14070,0x1418a),'terrain-height-stream-open':(0x141c0,0x14340),'gpu-patch-upload':(0x1f5c0,0x1f930),
 'border-indices':(0x5e10,0x6370),'native-leaf-export':(0x3960,0x3a20)},
 'exe':{'water-plane-constructor':(0x9d6000,0x9d67c0)},'ogre':{}}
report={}
for key,(module_name,want) in profiles.items():
 data=(GAME/module_name).read_bytes();assert hashlib.sha256(data).hexdigest()==want
 pe=pefile.PE(data=data);base=pe.OPTIONAL_HEADER.ImageBase;md=Cs(CS_ARCH_X86,CS_MODE_64);md.detail=True
 imports={s.address:s.name.decode() if s.name else str(s.ordinal) for d in pe.DIRECTORY_ENTRY_IMPORT for s in d.imports}
 exports={s.name.decode():s.address for s in pe.DIRECTORY_ENTRY_EXPORT.symbols if s.name} if hasattr(pe,'DIRECTORY_ENTRY_EXPORT') else {}
 if key=='ogre':
  for name,rva in exports.items():
   if name.startswith('?normalise@Vector3@'):
    windows[key]['ogre-normalise']=(rva,rva+160)
 if key=='terrain':
  # Read all signed ring constants directly from RIP operands, not remembered literals.
  constants={hex(r):struct.unpack('<f',pe.get_data(r,4))[0] for r in [0x35d9c,0x35cd0,0x37200]}
  assert constants['0x35d9c']==-1 and constants['0x35cd0']==1
  report['terrainConstants']=constants
 for label,(start,end) in windows[key].items():
  code=pe.get_data(start,end-start);rows=[]
  for ins in md.disasm(code,base+start):
   note=''
   for op in ins.operands:
    if op.type==3 and ins.reg_name(op.mem.base)=='rip':
     target=ins.address+ins.size+op.mem.disp
     if target in imports:note=imports[target]
   rows.append(f'{ins.address-base:08x} {ins.bytes.hex():26} {ins.mnemonic:9} {ins.op_str}'+(' ; '+note if note else ''))
  assert rows,label
  (HERE/(label+'.asm')).write_text('\n'.join(row.rstrip() for row in rows)+'\n')
  report[label]={'module':key,'rva':hex(start),'bytes':len(code),'sha256':hashlib.sha256(code).hexdigest(),'decodedInstructions':len(rows)}
 report[key+'Profile']={'path':str(GAME/module_name),'sha256':want}
(HERE/'native-audit.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS',len(report)-4,'native windows/constants; file identity pinned; no native calls')
