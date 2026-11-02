"""Read-only authentication of native evidence, source locales and candidate PE.
No DLL loading, process access, installation or personal settings writes.
"""
from pathlib import Path
import gettext, hashlib, importlib.util, io, json, re, struct, sys
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1]
TOOLS=ROOT.parents[1]/'tools'
def module(name,path):
 spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
peutil=module('pe_check',TOOLS/'qol_binary_check.py')
po=module('po2mo',ROOT.parent/'tools/po2mo.py')
game=Path('D:/SteamLibrary/steamapps/common/Kenshi')
exe=game/'RE_Kenshi/Kenshi_x64.exe';native=peutil.PE(exe)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert sha(exe)=='504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e'
assert sha(game/'KenshiLib.dll')=='5b6240622d06801dc39896e385efb834b8e1fe2fcdf86067d071ae7a04f6c83b'
lib=peutil.PE(game/'KenshiLib.dll');exp=lib.offset(lib.u32(lib.dirs));n=lib.u32(exp+24)
functions=lib.offset(lib.u32(exp+28));names=lib.offset(lib.u32(exp+32));ordinals=lib.offset(lib.u32(exp+36));stubs={}
for i in range(n):
 name=lib.string(lib.u32(names+4*i));address=lib.u32(functions+4*lib.u16(ordinals+2*i));at=lib.offset(address)
 if lib.data[at:at+2]==b'\xff\x25':stubs[name]=address+6+struct.unpack_from('<i',lib.data,at+2)[0]
base=min(stubs.values());table=(game/'RE_Kenshi/RVAs/Steam_1.0.65.br').read_bytes()
windows=[];total=0
for path in sorted((ROOT/'tests/map-native').glob('*.txt')):
 lines=path.read_text().splitlines();head=re.fullmatch(r'(.+) slot=(\d+) Steam=0x([0-9a-f]+)',lines[0]);assert head,path
 symbol,slot,rva=head.groups();slot=int(slot);rva=int(rva,16)
 assert (stubs[symbol]-base)//8==slot,path
 assert struct.unpack_from('<I',table,4*slot)[0]==rva,path
 count=0
 for line in lines[1:]:
  match=re.match(r'^([0-9a-f]+) ([0-9a-f]+) ',line)
  if not match:continue
  addr,raw=match.groups();raw=bytes.fromhex(raw);at=native.offset(int(addr,16))
  assert native.data[at:at+len(raw)]==raw,(path,line)
  count+=1
 assert count>0,path
 total+=count;windows.append(dict(file=path.name,sha256=sha(path),symbol=symbol,slot=slot,rva=hex(rva),instructions=count))
def raw(rva,n):return native.data[native.offset(rva):native.offset(rva)+n]
def target(rva):
 code=raw(rva,5);assert code[0] in (0xe8,0xe9),(hex(rva),code.hex());return rva+5+struct.unpack_from('<i',code,1)[0]
assert target(0x7b4304)==0x4bd58 and target(0x75ea31)==0x4bd58
assert target(0x4bd58)==0x75de40 # tooltip/consume both invoke the mutating selector
assert target(0x75e1af)==0xd4db and target(0xd4db)==0x927c60
assert raw(0x70c080,8)==bytes.fromhex('0fb781ec010000c3') # ushort consumed counter
assert raw(0x75ea38,7)==bytes.fromhex('6689abec010000')
assert raw(0x75eaa6,7)==bytes.fromhex('66ff83ec010000')
assert raw(0x950320,8)==bytes.fromhex('0fb68180010000c3')
assert raw(0x75e1cc,6)==bytes.fromhex('ff9028030000') # discovery vcall before swap removal
messages=re.findall(r'Tr\("([^"\\]*(?:\\.[^"\\]*)*)"\)',(ROOT/'ItemMarkers.cpp').read_text(encoding='utf-8-sig'))
locales=[];out=ROOT/'tests/_build-known-maps/locales';out.mkdir(parents=True,exist_ok=True)
for lang in ('en_GB','ru_RU','zh_CN'):
 path=ROOT/f'locale/{lang}/LC_MESSAGES/item_markers.po';catalog=po.parse_po(str(path))
 assert all(catalog.get(msg) for msg in messages),(lang,set(messages)-catalog.keys())
 result=out/f'{lang}.mo';po.write_mo(catalog,str(result))
 translated=gettext.GNUTranslations(io.BytesIO(result.read_bytes()))
 for msg in messages:assert translated.gettext(msg)==catalog[msg]
 locales.append(dict(language=lang,messages=len(catalog),po_sha256=sha(path),mo_sha256=sha(result)))
dll=ROOT.parents[1]/'build/ItemMarkers/ItemMarkers.dll';candidate=peutil.PE(dll)
assert candidate.machine==0x8664
assert {'?startPlugin@@YAXXZ','MCM_Describe'}<=candidate.exports()
imports=0
for dep,names in candidate.imports().items():
 if dep.lower() in ('kenshilib.dll','ogremain_x64.dll','myguiengine_x64.dll'):
  assert not set(names)-peutil.PE(game/dep).exports(),(dep,set(names)-peutil.PE(game/dep).exports())
  imports+=len(names)
report=dict(native_exe_sha256=sha(exe),native_windows=windows,instruction_pins=total,call_edges=5,locales=locales,
 candidate=dict(path=str(dll),bytes=dll.stat().st_size,sha256=sha(dll),game_imports=imports),no_install=True,no_live_process_access=True)
(ROOT/'tests/known-maps-native-report.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'PASS {len(windows)} native windows / {total} instruction pins / 5 call edges; 3 complete compiled locales; {imports} candidate game imports')
