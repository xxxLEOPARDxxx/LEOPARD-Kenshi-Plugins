"""Publish verified MCM13 after owner-confirmed Nexus2202 update."""
from datetime import datetime,timezone,timedelta
from pathlib import Path
import ctypes,hashlib,json,re,zipfile
import publish_keo_20261105 as p
MOD='ModConfigMenu';ID=3813761584
REPORT=p.O/'docs/releases/2026-11-05-mcm13-publication.json'
def save(s):REPORT.write_text(json.dumps(s,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def main():
    p.validate_descriptions.validate({MOD})
    meta=json.loads((p.W/MOD/'item.json').read_text(encoding='utf-8'))
    assert (meta['version'],meta['nexus'],meta['workshop_id'])==('13',2202,ID)
    with zipfile.ZipFile(p.W/'nexus/ModConfigMenu-13.zip')as z:
        assert z.testzip()is None and len(z.namelist())==15
        assert hashlib.sha256(z.read(MOD+'/ModConfigMenu.dll')).hexdigest()=='eceff075d5ae576dc66f0999049d872701da9eb846fcc8d01d8a6e8ed934e58b'
        for n in z.namelist():assert z.read(n)==(p.W/MOD/'content'/n.removeprefix(MOD+'/')).read_bytes()
    if REPORT.exists():
        previous=json.loads(REPORT.read_text(encoding='utf-8'))
        assert previous.get('steam_status')not in ('submitting','published_verified'),'Inspect earlier submission before retry'
    state=dict(mod=MOD,version='13',nexus=2202,nexus_status='update_confirmed_by_owner',steam_status='connecting',time_msk=p.stamp())
    save(state);st=p.Steam()
    try:
        user=p.bind(st,'SteamAPI_ISteamClient_GetISteamUser',ctypes.c_void_p,ctypes.c_void_p,ctypes.c_int,ctypes.c_int,ctypes.c_char_p)(st.SteamClient(),st.HUser(),st.HPipe(),b'SteamUser019')
        assert user and p.bind(st,'SteamAPI_ISteamUser_GetSteamID',ctypes.c_uint64,ctypes.c_void_p)(user)==p.OWNER
        assert p.bind(st,'SteamAPI_ISteamUtils_GetAppID',ctypes.c_uint32,ctypes.c_void_p)(st.utils)==233860
        state['remote_before']={lang:p.query(st,ID,lang)for lang in ('english','russian')}
        assert re.search(r'Version 12\b',state['remote_before']['english']['description'])
        state['steam_status']='submitting';save(state)
        p.u.upload(st,MOD,p.u.change_note(MOD,'13'))
        state['remote_after']={}
        for lang,file in [('english','description_en.txt'),('russian','description_ru.txt')]:
            remote=p.query(st,ID,lang)
            assert remote['description'].strip()==(p.W/MOD/file).read_text(encoding='utf-8').strip()
            assert remote['title']==meta['title']
            state['remote_after'][lang]=remote
        state.update(steam_status='published_verified',verified_msk=p.stamp(),workshop_id=ID,workshop_url=f'https://steamcommunity.com/sharedfiles/filedetails/?id={ID}')
        save(state)
        for root in (p.W,p.O/'tools/workshop'):
            path=root/'published_releases.json';ledger=json.loads(path.read_text(encoding='utf-8'));ledger['date']='2026-11-05';ledger['mods'][MOD]={'nexus':2202,'workshop_id':ID,'version':'13'};path.write_text(json.dumps(ledger,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
            path=root/'mcm_catalog.json';catalog=json.loads(path.read_text(encoding='utf-8'));entry=next(e for e in catalog if e['package']==MOD);entry['published_version']='13';path.write_text(json.dumps(catalog,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
        journal=p.journal.JOURNAL.read_text(encoding='utf-8');assert journal.count('Мод: ModConfigMenu | Версия: 13\n\n')==1
        print('MCM13 published; EN/RU verified, one journal entry, ledgers updated')
    finally:st.Shutdown()
if __name__=='__main__':main()
