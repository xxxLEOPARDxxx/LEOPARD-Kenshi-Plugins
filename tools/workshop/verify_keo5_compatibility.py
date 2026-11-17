"""Verify the prepared KEO5 package; no installation or publication."""
from pathlib import Path
import gettext, hashlib, importlib.util, io, json, re, zipfile

WORK=Path('D:/DEV/Kenshi/workshop')
ROOT=Path('D:/DEV/Kenshi/forks/LEOPARD-Kenshi-Forks/KEO')
GAME=Path('D:/SteamLibrary/steamapps/common/Kenshi')
STAGE=ROOT/'build/shared-menu-release'
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def load(p): return json.loads(p.read_text(encoding='utf-8-sig'))
archive=WORK/'nexus/KEO-5.zip'
expected={'KEO/'+n for n in ['KEO.dll','KEO.mod','LICENSE','README.md','RE_Kenshi.json','KEO.defaults.ini']+[f'locale/{lang}/LC_MESSAGES/keo.mo' for lang in ['en_GB','ru_RU','zh_CN']]}
with zipfile.ZipFile(archive) as z:
 assert z.testzip() is None
 assert len(z.namelist())==len(set(z.namelist()))==9 and set(z.namelist())==expected
 assert json.loads(z.read('KEO/RE_Kenshi.json'))=={'Plugins':['KEO.dll']}
 binary=z.read('KEO/KEO.dll')
 assert hashlib.sha256(binary).hexdigest()=='94a9593f34bfaafcebbb8e337a3d932076766ef7b9f93828d4e860e0b9e261c0'
 assert binary==(ROOT/'build/KEO_step4_prod/KEO.dll').read_bytes()
 for n in z.namelist():
  relative=n[4:];data=z.read(n)
  assert data==(WORK/'KEO/content'/relative).read_bytes(),n
  if relative=='KEO.defaults.ini':assert data==(ROOT/'KEO.ini').read_bytes()
  elif relative in ['README.md','RE_Kenshi.json','LICENSE']:assert data==(ROOT/relative).read_bytes()
  else:assert data==(STAGE/'inputs/KEO'/relative).read_bytes()
 for lang in ['en_GB','ru_RU','zh_CN']:
  p=f'locale/{lang}/LC_MESSAGES/keo.mo';data=z.read('KEO/'+p)
  assert data==(GAME/'mods/KEO'/p).read_bytes(),'Unchanged translations must match published4'
  t=gettext.GNUTranslations(io.BytesIO(data));assert len([k for k in t._catalog if k])==157
  if lang=='ru_RU':assert all(not re.search('[\u2010-\u2015\u2212]',v)for k,v in t._catalog.items()if k and isinstance(v,str))
 readme=z.read('KEO/README.md').decode('utf8');assert readme.splitlines()[0]=='# KEO (MCM fork) (QoL) - release 5'
 assert all(f'## Release {n} / Выпуск {n}'in readme for n in [3,4,5])
 assert '## Unreleased'not in readme and 'не опубликовано'not in readme
 assert 'KenshiLua' in readme and 'Animal Husbandry'in readme
 ini=z.read('KEO/KEO.defaults.ini').decode('utf8');assert ini.rstrip().endswith('# Debug=false')
 assert '# autoNavmeshRepair=false'in ini
for snapshot in ['protected-before.json','other-archives-before.json']:
 for name,digest in load(STAGE/snapshot).items():assert sha(Path(name))==digest,name
before=load(STAGE/'metadata-before.json')
for name,old in before.items():
 new=load(WORK/name)
 assert {k:v for k,v in new.items()if k!='KEO'}=={k:v for k,v in old.items()if k!='KEO'},name
notes=load(WORK/'release_notes.json')['KEO'];assert notes['version']=='5' and notes['published_version']=='4'
assert load(WORK/'published_releases.json')['mods']['KEO']['version']=='4'
assert load(WORK/'KEO/item.json')['version']=='5'
assert set(load(WORK/'changelog.json')['KEO'])>={'1','2','3','4','5'}
for lang in ['en','ru']:
 text=(WORK/f'KEO/description_{lang}.txt').read_text(encoding='utf8')
 assert len(text.encode())<=7900 and '\ufffd' not in text
 assert 'KenshiLua' in text and 'Animal Husbandry'in text
 assert not re.search('CHANGELOG|ЧТО ИЗМЕНЕНО В ВЕРСИИ|Version history|github.com/xxxLEOPARDxxx',text,re.I)
 assert 'Исходники' in text if lang=='ru' else 'source'in text.lower()
assert (WORK/'images/KEO').is_dir()
assert not (WORK/'nexus/KEO-4.zip').exists(),'Retire superseded binary archive only after checked new ZIP'
spec=importlib.util.spec_from_file_location('pe',ROOT/'tools/kenshilib/check_imports.py')
pe=importlib.util.module_from_spec(spec);spec.loader.exec_module(pe)
image=pe.PE(ROOT/'build/KEO_step4_prod/KEO.dll');assert image.machine==0x8664
assert {'?startPlugin@@YAXXZ','MCM_Describe'}<=image.exports()
imports={s for d,s in image.imports()if d.lower()=='kenshilib.dll'}
assert len(imports)==183 and not imports-pe.PE(GAME/'KenshiLib.dll').exports()
result={'mod':'KEO','version':'5','status':'prepared, not installed, not published',
 'archive':str(archive),'archive_sha256':sha(archive),'archive_bytes':archive.stat().st_size,
 'files':sorted(expected),'dll_sha256':hashlib.sha256(binary).hexdigest(),
 'code_commit':'0d4decc','change_date_msk':'2026-11-18','packaging_date_msk':'2026-11-18',
 'changes':{lang:notes[lang]for lang in ['en','ru']},'description_bytes':{lang:(WORK/f'KEO/description_{lang}.txt').stat().st_size for lang in ['en','ru']},
 'verification':'CRC and exact9-file payload;3 unchanged157-key MO locales; bilingual descriptions/history; protected installation/settings/publication ledger and other archives unchanged; DEV/PROD builds and183 SDK imports; independent code review;148 admission checks Od/O2;206 factory and321 runtime controls Od/O2',
 'limits':'Conflict reproduced offline from source-proven hook timing; no new player diagnostic runs or gameplay test; GOG uses frozen contract snapshot.',
 'publication':'Await owner Nexus publication; Steam not submitted; no personal published-changelog entry.'}
(STAGE/'verified-release.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
print(json.dumps(result,ensure_ascii=False,indent=2))
