"""Offline evidence collector for a legally obtained GOG Kenshi executable.

Never loads/calls game code, opens a process, downloads files or enables KEO.
The report identifies artifacts; it does not certify native hook compatibility.
"""
from __future__ import annotations

import argparse
import datetime
import hashlib
import json
from pathlib import Path
import struct

MAX_IMAGE_BYTES = 128 * 1024 * 1024
MAX_TABLE_BYTES = 400000
MAX_COVERAGE_BYTES = 4 * 1024 * 1024
KNOWN_EXE_MD5 = {
    '213349bf76a0a758067f9ed0aef2ab01': 'gog_1.0.65',
    'df4a5a7ef8a29deb24b70e7b7f4a222a': 'steam_1.0.65',
}
KNOWN_GOG_TABLES = {
    'a2a6394e95312a89e3d3f6842fbfda9c16e7d79749f1ee796fbd6168600bf347': {
        'entries': 7630,
        'matching_kenshilib_sha256': '5b6240622d06801dc39896e385efb834b8e1fe2fcdf86067d071ae7a04f6c83b',
    },
    '8910248abd88d78b777c8f77d5e8ef5287b87757001000cade44e7082e58cbc0': {
        'entries': 7678,
        'matching_kenshilib_sha256': '9c7f3c6f63f8faff3573095199494f8e985fda91f54b9c3cad7079d0a56b1886',
    },
}


def identify_executable(md5):
    return KNOWN_EXE_MD5.get(md5.lower(), 'unknown')


def bounded_file(path, maximum):
    path = Path(path).resolve(strict=True)
    if not path.is_file() or not 0 < path.stat().st_size <= maximum:
        raise ValueError('file missing, empty or larger than bounded limit: ' + str(path))
    with path.open('rb') as stream:
        raw = stream.read(maximum + 1)
    if len(raw) > maximum:
        raise ValueError('file grew past bounded limit')
    return path, raw


def parse_pe(raw):
    if len(raw) < 0x40 or raw[:2] != b'MZ':
        raise ValueError('missing DOS header')
    pe = struct.unpack_from('<I', raw, 0x3c)[0]
    if pe < 0x40 or pe + 24 > len(raw) or raw[pe:pe + 4] != b'PE\0\0':
        raise ValueError('invalid PE header')
    machine, count, stamp, _, _, optional_size, flags = struct.unpack_from('<HHIIIHH', raw, pe + 4)
    optional = pe + 24
    if machine != 0x8664 or not 1 <= count <= 96 or optional_size < 112 or optional + optional_size > len(raw):
        raise ValueError('not a bounded AMD64 PE32+ image')
    if struct.unpack_from('<H', raw, optional)[0] != 0x20b:
        raise ValueError('PE32+ required')
    image_size, headers = struct.unpack_from('<II', raw, optional + 56)
    if not headers <= len(raw) or image_size < headers:
        raise ValueError('invalid image/header extent')
    section_start = optional + optional_size
    if section_start + count * 40 > min(len(raw), headers):
        raise ValueError('section table outside headers')
    sections = []
    for index in range(count):
        name, virtual_size, rva, disk_size, disk_offset = struct.unpack_from('<8sIIII', raw, section_start + index * 40)
        if rva + max(virtual_size, disk_size) > image_size or disk_offset + disk_size > len(raw):
            raise ValueError('section outside image/file extent')
        if disk_size and disk_offset < headers:
            raise ValueError('section overlaps headers')
        for old in sections:
            if max(rva, old['rva']) < min(rva + max(virtual_size, disk_size), old['rva'] + max(old['virtual_size'], old['disk_size'])):
                raise ValueError('overlapping virtual sections')
        sections.append(dict(name=name.rstrip(b'\0').decode('ascii', errors='replace'), rva=rva,
                             virtual_size=virtual_size, disk_size=disk_size, disk_offset=disk_offset))
    return dict(machine=machine, timestamp=stamp, image_size=image_size, headers=headers,
                image_base=struct.unpack_from('<Q', raw, optional + 24)[0],
                entrypoint=struct.unpack_from('<I', raw, optional + 16)[0], flags=flags, sections=sections)


