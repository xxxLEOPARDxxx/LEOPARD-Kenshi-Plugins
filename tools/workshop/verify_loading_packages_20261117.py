"""Read-only verification of the prepared3/5/5 packages; no publication."""
from pathlib import Path
import gettext, hashlib, importlib.util, io, json, re, zipfile

HERE = Path(__file__).resolve().parent
WORK = Path('D:/DEV/Kenshi/workshop')
GAME = Path('D:/SteamLibrary/steamapps/common/Kenshi')
STAGE = GAME / '_codex_work/release-loading-20261117'
spec = importlib.util.spec_from_file_location('pe', Path('D:/DEV/Kenshi/forks/LEOPARD-Kenshi-Forks/KEO/tools/kenshilib/check_imports.py'))
pe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(pe)
results = []
for mod, version in [('KEO', '3'), ('SkillLevelUpInfo', '5'), ('BestMinimap', '5')]:
    archive = WORK / 'nexus' / (mod + '-' + version + '.zip')
    with zipfile.ZipFile(archive) as z:
        assert z.testzip() is None
        names = z.namelist()
        assert len(names) == len(set(names)) and all(n.startswith(mod + '/') for n in names)
        assert not any(n.lower().endswith(('.po', '.pdb', '.obj', '.log')) or '99_useroverride' in n.lower() or 'mod-config.json' in n.lower() for n in names)
        catalogs = [n for n in names if n.endswith('.mo')]
        assert len(catalogs) == 3 and {n.split('/locale/')[1].split('/')[0] for n in catalogs} == {'en_GB', 'ru_RU', 'zh_CN'}
        assert len([n for n in names if Path(n).name.lower() in ('readme.md', 'readme.txt')]) == 1
        for name in names:
            relative = name[len(mod) + 1:]
            data = z.read(name)
            assert data == (WORK / mod / 'content' / relative).read_bytes(), name
            if relative.endswith('.ini'):
                assert relative == mod + '.defaults.ini'
                assert not re.search(r'(?mi)^Debug\s*=\s*(1|true)\s*$', data.decode('utf-8-sig'))
            if relative.endswith('.mo') and '/ru_RU/' in name:
                cat = gettext.GNUTranslations(io.BytesIO(data))
                assert not any(chr(c) in t for t in cat._catalog.values() if isinstance(t, str) for c in list(range(0x2010, 0x2016)) + [0x2212])
        dll = STAGE / 'inputs' / mod / (mod + '.dll')
        binary = z.read(mod + '/' + mod + '.dll')
        assert binary == dll.read_bytes() == (GAME / 'mods' / mod / (mod + '.dll')).read_bytes()
        image = pe.PE(dll)
        assert image.machine == 0x8664 and {'?startPlugin@@YAXXZ', 'MCM_Describe'} <= image.exports()
        imports = {symbol for name, symbol in image.imports() if name.lower() == 'kenshilib.dll'}
        assert not imports - pe.PE(GAME / 'KenshiLib.dll').exports()
        assert json.loads((WORK / mod / 'item.json').read_text())['version'] == version
        results.append({'mod': mod, 'version': version, 'archive': str(archive), 'bytes': archive.stat().st_size,
                        'files': len(names), 'sha256': hashlib.sha256(archive.read_bytes()).hexdigest(),
                        'dllSha256': hashlib.sha256(binary).hexdigest(), 'kenshiLibImports': len(imports)})
for before in ('personal-files-before.json', 'other-archives-before.json'):
    for name, expected in json.loads((STAGE / before).read_text()).items():
        assert hashlib.sha256(Path(name).read_bytes()).hexdigest() == expected, name
published = json.loads((WORK / 'published_releases.json').read_text())['mods']
assert [published[m]['version'] for m in ('KEO', 'SkillLevelUpInfo', 'BestMinimap')] == ['2', '4', '4']
readme = (STAGE / 'inputs/KEO/README.md').read_text()
assert 'Working release 2' not in readme and 'Рабочий выпуск 2' not in readme
assert '**Speed up model bone weights**' in readme
(STAGE / 'package-verification.json').write_text(json.dumps(results, indent=2))
print(json.dumps(results, indent=2))
print('CRC/content/defaults/locales/imports/MCM/versions/private preservation PASS; no publication')
