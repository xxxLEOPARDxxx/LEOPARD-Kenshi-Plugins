"""Read-only, SHA-pinned offline evidence; never opens/calls a game process."""
from pathlib import Path
import hashlib
import json
import struct
import sys

sys.path.insert(0,'D:/DEV/Kenshi/ours/SkillLevelUpInfo/tests/_tools')
import capstone
import pefile

HERE=Path(__file__).resolve().parent
EXE=Path('D:/SteamLibrary/steamapps/common/Kenshi/RE_Kenshi/Kenshi_x64.exe')
EXPECTED='504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e'

def run():
    raw=EXE.read_bytes();identity=hashlib.sha256(raw).hexdigest()
    if identity!=EXPECTED:raise ValueError('actual RE_Kenshi executable changed; repeat native audit')
    pe=pefile.PE(data=raw);base=pe.OPTIONAL_HEADER.ImageBase
    md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);md.detail=True
    def data(rva,n):
        offset=pe.get_offset_from_rva(rva);return raw[offset:offset+n]
    imports={entry.address-base:entry.name.decode('ascii') if entry.name else '#'+str(entry.ordinal)
             for module in pe.DIRECTORY_ENTRY_IMPORT for entry in module.imports}
    def instructions(rva,n):
        result=[]
        for ins in md.disasm(data(rva,n),base+rva):
            row=dict(rva=hex(ins.address-base),bytes=ins.bytes.hex(),assembly=ins.mnemonic+' '+ins.op_str)
            refs=[]
            for operand in ins.operands:
                if operand.type==capstone.x86.X86_OP_MEM and operand.mem.base==capstone.x86.X86_REG_RIP:
                    target=ins.address+ins.size+operand.mem.disp-base
                    ref=dict(rva=hex(target))
                    if target in imports:ref['import']=imports[target]
                    else:
                        block=data(target,80);ref['first16Hex']=block[:16].hex()
                        ref['firstFloat']=struct.unpack('<f',block[:4])[0]
                        text=block.split(b'\0')[0]
                        if text and all(32<=x<127 for x in text):ref['ascii']=text.decode('ascii')
                    refs.append(ref)
            if refs:row['ripReferences']=refs
            result.append(row)
        return result
    spans=[('heightGetter',0x9d5850,0x1d),('heightSetter',0x9d5a80,0x140),
           ('mapBounds',0x9d627c,0x165),('planeConstructorDimensions',0x9d65d0,0x64),('initialHeightCall',0x9d676c,0x10),
           ('setterThunk',0x1d881,5),('zoneParentAndPosition',0x9d7f20,0x379)]
    evidence={name:instructions(rva,n) for name,rva,n in spans}
    call=data(0x9d6777,5);assert call[0]==0xe8
    target=0x9d677c+struct.unpack('<i',call[1:])[0];assert target==0x1d881
    thunk=data(target,5);assert thunk[0]==0xe9 and target+5+struct.unpack('<i',thunk[1:])[0]==0x9d5a80
    init=list(md.disasm(data(0x9d676c,8),base+0x9d676c))[0]
    const=init.address+init.size+init.operands[1].mem.disp-base
    assert struct.unpack('<f',data(const,4))[0]==100
    assert any(ref.get('firstFloat')==500000 for row in evidence['planeConstructorDimensions'] for ref in row.get('ripReferences',[]))
    output=dict(executablePath=str(EXE),executableSha256=identity,scope='Offline static stock-global-plane evidence; no current waterY/material/runtime snapshot and no arbitrary mod-water support claim',instructions=evidence)
    out=HERE/'_generated';out.mkdir(exist_ok=True)
    (out/'native-water.json').write_text(json.dumps(output,indent=2,allow_nan=False)+'\n')
    print('PASS actual504b static water getter/setter, init100 thunk,500000 plane evidence; native-water.json (no runtime calls)')

if __name__=='__main__':run()
