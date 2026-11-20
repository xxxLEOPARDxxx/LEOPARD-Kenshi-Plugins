"""Offline native ABI proof; does not open a game process or launch Kenshi."""
from pathlib import Path
import hashlib, json, re, struct, sys
sys.path.insert(0,'D:/DEV/Kenshi/tools/_native_python')
import pefile
here=Path(__file__).resolve().parent
exe=Path('D:/SteamLibrary/steamapps/common/Kenshi/RE_Kenshi/Kenshi_x64.exe')
data=exe.read_bytes();pe=pefile.PE(data=data,fast_load=True)
assert hashlib.md5(data).hexdigest()=='df4a5a7ef8a29deb24b70e7b7f4a222a'
proof=json.loads((here/'engineer_material_native_contracts.json').read_text())
runtime=(here.parent/'EngineerMaterialsRuntime.h').read_text()
assert pe.FILE_HEADER.TimeDateStamp==proof['timestamp']
assert pe.OPTIONAL_HEADER.SizeOfImage==proof['imageSize']
for row in proof['windows']:
    off=pe.get_offset_from_rva(row['rva']);body=data[off:off+row['size']]
    assert body.hex()==row['bytes'],row['name']
    value=14695981039346656037
    for b in body:value=((value^b)*1099511628211)&0xffffffffffffffff
    assert value==int(row['fnv'],16),row['name']
    assert re.search(r'\{0x%X,%d,0x%016Xui64\}'%(row['rva'],row['size'],value),runtime)
base=pe.OPTIONAL_HEADER.ImageBase
for table,index,thunk,target in [(0x16BD7E0,2,0x43E3C,0x33CCB0),(0x16BD7E0,5,0x3E52C,0x341160),
                              (0x16BDE38,2,0x4C41A,0x34F2E0),(0x16BDEF8,2,0x29AF0,0x348810)]:
    assert struct.unpack_from('<Q',data,pe.get_offset_from_rva(table)+8*index)[0]==base+thunk
    at=pe.get_offset_from_rva(thunk);assert data[at]==0xe9
    assert thunk+5+struct.unpack_from('<i',data,at+1)[0]==target
# Verify the actual two-level factory branch for task247, not a stale SDK index.
index=data[pe.get_offset_from_rva(0x3303C8)+247-1]
assert struct.unpack_from('<I',data,pe.get_offset_from_rva(0x330218)+index*4)[0]==0x32F906
assert data[pe.get_offset_from_rva(0x341160):pe.get_offset_from_rva(0x341176)].hex()==\
    '4883ec1848c70424feffffffb8020000004883c418c3'
# Source removal is tied to the actual private action's exact call site;
# acquisition is observed at native Inventory::tryAddItem for both vanilla and
# KEO's direct backpack route, independently of Character::giveItem hook order.
assert '0x33CF1C' in runtime
assert '0x74B1E0' in runtime
assert 'AddHook(intptr_t(image+0x74B1E0)' not in runtime
assert 'AddHook(intptr_t(image+0x3BA11)' in runtime
for table in (0x170BF88,0x17367C8):
    assert struct.unpack_from('<Q',data,pe.get_offset_from_rva(table)+0x18)[0]==base+0x3BA11
assert data[pe.get_offset_from_rva(0x3BA11):pe.get_offset_from_rva(0x3BA11)+5].hex()=='e9caf77000'
# The native task releases its temporary advertised-material lektor through
# the same game CRT array delete that releases the plugin's observation list.
delete_thunk=0xED562E
delete_import=delete_thunk+6+struct.unpack_from('<i',data,pe.get_offset_from_rva(delete_thunk)+2)[0]
pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_IMPORT']])
assert any(d.dll.lower()==b'mfc100u.dll' and e.ordinal==266
           and e.address==base+delete_import for d in pe.DIRECTORY_ENTRY_IMPORT for e in d.imports)
delete_at=0x33CFBF
assert pe.get_data(delete_at,1)==b'\xe8'
assert delete_at+5+struct.unpack_from('<i',data,pe.get_offset_from_rva(delete_at)+1)[0]==delete_thunk
assert pe.get_data(0x33CF04,13).hex()=='498b0424498bccff9060010000' # root building getInventory
# Ordinary hauling and construction share the same map, but only the exact
# construction call may use the private receiver and its candidate filters.
def direct_target(at, opcode):
    code=pe.get_data(at,5)
    assert code[0]==opcode
    return at+5+struct.unpack_from('<i',code,1)[0]
