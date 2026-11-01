"""Read-only release inventory. Writes only this audit's JSON and Markdown."""
from pathlib import Path
import datetime
import gettext
import hashlib
import importlib.util
import json
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[2]
DEV = ROOT.parent
GAME = Path('D:/SteamLibrary/steamapps/common/Kenshi')
WORK = DEV / 'workshop'
CANON = ROOT / 'tools/workshop'
HERE = Path(__file__).resolve().parent
STEM = 'release-inventory-2026-11-01'

def read(path):
    return json.loads(path.read_text('utf-8-sig'))

def sha(data):
    return hashlib.sha256(data).hexdigest()

def file(path):
    if not path.is_file():
        return None
    return {'path': str(path), 'bytes': path.stat().st_size, 'sha256': sha(path.read_bytes())}

def readme(folder):
    if not folder.is_dir():
        return None
    candidates = sorted(p for p in folder.iterdir() if p.is_file() and p.name.lower() in {'readme.md', 'readme.txt'})
    return file(candidates[0]) if candidates else None

def source_root(path):
    for candidate in [path, *list(path.parents)[:3]]:
        if readme(candidate):
            return candidate
    return path

def locales(folder):
    catalogs = []
    if folder.is_dir():
        for p in sorted(folder.rglob('*.mo')):
            if 'locale' not in p.parts:
                continue
            parts = p.parts
            lang = parts[parts.index('locale') + 1]
            with p.open('rb') as stream:
                parsed = gettext.GNUTranslations(stream)
            catalogs.append({**file(p), 'language': lang, 'keys': len([k for k in parsed._catalog if k])})
    pos = list((folder/'locale').rglob('*.po')) if (folder/'locale').is_dir() else []
    po_languages = sorted(set(p.relative_to(folder/'locale').parts[0] for p in pos))
    mo_languages = sorted(set(c['language'] for c in catalogs))
    return {'catalogs': catalogs, 'languages': mo_languages, 'po_languages': po_languages,
            'all_languages': sorted(set(mo_languages+po_languages)), 'po_count': len(pos)}

queue = read(HERE/'localization-release-queue.json')
catalog = {x['package']: x for x in read(CANON/'mcm_catalog.json')}
build_records = read(HERE/'localization-builds.json')
baseline = read(WORK/'published_releases.json')
notes = read(CANON/'release_notes.json')
history = read(CANON/'changelog.json')
spec = importlib.util.spec_from_file_location('inventory_build_catalog', CANON/'build.py')
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)  # main build loop is guarded; no build function is called.
items = {item['mod']: item for item in builder.ITEMS}
aliases = {'Character-Highlight': 'CharacterHighlight', 'Character Inspector': 'CharacterInspector'}
extra = {
    'UITweaks': {'change': 'Save and Load previews; one-time higher opening anchor; hide slots without images; filtered lossless PNG up to 1600x900; legacy JPEG compatibility; next-open dialog height 120% (100..160).',
                 'commits': ['d9f6f9e', '805daf8', 'cc621bc', 'e8cf96e'],
                 'note_issue': 'README labels new preview/height work as version 12 although the local publication ledger already records 12. Prepare version 13 once after verifying the actual public version; update the full bilingual release note and player descriptions.'},
    'Container-Highlight': {'change': 'Retire stale model tint bindings after collection/restoration failure; preserve replacement materials; avoid repeated restoration faults.',
                            'commits': ['9101ad8'], 'note_issue': 'Pending README text is present; canonical release metadata/changelog still describes recorded version 2. Next recorded-baseline release would be 3.'},
    'Vital-Read': {'change': 'Preflight optional icon textures/aliases before native loading to avoid missing-texture diagnostic spam.', 'commits': ['594717a']},
    'Vital-Sense': {'change': 'Preflight optional icon textures/aliases and explain standard-icon fallback in settings.', 'commits': ['594717a', '75535ed']},
    'StackSort': {'change': 'Find optional Arrange buttons without noisy layout warnings, including equipment windows lacking the button.', 'commits': ['bec8201']},
    'ItemMarkers': {'change': 'Temporary stolen-badge suppression was reverted. Native stolen indication remains; rare ownership/stacking reports are unresolved and must not be described as fixed.', 'commits': ['6843b7a'], 'reverted_behavior': True},
    'Organize-the-Trader': {'change': 'German/unsupported-language fallback was audited: English fallback works; no production localization fix or new language was added.', 'commits': ['173e4bd'], 'test_only_extra': True},
}

