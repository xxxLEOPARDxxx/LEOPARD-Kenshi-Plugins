"""Offline native backend mapping evidence; no DLL loading/native invocation."""
import generate_models as g
import sys, struct, json
sys.path.insert(0,'D:/DEV/Kenshi/ours/SkillLevelUpInfo/tests/_tools')
import pefile,capstone
p=g.PE(g.GAME/'RenderSystem_Direct3D11_x64.dll');pf=pefile.PE(data=p.data)
md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);md.detail=True
ranges=[(e.struct.BeginAddress,e.struct.EndAddress) for e in pf.DIRECTORY_ENTRY_EXCEPTION]
rows=[]
for sec in pf.sections:
 if not sec.Name.startswith(b'.text'):continue
 data=sec.get_data();start=0
 while True:
  at=data.find(b'\xb0\x00\x00\x00',start)
  if at<0:break
  start=at+1
  if at<2:continue
  r=sec.VirtualAddress+at-2;ins=next(md.disasm(data[at-2:at+4],r),None)
  if not ins or ins.mnemonic!='call' or not any(o.type==capstone.x86.X86_OP_MEM and o.mem.disp==0xb0 for o in ins.operands):continue
  region=next(((a,b) for a,b in ranges if a<=r<b),None)
  if not region:continue
  a,b=region;raw=p.data[p.offset(a):p.offset(a)+b-a]
  name=f'backend-device-slot22-{a:x}.asm';(g.OUT/name).write_text('\n'.join(f'{i.address:x}: {i.bytes.hex():24} {i.mnemonic} {i.op_str}' for i in md.disasm(raw,a))+'\n')
  rows.append(dict(call=hex(r),region=[hex(a),hex(b)],file=name))
print(json.dumps(rows,indent=2))
import importlib.util
spec=importlib.util.spec_from_file_location('models_slots','D:/DEV/Kenshi/tools/slots.py');slots=importlib.util.module_from_spec(spec);spec.loader.exec_module(slots)
exe=g.PE(g.GAME/'RE_Kenshi/Kenshi_x64.exe');native=[]
for prefix in ['?getAllTowns@TownList','?isTown@Town@@','?isTown@TownBase@@','??0SharedKing@@','?getAllTownsOf@TownList']:
 for name,rva,slot in slots.resolve(prefix):
  data=exe.data[exe.offset(rva):exe.offset(rva)+768];lines=[]
  for ins in md.disasm(data,rva):
   if ins.mnemonic=='int3':break
   lines.append(f'{ins.address:x}: {ins.bytes.hex():24} {ins.mnemonic} {ins.op_str}')
  native.append(dict(name=name,rva=hex(rva),slot=slot,code24=data[:24].hex(),disassembly=lines))
(g.OUT/'passive-town-selection.json').write_text(json.dumps(native,indent=2)+'\n')
townTypes=[];base=struct.unpack_from('<Q',exe.data,exe.opt+24)[0]
for typename in ['Town','TownBase']:
 pos=exe.data.find(('.?AV'+typename+'@@').encode());assert pos>16
 td=next(v+pos-o-16 for v,s,o,n in exe.sections if o<=pos<o+n)
 for v,s,o,n in exe.sections:
  for delta in range(0,n-24,4):
   a=struct.unpack_from('<6I',exe.data,o+delta)
   if a[0]!=1 or a[1]!=0 or a[3]!=td or a[5]!=v+delta:continue
   col=v+delta;ref=exe.data.find(struct.pack('<Q',base+col));assert ref>=0
   vt=next(b+ref-z+8 for b,sz,z,nn in exe.sections if z<=ref<z+nn)
   method=struct.unpack_from('<Q',exe.data,exe.offset(vt)+0x268)[0]-base
   townTypes.append(dict(type=typename,vtable=hex(vt),col=hex(col),isTownSlot='0x268',target=hex(method)))
(g.OUT/'town-vtable-types.json').write_text(json.dumps(townTypes,indent=2)+'\n')
returns=[]
for number in [2,6,16,28,41]:
 needle=b'\xb8'+struct.pack('<I',number)+b'\xc3';start=0
 while True:
  at=p.data.find(needle,start)
  if at<0:break
  start=at+1;rva=next((v+at-o for v,s,o,n in p.sections if o<=at<o+n),None)
  if rva is not None and 0x28000<=rva<0x30000:returns.append((hex(rva),number))
(g.OUT/'mapping-return-candidates.json').write_text(json.dumps(returns,indent=2)+'\n')
for disp in [0x5b8,0x5bc]:
 hits=[]
 for sec in pf.sections:
  if not sec.Name.startswith(b'.text'):continue
  data=sec.get_data();start=0
  while True:
   at=data.find(struct.pack('<I',disp),start)
   if at<0:break
   start=at+1
   for back in [2,3]:
    if at<back:continue
    ins=next(md.disasm(data[at-back:at+8],sec.VirtualAddress+at-back),None)
    if ins and any(o.type==capstone.x86.X86_OP_MEM and o.mem.disp==disp for o in ins.operands):
     hits.append(f'{ins.address:x}: {ins.mnemonic} {ins.op_str}')
 (g.OUT/f'backend-desc-{disp:x}.txt').write_text('\n'.join(hits)+'\n')