def rva_bytes(raw, pe, rva, size):
    if rva < 0 or not 1 <= size <= 65536:
        raise ValueError('invalid RVA/size')
    for section in pe['sections']:
        relative = rva - section['rva']
        if 0 <= relative and relative + size <= section['disk_size']:
            start = section['disk_offset'] + relative
            return bytes(raw[start:start + size])
    raise ValueError('RVA window not fully backed by one file section')


def table_metadata(raw):
    if not raw or len(raw) % 4 or len(raw) > MAX_TABLE_BYTES:
        raise ValueError('RVA table must contain bounded raw little-endian uint32 entries')
    digest = hashlib.sha256(raw).hexdigest()
    known = KNOWN_GOG_TABLES.get(digest)
    entries = len(raw) // 4
    if known and entries != known['entries']:
        raise ValueError('known table count mismatch')
    return dict(bytes=len(raw), entries=entries, sha256=digest,
                known_gog_pair=known, zero_entries=sum(value[0] == 0 for value in struct.iter_unpack('<I', raw)))


def artifact(path, raw):
    return dict(path=str(path), bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest())


def collect(exe_path, table_path, coverage_path=None):
    exe, raw = bounded_file(exe_path, MAX_IMAGE_BYTES)
    pe = parse_pe(raw)
    exe_meta = artifact(exe, raw)
    exe_meta['md5'] = hashlib.md5(raw).hexdigest()
    exe_meta['identity'] = identify_executable(exe_meta['md5'])
    table, table_raw = bounded_file(table_path, MAX_TABLE_BYTES)
    report = dict(schema=1, captured_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
                  scope='Offline artifact identification only; no GOG hook/ABI/runtime compatibility claim',
                  executable=exe_meta, pe=pe, rvas=dict(path=str(table), **table_metadata(table_raw)),
                  native_profile_verified=False, runtime_tested=False, code_windows=[])
    is_gog = exe_meta['identity'] == 'gog_1.0.65'
    known_table = report['rvas']['known_gog_pair'] is not None
    report['status'] = ('gog_image_and_known_table_identified' if is_gog and known_table else 'required_gog_inputs_unverified')
    if coverage_path:
        coverage, coverage_raw = bounded_file(coverage_path, MAX_COVERAGE_BYTES)
        evidence = json.loads(coverage_raw)
        rows = evidence['manifest']
        if not isinstance(rows, list) or len(rows) > 512:
            raise ValueError('coverage manifest bound')
        report['coverage'] = dict(artifact(coverage, coverage_raw), source_head=evidence.get('source_head', evidence.get('head')),
                                  declared_rows=len(rows), unresolved=[])
        for row in rows:
            candidates = row.get('official_gog_rvas', row.get('official_table_matches', []))
            if not isinstance(candidates, list) or len(candidates) > 64:
                raise ValueError('symbol match bound')
            addresses = sorted(set(int(candidate, 16) if isinstance(candidate, str) else candidate['gog'] for candidate in candidates))
            if len(addresses) != 1:
                report['coverage']['unresolved'].append(dict(name=row['name'], kind=row['kind'], steam_rva=row['steam_rva']))
            elif is_gog:
                gog_rva = addresses[0]
                report['code_windows'].append(dict(name=row['name'], kind=row['kind'], rva=hex(gog_rva),
                                                   first32=rva_bytes(raw, pe, gog_rva, 32).hex()))
    return report


def write_report(path, report):
    # Exclusive creation preserves old evidence. The caller must name a new file.
    with Path(path).open('x', encoding='utf-8', newline='\n') as stream:
        json.dump(report, stream, ensure_ascii=False, indent=2)
        stream.write('\n')


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True, help='legitimate Kenshi_x64.exe to inspect; never executed')
    parser.add_argument('--rvas', required=True, help='GOG_1.0.65.br from the matching KenshiLib package')
    parser.add_argument('--coverage', help='current KEO coverage evidence JSON, optional')
    parser.add_argument('--output', required=True, help='new report JSON; existing files are never overwritten')
    args = parser.parse_args(argv)
    try:
        report = collect(args.exe, args.rvas, args.coverage)
        write_report(args.output, report)
    except (OSError, ValueError, KeyError, TypeError, struct.error) as exc:
        parser.exit(2, 'REFUSED: ' + str(exc) + '\n')
    print(report['status'] + '; native_profile_verified=false; runtime_tested=false')
    return 0 if report['status'] == 'gog_image_and_known_table_identified' else 2


if __name__ == '__main__':
    raise SystemExit(main())
