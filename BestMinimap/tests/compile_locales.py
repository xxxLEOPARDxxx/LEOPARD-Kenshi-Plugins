"""Compile source catalogs, checking active messages and supported RU/EN glyphs."""
import ast, gettext, pathlib, re, struct
ROOT = pathlib.Path(__file__).resolve().parents[1]
required = {ast.literal_eval(s) for s in re.findall(r'\bTr\(("(?:[^"\\]|\\.)*")\)', (ROOT/'BestMinimap.cpp').read_text(encoding='utf-8-sig'))}
for lang in ('en_GB', 'ru_RU', 'zh_CN'):
    po = ROOT/'locale'/lang/'LC_MESSAGES/best_minimap.po'
    catalog = {}; key = None; value = ''; mode = None
    for line in po.read_text(encoding='utf-8-sig').splitlines()+['']:
        if not line.strip():
            if key is not None: catalog[key] = value
            key = None; value = ''; mode = None
        elif line.startswith('msgid '): key = ast.literal_eval(line[6:]); mode = 'id'
        elif line.startswith('msgstr '): value = ast.literal_eval(line[7:]); mode = 'str'
        elif line.startswith('"'):
            if mode == 'id': key += ast.literal_eval(line)
            elif mode == 'str': value += ast.literal_eval(line)
        else: assert not line.startswith(('msgid_plural', 'msgstr[')), 'Plural parser required'
    assert required <= catalog.keys(), required-catalog.keys()
    assert all(not k or v for k,v in catalog.items())
    if lang in ('en_GB','ru_RU'):
        assert all(not re.search('[^\x00-\x7f\u0400-\u04ff]', v) for k,v in catalog.items() if k), 'Unsupported game-font glyph'
    keys = sorted(catalog); n = len(keys); offset = 28+16*n; tables = []; data = b''
    for values in ([k.encode('utf-8') for k in keys], [catalog[k].encode('utf-8') for k in keys]):
        table = []
        for v in values: table.append((len(v), offset+len(data))); data += v+b'\0'
        tables.append(b''.join(struct.pack('<II',*x) for x in table))
    mo = po.with_suffix('.mo')
    mo.write_bytes(struct.pack('<7I',0x950412de,0,n,28,28+8*n,0,0)+b''.join(tables)+data)
    with mo.open('rb') as f: translated = gettext.GNUTranslations(f)
    assert all(translated.gettext(k) == v for k,v in catalog.items() if k)
    print(lang, n-1, 'messages; active IDs, font glyphs and MO roundtrip OK')
