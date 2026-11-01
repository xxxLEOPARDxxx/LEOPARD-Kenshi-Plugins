"""Verify AI Fixed 2 plus the existing 36 candidates, without game mutations.

This is a new artifact/composition proof. It does not rerun, weaken or relabel
the historical 36-package installed-DLL-preservation check after installation.
"""
from pathlib import Path
import configparser
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

ROOT=Path(__file__).resolve().parents[2]
DEV=ROOT.parent
HERE=Path(__file__).resolve().parent
CANON=ROOT/'tools/workshop'
WORK=DEV/'workshop'
INPUT=WORK/'maintenance-inputs-20261102'
PROOF=DEV/'build/ReleaseMaintenance20261102'
EXPECTED='686e5a2073045bc038924cf11371424ca48626c543f6e864055184ea04c01c23'
EXPECTED_RU='731848996004d117f345a7bbf08c61ca318ae96888e0ea361e44df141f65cf24'
INSTALLED_AI=Path('D:/SteamLibrary/steamapps/common/Kenshi/mods/AIFixed/AIFixed.dll')
sys.path.insert(0,str(CANON))
from release_metadata import render_note


def read(p):return json.loads(p.read_text('utf-8-sig'))
def digest(data):return hashlib.sha256(data).hexdigest()
def sha(p):return digest(p.read_bytes())
def write(p,x):p.write_text(json.dumps(x,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')


def smoke_acceptance(smoke,actual_dll_sha,ru_path,actual_ru_sha,catalog):
    """Keep logged MCM evidence distinct from explicit user UI acceptance."""
    normalize=lambda p:os.path.normcase(os.path.normpath(str(p)))
    assert smoke['candidateSha256']==actual_dll_sha==EXPECTED
    assert smoke['startupAndMcm']=='PASS' and smoke['responding']
    assert any(normalize(p)==normalize(INSTALLED_AI) for p in smoke['loaded'])
    logs=smoke['log']
    assert any('AIFixed: installed' in line for line in logs)
    assert not any(re.search(r'\b(?:error|assert|failed|incomplete)\b',line,re.I) for line in logs)
    assert actual_ru_sha==EXPECTED_RU
    message_count=sum(bool(key) for key in catalog)
    assert message_count==42
    logged_ru=any("locale id='ru_RU', strings loaded=42" in line for line in logs)
    logged_page=any('page=AIFixed' in line for line in logs)
    details={'logged_mcm_page':logged_page,'logged_ru_catalog_summary':logged_ru,
             'installed_ru_catalog':{'path':str(ru_path),'sha256':actual_ru_sha,'messages':message_count,
                                     'evidence_kind':'installed_file_inspection'}}
    if logged_ru and logged_page:
        details.update({'status':'PASS: candidate startup and actual MCM/RU log traces verified',
                        'evidence_kind':'startup_and_logged_mcm_ru',
                        'summary':'The exact loaded candidate, installed hooks, MCM page trace and RU 42-string log trace are verified.'})
    else:
        assert smoke.get('startupHooksVerified') is True
        assert smoke.get('userPercentClarificationAccepted') is True
        evidence=smoke['mcmEvidence']
        assert evidence['kind']=='user_confirmation' and isinstance(evidence['text'],str) and evidence['text'].strip()
        declared=smoke['catalogs']
        assert normalize(declared['ruPath'])==normalize(ru_path)
        assert declared['ruSha256']==actual_ru_sha and declared['ruMessageCount']==message_count
        assert declared['percentageText'] in catalog.values() and declared['thresholdLabel'] in catalog.values()
        assert '%' in declared['thresholdLabel'] and 'процент' in declared['percentageText'].lower()
        details.update({'status':'PASS: candidate startup; user accepted percentage wording; installed RU catalog inspected',
                        'evidence_kind':'startup_user_confirmation_and_catalog_inspection',
                        'user_confirmation':evidence['text'],
                        'summary':'The exact loaded candidate and startup hooks are verified. The user explicitly accepted the percentage wording; the installed RU catalog file has 42 messages. Current MCM-page/42-string log traces are not claimed.'})
    return details


def main():
    prior=read(HERE/'release-preparation-proof-2026-11-02.json')
    baseline=read(PROOF/'aifixed-public-baseline.json')
    old=read(PROOF/'metadata-before.json')
    notes=read(CANON/'release_notes.json');history=read(CANON/'changelog.json')
    manifest={r['package']:r for r in read(INPUT/'release-inputs.json')['packages']}
    assert baseline['description_version']=='1' and notes['AIFixed']['version']=='2'
    assert notes['AIFixed']['published_version']=='1' and history['AIFixed']['2']==render_note(notes['AIFixed'])
    assert sha(CANON/'published_releases.json')==sha(WORK/'published_releases.json')==old['published_ledger_sha256']
    for name,versions in old['history'].items():
        for version,text in versions.items():assert history[name][version]==text,(name,version)
    forks=read(CANON/'fork_catalog.json')
    for name,data in old['forks'].items():
        for version,text in data.get('history',{}).items():assert forks[name]['history'][version]==text,(name,version)
    names=[r['package'] for r in prior['packages']]+['AIFixed']
    build_names={r['package']:r['build_name'] for r in read(HERE/'release-inventory-2026-11-01.json')['packages']}
    assert len(names)==len(set(names))==37 and set(names)==set(manifest)
    sys.path.insert(0,str(WORK))
    import build
    items={r['mod']:r for r in build.ITEMS}
    rows=[]
    for name in names:
        entry=manifest[name];version='2' if name=='AIFixed' else next(r['candidate_version'] for r in prior['packages'] if r['package']==name)
        assert str(items[name]['version'])==version==notes[name]['version']
        if name!='AIFixed':
            previous=next(r for r in prior['packages'] if r['package']==name)
            assert sha(WORK/'nexus'/(name+'-'+version+'.zip'))==previous['archive_sha256'],(name,'existing ZIP changed')
        archive=WORK/'nexus'/(name+'-'+version+'.zip');content=WORK/name/'content'
        with zipfile.ZipFile(archive) as z:
            assert z.testzip() is None
            members=z.namelist();assert len(members)==len(set(members))
            expected={name+'/'+str(p.relative_to(content)).replace('\\','/'):p for p in content.rglob('*') if p.is_file()}
            assert set(members)==set(expected)
            for member,p in expected.items():assert digest(z.read(member))==sha(p),(name,member)
            assert not any(n.lower().endswith(('.po','.log')) or '99_useroverride' in n.lower() or Path(n).name=='mod-config.json' for n in members)
            catalogs=[n for n in members if n.endswith('.mo')]
            assert len(catalogs)==3 and sorted(n.split('/locale/')[1].split('/')[0] for n in catalogs)==['en_GB','ru_RU','zh_CN']
            for n in catalogs:
                catalog=gettext.GNUTranslations(io.BytesIO(z.read(n)));assert catalog._catalog
            dll=name+'/'+entry['dll'];assert digest(z.read(dll))==sha(INPUT/name/entry['dll'])==entry['dll_sha256']
            build_name=build_names[name]
            assert sha(DEV/'build'/build_name/entry['dll'])==entry['dll_sha256']
            for n in members:
                if n.lower().endswith('.ini'):
                    assert n.lower().endswith('.defaults.ini') or ('/SourceData/' in n and Path(n).name in {'00_Vanilla.ini','50_OfficialExtensions.ini','90_ModSupport.ini'})
                    assert not re.search(r'(?mi)^Debug\s*=\s*(1|true)\s*$',z.read(n).decode('utf-8-sig',errors='replace'))
            if name=='AIFixed':
                assert entry['dll_sha256']==EXPECTED and len(z.read(dll))==176128
                assert len(members)==10
                for filename in ('README.txt','LICENSE','UPSTREAM.md'):
                    assert z.read('AIFixed/'+filename)==(ROOT/'AIFixed'/filename).read_bytes()
                assert digest(z.read('AIFixed/LICENSE'))=='230184f60bae2feaf244f10a8bac053c8ff33a183bcc365b4d8b876d2b7f4809'
                readme=z.read('AIFixed/README.txt').decode('utf-8-sig')
                assert readme.count('Prepared release 2 / Подготовленный выпуск 2')==1
                assert 'VERSION 1 CHANGES' in readme and 'ИСТОРИЯ: ВЕРСИЯ 1' in readme
                defaults=z.read('AIFixed/AIFixed.defaults.ini').decode('utf-8-sig');ini=configparser.ConfigParser();ini.read_string(defaults)
                keys=ini['AIFixed'];assert len(keys)==18
                assert all(keys.getint(k)==0 for k in ('AutoSleep','RepairHealth','GatePassage','Debug'))
                assert keys.getint('OwnBedsOnly')==1 and keys.getint('SelfRepair')==1
                assert [line.split('=',1)[0] for line in defaults.splitlines() if '=' in line and not line.lstrip().startswith(';')][-1]=='Debug'
                assert 'AIFixed/AIFixed.ini' not in members
        descriptions={lang:(WORK/name/('description_'+lang+'.txt')).read_text('utf-8') for lang in ('en','ru')}
        sizes={lang:len(body.encode('utf-8')) for lang,body in descriptions.items()};assert max(sizes.values())<=7900
        cover=WORK/'images'/name/'000_LEOPARD_Cover.png'
        with Image.open(cover) as image:image.verify()
        assert (WORK/name/'screens').is_dir()
        rows.append({'package':name,'candidate_version':version,'archive':str(archive),'archive_bytes':archive.stat().st_size,
                     'archive_sha256':sha(archive),'dll_sha256':entry['dll_sha256'],'files':len(members),'steam_description_bytes':sizes,
                     'languages':['en_GB','ru_RU','zh_CN'],'status':'local artifact verified; no publication'})
    assert not (WORK/'nexus/AIFixed-1.zip').exists()
    assert len(list((WORK/'nexus').glob('AIFixed-*.zip')))==1
    env=dict(os.environ,KENSHI_RELEASE_INPUT_ROOT=str(INPUT),PYTHONIOENCODING='utf-8')
    result=subprocess.run([sys.executable,str(WORK/'validate_descriptions.py'),*names],env=env,stdout=subprocess.PIPE,text=True,encoding='utf-8',check=True)
    assert json.loads(result.stdout)['descriptions_checked']==37
    (PROOF/'description-validation-37.json').write_text(result.stdout,encoding='utf-8')
    source_files=[p for p in CANON.iterdir() if p.is_file() and p.suffix in {'.py','.json','.md'}]+[CANON/'desc/make_desc.py',CANON/'desc/qol-20261029.json']
    assert all(p.read_bytes()==(WORK/p.relative_to(CANON)).read_bytes() for p in source_files)
    target=next(r for r in rows if r['package']=='AIFixed')
    now=datetime.datetime.now(datetime.timezone.utc).isoformat()
    smoke_path=Path('D:/SteamLibrary/steamapps/common/Kenshi/_codex_work/aifixed-20261102/before-basusu-integration/startup-smoke.json')
    startup={'status':'pending user launch/scene and root startup-log review','gameplay_verified':False,'fps_memory_measured':False}
    if smoke_path.is_file():
        smoke=read(smoke_path)
        smoke_digest=sha(smoke_path)
        archived_smoke=PROOF/('aifixed-startup-'+smoke['candidateSha256'][:16]+'-'+smoke_digest[:16]+'.json')
        if archived_smoke.exists():assert sha(archived_smoke)==smoke_digest
        else:archived_smoke.write_bytes(smoke_path.read_bytes())
        if smoke['candidateSha256']==EXPECTED:
            ru_path=INSTALLED_AI.parent/'locale/ru_RU/LC_MESSAGES/ai_fixed.mo'
            with ru_path.open('rb') as stream:catalog=gettext.GNUTranslations(stream)._catalog
            startup.update(smoke_acceptance(smoke,sha(INSTALLED_AI),ru_path,sha(ru_path),catalog))
            startup.update({'evidence':str(archived_smoke),'source_evidence':str(smoke_path),
                            'evidence_sha256':smoke_digest,'evidence_utc':smoke['utc'],
                            'observed_process_id':smoke['processId'],'new_automatic_modules_gameplay_verified':False})
    startup['historical_candidate_smoke']=[{'candidate_sha256':read(p)['candidateSha256'],
        'evidence':str(p),'evidence_sha256':sha(p),'evidence_utc':read(p)['utc'],
        'scope':'Historical load/MCM/RU smoke of a different candidate; not current-candidate runtime proof'}
        for p in sorted(PROOF.glob('aifixed-startup-*.json')) if read(p).get('candidateSha256')!=EXPECTED]
    startup_ok=startup['status'].startswith('PASS')
    state='37 local Nexus-ready artifact candidates; AI startup/UI acceptance verified as recorded; gameplay unverified; not uploaded' if startup_ok else '37 local artifacts verified; AI startup acceptance pending; not uploaded'
    report={'verified_at_utc':now,'status':state,
            'prepared':37,'AIFixed_public_baseline':baseline,'AIFixed':target,'AIFixed_startup':startup,
            'existing_36_archives_unchanged':True,'published_ledger_unchanged':True,'older_history_preserved':True,
            'historical_game_preservation_proof':{'path':str(HERE/'release-preparation-proof-2026-11-02.json'),
                'verified_at_utc':prior['verified_at_utc'],'scope':'34 installed DLLs unchanged before authorized VS/AI installation; not rerun, weakened or claimed current here'},
            'canonical_workshop_sync_files':len(source_files),'packages':rows}
    write(HERE/'release-preparation-37-proof-2026-11-02.json',report)
    write(HERE/'aifixed-release-verification-2026-11-02.json',{'verified_at_utc':now,'baseline':baseline,'candidate':target,'acceptance':startup,
        'checks':'CRC/exact content and file hashes, canonical DLL, ten-file clean payload, source README/GPL/provenance, 18 factory settings/default-off modules/Debug last, exactly three MO catalogs, complete bilingual notes/history/credits/guides, covers/screens, all 36 earlier archives unchanged'})
    lines=['# 37 local plugin candidates, 2026-11-02','','37 local archive payloads and descriptions pass verification. Nothing was uploaded. '+(startup['summary']+' ' if startup_ok else 'AI Fixed 2 startup acceptance is pending. ')+'Automatic-module gameplay, food/job/path cooperation, FPS and long-term memory are unverified.','',
           'AI Fixed 2 adds the reviewed GateFix/SleepFix adaptations with independent settings, explicit Basusu thanks/original Steam/source links and GPL provenance. New modules are off by default. The public guides describe player behavior; the README and changelog retain the full reference.','',
           'The existing 36 candidate versions and ZIP hashes are unchanged. The publication ledger and every older history entry are unchanged. AI Fixed 1 was removed only by the unified packer after AI Fixed 2 passed CRC/composition/hash checks. The historical installed-DLL preservation proof remains its pre-install snapshot; this is a separate artifact verification after authorized runtime installation.','',
           '| Package | Candidate | ZIP bytes | ZIP SHA256 |','| --- | --- | --- | --- |']
    for row in rows:lines.append(f'| {row["package"]} | {row["candidate_version"]} | {row["archive_bytes"]} | `{row["archive_sha256"]}` |')
    lines+=['','Publication order remains user Nexus upload first, then Steam only after the supplied Nexus links. Source/offline AI review: 342 checks, native clock audit and 210-import audit passed; these are not gameplay/FPS proof.','']
    (HERE/'release-preparation-37-proof-2026-11-02.md').write_text('\n'.join(lines),encoding='utf-8')
    queue=read(HERE/'localization-release-queue.json')
    queue['status']=state
    queue['policy']='Prepare locally; user uploads Nexus first and supplies links, then corresponding Steam updates. AI Fixed 2 includes the integrated Basusu modules and log fix in one version increment.'
    queue['holds']={'AIFixed':'Version 2 prepared and offline-reviewed; '+('startup/UI acceptance passed as recorded; ' if startup_ok else 'startup smoke pending; ')+'automatic-module gameplay/FPS/memory unverified. Not published.'}
    write(HERE/'localization-release-queue.json',queue)
    write(HERE/'release-candidate-plan-37-2026-11-02.json',{'status':state,
        'input_root':str(INPUT),'packages':[{'package':r['package'],'candidate_version':r['candidate_version']} for r in rows],
        'AIFixed_startup':startup,'publication_order':'User Nexus first; Steam only after supplied links'})
    print('PASS: AI Fixed 2 and all 37 artifact payloads/descriptions; earlier 36 ZIPs/ledger/history unchanged; '+startup['status']+'; gameplay/FPS unverified')


if __name__=='__main__':main()
