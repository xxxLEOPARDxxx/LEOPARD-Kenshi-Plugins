"""Read-only verification of the LoK Russian overlay and its preserved baseline."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import sys


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    for option in ('tools', 'english', 'full', 'overlay', 'before', 'changes', 'report'):
        ap.add_argument('--' + option, type=Path, required=True)
    args = ap.parse_args()
    sys.path.insert(0, str(args.tools))
    import moddiff
    import lok_translate
    from leopard_difficulty import serialize

    english = {r['string_id']: r for r in moddiff.records(str(args.english))}
    full = moddiff.records(str(args.full))
    overlay = moddiff.records(str(args.overlay))
    before = moddiff.records(str(args.before))
    changes = json.loads(args.changes.read_text(encoding='utf-8'))
    assert len(english) == len(full)
    assert len({r['string_id'] for r in overlay}) == len(overlay)
    # Full intermediate may only differ in strings/names, never in gameplay.
    gameplay = ('bool', 'float', 'int', 'vec3', 'vec4', 'file', 'refs', 'inst')
    for record in full:
        source = english[record['string_id']]
        assert source['type'] == record['type']
        for section in gameplay:
            assert record[section] == source[section], (record['string_id'], section)

    translated = {}
    for record in overlay:
        source = english[record['string_id']]
        assert record['type'] == source['type'] and record['id'] == 0
        assert not any(record[section] for section in gameplay)
        strings, original = dict(record['str']), dict(source['str'])
        assert len(strings) == len(record['str'])
        if record['change'] == 51:
            assert strings['_original_name'] == source['name']
            translated[record['string_id'] + '|name'] = record['name']
        else:
            assert record['change'] == 49 and record['name'] == source['name']
        for field, value in strings.items():
            if field.startswith('_original_'):
                continue
            assert field != 'class name'
            assert strings['_original_' + field] == original[field]
            translated[record['string_id'] + '|' + field] = value
    previous = {}
    for record in before:
        if record['change'] == 51:
            previous[record['string_id'] + '|name'] = record['name']
        previous.update({record['string_id'] + '|' + k: v for k, v in record['str']
                         if not k.startswith('_original_')})
    assert not previous.keys() - translated.keys(), 'Previous translations lost'
    for key, value in previous.items():
        assert translated[key] == changes.get(key, value), key
    for key, value in changes.items():
        assert translated[key] == value, key
        sid, field = key.split('|', 1)
        original = english[sid]['name'] if field == 'name' else dict(english[sid]['str'])[field]
        assert re.findall(r'/[A-Z_]+/', original) == re.findall(r'/[A-Z_]+/', value)
        assert not any(char in value for char in '\u2014\u2013\u2026\u00ab\u00bb')

    internal = []
    for record in full:
        for field, value in lok_translate.visible_units(record):
            if field == 'class name' or not lok_translate.LAT.search(value) or lok_translate.CYR.search(value):
                continue
            # Existing hidden FCS templates, town override labels and innate attacks.
            assert field == 'name'
            assert ('(override)' in value or value == 'GENERATION_TEMPLATE_CHARACTER'
                    or value.startswith(('1test', '_', 'FISTS-'))), (record['string_id'], value)
            internal.append({'id': record['string_id'], 'name': value})
    raw = args.overlay.read_bytes()
    body = b''.join(serialize(r) for r in overlay)
    start = len(raw) - len(body)
    assert raw[start:] == body
    assert struct.unpack_from('<2i', raw) == (17, start - 16)
    assert struct.unpack_from('<i', raw, start - 4)[0] == len(overlay)
    pos, header_strings = 12, []
    for _ in range(4):
        size = struct.unpack_from('<i', raw, pos)[0]
        header_strings.append(raw[pos + 4:pos + 4 + size].decode('utf-8'))
        pos += 4 + size
    assert header_strings[2] == 'Legendary of Kenshi.mod'
    result = {
        'english_records': len(english), 'overlay_records': len(overlay),
        'translated_names': sum(r['change'] == 51 for r in overlay),
        'translated_fields': len(translated), 'previous_fields_preserved': len(previous),
        'new_fields': len(translated.keys() - previous.keys()),
        'revised_fields': sum(previous[k] != translated[k] for k in previous),
        'stale_anchors': 0, 'missing_player_strings': 0, 'gameplay_overrides': 0,
        'intentional_internal_labels': internal,
        'english_sha256': hashlib.sha256(args.english.read_bytes()).hexdigest(),
        'overlay_sha256': hashlib.sha256(raw).hexdigest(),
    }
    args.report.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({k: v for k, v in result.items() if k != 'intentional_internal_labels'}, ensure_ascii=False))


if __name__ == '__main__':
    main()
