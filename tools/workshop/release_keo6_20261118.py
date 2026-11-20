"""Prepare KEO 6 and submit it to Steam under the owner's Steam-first instruction."""
from pathlib import Path
import argparse
import ctypes
import gettext
import hashlib
import io
import json
import os
import subprocess
import sys
import time
import zipfile

TOOLS = Path('D:/DEV/Kenshi/ours/tools/workshop')
OURS = TOOLS.parents[1]
WORK = Path('D:/DEV/Kenshi/workshop')
ROOT = Path('D:/DEV/Kenshi/forks/LEOPARD-Kenshi-Forks/KEO')
STAGE = ROOT / 'build/late-factory-release'
REPORT = OURS / 'docs/releases/2026-11-18-keo6-publication.json'
PREPARED = OURS / 'docs/releases/2026-11-18-keo6-preparation.json'
DLL = ROOT / 'build/KEO_step4_factorychain_prod/KEO.dll'
DLL_HASH = 'd0f442610ce92d362178ff924d006bc1672e10b62211e244653fbdddb986a033'
ARCHIVE = WORK / 'nexus/KEO-6.zip'
ITEM = 3834042449
CATALOGS = [f'locale/{lang}/LC_MESSAGES/keo.mo' for lang in ('en_GB', 'ru_RU', 'zh_CN')]
FILES = {'KEO.dll', 'KEO.mod', 'README.md', 'LICENSE', 'RE_Kenshi.json',
         'KEO.defaults.ini', *CATALOGS}
NOTE = {
    'version': '6',
    'published_version': '5',
    'en': [
        'Fixed a second compatibility issue that could prevent KEO jobs from registering when another plugin had already connected to the game’s job creation system. This could leave Shift + right-click unable to add Waypoint route.',
        'If registration still fails, the standard log now identifies which stage failed.',
    ],
    'ru': [
        'Исправлена ещё одна причина, по которой работы KEO могли не регистрироваться, если другой плагин уже подключился к системе создания заданий игры. Из-за этого Shift + ПКМ мог не добавлять работу «Маршрут по точкам».',
        'Если регистрация всё же не удаётся, в обычном журнале теперь указан этап сбоя.',
    ],
}


