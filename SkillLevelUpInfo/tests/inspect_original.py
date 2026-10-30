"""Read-only original DLL behavior evidence (pefile/capstone optional tooling)."""
from pathlib import Path
import sys, re, struct
sys.path.insert(0, str(Path(__file__).parent / '_tools'))
import pefile, capstone
p = pefile.PE(r'D:\SteamLibrary\steamapps\common\Kenshi\mods\xp_info\xp_info.dll')
base=p.OPTIONAL_HEADER.ImageBase
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64); cs.detail=True
imports={i.address:i.name.decode() for d in p.DIRECTORY_ENTRY_IMPORT for i in d.imports if i.name}
strings={base+p.get_rva_from_offset(m.start()):m.group().decode() for m in re.finditer(rb'[ -~]{5,}',p.__data__)}
funcs=[]
for d in p.DIRECTORY_ENTRY_EXCEPTION:
 a,b=d.struct.BeginAddress,d.struct.EndAddress
 funcs.append((base+a,base+b,list(cs.disasm(p.get_data(a,b-a),base+a))))
mode=sys.argv[1] if len(sys.argv)>1 else 'index'
for a,b,ins in funcs:
 refs=[]
 for i in ins:
  for o in i.operands:
   if o.type==capstone.x86.X86_OP_MEM and o.mem.base==capstone.x86.X86_REG_RIP:
    addr=i.address+i.size+o.mem.disp
    if addr in imports: refs.append(imports[addr])
    if addr in strings: refs.append(strings[addr])
 if mode=='all' or mode==hex(a) or (mode=='index' and any(any(t in r for t in ['xp_info','minPercent','floatingLabels','increaseStat','getGUIDataForMainInfo','createScreenLabel']) for r in refs)):
  print(hex(a),hex(b),' | '.join(refs))
  if mode!='index':
   for i in ins:
    suffix=''
    for o in i.operands:
     if o.type==capstone.x86.X86_OP_MEM and o.mem.base==capstone.x86.X86_REG_RIP:
      ad=i.address+i.size+o.mem.disp
      suffix=imports.get(ad,strings.get(ad,''))
    print(f'{i.address:x}: {i.mnemonic} {i.op_str} {suffix}')
