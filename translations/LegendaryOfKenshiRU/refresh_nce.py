"""Refresh only LoK armour in an existing THE_NCE_Patch; dry run by default.

Uses the installed Kakrain .patch formula, not a new armour balance. The legacy
FCS codec is supplied explicitly. Other NCE records/fields are retained verbatim.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import sys


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--tools', type=Path, required=True)
    ap.add_argument('--game', type=Path, required=True)
    ap.add_argument('--english', type=Path, required=True)
    ap.add_argument('--previous', type=Path, required=True)
    ap.add_argument('--translation', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--report', type=Path, required=True)
    args = ap.parse_args()
    sys.path.insert(0, str(args.tools))
    import moddiff
    import modconflicts
    from leopard_difficulty import serialize

    def encode_record(record):
        data = serialize(record)
        # Kakrain's existing v17 output stores zero here rather than byte size.
        # Preserve its record framing as well as all unmodified field bytes.
        return struct.pack('<i', record.get('instances', len(data))) + data[4:]

    previous = {r['string_id']: r for r in moddiff.records(str(args.previous))}
    latest = {r['string_id']: r for r in moddiff.records(str(args.english))}
    localized_names = {r['string_id']: r['name'] for r in moddiff.records(str(args.translation))}
    targets = {
        sid for sid, r in latest.items() if r['type'] == 3 and
        (sid not in previous or moddiff.fields(previous[sid]) != moddiff.fields(r))
    }
    patch_path = args.game / 'mods/THE_NCE_Patch/THE_NCE_Patch.mod'
    rule_path = patch_path.with_suffix('.patch')
    rule = rule_path.read_text(encoding='utf-8-sig')
    # Fail rather than silently use this evaluator for a different patch rule.
    expected_rule = (
        'BCD[0]:=0.04', 'BCD[1]:=0.22', 'BCD[2]:=0.24', 'BCD[3]:=0.3',
        'CF[0]:=1.0', 'CF[1]:=2.0', 'CF[2]:=2.25', 'CF[3]:=2.5', 'CF[4]:=2.75',
        'BCR[0]:=0.0', 'BCR[1]:=0.0', 'BCR[2]:=2.0', 'BCR[3]:=4.0',
        'allarmors:=(all)(A:ARMOUR|true);',
        'allarmors->SetField("cut def bonus",((((((BCD[GetField("material type")]*CF[GetField("class")])+GetField("cut def bonus"))*(95+GetField("level bonus")))+(BCR[GetField("material type")]*CF[GetField("class")]))*(1-GetField("cut into stun"))-(BCR[GetField("material type")]*CF[GetField("class")]))/(95+GetField("level bonus")))-(BCD[GetField("material type")]*CF[GetField("class")]))',
        'allarmors->SetField("cut into stun",0)',
    )
    statements = tuple(line.strip() for line in rule.splitlines()
                       if line.strip() and not line.lstrip().startswith(';'))
    if statements != expected_rule:
        raise ValueError('Installed NCE rule differs; inspect it before generating')

    order = [line.strip() for line in (args.game / 'data/mods.cfg').read_text(
        encoding='utf-8-sig').splitlines() if line.strip().endswith('.mod')]
    patch_name = patch_path.name
    if patch_name not in order:
        raise ValueError('THE_NCE_Patch is not enabled')
    # Only source layers preceding the generated patch can be corrected by it.
    source_order = ['gamedata.base', 'Newwworld.mod', 'Dialogue.mod', 'rebirth.mod']
    source_order += order[:order.index(patch_name)]
    merged = {sid: {} for sid in targets}
    current_old = {sid: {} for sid in targets}
    sources = []
    for name in source_order:
        if name in source_order[:4]:
            path = args.game / 'data' / name
        else:
            found = modconflicts.find_mod(name)
            if not found:
                raise FileNotFoundError(name)
            path = Path(found)
        if name == 'Legendary of Kenshi.mod':
            old_records = list(previous.values())
            records = list(latest.values())
            path = args.english
        else:
            raw = path.read_bytes()
            # Empty v17 DLL wrapper with a long description is valid too.
            is_empty = (len(raw) >= 16 and struct.unpack_from('<i', raw)[0] == 17
                        and struct.unpack_from('<i', raw, 4)[0] + 16 == len(raw)
                        and raw[-4:] == bytes(4))
            records = [] if is_empty else moddiff.records(str(path))
            old_records = records
        for collection, destination in [(records, merged), (old_records, current_old)]:
            for r in collection:
                if r['string_id'] in targets and r['type'] == 3:
                    destination[r['string_id']].update(modconflicts.scalars(r))
        sources.append({'name': name, 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()})

    def compensate(fields):
        material, cls = fields[('int', 'material type')], fields[('int', 'class')]
        if not 0 <= material < 4 or not 0 <= cls < 5:
            raise ValueError('Unsupported armour material/class')
        bcd = [0.04, 0.22, 0.24, 0.3][material]
        cf = [1.0, 2.0, 2.25, 2.5, 2.75][cls]
        bcr = [0.0, 0.0, 2.0, 4.0][material]
        cut = fields.get(('float', 'cut def bonus'), 0.0)
        efficiency_loss = fields.get(('float', 'cut into stun'), 0.0)
        level = 95 + fields.get(('int', 'level bonus'), 0)
        if level <= 0 or not 0 <= efficiency_loss <= 1:
            raise ValueError('Unexpected NCE input')
        # Exact operation order of the installed author-supplied patch.
        result = (((bcd * cf + cut) * level + bcr * cf) * (1 - efficiency_loss)
                  - bcr * cf) / level - bcd * cf
        if not math.isfinite(result):
            raise ValueError('Non-finite NCE result')
        return struct.unpack('<f', struct.pack('<f', result))[0]

    original = patch_path.read_bytes()
    edits = moddiff.records(str(patch_path))
    serialized = b''.join(encode_record(r) for r in edits)
    start = len(original) - len(serialized)
    if original[start:] != serialized:
        raise ValueError('Existing patch does not round-trip through the codec')
    indexed = {r['string_id']: r for r in edits}
    report = {'targets': len(targets), 'changes': [], 'sources': sources,
              'rule_sha256': hashlib.sha256(rule_path.read_bytes()).hexdigest()}
    baseline_mismatch = []
    for sid in sorted(targets):
        record = indexed.get(sid)
        before = dict(record['float']) if record else {}
        if sid in previous:
            expected_old = compensate(current_old[sid])
            if not record or not math.isclose(before.get('cut def bonus', math.nan),
                                             expected_old, rel_tol=1e-6, abs_tol=1e-7):
                baseline_mismatch.append({'id': sid, 'expected': expected_old, 'existing': before})
        target = compensate(merged[sid])
        name = localized_names[sid]
        old_name = record['name'] if record else None
        if (record and before.get('cut def bonus') == target
                and before.get('cut into stun') == 0 and old_name == name):
            continue
        if not record:
            record = {'type': 3, 'id': 0, 'instances': 0, 'string_id': sid, 'name': name,
                      'change': 33, **{k: [] for k in ('bool', 'float', 'int', 'vec3',
                                                      'vec4', 'str', 'file', 'refs', 'inst')}}
            edits.append(record)
        record['name'] = name
        values = dict(record['float'])
        values.update({'cut def bonus': target, 'cut into stun': 0.0})
        record['float'] = list(values.items())
        report['changes'].append({'id': sid, 'name': latest[sid]['name'], 'before': before,
                                  'after': values, 'new_record': sid not in indexed,
                                  'name_before': old_name, 'name_after': name})
    report['baseline_mismatch'] = baseline_mismatch
    args.report.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    if baseline_mismatch:
        raise ValueError(f'{len(baseline_mismatch)} existing NCE entries do not match old source; see report')
    header = bytearray(original[:start])
    struct.pack_into('<i', header, start - 4, len(edits))
    args.output.write_bytes(bytes(header) + b''.join(encode_record(r) for r in edits))
    assert len(moddiff.records(str(args.output))) == len(edits)
    print(f"Checked {len(targets)} LoK armour records; existing formula reproduced; "
          f"changed/appended {len(report['changes'])}; output {args.output}")


if __name__ == '__main__':
    main()
