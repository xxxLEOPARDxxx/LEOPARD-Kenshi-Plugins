"""Read-only proof of original stat coverage against the unpacked Steam exe.

Optional dependencies: pip install --target tests/_tools pefile capstone.
No process handle or runtime invocation is used.
"""
from pathlib import Path
import sys,struct,re
sys.path[:0]=[str(Path(__file__).parent/'_tools'),r'D:\DEV\Kenshi\tools']
import pefile,capstone,slots
exe=Path(r'D:\SteamLibrary\steamapps\common\Kenshi\RE_Kenshi\Kenshi_x64.exe')
p=pefile.PE(str(exe));base=p.OPTIONAL_HEADER.ImageBase
rva=slots.resolve('getStatRef')[0][1]
assert rva==0x8832c0
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
table=struct.unpack('<38I',p.get_data(0x8833fc,38*4))
native={}
for stat,target in enumerate(table,1):
 instruction=next(cs.disasm(p.get_data(target,8),base+target))
 match=re.fullmatch(r'rax, \[rcx \+ (0x[0-9a-f]+)\]',instruction.op_str)
 native[stat]=int(match[1],16) if match else None
assert native[9]==native[14]==native[15]==0x98
assert native[31]==0x9c and native[33] is None and native[20] is None
refs={v for k,v in native.items() if k!=20 and v is not None}
assert len(refs)==34 and all(v<0x230 for v in refs)
print('Native getStatRef ABI: 34 distinct supported fields; HiveMedic/Vet alias Medic; MassCombat valid; Survival/Weapons dummy.')