rows = []
for package in queue['packages']:
    entry = catalog[package]
    src = Path(entry['source'])
    alias = aliases.get(package, package)
    record = next(x for x in build_records if x['name'] == alias)
    src_root = source_root(src)
    dll_name = alias + '.dll'
    build = file(DEV/'build'/alias/dll_name)
    resolved = builder.mod_dir(package)  # reads local directories only.
    active_root = Path(resolved) if resolved else None
    active = file(active_root/dll_name) if active_root else None
    stage = file(DEV/'stage'/package/dll_name)
    installed = file(GAME/'mods'/package/dll_name)
    content = WORK/package/'content'
    staged = file(content/dll_name)
    pub = baseline['mods'].get(package)
    baseline_version = str(pub['version']) if pub else str(items[package]['version'])
    archives = sorted((WORK/'nexus').glob(package+'-*.zip'))
    archive_records = []
    for archive in archives:
        with zipfile.ZipFile(archive) as z:
            names = [n for n in z.namelist() if not n.endswith('/')]
            dlls = [n for n in names if Path(n).name == dll_name]
            mos = [n for n in names if n.endswith('.mo') and '/locale/' in '/'+n]
            langs = sorted(set(n.replace('\\','/').split('locale/')[1].split('/')[0] for n in mos))
            po_languages = sorted(set(n.replace('\\','/').split('locale/')[1].split('/')[0]
                                     for n in names if n.endswith('.po') and 'locale/' in n))
            valid_mo = True
            for n in mos:
                import io
                try:
                    gettext.GNUTranslations(io.BytesIO(z.read(n)))
                except Exception:
                    valid_mo = False
            archive_records.append({**file(archive), 'version': archive.stem[len(package)+1:],
                'crc_ok': z.testzip() is None, 'files': len(names),
                'dll_sha256': sha(z.read(dlls[0])) if len(dlls)==1 else None,
                'dll_entries': dlls, 'mo_count': len(mos), 'languages': langs, 'mo_parse_ok': valid_mo,
                'po_languages': po_languages, 'all_languages': sorted(set(langs+po_languages)),
                'po_entries': [n for n in names if n.endswith('.po')],
                'readme_entries': [n for n in names if Path(n).name.lower() in {'readme.md','readme.txt'}],
                'default_ini_entries': [n for n in names if n.lower().endswith('.defaults.ini') or
                    ('/sourcedata/' in n.lower() and Path(n).name in {'00_Vanilla.ini','50_OfficialExtensions.ini','90_ModSupport.ini'})],
                'private_entries': [n for n in names if n.lower().endswith('.log') or '99_useroverride' in n.lower() or
                    (n.lower().endswith('.ini') and not n.lower().endswith('.defaults.ini') and not
                     ('/sourcedata/' in n.lower() and Path(n).name in {'00_Vanilla.ini','50_OfficialExtensions.ini','90_ModSupport.ini'}))],
                'readme_sha256': next((sha(z.read(n)) for n in names if Path(n).name.lower() in {'readme.md','readme.txt'}), None)})
    current = next((a for a in archive_records if a['version'] == baseline_version), None)
    active_loc = locales(active_root) if active_root else None
    source_readme = readme(src_root)
    content_readme = readme(content)
    item_path = WORK/package/'item.json'
    metadata = read(item_path) if item_path.is_file() else {}
    release = notes.get(package, notes.get('mods', {}).get(package, {}))
    en_note = '\n'.join(release.get('en', []))
    ru_note = '\n'.join(release.get('ru', []))
    behavior = extra.get(package)
    row = {'package': package, 'source': str(src), 'source_root': str(src_root), 'build_name': alias,
        'recorded_published_version': str(pub['version']) if pub else None,
        'archive_baseline_version': baseline_version,
        'archive_baseline_source': 'publication ledger' if pub else 'ITEMS catalog only; publication ledger entry missing',
        'next_if_baseline_confirmed': str(int(baseline_version)+1),
        'canonical_items_version': items[package]['version'], 'working_item_version': metadata.get('version'),
        'release_note_version': release.get('version'), 'release_note_published_marker': release.get('published_version'),
        'has_bilingual_existing_release_notes': bool(en_note and ru_note),
        'changelog_has_baseline_entry': bool(history.get(package, {}).get(baseline_version)),
        'new_release_notes_required': True, 'build': build, 'installed_mods': installed, 'dev_stage': stage,
        'active_local_root': str(active_root) if active_root else None, 'active_local_dll': active,
        'workshop_content_dll': staged,
        'build_matches_active_local': bool(build and active and build['sha256']==active['sha256']),
        'build_matches_published_zip': bool(build and current and build['sha256']==current['dll_sha256']),
        'published_zip_matches_workshop_content': bool(current and staged and current['dll_sha256']==staged['sha256']),
        'archives': archive_records, 'current_archive': current['path'] if current else None,
        'active_local_locales': active_loc, 'workshop_content_locales': locales(content),
        'source_readme': source_readme, 'active_readme': readme(active_root) if active_root else None,
        'content_readme': content_readme,
        'source_readme_matches_content': bool(source_readme and content_readme and source_readme['sha256']==content_readme['sha256']),
        'cover_directory': str(WORK/'images'/package),
        'cover_present': (WORK/'images'/package/'000_LEOPARD_Cover.png').is_file(),
        'workshop_preview': [str(p) for p in (WORK/package).glob('preview.*') if p.is_file()],
        'additional_scope': behavior,
        'status': 'local source/build ahead of old package; prepare a new release after actual public-version verification'}
    rows.append(row)

