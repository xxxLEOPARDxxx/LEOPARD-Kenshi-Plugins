from pathlib import Path
import hashlib
import json
import sys
import struct

here = Path(__file__).resolve().parent
sys.path.insert(0,str(here.parents[2] / 'GlobalSatelliteMap/tests'))
from test_pe_contract import PE

profile = json.loads((here / 'render_abi_profile.json').read_text(encoding='utf-8'))
root = Path('D:/SteamLibrary/steamapps/common/Kenshi')
lines = ['struct AbiWindow { DWORD rva; BYTE code[24]; };']
for key,module in profile.items():
    prefix = {'ogre':'OgreAbi', 'terrain':'TerrainAbi', 'd3d11':'D3d11Abi'}[key]
    path = root / module['file']
    if not path.exists():
        path = root / 'plugins' / module['file']
    pe = PE(path)
    assert hashlib.sha256(pe.data).hexdigest() == module['sha256'], f'{path} hash mismatch'
    assert pe.u32(pe.opt-16) == int(module['timestamp'],16), 'timestamp mismatch'
    assert pe.u32(pe.opt+56) == int(module['image_size'],16), 'image size mismatch'
    exports = pe.exports()
    if key == 'd3d11':
        # Exact RTT dynamic type, not a MultiRenderTarget aggregate. Native
        # constructor assigns this vtable; getter returns the borrowed view.
        vtable = int(module['rtt_vtable_rva'],16)
        getter = int(module['rtt_getter_rva'],16)
        destructor = int(module['rtt_destructor_rva'],16)
        base = struct.unpack_from('<Q',pe.data,pe.opt+24)[0]
        assert struct.unpack_from('<Q',pe.data,pe.offset(vtable)+0x90)[0] == base+getter
        assert struct.unpack_from('<Q',pe.data,pe.offset(vtable))[0] == base+destructor
        texture_vtable = int(module['texture_vtable_rva'],16)
        array_upload = int(module['texture_array_upload_rva'],16)
        texture_srv_getter = int(module['texture_srv_getter_rva'],16)
        assert struct.unpack_from('<Q',pe.data,pe.offset(texture_vtable)+0x2d8)[0] == base+array_upload
        # Hidden nonvirtual accessor is profiled code, never a guessed C++ SDK
        # field getter. Only the COM view it returns is borrowed by our code.
        assert pe.data[pe.offset(texture_srv_getter):pe.offset(texture_srv_getter)+8] == bytes.fromhex('488b8108020000c3')
        assert any(int(w['rva'],16)==texture_srv_getter for w in module['windows'])
        # BC native creation drops two requested levels. Production compensates
        # the creation request, while the COM upload still requires actual9.
        assert pe.data[pe.offset(0x49154):pe.offset(0x49154)+3] == bytes.fromhex('8d41fe')
        assert {0x4914a,0x49154}.issubset({int(w['rva'],16) for w in module['windows']})
        lines += [f'static const DWORD PrivateRttVtableRva = 0x{vtable:x};',
                  f'static const DWORD PrivateRttGetterRva = 0x{getter:x};',
                  f'static const DWORD PrivateRttDestructorRva = 0x{destructor:x};',
                  f'static const DWORD PrivateArrayTextureVtableRva = 0x{texture_vtable:x};',
                  f'static const DWORD PrivateArrayUploadRva = 0x{array_upload:x};',
                  f'static const DWORD PrivateArraySrvGetterRva = 0x{texture_srv_getter:x};']
    checks = list(module['windows'])
    for entry in module['exports']:
        assert exports.get(entry['name']) == (int(entry['rva'],16),False), f'export mismatch {entry}'
        lines += [f'static const char {entry["key"]}Name[] = {json.dumps(entry["name"])};',
                  f'static const DWORD {entry["key"]}Rva = 0x{entry["rva"]};']
        if 'code' in entry:
            checks.append(entry)
    for entry in checks:
        expected = bytes.fromhex(entry['code'])
        assert len(expected) == 24
        offset = pe.offset(int(entry['rva'],16))
        assert pe.data[offset:offset+24] == expected, f'ABI code mismatch {path}:{entry["rva"]}'
    lines += [f'static const char {prefix}File[] = {json.dumps(module["file"])};',
              f'static const char {prefix}Hash[] = {json.dumps(module["sha256"])};',
              f'static const DWORD {prefix}Timestamp = 0x{module["timestamp"]};',
              f'static const DWORD {prefix}Size = 0x{module["image_size"]};',
              f'static const AbiWindow {prefix}Checks[] = {{']
    lines += ['{0x'+entry['rva']+',{'+','.join('0x'+format(b,'02x') for b in bytes.fromhex(entry['code']))+'}},'
              for entry in checks]
    lines += ['};']
    print(f'Exact render ABI: {path.name}, {len(module["exports"])} exports/{len(checks)} code windows')
(here/'build/RenderAbiProfile.h').write_text('\n'.join(lines)+'\n',encoding='ascii')
