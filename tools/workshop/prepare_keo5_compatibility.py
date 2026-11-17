"""Prepare only KEO5's ordinary compatibility update; never publish or install."""
from pathlib import Path
import ast, hashlib, json, shutil

# Both the canonical script and its working copy update the same metadata.
HERE = Path('D:/DEV/Kenshi/ours/tools/workshop')
WORK = Path('D:/DEV/Kenshi/workshop')
ROOT = Path('D:/DEV/Kenshi/forks/LEOPARD-Kenshi-Forks/KEO')
GAME = Path('D:/SteamLibrary/steamapps/common/Kenshi')
STAGE = ROOT/'build/shared-menu-release'

def load(name): return json.loads((HERE/name).read_text(encoding='utf-8-sig'))
def save(name, value): (HERE/name).write_text(json.dumps(value, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()

assert load('published_releases.json')['mods']['KEO']['version'] == '4', 'Prepare5 only against published4'
note = {'version':'5', 'published_version':'4',
 'en':[
  'Fixed custom-job initialization when KenshiLua scripts, including Animal Husbandry, already use the shared context menu. This could disable Waypoint route, Gather output and Stock shop counter.',
  'Existing job rows and personal settings are preserved. No save import or relocation between Workshop and local mods is required.'
 ],
 'ru':[
  'Исправлена инициализация работ KEO, когда скрипты KenshiLua, в том числе Animal Husbandry, уже используют общее контекстное меню. Из-за этого могли отключаться маршрут по точкам, сбор продукции и пополнение прилавка.',
  'Назначенные работы и личные настройки сохраняются. Импорт сохранения и перенос модов между Workshop и локальной папкой mods не требуются.'
 ]}
STAGE.mkdir(parents=True, exist_ok=True)
protected = [GAME/'mods/KEO/KEO.ini', GAME/'mods/KEO/KEO.dll', GAME/'data/mods.cfg',
 WORK/'published_releases.json', WORK/'mcm_catalog.json', WORK/'ALL_CHANGELOG_MODS.txt']
if not (STAGE/'protected-before.json').exists():
 (STAGE/'protected-before.json').write_text(json.dumps({str(p):sha(p) for p in protected},indent=2))
 (STAGE/'other-archives-before.json').write_text(json.dumps({str(p):sha(p) for p in (WORK/'nexus').glob('*.zip') if not p.name.startswith('KEO-')},indent=2))
 (STAGE/'metadata-before.json').write_text(json.dumps({n:load(n) for n in ['release_notes.json','fork_catalog.json','desc/qol-20261029.json']},ensure_ascii=False,indent=2),encoding='utf-8')
notes=load('release_notes.json');notes['KEO']=note;save('release_notes.json',notes)
path=HERE/'build.py';text=path.read_text(encoding='utf-8')
row=next(n for n in ast.walk(ast.parse(text)) if isinstance(n,ast.Call) and isinstance(n.func,ast.Name) and n.func.id=='dict' and any(k.arg=='mod' and isinstance(k.value,ast.Constant) and k.value.value=='KEO' for k in n.keywords))
value=next(k.value for k in row.keywords if k.arg=='version');assert value.value in ('4','5')
lines=text.splitlines(keepends=True);raw=lines[value.lineno-1].encode();lines[value.lineno-1]=(raw[:value.col_offset]+b'"5"'+raw[value.end_col_offset:]).decode();path.write_text(''.join(lines),encoding='utf-8')

descriptions=load('desc/qol-20261029.json')
for lang, marker, addition in [
 ('en','Requires Kenshi1.0.65','Custom jobs support shared context menus used by KenshiLua scripts, including Animal Husbandry.\n'),
 ('ru','Нужны Kenshi1.0.65','Работы совместимы с меню KenshiLua и Animal Husbandry.\n')]:
 text=descriptions['KEO'][lang]
 if addition not in text:
  assert marker in text;text=text.replace(marker,addition+marker,1)
 descriptions['KEO'][lang]=text
# Retain all instructions while making room inside Steam's byte limit.
for old,new in [
 ('Подготовка областей и путей, движение отрядов, оптимизации частиц, окружения и рюкзаков. Учитываются плавание и опасная вода. Дальние переходы ещё могут прерываться.', 'Подготовка областей/путей, движение отрядов, оптимизации частиц, окружения и рюкзаков. Учитываются плавание и опасная вода; дальние переходы могут прерываться.'),
 ('Работы совместимы с общим меню скриптов KenshiLua, включая Animal Husbandry.', 'Работы совместимы с меню KenshiLua и Animal Husbandry.'),
 ('(Pathing repairs itself из описания KenshiQOL)', '(Pathing repairs itself, KenshiQOL)'),
 ('MCM: загрузка, производительность, движение, правки и задания; без MCM - вкладка KEO. Отмеченные настройки, включая переносимый вес, требуют перезапуска. Сохраняйте KEO.ini.', 'MCM: загрузка, производительность, движение, правки и задания; без MCM - вкладка KEO. Отмеченные настройки (включая переносимый вес) требуют перезапуска. Сохраняйте KEO.ini.'),
 ('Нужны Kenshi1.0.65, RE_Kenshi0.3.5+ и совместимые библиотеки. Единый пакет Steam/GOG; GOG проверен не всесторонне, запуск подтверждён ранее.', 'Нужны Kenshi1.0.65, RE_Kenshi0.3.5+ и совместимые библиотеки. Один пакет Steam/GOG; GOG проверен частично, запуск подтверждён ранее.'),
 ('Временное сопровождение MCM/GOG и исправлений. Если автор вернётся к разработке, удалим форк во избежание путаницы.', 'Временное сопровождение MCM/GOG и исправлений. При возвращении автора к разработке удалим форк во избежание путаницы.')]:
 descriptions['KEO']['ru']=descriptions['KEO']['ru'].replace(old,new)
save('desc/qol-20261029.json',descriptions)
forks=load('fork_catalog.json');forks['KEO']['version']='5';forks['KEO']['history']['5']={lang:note[lang] for lang in ('en','ru')};save('fork_catalog.json',forks)

readme=ROOT/'README.md';text=readme.read_text(encoding='utf-8')
text=text.replace(' - release 4\n',' - release 5\n',1)
heading='## Unreleased compatibility fix / Невыпущенное исправление совместимости'
if heading in text:
 start=text.index(heading);end=text.index('## English',start);text=text[:start]+text[end:]
if '## Release 5 / Выпуск 5' not in text:
 text+='\n## Release 5 / Выпуск 5\n\n'
 for lang,title in [('en','English'),('ru','Русский')]:text+='### '+title+'\n\n'+''.join('- '+line+'\n' for line in note[lang])+'\n'
readme.write_text(text,encoding='utf-8')

candidate=STAGE/'inputs/KEO';candidate.mkdir(parents=True,exist_ok=True)
binary=ROOT/'build/KEO_step4_prod/KEO.dll'
assert sha(binary)=='94a9593f34bfaafcebbb8e337a3d932076766ef7b9f93828d4e860e0b9e261c0'
for source,name in [(binary,'KEO.dll'),(WORK/'KEO/content/KEO.mod','KEO.mod'),(ROOT/'LICENSE','LICENSE'),(ROOT/'README.md','README.md'),(ROOT/'RE_Kenshi.json','RE_Kenshi.json')]:shutil.copyfile(source,candidate/name)
for lang in ['en_GB','ru_RU','zh_CN']:
 target=candidate/f'locale/{lang}/LC_MESSAGES/keo.mo';target.parent.mkdir(parents=True,exist_ok=True)
 shutil.copyfile(GAME/f'mods/KEO/locale/{lang}/LC_MESSAGES/keo.mo',target)
for name in ['build.py','release_notes.json','fork_catalog.json','desc/qol-20261029.json']:shutil.copyfile(HERE/name,WORK/name)
print('KEO5 metadata and clean candidate prepared; published registry and installed game unchanged')
