from pathlib import Path
import hashlib,importlib.util,json,struct
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
s=importlib.util.spec_from_file_location('p',HERE.parents[1]/'tests/test_pe_contract.py');m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
game=Path('D:/SteamLibrary/steamapps/common/Kenshi')
p=m.PE(game/'RE_Kenshi/Kenshi_x64.exe');og=m.PE(game/'OgreMain_x64.dll')
assert hashlib.sha256(p.data).hexdigest()=='504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e'
assert hashlib.sha256(og.data).hexdigest()=='7d3888d21398647519f10a89b1f300d8606f761ad159e6021a797b1b9b3df5f6'
# Native field-read/producer instructions, never native mutation calls.
rvas=[0x569960,0x933213,0x933267,0x9334aa,0x9334dd,0x547bb6,0x547bc9,0x547bd3,0x5496ed,0x549704,0x4cd2c0,0x4cd2cc,0x4cd2dc,0x92fd2b]
head=['#pragma once','struct CensusCodePin {DWORD rva;BYTE bytes[24];};','static const CensusCodePin CensusPins[]={']
for r in rvas:
 b=p.data[p.offset(r):p.offset(r)+24];head+=['{0x%x,{%s}},'%(r,','.join(str(v) for v in b))]
head+=['};','static const SIZE_T CensusTownVtable=0x1734c08;']
# Exact RTTI primary InstancedEntity type; skip rather than invoke virtual methods.
raw=og.data;needle=b'.?AVInstancedEntity@Ogre@@';pos=raw.find(needle);assert pos>=16
td=next(v+pos-o-16 for v,sz,o,n in og.sections if o<=pos<o+n);base=struct.unpack_from('<Q',raw,og.opt+24)[0];vts=[]
for v,sz,o,n in og.sections:
 for d in range(0,n-24,4):
  a=struct.unpack_from('<6I',raw,o+d)
  if a[0]!=1 or a[1]!=0 or a[3]!=td or a[5]!=v+d:continue
  ref=raw.find(struct.pack('<Q',base+v+d));assert ref>=0
  vts.append(next(b+ref-z+8 for b,ss,z,nn in og.sections if z<=ref<z+nn))
assert len(vts)==1
head+=['static const DWORD CensusInstancedVtable=0x%x;'%vts[0]]
(OUT/'LoadedProfile.h').write_text('\n'.join(head)+'\n')
(OUT/'profile.json').write_text(json.dumps(dict(exeSha256=hashlib.sha256(p.data).hexdigest(),ogreSha256=hashlib.sha256(og.data).hexdigest(),windows=[hex(r) for r in rvas],instancedVtable=hex(vts[0])),indent=2)+'\n')
print('PASS 14 exact native code24 windows and InstancedEntity primary RTTI')
