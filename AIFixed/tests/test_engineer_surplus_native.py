from pathlib import Path
import sys,re,struct,hashlib,importlib.util
sys.dont_write_bytecode=True;sys.path.insert(0,'D:/DEV/Kenshi/tools/_native_python');import pefile
here=Path(__file__).resolve().parent;s=(here.parent/'EngineerSurplusRuntime.h').read_text();p=Path('D:/SteamLibrary/steamapps/common/Kenshi/RE_Kenshi/Kenshi_x64.exe');d=p.read_bytes();pe=pefile.PE(data=d,fast_load=True)
assert hashlib.sha256(d).hexdigest()=='504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e'
pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_BASERELOC']])
relocations=[e.rva for b in getattr(pe,'DIRECTORY_ENTRY_BASERELOC',[]) for e in b.entries if e.type==10]
checks=1
for a,hexes,n in re.findall(r'(?:Bytes|SharedEntry)\(0x([0-9a-f]+),"((?:\\x[0-9a-f]{2})+)",(\d+)\)',s):
 expected=bytes.fromhex(hexes.replace('\\x',''));assert len(expected)==int(n)
 off=pe.get_offset_from_rva(int(a,16));assert d[off:off+len(expected)]==expected,hex(int(a,16));checks+=1
for a,n,h in re.findall(r'Contract\(0x([0-9a-f]+),0x([0-9a-f]+),0x([0-9a-f]+)ULL\)',s):
 off=pe.get_offset_from_rva(int(a,16));raw=d[off:off+int(n,16)]
 assert not any(int(a,16)<=r<int(a,16)+int(n,16) for r in relocations),'relocated native body cannot use raw hash'
 def fnv(v):
  h=14695981039346656037
  for b in v:h=((h^b)*1099511628211)&((1<<64)-1)
  return h
 assert fnv(raw)==int(h,16);changed=bytearray(raw);changed[len(raw)//2]^=1;assert fnv(changed)!=int(h,16);checks+=2
spec=importlib.util.spec_from_file_location('slots','D:/DEV/Kenshi/tools/slots.py');slots=importlib.util.module_from_spec(spec);spec.loader.exec_module(slots)
for name,at in [('?runGOAP@AITaskSytem',0x50f380),('?score@Tasker',0x60f2f0),('?removeItemDontDestroy_returnsItem@Inventory',0x749e10),('?addItem@InventorySection',0x74bd70),('?copyFrom@ActionsListMgr',0x50c850),('?setCurrentTask@AITaskSytem',0x50cab0),('?choosePermaJob@AITaskSytem',0x50db20)]:
 rows=slots.resolve(name);assert len(rows)==1 and rows[0][1]==at;checks+=1
assert 'normalJobScope!=ai->taskSystemAI' in s and 'from!=reinterpret_cast<ULONG_PTR>(image)+0x50fd28' in s;checks+=1
print('PASS',checks,'actual Steam export/body/callsite checks and middle-body negative controls; no game execution')
