"""Read actual PE tables without loading DLLs or invoking native functions.

RE_Kenshi-src/Plugins.cpp:43 resolves ?startPlugin@@YAXXZ exactly.
MCM independently resolves the undecorated C export MCM_Describe.
"""
from pathlib import Path
import argparse
import hashlib
import json
import struct

START = '?startPlugin@@YAXXZ'
NATIVE_DLLS = {'kenshilib.dll', 'ogremain_x64.dll', 'myguiengine_x64.dll'}


class PE:
    def __init__(self, path):
        self.path = Path(path)
        self.data = self.path.read_bytes()
        if self.data[:2] != b'MZ':
            raise ValueError('missing MZ header')
        nt = self.u32(0x3c)
        if self.data[nt:nt + 4] != b'PE\0\0':
            raise ValueError('missing PE header')
        self.opt = nt + 24
        if self.u16(self.opt) != 0x20b or self.u16(nt + 4) != 0x8664:
            raise ValueError('expected x64 PE32+ image')
        start = self.opt + self.u16(nt + 20)
        self.sections = []
        for i in range(self.u16(nt + 6)):
            at = start + 40 * i
            size, rva, raw_size, raw = struct.unpack_from('<IIII', self.data, at + 8)
            self.sections.append((rva, max(size, raw_size), raw, raw_size))

    def u16(self, at):
        return struct.unpack_from('<H', self.data, at)[0]

    def u32(self, at):
        return struct.unpack_from('<I', self.data, at)[0]

    def offset(self, rva):
        for base, size, raw, raw_size in self.sections:
            if base <= rva < base + size and rva - base < raw_size:
                return raw + rva - base
        raise ValueError(f'unbacked RVA {rva:#x}')

    def string(self, rva):
        at = self.offset(rva)
        return self.data[at:self.data.index(b'\0', at)].decode('ascii')

    def exports(self):
        rva, size = struct.unpack_from('<II', self.data, self.opt + 112)
        if not rva:
            return {}
        at = self.offset(rva)
        count = self.u32(at + 24)
        functions = self.offset(self.u32(at + 28))
        names = self.offset(self.u32(at + 32))
        ordinals = self.offset(self.u32(at + 36))
        result = {}
        for i in range(count):
            name = self.string(self.u32(names + 4 * i))
            target = self.u32(functions + 4 * self.u16(ordinals + 2 * i))
            result[name] = (target, rva <= target < rva + size)
        return result

    def imports(self):
        rva = self.u32(self.opt + 120)
        if not rva:
            return {}
        at = self.offset(rva)
        result = {}
        while self.u32(at + 12):
            dll = self.string(self.u32(at + 12))
            thunk = self.offset(self.u32(at) or self.u32(at + 16))
            symbols = []
            while True:
                value = struct.unpack_from('<Q', self.data, thunk)[0]
                if not value:
                    break
                if value >> 63:
                    symbols.append(value & 0xffff)
                else:
                    symbols.append(self.string(value + 2))
                thunk += 8
            result[dll] = symbols
            at += 20
        return result


def verify(path, mcm=False, imports_root=None):
    pe = PE(path)
    exports = pe.exports()
    if START not in exports:
        raise ValueError(f'RE_Kenshi cannot initialize {pe.path.name}: missing exact export {START}; '
                         f'undecorated startPlugin present={"startPlugin" in exports}')
    if exports[START][0] == 0 or exports[START][1]:
        raise ValueError('startup export must resolve to a local implementation')
    if 'startPlugin' in exports:
        raise ValueError('obsolete C startup export must be absent')
    if mcm and 'MCM_Describe' not in exports:
        raise ValueError('MCM requires the exact undecorated C export MCM_Describe')
    report = {'dll': str(pe.path), 'bytes': len(pe.data),
              'sha256': hashlib.sha256(pe.data).hexdigest(), 'startup_export': START,
              'mcm_export': 'MCM_Describe' if mcm else None, 'native_imports': {}}
    if imports_root:
        for dll, symbols in pe.imports().items():
            if dll.lower() not in NATIVE_DLLS:
                continue
            available = PE(Path(imports_root) / dll).exports()
            missing = [symbol for symbol in symbols if symbol not in available]
            if missing:
                raise ValueError(f'unresolved {dll} imports: {missing}')
            report['native_imports'][dll] = len(symbols)
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dll')
    parser.add_argument('--mcm', action='store_true')
    parser.add_argument('--imports-root')
    parser.add_argument('--report')
    args = parser.parse_args()
    try:
        report = verify(args.dll, args.mcm, args.imports_root)
    except (ValueError, OSError, struct.error) as error:
        parser.exit(1, f'FAIL: {error}\n')
    if args.report:
        Path(args.report).write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(f'PASS {Path(args.dll).name}: {START}; native imports '
          f'{sum(report["native_imports"].values())}; C MCM={args.mcm}')