for caller,thunk,target in ((0x5A20CB,0x1515E,0x59C570),
                            (0x59D166,0x1515E,0x59C570),
                            (0x59C7B0,0xA81C,0x996EB0),
                            (0x59C88D,0xF222,0x8CAFD0)):
    assert direct_target(caller,0xE8)==thunk
    assert direct_target(thunk,0xE9)==target
assert pe.get_data(0x59D15F,7).hex()=='498d8e08020000' # native actor's cache at +0x208
assert pe.get_data(0x640396,3).hex()=='488911' # native scratch constructor stores AI* at +0
assert struct.unpack('<f',pe.get_data(0x167B308,4))[0]==1.0
# The allocation-free shortcut recognizes only the original single-output
# method. Multi-output crafting has a different dispatch target.
for thunk,target in [(0x481D5,0x299AC0),(0x16D9C,0x2B5410)]:
    code=pe.get_data(thunk,5)
    assert code[0]==0xe9 and thunk+5+struct.unpack_from('<i',code,1)[0]==target
assert pe.get_data(0x299AD3,6).hex()=='ff9080050000' # isProductionEmpty, slot +0x580
assert pe.get_data(0x299AE3,6).hex()=='ff9050050000' # getProductionItemData, slot +0x550
for name,rva in [('thunk',0x481D5),('offerSingle',0x299AC0)]:
    match=re.search(r'unsigned char '+name+r'\[\]=\{([^}]+)\};',runtime)
    expected=bytes(int(value,16) for value in re.findall(r'0x[0-9a-f]+',match[1]))
    assert pe.get_data(rva,len(expected))==expected
assert pe.get_data(0x296f4f,4).hex()=='488b4918'     # native quantity uses output section
assert pe.get_data(0x745d27,6).hex()=='03812c010000' # section count adds item unit quantities
body=next(x for x in proof['windows'] if x['name']=='body')
tail=14695981039346656037
for b in bytes.fromhex(body['bytes'])[5:]:tail=((tail^b)*1099511628211)&0xffffffffffffffff
assert tail==int(proof['sharedBodyTail'],16)
assert ('0x%016Xui64'%tail) in runtime
dest=next(x for x in proof['windows'] if x['name']=='destination')
tail=14695981039346656037
for b in bytes.fromhex(dest['bytes'])[5:]:tail=((tail^b)*1099511628211)&0xffffffffffffffff
assert tail==int(proof['sharedDestinationTail'],16)
assert ('0x%016Xui64'%tail) in runtime
assert struct.unpack_from('<Q',data,pe.get_offset_from_rva(0x1740910)+0x20)[0]==base+0x5305D
carry=next(x for x in proof['windows'] if x['name']=='carryWeight');tail=14695981039346656037
for b in bytes.fromhex(carry['bytes'])[5:]:tail=((tail^b)*1099511628211)&0xffffffffffffffff
assert tail==int(proof['sharedCarryTail'],16)
assert ('0x%016Xui64'%tail) in runtime
for at,target in [(0x883916,0x2132E9C),(0x883922,0x2132E98)]:
    off=pe.get_offset_from_rva(at);assert at+8+struct.unpack_from('<i',data,off+4)[0]==target
table=Path('D:/SteamLibrary/steamapps/common/Kenshi/RE_Kenshi/RVAs/Steam_1.0.65.br').read_bytes()
assert struct.unpack_from('<I',table,7623*4)[0]==0x2132D70
assert struct.unpack_from('<I',data,pe.get_offset_from_rva(0x883E8C)+4)[0]==0x883CB9
assert '_MaxCarryWeight' not in runtime
for table,slot,thunk,target in proof['stopVirtuals']:
    assert '{0x%X,%d,0x%X,0x%X}'%(table,slot,thunk,target) in runtime
    assert struct.unpack_from('<Q',data,pe.get_offset_from_rva(table)+slot*8)[0]==base+thunk
    off=pe.get_offset_from_rva(thunk);assert data[off]==0xe9
    assert thunk+5+struct.unpack_from('<i',data,off+1)[0]==target
assert 'MoveBowRange' in runtime and not any(row[0]==0x16BCE40 for row in proof['stopVirtuals'])
print('engineer materials native contracts: %d windows, 38 RTTI virtuals, task247 factory, 3 shared entry tails, virtual thunk vs stock body, native carry/Move end-action PASS'%len(proof['windows']))
