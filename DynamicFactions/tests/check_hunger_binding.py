"""Read-only final-PE regression: hook the SDK entry, never a virtual thunk.

Uses the same pefile/capstone tooling as the local native diagnostics. Does not
load the plugin or call any game function; accepts a DLL path for build checks.
"""
from pathlib import Path
import hashlib
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[3] / 'tools'))
sys.path.insert(0, r'D:/SteamLibrary/steamapps/common/Kenshi/_codex_work/qol-review/pythonlibs')
import pefile
import capstone
import slots

path = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parents[3] / 'build/DynamicFactions/DynamicFactions.dll'
pe = pefile.PE(str(path))
base = pe.OPTIONAL_HEADER.ImageBase
imports = {s.address: s.name.decode('ascii') for d in pe.DIRECTORY_ENTRY_IMPORT for s in d.imports if s.name}
resolver = next(a for a, n in imports.items() if n.startswith('?GetRealAddress@'))
expected = '?_NV_getMagicHungerSetting@Character@@QEAAMXZ'
cs = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
failures, verified = [], []
for section in pe.sections:
    if not section.Characteristics & 0x20000000:
        continue
    data = section.get_data()
    for n in range(len(data) - 6):
        call = base + section.VirtualAddress + n
        if data[n:n+2] != b'\xff\x15' or call + 6 + struct.unpack_from('<i', data, n+2)[0] != resolver:
            continue
        # VC10 /GL emits RIP-relative RCX argument setup. Find its last definition
        # in this short straight-line call setup, decoding the actual instruction.
        definitions = [k for k in range(max(0, n-48), n-6) if data[k:k+3] in (b'\x48\x8b\x0d', b'\x48\x8d\x0d')]
        if not definitions:
            continue
        k = definitions[-1]
        target = base + section.VirtualAddress + k + 7 + struct.unpack_from('<i', data, k+3)[0]
        if data[k:k+3] == b'\x48\x8b\x0d' and imports.get(target) == expected:
            verified.append(hex(call-base))
        elif data[k:k+3] == b'\x48\x8d\x0d' and base <= target < base + pe.OPTIONAL_HEADER.SizeOfImage:
            code = list(cs.disasm(pe.get_data(target-base, 16), target))
            if len(code) >= 2 and code[0].mnemonic == 'mov' and code[0].op_str == 'rax, qword ptr [rcx]' and code[1].mnemonic == 'jmp' and '[rax' in code[1].op_str:
                failures.append('GetRealAddress receives local virtual-dispatch thunk at DLL+' + hex(target-base))
if not verified:
    failures.append('No GetRealAddress call bound to the nonvirtual hunger SDK import')
native = slots.resolve('_NV_getMagicHungerSetting@Character')
if len(native) != 1 or native[0][1] != 0x5c9210:
    failures.append('Installed KenshiLib hunger mapping differs from the audited Steam function')
print('DLL SHA256:', hashlib.sha256(path.read_bytes()).hexdigest())
print('Nonvirtual hunger binding:', verified, 'native Steam RVA:', [hex(x[1]) for x in native])
for failure in failures:
    print('FAIL:', failure)
print('Binding check:', 'FAIL' if failures else 'PASS')
sys.exit(bool(failures))
