"""Publish reviewed3/5/5 updates after the owner's Nexus confirmation.

An existing receipt must be inspected before retrying: never blindly resubmit.
Uses only the existing logged-in Steam IPC client; no client/registry changes.
"""
from pathlib import Path
from datetime import datetime
import ctypes, hashlib, json, os, subprocess, sys, time

O = Path('D:/DEV/Kenshi/ours')
W = Path('D:/DEV/Kenshi/workshop')
STAGE = Path('D:/SteamLibrary/steamapps/common/Kenshi/_codex_work/release-loading-20261117')
REPORT = O / 'docs/releases/2026-11-17-loading-publication.json'
sys.path.insert(0, str(O / 'tools/workshop'))
import publish_keo_20261105 as p


def save(state):
    REPORT.parent.mkdir(exist_ok=True)
    REPORT.write_text(json.dumps(state, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


class BoundedSteam(p.Steam):
    def wait(self, call, struct, cb, update=None):
        failed = ctypes.c_bool(False)
        deadline = time.monotonic() + 180
        while not self.IsDone(self.utils, call, ctypes.byref(failed)):
            if time.monotonic() > deadline:
                raise TimeoutError('Inspect remote item before retrying this submission')
            self.RunCallbacks()
            time.sleep(.1)
        result = struct()
        ok = self.GetResult(self.utils, call, ctypes.byref(result), ctypes.sizeof(result), cb, ctypes.byref(failed))
        if not ok or failed.value:
            raise RuntimeError('Steam API failure: %s' % self.FailReason(self.utils, call))
        return result


def verified_query(st, item, language, expected, title):
    deadline = time.monotonic() + 45
    while True:
        remote = p.query(st, item, language)
        if remote['description'].strip() == expected.strip() and remote['title'] == title:
            return remote
        if time.monotonic() >= deadline:
            raise RuntimeError('Remote metadata differs after submission; inspect before retry')
        time.sleep(2)


def main():
    if REPORT.exists():
        raise RuntimeError('Publication receipt exists; inspect it before any retry')
    prepared = json.loads((STAGE / 'prepared-release-receipt.json').read_text())
    expected = {'KEO': ('3', 2271, 3834042449),
                'SkillLevelUpInfo': ('5', 2257, 3829848328),
                'BestMinimap': ('5', 2242, 3824087152)}
    assert {row['mod'] for row in prepared['packages']} == set(expected)
    p.validate_descriptions.validate(set(expected))
    state = {'nexus_status': 'published_confirmed_by_owner', 'confirmed_msk': p.stamp(),
             'packages': {}, 'status': 'preflight'}
    for row in prepared['packages']:
        mod = row['mod']
        meta = json.loads((W / mod / 'item.json').read_text())
        assert (meta['version'], meta['nexus'], meta['workshop_id']) == expected[mod]
        assert row['version'] == meta['version']
        assert hashlib.sha256(Path(row['archive']).read_bytes()).hexdigest() == row['sha256']
        assert hashlib.sha256((W / mod / 'content' / (mod + '.dll')).read_bytes()).hexdigest() == row['dllSha256']
        state['packages'][mod] = dict(version=row['version'], nexus=meta['nexus'], workshop_id=meta['workshop_id'],
                                      archive_sha256=row['sha256'], dll_sha256=row['dllSha256'], steam_status='pending')
    save(state)
    # Nexus release is already confirmed. This explicit writer is idempotent;
    # the uploader's later same-version record must not create a duplicate.
    for mod, entry in state['packages'].items():
        text = p.journal.russian(p.u.change_note(mod, entry['version']))
        subprocess.run([sys.executable, str(W / 'personal_changelog.py'), '--published', '--mod', mod,
                        '--version', entry['version'], '--text', text], check=True)
    lock = W / '.loading-publication.lock'
    with lock.open('x', encoding='ascii') as f:
        f.write(str(os.getpid()))
    st = None
    try:
        st = BoundedSteam()
        user = p.bind(st, 'SteamAPI_ISteamClient_GetISteamUser', ctypes.c_void_p, ctypes.c_void_p,
                      ctypes.c_int, ctypes.c_int, ctypes.c_char_p)(st.SteamClient(), st.HUser(), st.HPipe(), b'SteamUser019')
        assert user and p.bind(st, 'SteamAPI_ISteamUser_GetSteamID', ctypes.c_uint64, ctypes.c_void_p)(user) == p.OWNER
        assert p.bind(st, 'SteamAPI_ISteamUtils_GetAppID', ctypes.c_uint32, ctypes.c_void_p)(st.utils) == 233860
        for mod, entry in state['packages'].items():
            print('Publishing %s %s' % (mod, entry['version']), flush=True)
            entry['remote_before'] = p.query(st, entry['workshop_id'], 'english')
            entry['steam_status'] = 'submitting'; save(state)
            p.u.upload(st, mod, p.u.change_note(mod, entry['version']))
            entry['steam_status'] = 'submitted'; save(state)
            meta = json.loads((W / mod / 'item.json').read_text())
            entry['remote_after'] = {}
            for language, filename in [('english', 'description_en.txt'), ('russian', 'description_ru.txt')]:
                title = meta.get('title_ru', meta['title']) if language == 'russian' else meta['title']
                entry['remote_after'][language] = verified_query(st, entry['workshop_id'], language,
                                                               (W / mod / filename).read_text(encoding='utf-8'), title)
            entry.update(steam_status='published_verified', verified_msk=p.stamp())
            save(state)
            for folder in (W, O / 'tools/workshop'):
                path = folder / 'published_releases.json'
                ledger = json.loads(path.read_text())
                ledger['date'] = datetime.now(p.journal.MOSCOW).strftime('%Y-%m-%d')
                ledger['mods'][mod].update({k: entry[k] for k in ('nexus', 'workshop_id', 'version')})
                ledger['mods'][mod].update(archive_sha256=entry['archive_sha256'], dll_sha256=entry['dll_sha256'],
                                           published_msk=entry['verified_msk'], bilingual_change_note_sha256=hashlib.sha256(p.u.change_note(mod, entry['version']).encode()).hexdigest())
                path.write_text(json.dumps(ledger, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
                path = folder / 'mcm_catalog.json'
                catalog = json.loads(path.read_text())
                for item in catalog:
                    if item['package'] == mod:
                        item['published_version'] = entry['version']
                        if 'nexus_published_version' in item: item['nexus_published_version'] = entry['version']
                path.write_text(json.dumps(catalog, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
            journal = p.journal.JOURNAL.read_text(encoding='utf-8')
            assert journal.count('Мод: %s | Версия: %s\n\n' % (mod, entry['version'])) == 1
            print('Verified %s %s: Steam EN/RU, ledger, one journal entry' % (mod, entry['version']), flush=True)
        state['status'] = 'all_three_published_verified'; save(state)
    finally:
        if st is not None: st.Shutdown()
        lock.unlink()


if __name__ == '__main__':
    main()
