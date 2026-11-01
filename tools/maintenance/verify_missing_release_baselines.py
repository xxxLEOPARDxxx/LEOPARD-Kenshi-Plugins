"""Read eight missing publication baselines from official public services only."""
from pathlib import Path
import datetime
import hashlib
import json
import re
import urllib.error
import urllib.parse
import urllib.request

HERE=Path(__file__).resolve().parent
WORK=Path('D:/DEV/Kenshi/workshop')
OUT=Path('D:/DEV/Kenshi/build/ReleaseBaselineAudit')
OUT.mkdir(parents=True,exist_ok=True)
inventory=json.loads((HERE/'release-inventory-2026-11-01.json').read_text('utf-8'))
missing=[r for r in inventory['packages'] if r['recorded_published_version'] is None]
items={r['package']:json.loads((WORK/r['package']/'item.json').read_text('utf-8')) for r in missing}
api='https://api.steampowered.com/ISteamRemoteStorage/GetPublishedFileDetails/v1/'
form={'itemcount':str(len(items))}
for i,item in enumerate(items.values()):form[f'publishedfileids[{i}]']=str(item['workshop_id'])
request=urllib.request.Request(api,data=urllib.parse.urlencode(form).encode('ascii'),
                               headers={'User-Agent':'Kenshi-local-release-audit/1.0'},method='POST')
response=None
api_error=None
try:
    with urllib.request.urlopen(request,timeout=30) as stream:
        raw=stream.read(2*1024*1024)
    (OUT/'steam-public-file-details.json').write_bytes(raw)
    response=json.loads(raw)
    raw_hash=hashlib.sha256(raw).hexdigest()
except Exception as exc:
    api_error=repr(exc)
    raw_hash=None
details={str(r.get('publishedfileid')):r for r in response.get('response',{}).get('publishedfiledetails',[])} if response else {}
rows=[]
for package,item in items.items():
    detail=details.get(str(item['workshop_id']),{})
    description=detail.get('description','')
    # The existing official item descriptions use an explicit version footer.
    # Multiple conflicting markers are retained for review rather than guessed.
    matches=re.findall(r'(?i)(?:\bVersion\s*[: ]*|\bВерсия\s*[: ]*)(\d+)\b',description)
    versions=sorted(set(matches),key=int)
    valid=detail.get('result')==1 and detail.get('consumer_app_id')==233860 and str(detail.get('creator'))=='76561198098571264'
    version=versions[0] if valid and len(versions)==1 else None
    row={'package':package,'checked_at_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
         'steam_url':f'https://steamcommunity.com/sharedfiles/filedetails/?id={item["workshop_id"]}',
         'steam_api_url':api,'nexus_url':f'https://www.nexusmods.com/kenshi/mods/{item["nexus"]}',
         'local_catalog_version':str(item['version']), 'steam_result':detail.get('result'),
         'consumer_app_id':detail.get('consumer_app_id'),'creator':detail.get('creator'),
         'title':detail.get('title'),'description_sha256':hashlib.sha256(description.encode('utf-8')).hexdigest(),
         'version_markers':versions,'verified_steam_description_version':version,
         'steam_time_updated':detail.get('time_updated'),'steam_file_size':detail.get('file_size'),
         'steam_content_manifest':detail.get('hcontent_file'),
         'version_evidence':[description[max(0,m.start()-25):min(len(description),m.end()+25)]
                             for m in re.finditer(r'(?i)(?:\bVersion\s*[: ]*|\bВерсия\s*[: ]*)(\d+)\b',description)],
         'status':'verified official Steam description version' if version else 'unverified; conditional local catalog only'}
    rows.append(row)
report={'read_only':True,'scope':'Eight packages missing from the local publication ledger. Official public Steam metadata only; no authentication, uploads, version edits or package changes.',
        'api_error':api_error,'raw_api_response':str(OUT/'steam-public-file-details.json') if response else None,
        'raw_api_response_sha256':raw_hash,'packages':rows}
(HERE/'public-baseline-verification-2026-11-01.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
lines=['# Missing release baseline verification, 2026-11-01','',
       'Read-only official Steam public metadata check. The explicit description version footer was read for all eight packages missing from the local ledger. Consumer app 233860 and creator 76561198098571264 were checked. No local publication metadata, packages, installations or remote content were changed. Nexus versions were not checked.','',
       '| Package | Steam description version | Checked UTC | Official item | Description SHA256 |',
       '| --- | --- | --- | --- | --- |']
for row in rows:
    lines.append(f'| {row["package"]} | {row["verified_steam_description_version"] or "UNVERIFIED"} | {row["checked_at_utc"]} | [Steam]({row["steam_url"]}) | `{row["description_sha256"]}` |')
lines+=['',f'Official API: [GetPublishedFileDetails]({api}). Raw response: `{report["raw_api_response"]}`; SHA256 `{raw_hash}`.',
        '', 'These version footers support the recorded release-number baseline; they do not verify the contents of every downloadable binary or establish Nexus synchronization. Preparing new packages still follows the user’s Nexus-first, Steam-after-links publication order.','']
(HERE/'public-baseline-verification-2026-11-01.md').write_text('\n'.join(lines),encoding='utf-8')
print(json.dumps({'api_error':api_error,'versions':[(r['package'],r['verified_steam_description_version'],r['version_markers'],r['title']) for r in rows]},ensure_ascii=False,indent=2))
