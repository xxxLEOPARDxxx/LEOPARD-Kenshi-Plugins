"""Actual installed Steam EXE / KLib contracts; never loads or executes game code."""
from pathlib import Path
import hashlib,os,re,struct,sys
here=Path(__file__).resolve().parent
sys.path.insert(0,r'D:/DEV/Kenshi/tools/_native_python')
import pefile,capstone
root=Path(os.environ.get('KENSHI_ROOT',r'D:/SteamLibrary/steamapps/common/Kenshi'))
raw=(root/'RE_Kenshi/Kenshi_x64.exe').read_bytes()
assert hashlib.sha256(raw).hexdigest()=='504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e'
pe=pefile.PE(data=raw);lib=pefile.PE(str(root/'KenshiLib.dll'))
table=struct.unpack('<7630I',(root/'RE_Kenshi/RVAs/Steam_1.0.65.br').read_bytes())
resolved={}
for symbol in lib.DIRECTORY_ENTRY_EXPORT.symbols:
 if symbol.name and 0x15B46<=symbol.address<0x15B46+6*len(table) and (symbol.address-0x15B46)%6==0:
  assert lib.get_data(symbol.address,2)==b'\xff\x25'
  resolved[symbol.name.decode()]=table[(symbol.address-0x15B46)//6]
for prefix,rva in [('?findSomethingToBuild@AI@@',0x998190),('?createBuilding@RootObjectFactory@@',0x57C1E0),('?findAllBuildings@ZoneMapContent@@QEAAXPEAVTownBase@@',0x9FA9B0)]:
 assert [v for n,v in resolved.items() if n.startswith(prefix)]==[rva],(prefix,[(n,hex(v)) for n,v in resolved.items() if n.startswith(prefix)])
source=(here.parent/'EngineerPriorityNative.h').read_text(encoding='utf-8-sig')
pins=[(int(a,16),int(b,16),int(c,16)) for a,b,c in re.findall(r'\{0x([0-9A-Fa-f]+),0x([0-9A-Fa-f]+),0x([0-9A-Fa-f]+)ULL\}',source)]
assert len(pins)==7
for rva,length,expected in pins:
 h=14695981039346656037
 for byte in pe.get_data(rva,length):h=((h^byte)*1099511628211)&((1<<64)-1)
 assert h==expected,(hex(rva),hex(h),hex(expected))
 assert not any(rva<=entry.rva<rva+length for block in pe.DIRECTORY_ENTRY_BASERELOC for entry in block.entries if entry.type)
md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
def decoded(rva,length):return [(i.address,i.mnemonic,i.op_str) for i in md.disasm(pe.get_data(rva,length),rva)]
assert decoded(0x9A6050,16)==[(0x9A6050,'mov','qword ptr [rbp + 0x118], rsi'),(0x9A6057,'comiss','xmm6, xmm7'),(0x9A605A,'jbe','0x9a6109')]
assert decoded(0x9A60E2,17)==[(0x9A60E2,'lea','rdx, [rbp + 0x118]'),(0x9A60E9,'lea','rcx, [rsp + 0x48]'),(0x9A60EE,'call','0x26b5c')]
assert decoded(0x9A60F3,4)==[(0x9A60F3,'movss','dword ptr [rax], xmm6')]
assert pe.get_data(0x26B5C,1)==b'\xe9' and 0x26B5C+5+struct.unpack('<i',pe.get_data(0x26B5D,4))[0]==0x8D9600
assert decoded(0x8D961B,3)==[(0x8D961B,'mov','r8, qword ptr [rdx]')]
assert decoded(0x8D9647,4)==[(0x8D9647,'lea','rax, [rbx + 0x20]')]
assert decoded(0x4D6DE0,5)==[(0x4D6DE0,'call','0x3db31')]
assert pe.get_data(0x3DB31,1)==b'\xe9' and 0x3DB31+5+struct.unpack('<i',pe.get_data(0x3DB32,4))[0]==0x57C1E0
assert decoded(0x4D6DA3,12)==[(0x4D6DA3,'xor','eax, eax'),(0x4D6DA5,'mov','qword ptr [rsp + 0x48], rax'),(0x4D6DAA,'mov','qword ptr [rsp + 0x40], rax')]
assert 'const void* caller=_ReturnAddress();return ScoreSlotAt' in source
assert 'const void* caller=_ReturnAddress();return FactoryAt' in source
assert '__declspec(noinline) float* ScoreSlotHook' in source
assert '__declspec(noinline) Building* FactoryHook' in source
print('PASS actual SHA256, 3 FF25/export mappings, 6 relocation-free complete bodies + 1 public entry ABI pin, positive branch/helper ABI, placement caller ABI, return-address bridge declarations')
