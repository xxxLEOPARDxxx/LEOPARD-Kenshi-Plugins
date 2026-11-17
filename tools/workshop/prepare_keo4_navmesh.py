"""Prepare KEO4 metadata only. Native repair+full generation; never publish."""
from pathlib import Path
import ast,json,shutil
HERE=Path(__file__).resolve().parent
WORK=Path('D:/DEV/Kenshi/workshop')
def load(n):return json.loads((HERE/n).read_text(encoding='utf-8-sig'))
def save(n,d):(HERE/n).write_text(json.dumps(d,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
published=load('published_releases.json')
assert published['mods']['KEO']['version']=='3','KEO4 is only prepared against published3'
note={'version':'4','published_version':'3',
 'en':[
  'Added optional automatic Ctrl+Shift+F11-style building repair and full exterior/interior navmesh generation once per visited zone after each save load.',
  'Uses the existing KEO background generation, waits for idle loading, physics and navigation, excludes speculative preloads and spaces requests.',
  'The live setting is named Pathing repairs itself, following the KenshiQOL feature description, and is off by default.',
  'Added English, Russian and Simplified Chinese help and optional detailed operation timings. Personal settings are preserved.',
  'Corrected the package loader manifest so it no longer requests a compatibility DLL absent from the package.'
 ],
 'ru':[
  'Добавлены необязательный автоматический ремонт построек по Ctrl+Shift+F11 и полная генерация внешней/внутренней навсетки один раз для каждой посещённой зоны после загрузки сохранения.',
  'Используются фоновые потоки KEO; при занятой загрузке мира, физике или навигации новая заявка ждёт. Фоновые предзагрузки исключены, между заявками есть пауза.',
  'Настройка «Автовосстановление путей» (Pathing repairs itself, название из описания KenshiQOL) переключается без перезапуска и по умолчанию выключена.',
  'Добавлены подсказки на русском, английском и китайском и необязательные подробные замеры отдельных операций. Личные настройки сохраняются.',
  'Исправлен манифест загрузки: убрана ссылка на DLL совместимости, отсутствующую в пакете.'
 ]}
notes=load('release_notes.json');notes['KEO']=note;save('release_notes.json',notes)
path=HERE/'build.py';text=path.read_text(encoding='utf-8')
call=next(n for n in ast.walk(ast.parse(text)) if isinstance(n,ast.Call) and isinstance(n.func,ast.Name) and n.func.id=='dict' and any(k.arg=='mod' and isinstance(k.value,ast.Constant) and k.value.value=='KEO' for k in n.keywords))
value=next(k.value for k in call.keywords if k.arg=='version');assert value.value in ('3','4')
lines=text.splitlines(keepends=True);raw=lines[value.lineno-1].encode();lines[value.lineno-1]=(raw[:value.col_offset]+b'"4"'+raw[value.end_col_offset:]).decode();path.write_text(''.join(lines),encoding='utf-8')
desc=load('desc/qol-20261029.json')
sections={
 'en':'''[h2]AUTOMATIC PATH REPAIR[/h2]
[b]Pathing repairs itself[/b] follows the KenshiQOL feature name. Enable it in MCM > KEO > Gameplay fixes; it is off by default and applies live. After each save load, each actually visited active zone receives one native Ctrl+Shift+F11-style pass: building/furniture repair followed by full exterior and eligible interior navmesh generation. Repair covers all active zones, as with the native command, and can briefly pause the game. Requests wait for idle loading, physics and navigation and are spaced apart; private preloads are excluded. Existing KEO workers handle generation; navmesh caching is required. Native loading messages remain. Disabling does not cancel submitted work. Loading a save resets visit history. This does not teleport actors or guarantee every stuck path is fixed.

''',
 'ru':'''[h2]АВТОВОССТАНОВЛЕНИЕ ПУТЕЙ[/h2]
[b]Автовосстановление путей[/b] (Pathing repairs itself из описания KenshiQOL): MCM > KEO > Исправления игры. Выключено по умолчанию, действует сразу. Раз за посещённую зону после загрузки сейва выполняет штатный ремонт построек/мебели всех активных зон и полную внешнюю/внутреннюю генерацию навсетки, как Ctrl+Shift+F11. Ремонт может задержать кадр. При занятой загрузке, физике или навигации заявка ждёт; между заявками пауза. Фоновые предзагрузки исключены. Нужен кэш навсетки; используются потоки KEO и штатные сообщения загрузки. Выключение не отменяет задачи; загрузка сейва очищает историю. Телепортации нет, исправление любого пути не гарантируется.

'''}
for lang,section in sections.items():
 text=desc['KEO'][lang]
 heading='[h2]AUTOMATIC PATH REPAIR[/h2]' if lang=='en' else '[h2]АВТОВОССТАНОВЛЕНИЕ ПУТЕЙ[/h2]'
 marker='[h2]LOADING AND MEMORY[/h2]' if lang=='en' else '[h2]ЗАГРУЗКА И ПАМЯТЬ[/h2]'
 if heading in text:text=text[:text.index(heading)]+text[text.index(marker):]
 assert marker in text
 desc['KEO'][lang]=text.replace(marker,section+marker,1)
replacements={
 'KEO подготавливает соседние области и пути, согласует движение отрядов, оптимизирует частицы, окружение и рюкзаки. Учитываются плавание и опасная вода. Дальние переходы ещё могут прерываться: исправление всех проблем поиска пути не заявляется.':'Подготовка областей и путей, движение отрядов, оптимизации частиц, окружения и рюкзаков. Учитываются плавание и опасная вода. Дальние переходы ещё могут прерываться.',
 'Необязательная подготовка ресурсов в ОЗУ: стартовые файлы и соседний рельеф для более плавного возвращения в посещённые места.':'Подготовка стартовых файлов и соседнего рельефа в ОЗУ для более плавных повторных посещений.',
 'MCM > KEO > Память ресурсов: сначала включите ОЗУ, затем прогрев или удержание моделей/текстур. Настройки памяти по умолчанию выключены, применяются на ходу. Оставляйте память игре. Результат зависит от сейва и области; все NPC заранее не создаются, любые паузы не исключаются.':'Память ресурсов в MCM: сначала включите ОЗУ, затем прогрев или удержание моделей/текстур. По умолчанию выключено, действует сразу. Оставляйте память игре. Результат зависит от сейва/области; NPC заранее не создаются, паузы возможны.',
 'Доставляются переносимые продукты назначенных задач сбора, а без них - обычные ресурсы. Для доставки от производства назначьте оба задания. Это не поиск произвольной экипировки в хранилищах и не денежная продажа.':'Доставляет переносимые продукты назначенных задач сбора, без них - обычные ресурсы. Для доставки с производства нужны оба задания. Экипировку из хранилищ не ищет, денежную продажу не создаёт.',
 'Строки переставляются/удаляются в списке работ. Маршрут зациклен и сохраняется с игрой. Выключение приостанавливает его, сохраняя строки.':'Переставляйте/удаляйте строки в работах. Маршрут зациклен, сохраняется с игрой; выключение приостанавливает его, сохраняя строки.',
 'Нужны Kenshi1.0.65 и RE_Kenshi0.3.5+ с совместимыми библиотеками. Один пакет выбирает Steam/GOG. GOG работает, но проверен не всесторонне; запуск подтверждён на более ранней сборке.':'Нужны Kenshi1.0.65, RE_Kenshi0.3.5+ и совместимые библиотеки. Единый пакет Steam/GOG; GOG проверен не всесторонне, запуск подтверждён ранее.',
 'MCM: загрузка, производительность, движение, правки игры и задания; без MCM - вкладка KEO. Отмеченным настройкам нужен перезапуск, в том числе переносимому весу. Сохраняйте KEO.ini. Подробный журнал - последний, по умолчанию выключен; ошибки видимы всегда. Интерфейс: русский, английский или китайский. Прирост FPS различается.':'MCM: загрузка, производительность, движение, правки и задания; без MCM - вкладка KEO. Отмеченные настройки, включая переносимый вес, требуют перезапуска. Сохраняйте KEO.ini. Последний пункт - подробный журнал, по умолчанию выключен; ошибки видимы всегда. Русский/английский/китайский интерфейс; прирост FPS различается.'
}
replacements.update({
 'Форк заменяет оригинальный KEO; не запускайте одновременно KenshiQOL. Спасибо Steel за объединённые функции веса/заданий.':'Заменяет KEO. Не запускайте вместе с KenshiQOL. Спасибо Steel за исходные функции.',
 'Форк сопровождает исправления и совместимость MCM/GOG. Если автор вернётся и продолжит разработку, мы удалим нашу версию, чтобы не путать игроков.':'Временное сопровождение MCM/GOG и исправлений. Если автор вернётся к разработке, удалим форк во избежание путаницы.',
 'Последний пункт - подробный журнал, по умолчанию выключен; ошибки видимы всегда.':'Журнал последний, выключен; ошибки видимы всегда.',
 'Необязательный текст прогресса слева сверху скрывается после работы и появляется при новых запросах.':'Текст прогресса слева сверху скрывается по завершении и возвращается при новых запросах.'
 ,'Независимые добавления по умолчанию выключены. Включаются в MCM > KEO > Рюкзаки и работа:':'Добавления выключены по умолчанию: MCM > KEO > Рюкзаки и работа.'
 ,'Лимиты груза операторов по весу и занятым ячейкам задаются с шагом5%; доставка начинается по первому пределу.':'Груз оператора: вес и занятые ячейки с шагом5%; доставка по первому пределу.'
})
for old,new in replacements.items():desc['KEO']['ru']=desc['KEO']['ru'].replace(old,new)
save('desc/qol-20261029.json',desc)
forks=load('fork_catalog.json');row=forks['KEO'];row['version']='4';row['history']['4']={lang:note[lang] for lang in ('en','ru')}
for field in ('changes','player_changes'):
 for lang,line in [('en','Optional automatic native building repair and full exterior/interior navmesh regeneration for visited zones, following KenshiQOL.'),('ru','Необязательный штатный ремонт построек и полная генерация внешней/внутренней навсетки посещённых зон по образцу KenshiQOL.')]:
  if line not in row[field][lang]:row[field][lang].append(line)
 row[field]['ru']=[
  'Страница MCM; без MCM доступна исходная вкладка KEO.',
  'Русский/английский/китайский интерфейс; для других языков - английский. Подробный журнал выключен.',
  'Единый пакет Steam/GOG; GOG проверен не всесторонне.',
  'Функции Steel (KenshiQOL): переносимый вес, сбор продукции, пополнение прилавков и циклические маршруты.',
  'Исправления маршрутов, устаревших точек и готовности пути в городах.',
  'Необязательная подготовка RAM/VRAM и ускорение загрузки игры, сейвов и NPC.',
  'Автоматический ремонт построек и полной навсетки посещённых зон по образцу KenshiQOL.'
 ]
save('fork_catalog.json',forks)
for name in ('build.py','release_notes.json','fork_catalog.json','desc/qol-20261029.json'):shutil.copyfile(HERE/name,WORK/name)
print('KEO4 metadata ready; published registry unchanged; no publication')