def load(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def save(path, value):
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_package():
    meta = load(WORK / 'KEO/item.json')
    assert (meta['version'], meta['workshop_id'], meta['nexus']) == ('6', ITEM, 2271)
    content = WORK / 'KEO/content'
    assert {p.relative_to(content).as_posix() for p in content.rglob('*') if p.is_file()} == FILES
    with zipfile.ZipFile(ARCHIVE) as bundle:
        assert bundle.testzip() is None
        assert sorted(bundle.namelist()) == sorted('KEO/' + name for name in FILES)
        for name in FILES:
            assert bundle.read('KEO/' + name) == (content / name).read_bytes(), name
    assert sha(content / 'KEO.dll') == DLL_HASH
    assert load(content / 'RE_Kenshi.json') == {'Plugins': ['KEO.dll']}
    for name in CATALOGS:
        catalog = gettext.GNUTranslations(io.BytesIO((content / name).read_bytes()))
        assert len(catalog._catalog) == 158
        if '/ru_RU/' in name:
            assert all(not any(c in value for c in '‐‑‒–—―−')
                       for key, value in catalog._catalog.items() if key)
    readme = (content / 'README.md').read_text(encoding='utf-8')
    assert readme.startswith('# KEO (MCM fork) (QoL) - release 6\n')
    assert all(line in readme for lang in ('en', 'ru') for line in NOTE[lang])
    return meta


def prepare():
    assert not REPORT.exists() and not PREPARED.exists()
    assert load(TOOLS / 'published_releases.json')['mods']['KEO']['version'] == '5'
    assert sha(DLL) == DLL_HASH
    old_zip = WORK / 'nexus/KEO-5.zip'
    assert sha(old_zip) == load(TOOLS / 'published_releases.json')['mods']['KEO']['archive_sha256']
    candidate = STAGE / 'inputs/KEO'
    candidate.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(old_zip) as previous:
        for name in FILES - {'KEO.defaults.ini', 'KEO.dll', 'README.md'}:
            path = candidate / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(previous.read('KEO/' + name))
    (candidate / 'KEO.dll').write_bytes(DLL.read_bytes())
    readme = ROOT / 'README.md'
    doc = readme.read_text(encoding='utf-8')
    assert doc.startswith('# KEO (MCM fork) (QoL) - release 5\n')
    doc = doc.replace(' - release 5\n', ' - release 6\n', 1)
    doc = doc.rstrip() + '\n\n## Release 6 / Выпуск 6\n'
    for lang, title in [('en', 'English'), ('ru', 'Русский')]:
        doc += '\n### ' + title + '\n\n' + ''.join('- ' + line + '\n' for line in NOTE[lang])
    readme.write_text(doc, encoding='utf-8')
    (candidate / 'README.md').write_text(doc, encoding='utf-8')
    notes_path = TOOLS / 'release_notes.json'
    notes = load(notes_path)
    notes['KEO'] = NOTE
    save(notes_path, notes)
    forks_path = TOOLS / 'fork_catalog.json'
    forks = load(forks_path)
    forks['KEO']['version'] = '6'
    forks['KEO']['history']['6'] = {lang: NOTE[lang] for lang in ('en', 'ru')}
    save(forks_path, forks)
    build = TOOLS / 'build.py'
    text = build.read_text(encoding='utf-8')
    old = 'dict(mod="KEO", title="KEO (MCM fork) (QoL)", nexus=2271, version="5",'
    assert text.count(old) == 1
    build.write_text(text.replace(old, old.replace('version="5"', 'version="6"')), encoding='utf-8')
    for name in ('build.py', 'release_notes.json', 'fork_catalog.json'):
        (WORK / name).write_bytes((TOOLS / name).read_bytes())
    subprocess.run([sys.executable, str(WORK / 'prepare_release.py'), 'KEO'], check=True,
                   env=dict(os.environ, KENSHI_RELEASE_INPUT_ROOT=str(candidate.parent)))
    verify_package()
    save(PREPARED, dict(mod='KEO', version='6', code_commit='0dbe9c7', archive=str(ARCHIVE),
        archive_sha256=sha(ARCHIVE), dll_sha256=DLL_HASH, files=sorted(FILES), changes=NOTE,
        nexus_status='prepared_for_owner', verification='CRC, exact Steam/Nexus parity, three MO catalogs'))


def publish():
    sys.path.insert(0, str(TOOLS))
    from publish_loading_20261117 import BoundedSteam, verified_query
    import publish_keo_20261105 as p

    assert not REPORT.exists(), 'Inspect the existing submission receipt before any retry'
    prepared = load(PREPARED)
    assert sha(ARCHIVE) == prepared['archive_sha256']
    meta = verify_package()
    p.validate_descriptions.validate({'KEO'})
    assert load(TOOLS / 'published_releases.json')['mods']['KEO']['version'] == '5'
    note = p.u.change_note('KEO', '6')
    assert 'English' in note and 'Русский' in note
    state = dict(mod='KEO', version='6', workshop_id=ITEM, nexus=2271,
                 nexus_status='prepared_for_owner', archive_sha256=sha(ARCHIVE),
                 dll_sha256=DLL_HASH, steam_status='connecting',
                 bilingual_change_note_sha256=hashlib.sha256(note.encode()).hexdigest())
    st = BoundedSteam()
    try:
        user = p.bind(st, 'SteamAPI_ISteamClient_GetISteamUser', ctypes.c_void_p,
                      ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_char_p)(
                          st.SteamClient(), st.HUser(), st.HPipe(), b'SteamUser019')
        assert user and p.bind(st, 'SteamAPI_ISteamUser_GetSteamID', ctypes.c_uint64,
                               ctypes.c_void_p)(user) == p.OWNER
        assert p.bind(st, 'SteamAPI_ISteamUtils_GetAppID', ctypes.c_uint32, ctypes.c_void_p)(st.utils) == 233860
        state['remote_before'] = p.query(st, ITEM, 'english')
        state['steam_status'] = 'submitting'
        save(REPORT, state)
        p.u.upload(st, 'KEO', note)
        state['steam_status'] = 'submitted'
        save(REPORT, state)
        remote = {}
        for language, suffix in [('english', 'en'), ('russian', 'ru')]:
            title = meta.get('title_ru', meta['title']) if suffix == 'ru' else meta['title']
            description = (WORK / 'KEO' / f'description_{suffix}.txt').read_text(encoding='utf-8')
            deadline = time.monotonic() + 45
            while True:
                row = verified_query(st, ITEM, language, description, title)
                if row['time_updated'] > state['remote_before']['time_updated']:
                    remote[language] = row
                    break
                if time.monotonic() >= deadline:
                    raise RuntimeError('Steam did not yet report the new update timestamp')
                time.sleep(2)
        state.update(steam_status='published_verified', verified_msk=p.stamp(), remote_after=remote)
        save(REPORT, state)
    finally:
        st.Shutdown()

    for folder in (WORK, TOOLS):
        path = folder / 'published_releases.json'
        ledger = load(path)
        row = ledger['mods']['KEO']
        assert row['version'] == '5'
        row.update(version='6', steam_version='6', nexus_version='5',
                   archive_sha256=state['archive_sha256'], dll_sha256=DLL_HASH,
                   published_msk=state['verified_msk'],
                   bilingual_change_note_sha256=state['bilingual_change_note_sha256'])
        ledger['date'] = state['verified_msk'][:10]
        save(path, ledger)
        path = folder / 'mcm_catalog.json'
        catalog = load(path)
        row = next(row for row in catalog if row['package'] == 'KEO')
        assert row['nexus_published_version'] == '5'
        row['published_version'] = '6'
        save(path, catalog)
    assert p.journal.JOURNAL.read_text(encoding='utf-8').count('Мод: KEO | Версия: 6\n\n') == 1
    print('KEO 6 published in Steam and verified. Nexus package ready.')


def confirm_nexus():
    from datetime import datetime, timedelta, timezone

    state = load(REPORT)
    assert state['steam_status'] == 'published_verified'
    assert state['nexus_status'] in ('prepared_for_owner', 'published_confirmed_by_owner')
    for folder in (WORK, TOOLS):
        path = folder / 'published_releases.json'
        ledger = load(path)
        row = ledger['mods']['KEO']
        assert row['version'] == row['steam_version'] == '6'
        assert row['archive_sha256'] == state['archive_sha256']
        row['nexus_version'] = '6'
        save(path, ledger)
        path = folder / 'mcm_catalog.json'
        catalog = load(path)
        row = next(row for row in catalog if row['package'] == 'KEO')
        assert row['published_version'] == '6'
        row['nexus_published_version'] = '6'
        save(path, catalog)
    state['nexus_status'] = 'published_confirmed_by_owner'
    state.setdefault('nexus_confirmed_msk', datetime.now(timezone(timedelta(hours=3))).isoformat())
    save(REPORT, state)
    print('Owner confirmed Nexus 6. Steam and Nexus recorded as version 6.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    actions = {'prepare': prepare, 'publish': publish, 'confirm-nexus': confirm_nexus}
    parser.add_argument('action', choices=actions)
    actions[parser.parse_args().action]()
