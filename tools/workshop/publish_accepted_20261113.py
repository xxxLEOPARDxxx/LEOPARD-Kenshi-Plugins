"""Publish six accepted updates after owner-confirmed Nexus release."""
from pathlib import Path
from datetime import datetime
import ctypes, hashlib, json, subprocess, sys, time

sys.path.insert(0, 'D:/DEV/Kenshi/ours/tools/workshop')
import publish_keo_20261105 as p

ROOT = p.O/'docs/releases'
REPORT = ROOT/'2026-11-13-six-publication.json'
PREPARED = ROOT/'2026-11-13-accepted-six-prepared.json'

def save(state):
    REPORT.write_text(json.dumps(state, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')

class BoundedSteam(p.Steam):
    def wait(self, call, struct, cb, update=None):
        failed = ctypes.c_bool(False)
        deadline = time.monotonic()+180
        while not self.IsDone(self.utils, call, ctypes.byref(failed)):
            if time.monotonic() > deadline:
                raise TimeoutError('Inspect remote item before retrying this submission')
            self.RunCallbacks()
            time.sleep(.1)
        result = struct()
        ok = self.GetResult(self.utils, call, ctypes.byref(result), ctypes.sizeof(result), cb, ctypes.byref(failed))
        if not ok or failed.value:
            raise RuntimeError(f'Steam API failure: {self.FailReason(self.utils, call)}')
        return result

def main():
    prepared = json.loads(PREPARED.read_text(encoding='utf-8'))['packages']
    if REPORT.exists():
        raise RuntimeError('Publication receipt exists; inspect it before any retry')
    p.validate_descriptions.validate(set(prepared))
    state = dict(nexus_status='published_confirmed_by_owner', confirmed_msk=p.stamp(), packages={})
    for mod, entry in prepared.items():
        archive = Path(entry['archive'])
        assert hashlib.sha256(archive.read_bytes()).hexdigest() == entry['sha256']
        meta = json.loads((p.W/mod/'item.json').read_text(encoding='utf-8'))
        ledger = json.loads((p.W/'published_releases.json').read_text(encoding='utf-8'))['mods'][mod]
        assert meta['version'] == entry['version']
        assert meta['workshop_id'] == ledger['workshop_id'] and meta['nexus'] == ledger['nexus']
        state['packages'][mod] = dict(version=entry['version'], nexus=meta['nexus'], workshop_id=meta['workshop_id'], steam_status='pending')
        # Nexus is now a confirmed release. Use the canonical --published writer.
        text = p.journal.russian(p.u.change_note(mod, entry['version']))
        subprocess.run([sys.executable, str(p.W/'personal_changelog.py'), '--published', '--mod', mod,
                        '--version', entry['version'], '--text', text], check=True)
    save(state)
    st = BoundedSteam()
    try:
        user = p.bind(st,'SteamAPI_ISteamClient_GetISteamUser',ctypes.c_void_p,ctypes.c_void_p,ctypes.c_int,ctypes.c_int,ctypes.c_char_p)(st.SteamClient(),st.HUser(),st.HPipe(),b'SteamUser019')
        assert user and p.bind(st,'SteamAPI_ISteamUser_GetSteamID',ctypes.c_uint64,ctypes.c_void_p)(user)==p.OWNER
        assert p.bind(st,'SteamAPI_ISteamUtils_GetAppID',ctypes.c_uint32,ctypes.c_void_p)(st.utils)==233860
        for mod, entry in state['packages'].items():
            print(f"Publishing {mod} {entry['version']}", flush=True)
            entry['remote_before'] = p.query(st, entry['workshop_id'], 'english')
            entry['steam_status'] = 'submitting'; save(state)
            p.u.upload(st, mod, p.u.change_note(mod, entry['version']))
            entry['steam_status'] = 'submitted'; save(state)
            entry['remote_after'] = {}
            meta = json.loads((p.W/mod/'item.json').read_text(encoding='utf-8'))
            for language, filename in [('english','description_en.txt'),('russian','description_ru.txt')]:
                remote = p.query(st, entry['workshop_id'], language)
                assert remote['description'].strip() == (p.W/mod/filename).read_text(encoding='utf-8').strip(), (mod,language)
                assert remote['title'] == meta.get('title_ru',meta['title']) if language=='russian' else remote['title']==meta['title']
                entry['remote_after'][language] = remote
            entry.update(steam_status='published_verified', verified_msk=p.stamp())
            save(state)
            for directory in (p.W, p.O/'tools/workshop'):
                path = directory/'published_releases.json'
                ledger = json.loads(path.read_text(encoding='utf-8'))
                ledger['date'] = datetime.now(p.journal.MOSCOW).strftime('%Y-%m-%d')
                ledger['mods'][mod] = {k:entry[k] for k in ('nexus','workshop_id','version')}
                path.write_text(json.dumps(ledger,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
                path = directory/'mcm_catalog.json'
                catalog = json.loads(path.read_text(encoding='utf-8'))
                for item in catalog:
                    if item['package']==mod:item['published_version']=entry['version']
                path.write_text(json.dumps(catalog,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
            journal = p.journal.JOURNAL.read_text(encoding='utf-8')
            assert journal.count(f"Мод: {mod} | Версия: {entry['version']}\n\n")==1
            print(f"Verified {mod} {entry['version']}: Steam EN/RU, release ledger and one journal entry",flush=True)
        state['status']='all_six_published_verified';save(state)
    finally:
        st.Shutdown()

if __name__=='__main__':main()