sync = []
for path in sorted(CANON.iterdir()):
    if path.is_file() and path.suffix.lower() in {'.py','.json','.ps1'}:
        other = WORK/path.name
        sync.append({'name': path.name, 'canonical': file(path), 'working': file(other),
                     'identical': other.is_file() and path.read_bytes()==other.read_bytes()})

summary = {
    'packages': len(rows),
    'packages_with_publication_ledger_entry': sum(r['recorded_published_version'] is not None for r in rows),
    'packages_missing_publication_ledger_entry': sum(r['recorded_published_version'] is None for r in rows),
    'archives': sum(len(r['archives']) for r in rows),
    'archives_crc_ok': sum(a['crc_ok'] for r in rows for a in r['archives']),
    'build_matches_active_local': sum(r['build_matches_active_local'] for r in rows),
    'build_differs_published_zip': sum(not r['build_matches_published_zip'] for r in rows),
    'published_zip_matches_workshop_content': sum(r['published_zip_matches_workshop_content'] for r in rows),
    'archives_exact_three_mo_no_po': sum(a['mo_count']==3 and a['languages']==['en_GB','ru_RU','zh_CN'] and not a['po_entries'] and a['mo_parse_ok'] for r in rows for a in r['archives']),
    'archives_po_only': sum(a['mo_count']==0 and bool(a['po_entries']) for r in rows for a in r['archives']),
    'archives_mixed_po_and_mo': sum(a['mo_count']>0 and bool(a['po_entries']) for r in rows for a in r['archives']),
    'archives_with_readme': sum(bool(a['readme_entries']) for r in rows for a in r['archives']),
    'archives_with_private_files': sum(bool(a['private_entries']) for r in rows for a in r['archives']),
    'source_readme_present': sum(bool(r['source_readme']) for r in rows),
    'source_readme_matches_content': sum(r['source_readme_matches_content'] for r in rows),
    'covers_present': sum(r['cover_present'] for r in rows),
    'workshop_previews_present': sum(bool(r['workshop_preview']) for r in rows),
    'active_local_directories_with_three_mo_languages': sum(bool(r['active_local_locales']) and r['active_local_locales']['languages']==['en_GB','ru_RU','zh_CN'] for r in rows),
    'active_local_directories_with_three_languages_mo_or_po': sum(bool(r['active_local_locales']) and r['active_local_locales']['all_languages']==['en_GB','ru_RU','zh_CN'] for r in rows),
    'archives_with_three_languages_mo_or_po': sum(a['all_languages']==['en_GB','ru_RU','zh_CN'] for r in rows for a in r['archives']),
    'packages_with_explicit_bilingual_release_notes_entry': sum(r['has_bilingual_existing_release_notes'] for r in rows),
    'packages_with_existing_baseline_changelog': sum(r['changelog_has_baseline_entry'] for r in rows),
    'stale_release_note_published_markers': sum(r['release_note_published_marker'] is not None and r['recorded_published_version'] is not None and str(r['release_note_published_marker'])!=str(r['recorded_published_version']) for r in rows),
    'canonical_workshop_files_checked': len(sync), 'canonical_workshop_files_identical': sum(s['identical'] for s in sync),
}
report = {'audited_at_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
    'scope': 'Local recorded baseline only. No external service verification, builds, packaging, synchronization, version edits, deletions or publication.',
    'baseline': {'path': str(WORK/'published_releases.json'), 'recorded_date': baseline.get('date'),
                 'warning': 'Verify actual published Nexus/Steam versions before preparation; this ledger is not proof of current external state.'},
    'policy': queue['policy'], 'excluded': queue['excluded'],
    'catalog_packages_outside_queue': sorted(set(items)-set(queue['packages'])),
    'repository_heads': {str(p): subprocess.check_output(['git','rev-parse','HEAD'],cwd=p,text=True).strip()
                         for p in [ROOT,DEV/'forks/LEOPARD-Kenshi-Forks']},
    'summary': summary, 'packages': rows, 'workshop_sync': sync}
