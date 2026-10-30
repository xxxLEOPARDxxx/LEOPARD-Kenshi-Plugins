"""Read-only native geometry proof from unpacked Steam code, not a live UI scan.

Optional tools: pip install --target tests/_tools pefile capstone.
No process access, native calls, injection, widget discovery or file installation.
"""
from pathlib import Path
import sys,struct
sys.path.insert(0,str(Path(__file__).parent/'_tools'))
import pefile,capstone
game=Path(r'D:\SteamLibrary\steamapps\common\Kenshi')
exe=pefile.PE(str(game/'RE_Kenshi'/'Kenshi_x64.exe'))
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True
def native_float(instruction):
 ins=next(cs.disasm(exe.get_data(instruction,16),exe.OPTIONAL_HEADER.ImageBase+instruction))
 memory=[o for o in ins.operands if o.type==capstone.x86.X86_OP_MEM and o.mem.base==capstone.x86.X86_REG_RIP]
 assert len(memory)==1,(hex(instruction),ins.mnemonic,ins.op_str)
 rva=ins.address+ins.size+memory[0].mem.disp-exe.OPTIONAL_HEADER.ImageBase
 return struct.unpack('<f',exe.get_data(rva,4))[0]
w1left,w2left=native_float(0x6f8f7d),native_float(0x6f910a)
w1width,w2width=native_float(0x6f8f75),native_float(0x6f9102)
barwidth=native_float(0x6fa7f3)
assert abs(w1left-.03)<1e-6 and w1left==w2left
assert abs(w1width-.96)<1e-6 and abs(w2width-.93)<1e-6
assert abs(barwidth-.48)<1e-6
assert exe.get_data(0x6e20e0,8)==bytes.fromhex('48 8b 41 08 8b 40 2c c3')
original=pefile.PE(str(game/'mods'/'xp_info'/'xp_info.dll'))
alignment=list(cs.disasm(original.get_data(0x2c9c,10),original.OPTIONAL_HEADER.ImageBase+0x2c9c))
assert [(i.mnemonic,i.op_str) for i in alignment[:3]]==[
 ('mov','r12d, dword ptr [rsi + 0x28]'),('sub','ebp, r12d'),('add','ebp, 0xa')]
print('Native basic name/value boxes overlap: both left=3% parent width; widths96%/93%.')
print('Native keyed progress bar width=48%; original XP Info left=valueRight-nativeBarWidth+10px.')
print('GUIWindow.getHeight reads win Widget.height; content-height getter is separate.')
print('At400px: vanilla name/value left12, valueRight384; XP bar left202 width192, right394.')

# getLineByNum's inner walk is std::map in-order iteration, unrelated to
# widget coordinates. Original XP Info explicitly sorts native row bottoms.
def instructions(pe,rva,size):
 return [(i.mnemonic,i.op_str) for i in cs.disasm(pe.get_data(rva,size),pe.OPTIONAL_HEADER.ImageBase+rva)]
walk=instructions(exe,0x6fc470,0x56)
assert ('mov','rax, qword ptr [rbx + 0x10]') in walk
assert ('mov','rbx, rax') in walk and ('mov','rax, qword ptr [rax]') in walk
assert ('mov','rax, qword ptr [rbx + 8]') in walk
helper=instructions(original,0x2b05,0x49)
assert ('call','0x180016190') in helper # original sort<int>
assert ('sub','eax, dword ptr [rdi + rdx*4 - 4]') in helper
assert ('cmp','eax, esi') in helper and ('mov','esi, eax') in helper
skip=instructions(original,0x2972,0x20)
assert ('cmp','eax, 0xb') in skip # DPL_PROGRESS excluded from native baseline
assert ('call','qword ptr [rdx + 0x58]') in skip # DataPanelLine::getBottom
print('Native getLineByNum walks map successors; original sorts non-progress bottoms and takes minimum positive adjacent gap.')
