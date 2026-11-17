"""Keep source player references and full bilingual release history current."""
from pathlib import Path
import json

HERE = Path('D:/DEV/Kenshi/ours/tools/workshop')
OURS = HERE.parents[1]
KEO = Path('D:/DEV/Kenshi/forks/LEOPARD-Kenshi-Forks/KEO')
notes = json.loads((HERE / 'release_notes.json').read_text(encoding='utf-8'))


def history(mod):
    note = notes[mod]
    return '\n\n## Release ' + note['version'] + ' / Выпуск ' + note['version'] + '\n\nEnglish\n\n' + '\n'.join('- ' + s for s in note['en']) + '\n\nРусский\n\n' + '\n'.join('- ' + s for s in note['ru']) + '\n'


p = KEO / 'README.md'
text = p.read_text(encoding='utf-8')
text = '# KEO (MCM fork) (QoL) - release 3\n\n' + text[text.index('## English'):]
text = text.replace('working release 2', 'release 3 implementation').replace('local KEO 2', 'KEO 3 implementation')
text = text.replace('рабочий выпуск 2', 'реализация выпуска 3')
text = text.replace('Working release 2, 2026-11-16 (local candidate, not published)', 'Release 3 implementation, code dated 2026-11-16')
text = text.replace('Рабочий выпуск 2, 16.11.2026 (локальная сборка, не опубликована)', 'Реализация выпуска 3, код от 16.11.2026')
text = text.replace('No new published version', 'Implementation reference')
en = '''### Resource preparation and loading

The **Resource memory** group contains optional file preparation in RAM,
startup warmup, nearby-terrain preparation and retention of loaded model
geometry/textures in video memory. These memory options are OFF by default.
Enable the RAM resource option first, then startup preparation if wanted.
Startup preparation reads registered local mesh/skeleton/texture resources
within the configured memory limit; reaching that limit does not mean the
entire game is loaded. It does not create NPCs early. RAM/VRAM limits and
background reading speed are configurable; keep room for the rest of the game.
These settings apply live. **Show warmup text** displays work at the upper left
and hides it automatically when finished, returning when new work is queued.

Under **Performance**, the loading improvements include skeleton/model and
material storage preparation, model bone weights/index tables, body-shape
preparation, equipment/animation reads and save-file parsing. Individual options
can be disabled live. New model work uses supported engine contracts and keeps
native handling for unsupported cases. Existing game simulation and NPC spawn
order remain in the game; the whole NPC pipeline is not moved onto workers.

The owner confirmed faster save loading and smoother warmed area transitions
on17 November2026. This is gameplay feedback for the combined update, not a
controlled timing multiplier or a promise of identical results on every save.
Cold areas, physics, NPC spawning and rendering can still cause pauses. General
developed-save FPS tuning is a separate task. Detailed implementation evidence
and the three bone-processing helpers are documented in the corresponding
source's `leopard` verification reports; isolated helper ratios are not overall
loading ratios.

'''
ru = '''### Подготовка ресурсов и загрузка

Группа **Память ресурсов** содержит необязательные подготовку файлов в ОЗУ,
стартовый прогрев, подготовку соседнего рельефа и удержание загруженной геометрии
моделей/текстур в видеопамяти. Эти настройки памяти по умолчанию ВЫКЛЮЧЕНЫ.
Сначала включите подготовку ресурсов в ОЗУ, затем при необходимости стартовый
прогрев. Он читает зарегистрированные локальные модели, скелеты и текстуры в
пределах выбранного лимита; достижение предела не означает загрузку всей игры.
Прогрев не создаёт NPC заранее. Пределы ОЗУ/видеопамяти и скорость фонового
чтения настраиваются; оставляйте память самой игре. Настройки применяются на
ходу. **Показывать текст прогрева** выводит текущую работу слева сверху;
после её завершения текст скрывается, а при новых запросах появляется снова.

В группе **Производительность** находятся ускорения подготовки памяти скелетов,
моделей и материалов, весов и таблиц костей, форм тела, чтения снаряжения и
анимации, а также разбора сейвов. Отдельные опции отключаются на ходу.
Для неподдерживаемых случаев сохраняется штатная обработка. Симуляцией и
порядком появления NPC по-прежнему управляет игра; весь этот процесс не
переносится в рабочие потоки.

Владелец17 ноября2026 подтвердил более быструю загрузку сейвов и более плавные
переходы между прогретыми областями. Это игровая проверка совокупного обновления,
а не контролируемый коэффициент ускорения или обещание одинакового результата
для любого сейва. Холодные области, физика, появление NPC и отрисовка могут
по-прежнему давать паузы. Общая оптимизация FPS развитой катки остаётся отдельной
задачей. Подробные проверки реализации и трёх этапов подготовки костей сохранены
в отчётах `leopard` соответствующих исходников; результаты отдельных функций
не являются коэффициентами ускорения всей загрузки.

'''
if en not in text: text = text.replace('### Operator cargo limit', en + '### Operator cargo limit', 1)
if ru not in text: text = text.replace('### Необязательный вес и задания', ru + '### Необязательный вес и задания', 1)
bone_en = '''### Bone-weight and index preparation

**Speed up model bone weights** (`meshWeights`) is ON by default and switches
live. It covers three independent supported stages: linear native-order weight
normalization, ordered assignment import without a repeated root search, and
bone lookup tables collected in a bounded8320-byte local bitset hierarchy.
Weights, equal-index order and sorted bone ranks remain unchanged. Ogre owns
imported nodes and output-array growth; old table gaps and allocation-error
partial state are preserved. Unusual weights, unordered imports and unsupported
cases keep the appropriate native stage. No persistent cache, per-frame scan
or new worker is added. The option affects newly prepared models; isolated
stage timings are not whole-save speedup factors.

'''
bone_ru = '''### Подготовка весов и таблиц костей

**Ускорять подготовку весов моделей** (`meshWeights`) по умолчанию включено и
переключается на ходу. Опция объединяет три независимых поддерживаемых этапа:
линейную нормализацию весов в штатном порядке, импорт упорядоченных привязок
без повторного поиска от корня и сбор таблиц костей локальной битовой структурой
размером8320 байт. Веса, порядок одинаковых индексов и ранги костей сохраняются.
Узлами импорта и ростом выходных массивов управляет Ogre; старые промежутки
таблиц и частичное состояние при ошибке выделения памяти сохранены. Необычные
веса, неупорядоченный импорт и неподдерживаемые случаи оставляют соответствующий
этап штатным. Постоянного кэша, покадрового обхода и нового потока нет. Опция
действует на вновь подготавливаемые модели; время отдельного этапа не является
коэффициентом ускорения всего сейва.

'''
if bone_en not in text: text = text.replace('### Operator cargo limit', bone_en + '### Operator cargo limit', 1)
if bone_ru not in text: text = text.replace('### Необязательный вес и задания', bone_ru + '### Необязательный вес и задания', 1)
# Historical release2 notes stay intact; current notes are a separate section.
if '## Release 3 / Выпуск 3' not in text: text += history('KEO')
p.write_text(text, encoding='utf-8')

