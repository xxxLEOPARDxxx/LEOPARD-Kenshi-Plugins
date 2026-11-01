"""Read-only native clock proof for installed 504b Steam image; no process API."""
from pathlib import Path
import hashlib,importlib.util,json,struct,sys
sys.dont_write_bytecode=True
sys.path.insert(0,'D:/DEV/Kenshi/ours/SkillLevelUpInfo/tests/_tools')
import pefile,capstone
spec=importlib.util.spec_from_file_location('slots','D:/DEV/Kenshi/tools/slots.py')
slots=importlib.util.module_from_spec(spec);spec.loader.exec_module(slots)
game=Path('D:/SteamLibrary/steamapps/common/Kenshi')
path=game/'RE_Kenshi/Kenshi_x64.exe';raw=path.read_bytes()
digest=hashlib.sha256(raw).hexdigest()
assert digest=='504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e'
pe=pefile.PE(data=raw,fast_load=True)
md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);md.detail=True
tables={kind:struct.unpack('<%dI'%(len(data)//4),data) for kind in ('Steam','GOG') for data in [(game/f'RE_Kenshi/RVAs/{kind}_1.0.65.br').read_bytes()]}
rows=[]
for name in ('?getTimeStamp@GameWorld','?getTimeStamp_inGameHours@GameWorld',
             '?getLengthOfHourInRealSeconds@GameWorld','?getTotalHours@TimeOfDay',
             '?getRealLifeSeconds@TimeOfDay','?isPaused@GameWorld','?userPause@GameWorld'):
    symbol,rva,index=slots.resolve(name)[0]
    assert rva==tables['Steam'][index]
    at=pe.get_offset_from_rva(rva)
    rows.append(dict(symbol=symbol,slot=index,steam=hex(rva),gog=hex(tables['GOG'][index]),first24=raw[at:at+24].hex()))

def instructions(rva,size):
    at=pe.get_offset_from_rva(rva)
    return list(md.disasm(raw[at:at+size],rva))
def target(instruction):
    operand=next(o for o in instruction.operands if o.type==capstone.x86.X86_OP_MEM and o.mem.base==capstone.x86.X86_REG_RIP)
    return instruction.address+instruction.size+operand.mem.disp

hours=instructions(0x66c180,21)
assert target(hours[0])==0x212f3c0 and hours[1].op_str=='rcx, qword ptr [rax + 0xa0]'
paused=instructions(0xdedc0,8)
assert paused[0].op_str=='eax, byte ptr [rcx + 0x8b9]'
hour_length=instructions(0x66c960,9)
length=struct.unpack_from('<f',raw,pe.get_offset_from_rva(target(hour_length[0])))[0]
assert abs(length-109.090909)<.00001

# User pause sets the shared speed to zero; clock advance consumes that speed.
user=instructions(0x7874d5,22)
assert user[2].mnemonic=='xorps' and user[2].op_str=='xmm6, xmm6'
assert target(user[-1])==0x21337b0 and user[-1].mnemonic=='movss'
advance=instructions(0x66e944,39)
assert advance[0].mnemonic=='cmp' and target(advance[0])==0x2133969
assert any(i.mnemonic=='movss' and target(i)==0x21337b0 for i in advance if 'rip' in i.op_str)
rate_instruction=next(i for i in advance if i.mnemonic=='mulss')
rate=struct.unpack_from('<f',raw,pe.get_offset_from_rva(target(rate_instruction)))[0]
assert abs(rate*length-1)<.000001
assert advance[-1].op_str=='dword ptr [rax + 0x78], xmm0'
write=instructions(0x66f52a,88)
assert any(i.op_str=='qword ptr [rbx + 0xa0], xmm1' for i in write)

windows={hex(start):[dict(rva=hex(i.address),bytes=i.bytes.hex(),mnemonic=i.mnemonic,operands=i.op_str)
    for i in instructions(start,size)] for start,size in ((0x66c180,21),(0x25b4b0,29),(0xdedc0,8),
    (0x7874d5,22),(0x66e944,39),(0x66e96b,52),(0x66f52a,88),(0x66c960,9),(0x850b60,37))}
out=Path(__file__).resolve().parent/'_generated';out.mkdir(exist_ok=True)
(out/'rest-clock-native.json').write_text(json.dumps(dict(executable=str(path),sha256=digest,methods=rows,
    native_hour_seconds=length,native_hour_rate=rate,windows=windows),indent=2),encoding='utf-8')
print('AIFixed native rest clock: PASS; actual Steam pause/speed/hour update/getter verified; GOG bindings table-only')
