"""Prepare reviewed loading/UI release metadata; does not publish anything."""
from pathlib import Path
import ast, json, re, shutil

HERE = Path('D:/DEV/Kenshi/ours/tools/workshop')
WORK = Path('D:/DEV/Kenshi/workshop')
OURS = HERE.parents[1]
KEO = Path('D:/DEV/Kenshi/forks/LEOPARD-Kenshi-Forks/KEO')


def load(name):
    return json.loads((HERE / name).read_text(encoding='utf-8-sig'))


def save(name, data):
    (HERE / name).write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')


notes = load('release_notes.json')
notes['KEO'] = {
    'version': '3', 'published_version': '2',
    'en': [
        'Added optional resource preparation in RAM, startup warmup and advance preparation of nearby terrain. Reusing warmed resources reduces loading interruptions when revisiting areas.',
        'Added bounded retention of loaded model geometry and textures in video memory, with configurable limits.',
        'Accelerated startup before the main menu and startup resource preparation.',
        'Reduced CPU work when loading saves and preparing NPC skeletons, models, body shapes, equipment and bone data.',
        'Warmup text shows current progress and disappears when preparation finishes; it reappears for new queued work.',
        'Reduced redundant resource bookkeeping and corrected resource lifetime, cancellation and compatibility fallback handling.',
        'Updated Rotate Items compatibility for counter stocking. Unavailable stocking no longer disables production collection or waypoint routes.'
    ],
    'ru': [
        'Добавлены необязательная подготовка ресурсов в ОЗУ, стартовый прогрев и предварительная подготовка соседнего рельефа. Повторное использование прогретых ресурсов уменьшает задержки при возвращении в уже посещённые области.',
        'Добавлено удержание загруженной геометрии моделей и текстур в видеопамяти с настраиваемыми пределами.',
        'Ускорены запуск до главного меню и стартовая подготовка ресурсов.',
        'Уменьшена работа CPU при загрузке сейвов и подготовке скелетов, моделей, форм тела, снаряжения и данных костей NPC.',
        'Текст прогрева показывает текущий прогресс и исчезает после подготовки; при новых запросах появляется снова.',
        'Убрана лишняя обработка ресурсов, исправлены их освобождение, отмена загрузки и переход к штатной обработке при несовместимости.',
        'Обновлена совместимость пополнения прилавка с Rotate Items. Недоступность пополнения больше не отключает сбор продукции и маршруты по точкам.'
    ]
}
notes['SkillLevelUpInfo'] = {
    'version': '5', 'published_version': '4',
    'en': ['Excluded the unused Mass combat skill from XP and level-up notifications and recent-training bars. Other skills and the game\'s XP calculation are unchanged.'],
    'ru': ['Неиспользуемый навык «Массовый бой» исключён из сообщений об опыте, повышении уровня и полос недавних тренировок. Остальные навыки и расчёт опыта игры не изменены.']
}
notes['BestMinimap'] = {
    'version': '5', 'published_version': '4',
    'en': [
        'Joined the rectangular minimap and information strip in one game-style frame, removing the gap between them.',
        'Centred the visible text and its shadow horizontally and vertically, with room for descenders and skin-specific margins.',
        'Fixed stale town-icon references when the game replaces or destroys map widgets, including during long trips. The rendering worker is preserved.'
    ],
    'ru': [
        'Прямоугольная миникарта и информационная строка объединены общей игровой рамкой без зазора.',
        'Видимый текст вместе с тенью центрируется по горизонтали и вертикали; нижние элементы букв и отступы скина помещаются целиком.',
        'Исправлены устаревшие ссылки на значки городов при замене или удалении виджетов карты, в том числе на дальних переходах. Рабочий поток отрисовки сохранён.'
    ]
}
save('release_notes.json', notes)