(HERE/(STEM+'.json')).write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

lines = ['# Local plugin release inventory, 2026-11-01', '',
    'Read-only audit of the 37-package localization-log queue. Publication versions below come from the local ledger dated '+str(baseline.get('date'))+'; no external publication state was checked. Verify actual versions before preparing new releases. No packages or metadata were changed.', '',
    '## Counts', '', *[f'- {key}: {value}' for key,value in summary.items()], '',
    'All queued packages need a newly numbered release and fresh bilingual notes for the detailed-log change. Existing archives are the old release baseline, not the new maintenance update. The next numbers below are provisional until actual public versions are verified. Nexus is uploaded by the user first; Steam follows the supplied Nexus links.', '',
    '## Additional changes and note problems', '']
for package, info in extra.items():
    lines.append(f'- **{package}:** {info["change"]} Evidence: '+', '.join(info['commits'])+'. '+info.get('note_issue','Existing metadata must be updated for this scope before the next release.'))
lines += ['', '## Per-package status', '',
    '| Package | Recorded → next | Build vs active local | Build vs old ZIP | ZIP locale form | ZIP README |',
    '| --- | --- | --- | --- | --- | --- |']
for r in rows:
    good = all(a['crc_ok'] and a['mo_count']==3 and not a['po_entries'] and a['mo_parse_ok'] for a in r['archives'])
    recorded = r['recorded_published_version'] or ('unrecorded; catalog '+r['archive_baseline_version'])
    a=r['archives'][0]
    locale_form='3 MO' if good else ('PO only' if not a['mo_count'] else 'PO + MO')
    lines.append(f'| {r["package"]} | {recorded} → {r["next_if_baseline_confirmed"]}? | '+('same' if r['build_matches_active_local'] else '**different/missing**')+' | '+('same' if r['build_matches_published_zip'] else 'different')+' | '+locale_form+' | '+('yes' if a['readme_entries'] else '**missing**')+' |')
lines += ['', '## Outliers and synchronization', '']
for r in rows:
    if r['recorded_published_version'] is None:
        lines.append(f'- **{r["package"]}:** missing from the publication ledger. Catalog/archive version {r["archive_baseline_version"]} is only a local fallback, not verified publication state.')
    if not r['build_matches_active_local']:
        lines.append(f'- **{r["package"]}:** build differs from its currently resolved local DLL. See JSON hashes and paths; do not assume the new candidate is installed.')
    if not r['published_zip_matches_workshop_content']:
        lines.append(f'- **{r["package"]}:** old ZIP and Workshop content DLL differ or are missing; inspect before preparation.')
    if r['release_note_published_marker'] is not None and r['recorded_published_version'] is not None and str(r['release_note_published_marker'])!=str(r['recorded_published_version']):
        lines.append(f'- **{r["package"]}:** release_notes published_version marker {r["release_note_published_marker"]} is behind ledger {r["recorded_published_version"]}; refresh it during the next release preparation.')
    for a in r['archives']:
        if a['private_entries']:
            lines.append(f'- **{r["package"]}:** archive private-file candidates: '+', '.join(a['private_entries']))
for s in sync:
    if not s['identical']:
        lines.append(f'- Canonical/working Workshop mismatch or missing working copy: `{s["name"]}`.')
lines += ['', 'The JSON contains exact SHA256 identities, source/installed/stage paths, MO languages and sizes, archive contents, README and cover status for each package. A DLL hash difference establishes different files, not a semantic code comparison or public-site update. Private DynamicFactions, GlobalSatelliteMap, Organize-the-Inventory and the noncanonical XP_Overhaul build remain excluded.', '']
lines += ['All 37 current local directories and old archives contain all three languages through MO or PO; the issue is release format/refresh, not evidence that translations are missing. Default INI templates and the official MultiXPMining SourceData INIs are allowed data, not personal settings. The source release_notes file has explicit entries for only 17 queued packages; existing changelog history is checked separately and must be preserved. Formations and Dark UI Advanced are outside this 37-package logging queue.', '']
(HERE/(STEM+'.md')).write_text('\n'.join(lines),encoding='utf-8')
print(json.dumps(summary,indent=2))
