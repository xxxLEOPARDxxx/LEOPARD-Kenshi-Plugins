"""Prepare and optionally install the private KEO jobs candidate. Never publishes."""
from pathlib import Path
import argparse,hashlib,json,subprocess,sys,zipfile,os,shutil
from datetime import datetime,timezone,timedelta
sys.stdout.reconfigure(encoding='utf-8')
R=Path('D:/DEV/Kenshi/forks/KEO');GAME=Path('D:/SteamLibrary/steamapps/common/Kenshi')
OUT=Path('D:/DEV/Kenshi/workshop/testing/KEO');HERE=Path(__file__).resolve().parent
SHA=lambda b:hashlib.sha256(b).hexdigest()
def main():
 p=argparse.ArgumentParser();p.add_argument('--install',action='store_true');a=p.parse_args()
 commit=subprocess.check_output(['git','-C',str(R),'rev-parse','HEAD'],text=True).strip()
 assert commit=='c6802eaa4457f6d29942b6f60642213625be01d9',commit
 data=(R/'build/KEO_step4_prod/KEO.dll').read_bytes()
 assert SHA(data)=='e3f64a0bc0719d3038da592f53bbde68340bc35566ce892fdc0651e59e6a6243'
 sys.path.insert(0,'D:/DEV/Kenshi/tools');from qol_binary_check import PE
 pe=PE(R/'build/KEO_step4_prod/KEO.dll');assert {'?startPlugin@@YAXXZ','MCM_Describe'}<=pe.exports()
 imports={}
 for name,syms in pe.imports().items():
  if name.lower() in ('kenshilib.dll','myguiengine_x64.dll','ogremain_x64.dll','ois_x64.dll'):
   assert set(syms)<=PE(GAME/name).exports(),name
   imports[name]=len(syms)
 with zipfile.ZipFile(OUT/'KEO-weight-test-20261105.zip')as z:
  assert z.testzip()is None
  files={n:z.read(n)for n in z.namelist()if n!='TEST_README_RU_EN.txt'}
 files['KEO/KEO.dll']=data
 files['KEO/README.md']=(R/'README.md').read_bytes()
 files['KEO/KEO.ini.template']=(R/'build/KEO_step4_prod/KEO.ini.template').read_bytes()
 files['KEO/ORIGIN_Steel_jobs.md']=(R/'leopard/steel_jobs/ORIGIN.md').read_bytes()
 for lang in ('en_GB','ru_RU','zh_CN'):
  n=f'locale/{lang}/LC_MESSAGES/keo.mo';files['KEO/'+n]=(R/'build/KEO_step4_prod'/n).read_bytes()
 files['TEST_README_RU_EN.txt']=('''KEO: закрытый тест заданий, 05.11.2026. Не публичный выпуск.
Включайте функции по отдельности в MCM -> KEO -> Рюкзаки и работа:
1. Сбор продукции машин: ПКМ по производственной машине, SHIFT добавляет постоянное задание.
2. Пополнение торговых прилавков: только свой прилавок. Переносит предметы, не продаёт их за деньги.
3. Циклический маршрут: SHIFT + ПКМ по земле добавляет точки. Перестановка/удаление - в штатных задачах.
Все три опции по умолчанию выключены и включаются без перезапуска. OFF сохраняет очередь.
Первую проверку проводите в отдельном тестовом слоте: сбор -> прилавок -> сохранить -> загрузить.
Не включайте оригинальный KenshiQOL одновременно. Свою KEO.ini не заменяйте шаблоном.
Код собран и проверен вне игры; игровой цикл, FPS/память и сохранение ещё требуют проверки.
Исправление ошибки SDK 0xf1880 также включено. Для отдельной проверки запуска GOG используйте KEO-GOG-test-20261105.zip: там пока нет трёх новых заданий.
Если возникла ошибка, пришлите RE_Kenshi_log.txt и действия для повторения.

KEO private jobs test, 05 Nov 2026. Not a public release.
Enable Gather Output, Stock Counter and Cyclic waypoints independently under MCM -> KEO -> Backpacks and jobs.
Use right-click machine/counter commands; SHIFT creates recurring jobs. SHIFT + right-click on ground appends route points.
Stocking moves physical items into an owned counter; it makes no monetary sale. OFF retains queued rows.
All three default OFF and change live. Use a separate save for collection -> delivery -> save/load acceptance.
Do not run original KenshiQOL simultaneously or replace your personal KEO.ini with the template.
Host/native contract checks passed; gameplay, FPS/memory and persistence acceptance remain pending.
The 0xf1880 SDK fix is included. The separate KEO-GOG-test-20261105.zip remains a base-startup test without these jobs.
''').encode('utf-8')
 assert 'KEO/KEO.ini'not in files
 assert not any(n.lower().endswith(('.po','.exe','.log'))or'..'in Path(n).parts for n in files)
 path=OUT/'KEO-jobs-test-20261105.zip';tmp=path.with_suffix('.zip.partial')
 with zipfile.ZipFile(tmp,'w',zipfile.ZIP_DEFLATED,compresslevel=9)as z:
  for n,b in files.items():z.writestr(n,b)
 with zipfile.ZipFile(tmp)as z:
  assert z.testzip()is None and set(z.namelist())==set(files)
  assert all(z.read(n)==b for n,b in files.items())
 os.replace(tmp,path)
 now=datetime.now(timezone(timedelta(hours=3)))
 receipt={'time_msk':now.isoformat(),'source_commit':commit,'status':'prepared_private_not_published',
  'dll_sha256':SHA(data),'archive':str(path),'archive_bytes':path.stat().st_size,'archive_sha256':SHA(path.read_bytes()),'files':len(files),'imports':imports,
  'changes_ru':['Три отдельные живые опции: забор продукции, пополнение своего прилавка без денежной сделки, циклический маршрут по точкам.','Выключение сохраняет очередь; маршрут хранится в обычном сохранении.','Исправлен некорректный вызов SDK проверки веса (адрес 0xf1880).','Регистрация заданий совместима с проверенными обработчиками Stack Sort, Rotate Items и Kenshi Extension Plugin.'],
  'runtime_acceptance':'pending; no game FPS/leak/save-load claim','crc':'passed'}
 if a.install:
  count=subprocess.check_output(['powershell','-NoProfile','-Command','@(Get-Process kenshi_x64 -ErrorAction SilentlyContinue).Count'],text=True).strip()
  assert count=='0','Kenshi is running; no installation attempted'
  preserved=[GAME/'mods/KEO/KEO.ini',GAME/'mods/KEO/KEOCompat.ini',GAME/'mods/KEO/RE_Kenshi.json',GAME/'data/mods.cfg']
  before={str(x):x.read_bytes()for x in preserved if x.exists()}
  target=GAME/'mods/KEO';backup=Path('D:/DEV/Kenshi/backup')/('keo-jobs-'+now.strftime('%Y%m%d-%H%M%S'));backup.mkdir(parents=True,exist_ok=False)
  install={'KEO.dll':data,'README.md':files['KEO/README.md'],'ORIGIN_Steel_jobs.md':files['KEO/ORIGIN_Steel_jobs.md']}
  for lang in ('en_GB','ru_RU','zh_CN'):
   n=f'locale/{lang}/LC_MESSAGES/keo.mo';install[n]=files['KEO/'+n]
  for n,b in install.items():
   dest=target/n
   if dest.exists():
    old=backup/n;old.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(dest,old)
   dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(b);assert dest.read_bytes()==b
  assert all(Path(n).read_bytes()==b for n,b in before.items())
  receipt.update(status='installed_local_private_not_published',backup=str(backup),preserved_sha256={n:SHA(b)for n,b in before.items()})
  (backup/'installation.json').write_text(json.dumps(receipt,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
 (OUT/'KEO-jobs-test-20261105.json').write_text(json.dumps(receipt,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
 (HERE/'steel-jobs-test-20261105.json').write_text(json.dumps(receipt,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
 print(json.dumps(receipt,ensure_ascii=False,indent=2))
if __name__=='__main__':main()
