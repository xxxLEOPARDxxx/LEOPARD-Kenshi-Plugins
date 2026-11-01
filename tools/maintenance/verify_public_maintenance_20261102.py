"""Independent final composition/content verification of the 36 local ZIPs."""
from pathlib import Path
import datetime
import gettext
import hashlib
import io
import json
import os
import re
import subprocess
import sys
import zipfile
from PIL import Image

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1];DEV=ROOT.parent;CANON=ROOT/'tools/workshop';WORK=DEV/'workshop'
PROOF=DEV/'build/ReleaseMaintenance20261102'
def read(p):return json.loads(p.read_text('utf-8-sig'))
def digest(data):return hashlib.sha256(data).hexdigest()
def sha(p):return digest(p.read_bytes())
def write(p,x):p.write_text(json.dumps(x,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
plan=read(HERE/'release-candidate-plan-2026-11-02.json')
baseline=read(HERE/'all-ready-steam-baselines-2026-11-02.json')
before=read(PROOF/'metadata-before.json')
old_inventory=read(HERE/'release-inventory-2026-11-01.json')
old_rows={r['package']:r for r in old_inventory['packages']}
inputs=Path(plan['input_root']);manifest={r['package']:r for r in read(inputs/'release-inputs.json')['packages']}
names=[r['package'] for r in plan['packages']]
assert len(names)==36 and 'AIFixed' not in names and set(names)==set(manifest)
notes=read(CANON/'release_notes.json');history=read(CANON/'changelog.json');forks=read(CANON/'fork_catalog.json')
assert sha(CANON/'published_releases.json')==before['published_ledger_sha256']==sha(WORK/'published_releases.json')
assert notes['AIFixed']==before['notes']['AIFixed']
assert read(CANON/'desc/qol-20261029.json')['AIFixed']==before['guides']['AIFixed']
for package,versions in before['history'].items():
    for version,text in versions.items():assert history[package][version]==text,(package,version)
for package,data in before['forks'].items():
    for version,text in data.get('history',{}).items():assert forks[package]['history'][version]==text,(package,version)
ai_archive=WORK/'nexus/AIFixed-1.zip'
assert sha(ai_archive)==old_rows['AIFixed']['archives'][0]['sha256']
assert not (WORK/'nexus/AIFixed-2.zip').exists()
assert (WORK/'nexus/Formations-2.zip').is_file() and (WORK/'nexus/Dark UI Advanced-2.zip').is_file()
rows=[]
unchanged_game_dlls=[]
for item in plan['packages']:
    name=item['package'];version=item['candidate_version'];entry=manifest[name]
    installed=old_rows[name].get('installed_mods')
    if installed:
        assert sha(Path(installed['path']))==installed['sha256'],(name,'game DLL changed during preparation')
        unchanged_game_dlls.append(installed['path'])
    assert int(version)==int(item['verified_steam_version'])+1
    assert notes[name]['version']==version and notes[name]['published_version']==item['verified_steam_version']
    archive=WORK/'nexus'/(name+'-'+version+'.zip')
    content=WORK/name/'content';input_dir=inputs/name
    with zipfile.ZipFile(archive) as z:
        assert z.testzip() is None
        members=z.namelist();assert len(members)==len(set(members))
        assert all(n.startswith(name+'/') and '..' not in n.split('/') for n in members)
        expected={name+'/'+str(p.relative_to(content)).replace('\\','/'):p for p in content.rglob('*') if p.is_file()}
        assert set(members)==set(expected),(name,'content composition')
        for member,path in expected.items():assert digest(z.read(member))==sha(path),(name,member)
        dlls=[n for n in members if Path(n).name==entry['dll']]
        assert len(dlls)==1 and digest(z.read(dlls[0]))==entry['dll_sha256']
        current_build=DEV/'build'/old_rows[name]['build_name']/entry['dll']
        assert sha(current_build)==entry['dll_sha256']==sha(input_dir/entry['dll'])
        assert not any(n.endswith('.po') or n.lower().endswith('.log') or '99_useroverride' in n.lower() or Path(n).name=='mod-config.json' for n in members)
        for n in members:
            if n.lower().endswith('.ini'):
                assert n.lower().endswith('.defaults.ini') or ('/SourceData/' in n and Path(n).name in {'00_Vanilla.ini','50_OfficialExtensions.ini','90_ModSupport.ini'}),(name,n)
                text=z.read(n).decode('utf-8-sig',errors='replace')
                assert not re.search(r'(?mi)^Debug\s*=\s*(1|true)\s*$',text),(name,'Detailed log default')
        catalogs=[n for n in members if n.endswith('.mo')]
        assert len(catalogs)==3
        languages=sorted(n.split('/locale/')[1].split('/')[0] for n in catalogs)
        assert languages==['en_GB','ru_RU','zh_CN']
        for n in catalogs:
            catalog=gettext.GNUTranslations(io.BytesIO(z.read(n)))
            assert catalog._catalog
            rel=n[len(name)+1:];assert digest(z.read(n))==sha(input_dir/rel)
        readmes=[n for n in members if Path(n).name.lower() in {'readme.txt','readme.md'}]
        assert readmes
        assert any(('Prepared release '+version+' / Подготовленный выпуск '+version) in z.read(n).decode('utf-8-sig') for n in readmes)
        assert all(notes[name].get(lang) for lang in ('en','ru'))
        # Every available original license in the source root travels with a fork.
        source=Path(old_rows[name]['source_root'])
        license_sources=[p for p in source.iterdir() if p.is_file() and p.name.upper().startswith(('LICENSE','COPYING'))]
        missing_licenses=[p.name for p in license_sources if not any(Path(n).name==p.name for n in members)]
        assert not missing_licenses,(name,'source license omitted',missing_licenses)
        descriptions={lang:(WORK/name/('description_'+lang+'.txt')).read_text('utf-8') for lang in ('en','ru')}
        sizes={lang:len(text.encode('utf-8')) for lang,text in descriptions.items()}
        assert max(sizes.values())<=7900
        if name in forks:
            for lang,text in descriptions.items():
                assert forks[name]['author'] in text and forks[name]['original_url'] in text
                assert ('Original source code:' if lang=='en' else 'Исходники оригинала:') in text
        nexus=(WORK/'nexus'/(name+'_description.txt')).read_text('utf-8')
        assert '[size=5][b]Русский[/b][/size]' in nexus and '[line]' in nexus
        assert not any(s in nexus for s in ("What's changed in version ",'Что изменено в версии ','OUR FORK VERSION HISTORY','ИСТОРИЯ ВЕРСИЙ НАШЕГО ФОРКА'))
        cover=WORK/'images'/name/'000_LEOPARD_Cover.png';assert cover.is_file()
        with Image.open(cover) as image:image.verify()
        preview=next((WORK/name).glob('preview.*'))
        with Image.open(preview) as image:
            image.verify()
        assert (WORK/name/'screens').is_dir()
        if name=='UITweaks':
            assert not any(label in nexus for label in ('Sequential squad trading','Последовательный обмен'))
            defaults=z.read('UITweaks/UITweaks.defaults.ini').decode('utf-8-sig')
            assert 'SavePreviews=1' in defaults and 'SaveDialogHeightPercent=120' in defaults and defaults.index('SaveDialogHeightPercent=120')<defaults.index('Debug=0')
            cpp=(ROOT/'UITweaks/UITweaks.cpp').read_text('utf-8-sig')
            assert re.search(r'"SaveDialogHeightPercent"[\s\S]*?120, 100, 160, 0\)',cpp)
            for n in catalogs:
                catalog=gettext.GNUTranslations(io.BytesIO(z.read(n)))
                assert catalog.gettext('Save/load window height, %')
            assert descriptions['en'].count('[b]Save previews.[/b]')==1
            assert descriptions['ru'].count('[b]Превью сохранений.[/b]')==1
            assert not any(token in nexus for token in ('1600x900','lossless PNG','PNG без потерь'))
        rows.append({'package':name,'verified_steam_version':item['verified_steam_version'],'candidate_version':version,
                     'archive':str(archive),'archive_bytes':archive.stat().st_size,'archive_sha256':sha(archive),
                     'dll_sha256':entry['dll_sha256'],'files':len(members),'languages':languages,
                     'readmes':[Path(n).name for n in readmes],'license_files':[Path(n).name for n in members if Path(n).name.upper().startswith(('LICENSE','COPYING'))],
                     'steam_description_bytes':sizes,'screens':len(list((WORK/name/'screens').iterdir())),
                     'status':'verified local Nexus-ready candidate; not uploaded'})
        assert len(list((WORK/'nexus').glob(name+'-*.zip')))==1
env=dict(os.environ,KENSHI_RELEASE_INPUT_ROOT=str(inputs),PYTHONIOENCODING='utf-8')
subprocess.run([sys.executable,str(WORK/'validate_descriptions.py'),*names],env=env,stdout=(PROOF/'description-validation.json').open('w',encoding='utf-8'),check=True)
sync=[]
for p in CANON.iterdir():
    if p.is_file() and p.suffix in {'.py','.json'}:
        assert (WORK/p.name).is_file() and p.read_bytes()==(WORK/p.name).read_bytes(),p.name
        sync.append(p.name)
review={'reviewer':'parent agent /root','result':'CLEAR','recorded_date':'2026-11-02 Europe/Moscow',
        'evidence':['Independent review of fail-closed resolver, exact-payload packer, full proof, release-note examples, UI README and factory defaults.',
                    'Fresh independent verify_public_maintenance_20261102.py passed all 36 candidates.',
                    'Fresh independent input-override and pack-boundary tests passed, including old-ZIP preservation.']}
report={'verified_at_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'status':'36 accepted local Nexus-ready candidates; no upload',
        'independent_review':review,
        'candidate_refinements': {'date': '2026-11-02 Europe/Moscow', 'Vital-Sense': {'version': '7', 'source_review': 'CLEAR by parent /root', 'dll_sha256': 'cbc895107ec0e4cc5b6a42b53f946ca450d1311cea15c80a2a09caf8ed506ad3', 'change': 'Native Knockout artwork preserves its own RGB for unconscious/recovery fallback icons.'}, 'UITweaks': {'version': '13', 'change': 'Vanilla sequential squad trading removed from EN/RU public feature guides and generator reinsertion; source README/history retained.', 'archive_unchanged': True, 'public_summary': 'One simple optional save-preview feature and one MCM save-list-height feature per language; implementation detail remains in README/changelog.'}, 'game_preservation_checked_at_utc': '2026-11-01T22:06:43.733687+00:00', 'game_preservation_scope': 'All 34 installed DLLs verified unchanged before parent-authorized Vital Sense installation; the preservation assertion was not weakened.'},
        'prepared':len(rows),'held':{'AIFixed':'Release 1 archive unchanged; release 2 awaits GateFix/SleepFix integration acceptance.'},
        'publication_ledger_unchanged':True,'past_changelog_and_fork_history_preserved':True,
        'game_dlls_verified_unchanged':unchanged_game_dlls,
        'checks':'CRC, exact content/source/DLL SHA256, no PO/personal INI/logs/99 override, three MO languages, current bilingual README/notes, source licenses, full EN/RU descriptions and BBCode/Steam limits, existing cover/screen folders',
        'scripts_and_metadata_identical':sync,'packages':rows}
write(HERE/'release-preparation-proof-2026-11-02.json',report)
lines=['# Prepared public plugin updates, 2026-11-02','','36 local Nexus-ready candidates are verified and accepted after independent parent-agent review. Nothing was uploaded to Nexus or Steam. The user uploads Nexus first and supplies links; Steam publication follows. AIFixed remains held for one integrated release 2; its release 1 ZIP is unchanged.','',
       'All 36 versions are exactly the current official Steam description baseline plus one. CRC and every archived file hash match prepared content; primary DLLs match canonical builds; all packages contain current README and three MO catalogs only. Personal settings/logs/99 overrides are excluded. Bilingual descriptions, BBCode, Steam byte limits, source licenses, covers and screenshot folders pass. Prior changelog/fork-history entries and the published ledger are unchanged.','',
       '| Package | Steam baseline → local candidate | ZIP bytes | SHA256 |','| --- | --- | --- | --- |']
for r in rows:lines.append(f'| {r["package"]} | {r["verified_steam_version"]} → {r["candidate_version"]} | {r["archive_bytes"]} | `{r["archive_sha256"]}` |')
lines+=['','UI Tweaks 13 includes the new Save/Load PNG previews and configurable dialog height; historical version 12 trading investigation stays in README/history and vanilla sequential squad trading is excluded from public feature descriptions. Container Highlight 3 includes stale-model restoration protection. Vital Read 6/Vital Sense 7 include optional-icon preflight; Vital Sense 7 also preserves the native coloured Knockout glyph beside green labels. StackSort 3 includes quiet optional Arrange lookup. ItemMarkers retains the native stolen badge without claiming an ownership/stacking cure. All 36 include the per-plugin detailed localization-log fix.','',
        'StackSort, Hidden Faction Relations and Map markers were freshly built and import-checked; StackSort production tests pass. Only the disabled DEV stage copies of HFR/Map were synchronized. Clean release input folders were used, with no installation of game DLLs or edits to personal INI. RangedDamageXp factory Debug was corrected to 0; UI Tweaks factory defaults include SaveDialogHeightPercent=120 before Debug.','',
        'Independent parent-agent review on 2026-11-02: CLEAR. The reviewer inspected the resolver, packer, release notes, UI README and defaults, then independently reran the 36-package verifier and both override/pack-boundary tests successfully, including old-ZIP preservation. The local candidates are accepted; this does not authorize or claim publication.','',
        'Runtime acceptance, FPS and long-term memory claims are outside package verification. See the JSON proof for exact DLL identities, composition, descriptions and artifact paths.','']
(HERE/'release-preparation-proof-2026-11-02.md').write_text('\n'.join(lines),encoding='utf-8')
queue=read(HERE/'localization-release-queue.json');queue['status']='36 verified local Nexus-ready candidates prepared; AIFixed held; no uploads'
write(HERE/'localization-release-queue.json',queue)
print(f'PASS: {len(rows)} complete local candidates; preserved publication ledger/history/AIFixed; {len(sync)} synchronized release files')