path = HERE / 'build.py'
text = path.read_text(encoding='utf-8')
for mod, old, new in [('KEO', '2', '3'), ('BestMinimap', '4', '5')]:
    # Exact AST keyword boundaries keep repeat runs inside this mod's record.
    call = next(node for node in ast.walk(ast.parse(text)) if isinstance(node, ast.Call)
                and isinstance(node.func, ast.Name) and node.func.id == 'dict'
                and any(k.arg == 'mod' and isinstance(k.value, ast.Constant) and k.value.value == mod for k in node.keywords))
    value = next(k.value for k in call.keywords if k.arg == 'version')
    assert value.value in (old, new), (mod, value.value)
    lines = text.splitlines(keepends=True)
    assert value.lineno == value.end_lineno
    raw = lines[value.lineno - 1].encode('utf-8')
    lines[value.lineno - 1] = (raw[:value.col_offset] + json.dumps(new).encode() + raw[value.end_col_offset:]).decode('utf-8')
    text = ''.join(lines)
text = text.replace("'mod': 'SkillLevelUpInfo', 'title': 'Skill Level Up Info (QoL)', 'nexus': 2257, 'version': \"4\"", "'mod': 'SkillLevelUpInfo', 'title': 'Skill Level Up Info (QoL)', 'nexus': 2257, 'version': \"5\"")
path.write_text(text, encoding='utf-8')

desc = load('desc/qol-20261029.json')
for lang in ('en', 'ru'):
    skill_line = ('The unused Mass combat skill is excluded from messages and recent-training bars.' if lang == 'en'
                  else 'Неиспользуемый навык «Массовый бой» не появляется в сообщениях и полосах недавних тренировок.')
    if skill_line not in desc['SkillLevelUpInfo'][lang]:
        desc['SkillLevelUpInfo'][lang] = desc['SkillLevelUpInfo'][lang].replace('[/list]', '[*]' + skill_line + '\n[/list]', 1)
    old = ('The compact strip disappears when all fields are off.' if lang == 'en'
           else 'Панель компактная; когда все поля выключены, её нет.')
    new = ('The strip shares the rectangular map\'s game-style frame. Text is centred with room for complete letters; the strip disappears when all fields are off.' if lang == 'en'
           else 'Строка примыкает к прямоугольной карте общей игровой рамкой. Текст центрируется и помещается целиком; когда все поля выключены, строки нет.')
    desc['BestMinimap'][lang] = desc['BestMinimap'][lang].replace(old, new)
save('desc/qol-20261029.json', desc)

forks = load('fork_catalog.json')
row = forks['KEO']
row['version'] = '3'
row['history']['3'] = {lang: notes['KEO'][lang] for lang in ('en', 'ru')}
for field in ('changes', 'player_changes'):
    for lang in ('en', 'ru'):
        row[field][lang] = [line for line in row[field][lang] if line not in notes['KEO'][lang][:5]]
        line = ('Optional RAM/VRAM resource preparation and faster startup, save and NPC loading.' if lang == 'en'
                else 'Необязательная подготовка ресурсов в ОЗУ/VRAM и ускорение запуска, загрузки сейвов и NPC.')
        if line not in row[field][lang]: row[field][lang].append(line)
save('fork_catalog.json', forks)

catalog = load('mcm_catalog.json')
for row in catalog:
    package = row.get('package')
    if package not in ('KEO', 'BestMinimap', 'SkillLevelUpInfo'): continue
    source = KEO / 'src/gui/mcm_bridge.cpp' if package == 'KEO' else OURS / package / (package + '.cpp')
    lines = source.read_text(encoding='utf-8-sig').splitlines()
    line = next(i + 1 for i, value in enumerate(lines) if 'MCM_Describe(' in value)
    row['source'] = str(KEO if package == 'KEO' else OURS / package)
    row['source_export'] = [{'path': str(source), 'line': line, 'method': 'direct'}]
    row['published_version'] = '2' if package == 'KEO' else '4'
    if 'nexus_published_version' in row: row['nexus_published_version'] = row['published_version']
save('mcm_catalog.json', catalog)

# Synchronize canonical tools only; generated packages remain the preparer's job.
for name in ('build.py', 'release_notes.json', 'fork_catalog.json', 'mcm_catalog.json', 'desc/qol-20261029.json'):
    shutil.copyfile(HERE / name, WORK / name)
print('Metadata prepared: KEO3, SkillLevelUpInfo5, BestMinimap5; published registry untouched')
