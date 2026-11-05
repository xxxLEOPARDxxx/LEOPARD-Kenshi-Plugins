"""Install pinned private KEO fix, undo control, then verified local LoK sync."""
from pathlib import Path
import hashlib,json,subprocess,zipfile,shutil,sys
from datetime import datetime,timezone,timedelta
sys.stdout.reconfigure(encoding='utf-8')
R=Path('D:/DEV/Kenshi/forks/KEO');G=Path('D:/SteamLibrary/steamapps/common/Kenshi')
D=G/'_codex_work/20261105-squin-second-load';O=Path('D:/DEV/Kenshi/workshop/testing/KEO')
H=Path(__file__).parent;sha=lambda b:hashlib.sha256(b).hexdigest()
commit=subprocess.check_output(['git','-C',str(R),'rev-parse','HEAD'],text=True).strip()
assert commit=='7609b76e125ab0709a804325fc7e631064875eeb'
data=(R/'build/KEO_step4_prod/KEO.dll').read_bytes()
assert sha(data)=='ea56647bb504a5287ee2589b815d9b1ac1cef6e760d6921048420908ede86751'
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
files['KEO/PATH_TEST_NOTES.md']=(R/'leopard/native-readiness-fix-20261105.md').read_bytes()
files['TEST_README_RU_EN.txt']='''KEO: закрытая тестовая сборка 7609b76, 05.11.2026.
Исправлено преждевременное завершение загрузки города до готовности навмеша. Фоновая генерация, предзагрузка, кэши и приоритеты остаются.
Повторить из главного меню: импорт LEO4 -> Скуин -> Хаб до выгрузки Сквина -> Скуин. Жители должны остаться внутри города.
Личный INI не заменять шаблоном. Временный контроль readinessOverrides=false после установки больше не требуется: текущий код всегда сохраняет штатный ответ.
Это не публичный релиз. Качество маршрутов, три производственных задания и новая DLL на GOG требуют отдельной игровой проверки.

KEO private test 7609b76, 05 Nov 2026. City loading now preserves native navmesh readiness. Background workers, preload, caches and priorities remain.
Repeat fresh LEO4 import -> Squin -> Hub until unload -> Squin. Keep personal INI. The temporary readiness control is no longer necessary.
Not a public release. Broad route quality, production jobs and this exact DLL on GOG need separate acceptance.
'''.encode('utf-8')
assert'KEO/KEO.ini'not in files and not any(n.lower().endswith(('.exe','.po','.log'))or'..'in Path(n).parts for n in files)
archive=O/'KEO-native-readiness-test-20261105.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=9)as z:
 for n,b in files.items():z.writestr(n,b)
with zipfile.ZipFile(archive)as z:
 assert z.testzip()is None and set(z.namelist())==set(files)
 assert all(z.read(n)==b for n,b in files.items())
assert subprocess.check_output(['powershell','-NoProfile','-Command','@(Get-Process Kenshi* -ErrorAction SilentlyContinue).Count'],text=True).strip()=='0','Game running; no installation'
control=json.loads((D/'native-ready-control.json').read_text(encoding='utf-8'))
ini=G/'mods/KEO/KEO.ini';assert sha(ini.read_bytes())==control['control_sha256']
original=(D/'KEO.ini.before-native-ready-control').read_bytes();assert sha(original)==control['before_sha256']
personal=[G/'mods/KEO'/n for n in('KEOCompat.ini','RE_Kenshi.json')]+[G/'data/mods.cfg']
before={str(p):p.read_bytes()for p in personal}
now=datetime.now(timezone(timedelta(hours=3)))
backup=Path('D:/DEV/Kenshi/backup')/('keo-native-ready-'+now.strftime('%Y%m%d-%H%M%S'));backup.mkdir(parents=True,exist_ok=False)
target=G/'mods/KEO/KEO.dll';shutil.copy2(target,backup/'KEO.dll');shutil.copy2(ini,backup/'KEO.ini.control')
target.write_bytes(data);assert sha(target.read_bytes())==sha(data)
subprocess.run([sys.executable,str(D/'native_ready_control.py'),'restore'],check=True)
assert ini.read_bytes()==original and all(Path(n).read_bytes()==b for n,b in before.items())
receipt=dict(time_msk=now.isoformat(),source_commit=commit,status='installed_local_private_not_published',dll_sha256=sha(data),
 archive=str(archive),archive_sha256=sha(archive.read_bytes()),archive_bytes=archive.stat().st_size,files=len(files),crc='passed',imports=imports,
 backup=str(backup),original_ini_restored_sha256=sha(original),other_personal_sha256={n:sha(b)for n,b in before.items()},
 runtime_acceptance='control succeeded; exact new DLL pending user reproduction',gog_acceptance='user reports previous isolated GOG test had no errors/crashes; new DLL not yet tested there')
for p in(H/'native-readiness-installation-20261105.json',O/'KEO-native-readiness-test-20261105.json',backup/'installation.json'):
 p.write_text(json.dumps(receipt,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(receipt,ensure_ascii=False,indent=2))
subprocess.run(['pwsh','-NoProfile','-File',str(G/'_codex_work/20261105-lok-update/install-prepared-lok.ps1'),'-Install'],check=True)
