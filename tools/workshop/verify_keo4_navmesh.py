"""Read-only KEO4 release verification; never publishes or launches the game."""
from pathlib import Path
import gettext,hashlib,importlib.util,io,json,re,zipfile
HERE=Path(__file__).resolve().parent
ROOT=Path('D:/DEV/Kenshi/forks/LEOPARD-Kenshi-Forks/KEO')
WORK=Path('D:/DEV/Kenshi/workshop')
GAME=Path('D:/SteamLibrary/steamapps/common/Kenshi')
STAGE=ROOT/'build/auto-nav-release'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
archive=WORK/'nexus/KEO-4.zip'
spec=importlib.util.spec_from_file_location('pe',ROOT/'tools/kenshilib/check_imports.py')
pe=importlib.util.module_from_spec(spec);spec.loader.exec_module(pe)
with zipfile.ZipFile(archive) as z:
 assert z.testzip() is None
 names=z.namelist();assert len(names)==len(set(names))==9
 assert set(names)=={'KEO/'+n for n in ['KEO.dll','KEO.mod','LICENSE','README.md','RE_Kenshi.json','KEO.defaults.ini']+[f'locale/{lang}/LC_MESSAGES/keo.mo' for lang in ('en_GB','ru_RU','zh_CN')]}
 assert json.loads(z.read('KEO/RE_Kenshi.json'))=={'Plugins':['KEO.dll']}
 readme=z.read('KEO/README.md').decode('utf-8');assert ' - release 4' in readme and '## Release 4 / Выпуск 4' in readme
 assert 'Pathing repairs itself' in readme and 'native building/furniture repair' in readme
 assert 'Unreleased:' not in readme and 'Does not repair furniture' not in readme
 for n in names:
  data=z.read(n);relative=n[4:]
  assert data==(WORK/'KEO/content'/relative).read_bytes(),n
  if relative=='KEO.defaults.ini':assert data==(ROOT/'KEO.ini').read_bytes()
  elif relative=='RE_Kenshi.json':assert data==(ROOT/'RE_Kenshi.json').read_bytes()
  elif relative=='README.md':assert data==(ROOT/'README.md').read_bytes()
  elif relative.endswith('.mo'):
   lang=relative.split('/')[1];t=gettext.GNUTranslations(io.BytesIO(data))
   assert t.gettext('Pathing repairs itself')!='Pathing repairs itself' or lang=='en_GB'
   assert len([k for k in t._catalog if k])==157
   if lang=='ru_RU':assert all(not re.search('[\u2010-\u2015\u2212]',v) for k,v in t._catalog.items() if k and isinstance(v,str))
 ini=z.read('KEO/KEO.defaults.ini').decode('utf-8')
 assert '# autoNavmeshRepair=false' in ini and ini.rstrip().endswith('# Debug=false')
 binary=z.read('KEO/KEO.dll');assert binary==(ROOT/'build/KEO_step4_prod/KEO.dll').read_bytes()==(GAME/'mods/KEO/KEO.dll').read_bytes()
image=pe.PE(ROOT/'build/KEO_step4_prod/KEO.dll')
assert image.machine==0x8664 and {'?startPlugin@@YAXXZ','MCM_Describe'}<=image.exports()
imports={symbol for dll,symbol in image.imports() if dll.lower()=='kenshilib.dll'}
assert not imports-pe.PE(GAME/'KenshiLib.dll').exports()
for snapshot in ('protected-before.json','other-archives-before.json'):
 for name,digest in json.loads((STAGE/snapshot).read_text()).items():assert sha(Path(name))==digest,name
published=json.loads((WORK/'published_releases.json').read_text())['mods']['KEO'];assert published['version']=='3'
notes=json.loads((WORK/'release_notes.json').read_text())['KEO'];assert notes['version']=='4' and notes['published_version']=='3'
history=json.loads((WORK/'changelog.json').read_text())['KEO'];assert set(history)>={'1','2','3','4'} and 'English' in history['4'] and 'Русский' in history['4']
assert json.loads((WORK/'KEO/item.json').read_text())['version']=='4'
for lang in ('en','ru'):
 text=(WORK/f'KEO/description_{lang}.txt').read_text(encoding='utf-8')
 assert len(text.encode())<=7900 and '\ufffd' not in text
 assert not re.search(r'CHANGELOG|ЧТО ИЗМЕНЕНО В ВЕРСИИ|Version history',text,re.I)
 assert 'Ctrl+Shift+F11' in text and 'KenshiQOL' in text
 assert not re.search(r'github\.com/xxxLEOPARDxxx',text,re.I)
result={'status':'prepared, installed locally, not published','mod':'KEO','version':'4','archive':str(archive),'archive_sha256':sha(archive),'archive_bytes':archive.stat().st_size,'files':names,'dll_sha256':hashlib.sha256(binary).hexdigest(),'kenshilib_imports':len(imports),'description_bytes':{lang:(WORK/f'KEO/description_{lang}.txt').stat().st_size for lang in ('en','ru')},'game_performance':'not measured; UI check stopped by user; scheduling-only fake-boundary benchmark is separate','published_registry_version':'3'}
(STAGE/'verified-release.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(result,ensure_ascii=False,indent=2))
