"""Pinned private diagnostic; preserve all personal files and active mod data."""
from pathlib import Path
import hashlib,json,subprocess,zipfile,shutil,sys
from datetime import datetime,timezone,timedelta
sys.stdout.reconfigure(encoding='utf-8')
R=Path('D:/DEV/Kenshi/forks/KEO');G=Path('D:/SteamLibrary/steamapps/common/Kenshi')
O=Path('D:/DEV/Kenshi/workshop/testing/KEO');H=Path(__file__).parent
sha=lambda b:hashlib.sha256(b).hexdigest()
commit=subprocess.check_output(['git','-C',str(R),'rev-parse','HEAD'],text=True).strip()
assert commit=='3c54bd6566d0dab95f3ebde04a75655a1a89d24c'
data=(R/'build/KEO_step4_prod/KEO.dll').read_bytes()
assert sha(data)=='d2ec35317bd913c76f5df67a968f5c7cc0b760630eda989ac0dc7cecb764e665'
sys.path.insert(0,'D:/DEV/Kenshi/tools');from qol_binary_check import PE
pe=PE(R/'build/KEO_step4_prod/KEO.dll');assert {'?startPlugin@@YAXXZ','MCM_Describe'}<=pe.exports()
imports={}
for n,s in pe.imports().items():
 if n.lower()in('kenshilib.dll','myguiengine_x64.dll','ogremain_x64.dll','ois_x64.dll'):
  assert set(s)<=PE(G/n).exports(),n
  imports[n]=len(s)
base=O/'KEO-path-test-20261105.zip';assert sha(base.read_bytes())=='9a32dbd9955297c8e24d9b34875b29fb62837c2c69de0fa66abb1c19d8b324f7'
with zipfile.ZipFile(base)as z:
 assert z.testzip()is None
 files={n:z.read(n)for n in z.namelist()}
files['KEO/KEO.dll']=data
files['KEO/RELOAD_DIAGNOSTIC.md']=(R/'leopard/squin-reload-diagnostic-20261105.md').read_bytes()
files['TEST_README_RU_EN.txt']='''KEO - закрытая диагностика повторной загрузки города, 05.11.2026.
Поведение загрузки/спавна не изменено. Подробный журнал KEO должен быть включён.
Один непрерывный прогон: из главного меню импортировать LEO4 -> первый вход в Скуин -> Хаб до выгрузки Скуина -> вернуться в Скуин -> при появлении толпы поставить паузу и оставить игру открытой.
Не запускать контроль без KEO до копирования ON-журнала. Нужны mods/KEO/KEO.log и RE_Kenshi_log.txt.
Активный LoK оставлен прежним для чистоты этой проверки. Личный INI не заменять шаблоном.
Это не публичный релиз и не готовое исправление спавна. GOG проверка игроком ещё ожидается.

KEO private city-reload diagnostic, 05 Nov 2026. Loading/spawn behavior unchanged.
Enable detailed KEO logging. Import LEO4 from main menu -> Squin -> Hub until Squin unloads -> Squin again -> pause at crowd and leave the process open.
Copy the ON logs before running an OFF control. Keep personal INI. Not a public release or a proven spawn fix.
'''.encode('utf-8')
assert not any(n.lower().endswith(('.po','.log','.exe'))or'..'in Path(n).parts for n in files)
assert'KEO/KEO.ini'not in files
archive=O/'KEO-reload-diagnostic-20261105.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=9)as z:
 for n,b in files.items():z.writestr(n,b)
with zipfile.ZipFile(archive)as z:
 assert z.testzip()is None and set(z.namelist())==set(files)
 assert all(z.read(n)==b for n,b in files.items())
assert subprocess.check_output(['powershell','-NoProfile','-Command','@(Get-Process Kenshi* -ErrorAction SilentlyContinue).Count'],text=True).strip()=='0','Game is running; nothing installed'
personal=[G/'mods/KEO'/n for n in('KEO.ini','KEOCompat.ini','RE_Kenshi.json')]+[G/'data/mods.cfg']
before={str(p):p.read_bytes()for p in personal if p.exists()}
now=datetime.now(timezone(timedelta(hours=3)))
backup=Path('D:/DEV/Kenshi/backup')/('keo-reload-diag-'+now.strftime('%Y%m%d-%H%M%S'));backup.mkdir(parents=True,exist_ok=False)
target=G/'mods/KEO/KEO.dll';shutil.copy2(target,backup/'KEO.dll');target.write_bytes(data)
assert sha(target.read_bytes())==sha(data)and all(Path(n).read_bytes()==b for n,b in before.items())
receipt=dict(time_msk=now.isoformat(),source_commit=commit,status='installed_private_diagnostic_not_published',dll_sha256=sha(data),
 archive=str(archive),archive_sha256=sha(archive.read_bytes()),archive_bytes=archive.stat().st_size,files=len(files),crc='passed',imports=imports,
 backup=str(backup),preserved_sha256={n:sha(b)for n,b in before.items()},behavior_changes='none; diagnostics only',
 lok_status='prepared separately; active files held unchanged for controlled KEO reproduction',runtime_acceptance='pending user import/out/back reproduction')
for p in(H/'reload-diagnostic-20261105.json',O/'KEO-reload-diagnostic-20261105.json',backup/'installation.json'):
 p.write_text(json.dumps(receipt,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(receipt,ensure_ascii=False,indent=2))