p = OURS / 'SkillLevelUpInfo/README.txt'
text = p.read_text(encoding='utf-8')
local = 'Local correction / Локальная правка (2026-11-17; no new release / без нового выпуска)'
if local in text: text = text[:text.index(local)].rstrip() + '\n'
if '## Release 5 / Выпуск 5' not in text: text += history('SkillLevelUpInfo')
p.write_text(text, encoding='utf-8')

p = OURS / 'BestMinimap/README.txt'
text = p.read_text(encoding='utf-8')
text = text.replace('  * Тонкую линию вокруг ваших персонажей: докуда можно отвести камеру.\n    Предел плагин узнаёт сам - один раз отведите камеру до упора.\n', '')
text = text.replace('Information-strip implementation (release 4):\none root-owned thin game panel and at most four reusable inline fields;', 'Information-strip implementation (release 5):\none root-owned strip with at most four reusable inline fields; the rectangular\ngame-framed map shares its outer background/frame with that strip;')
text = text.replace('release 4 local candidate', 'release 5').replace('локальный кандидат выпуска 4', 'выпуск 5')
text = text.replace('Prepared update 2 / Подготовленный выпуск 2', 'Update 2 / Выпуск 2')
if '## Release 5 / Выпуск 5' not in text: text += history('BestMinimap')
p.write_text(text, encoding='utf-8')
print('README references/history updated; gameplay code unchanged')
