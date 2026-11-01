"""Prepare the approved 36 local candidates; never publish or install a game DLL."""
from pathlib import Path
import ast
import datetime
import gettext
import hashlib
import importlib.util
import json
import os
import re
import shutil
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[2]
DEV=ROOT.parent
CANON=ROOT/'tools/workshop'
WORK=DEV/'workshop'
HERE=Path(__file__).resolve().parent
INPUT=WORK/'maintenance-inputs-20261102'
PROOF=DEV/'build/ReleaseMaintenance20261102'
PROOF.mkdir(parents=True,exist_ok=True)
os.environ.pop('KENSHI_RELEASE_INPUT_ROOT',None)
sys.path.insert(0,str(CANON))
import build
from release_metadata import render_note

def read(path):return json.loads(path.read_text('utf-8-sig'))
def write(path,value):path.write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
baseline=read(HERE/'all-ready-steam-baselines-2026-11-02.json')
assert not baseline['api_error']
versions={r['package']:r['verified_steam_description_version'] for r in baseline['packages']}
assert len(versions)==36 and 'AIFixed' not in versions and all(versions.values())
inventory=read(HERE/'release-inventory-2026-11-01.json')
records={r['package']:r for r in inventory['packages']}
names=list(versions)
NEXT={name:str(int(v)+1) for name,v in versions.items()}

COMMON={
 'en':['[b]Optional language diagnostics.[/b] Language and translation-catalogue summaries follow this plugin\'s Detailed log setting, which is off by default.',
       '[b]Useful errors remain visible.[/b] Translation loading and format errors are still reported with Detailed log disabled. Enabling the setting after loading reports the saved summary once without reloading translations.',
       '[b]Clean update package.[/b] English, Russian and Simplified Chinese each use one compiled runtime catalogue. Personal settings and gameplay saves are preserved.'],
 'ru':['[b]Необязательная диагностика языка.[/b] Сводки языка и каталога переводов подчиняются настройке подробного журнала этого плагина, выключенной по умолчанию.',
       '[b]Ошибки остаются видимыми.[/b] Ошибки загрузки и формата переводов записываются и при выключенном подробном журнале. Включение настройки после загрузки выводит сохранённую сводку один раз без перезагрузки переводов.',
       '[b]Чистый пакет обновления.[/b] Английский, русский и упрощённый китайский используют по одному скомпилированному рабочему каталогу. Личные настройки и игровые сохранения сохраняются.']}
EXTRA={
 'UITweaks':{
  'en':['[b]Save and load previews.[/b] Select an existing slot in Save Game or Load Game to see its screenshot below the window. Successful manual saves, quicksaves and autosaves capture the game with its HUD while excluding save/load and pause menus. Slots without an image show no preview frame.',
        '[b]Stable, sharper previews.[/b] The dialog opens higher before selection and stays put when changing rows. Its full-width preview frame fits the complete image without cropping. New screenshots use filtered resizing and lossless PNG up to 1600x900; older JPEG previews remain readable.',
        '[b]A taller save list.[/b] MCM offers Save/load window height from 100% to 160%, default 120%. It applies on the next opening and respects the screen and preview space.'],
  'ru':['[b]Предпросмотр сохранений и загрузок.[/b] Выберите существующий слот в меню сохранения или загрузки, чтобы увидеть снимок под окном. Успешные ручные, быстрые и автоматические сохранения получают снимок игры с HUD, без меню сохранения, загрузки и паузы. У слотов без изображения рамка предпросмотра скрыта.',
        '[b]Стабильный и чёткий снимок.[/b] Окно сразу открывается выше и не двигается при смене строки. Рамка идёт по ширине окна, изображение вписывается целиком без обрезки. Новые снимки используют качественное уменьшение и PNG без потерь до 1600x900; старые JPEG по-прежнему читаются.',
        '[b]Более высокий список сейвов.[/b] В MCM настраивается высота окон сохранения и загрузки от 100% до 160%, по умолчанию 120%. Изменение применяется при следующем открытии с учётом экрана и места под снимок.']},
 'Container-Highlight':{
  'en':['[b]Safer highlight removal.[/b] Obsolete tint records are retired when container models change or disappear. A restoration failure stops further attempts on that record, while replacement materials are preserved.'],
  'ru':['[b]Защищённое снятие подсветки.[/b] Устаревшие записи окраски удаляются после замены или выгрузки модели контейнера. Ошибка восстановления прекращает повторные попытки с этой записью; новые материалы сохраняются.']},
 'Vital-Read':{
  'en':['[b]Optional icon compatibility.[/b] Optional custom textures are checked before loading, avoiding misleading missing-texture messages when the standard icon is used.'],
  'ru':['[b]Совместимость необязательных значков.[/b] Необязательные пользовательские текстуры проверяются до загрузки: использование штатного значка не вызывает ложных сообщений об отсутствующей текстуре.']},
 'Vital-Sense':{
  'en':['[b]Optional icon compatibility.[/b] Optional custom textures are checked before loading, avoiding misleading missing-texture messages. Settings explain the standard-icon fallback.'],
  'ru':['[b]Совместимость необязательных значков.[/b] Пользовательские текстуры проверяются до загрузки, без ложных сообщений об отсутствующих файлах. Подсказки настроек поясняют использование штатных значков.']},
 'StackSort':{
  'en':['[b]Quiet optional controls.[/b] Opening an inventory layout without the game\'s Arrange button no longer produces misleading missing-widget warnings. Existing sorting, merging, rotation and undo behavior is preserved.'],
  'ru':['[b]Необязательные кнопки без предупреждений.[/b] Открытие окна инвентаря без штатной кнопки раскладки больше не вызывает ложных предупреждений об отсутствующем виджете. Сортировка, объединение, повороты и отмена сохраняются.']},
 'ItemMarkers':{
  'en':['[b]Native stolen-item indication.[/b] The game\'s stolen-item indicator remains visible while dragging. Item ownership and stack-merging rules are unchanged.'],
  'ru':['[b]Штатная отметка краденого.[/b] Значок краденой вещи сохраняется при перетаскивании. Правила владения и объединения стопок не меняются.']}}
PLAYER={
 'Container-Highlight':{'en':'[b]Safer highlight removal.[/b] Clearing container colour is safer after its model changes or unloads; replacement colours from other mods are preserved.',
                       'ru':'[b]Устойчивое снятие подсветки.[/b] Окраска безопаснее снимается после смены или выгрузки модели контейнера; новые цвета других модов сохраняются.'},
 'Vital-Read':{'en':'[b]Optional icons.[/b] If custom icon textures are unavailable, the standard icon remains available without misleading missing-texture messages.',
               'ru':'[b]Необязательные значки.[/b] Если пользовательские текстуры недоступны, используется штатный значок без ложных сообщений об отсутствующей текстуре.'},
 'Vital-Sense':{'en':'[b]Optional icons.[/b] Settings explain when a custom icon falls back to the standard game icon; unavailable files no longer cause misleading texture messages.',
                'ru':'[b]Необязательные значки.[/b] Настройки поясняют замену пользовательского значка штатным; недоступные файлы не вызывают ложных сообщений о текстурах.'},
 'StackSort':{'en':'[b]Compatible inventory windows.[/b] Windows without the native Arrange button are skipped quietly, preserving their normal controls.',
              'ru':'[b]Совместимость окон инвентаря.[/b] Окна без штатной кнопки раскладки пропускаются без предупреждений и сохраняют обычные кнопки.'}}

def module(name):return build.source_dir(name)
def readme_path(name):
    root=Path(module(name))
    return root/'README.txt' if (root/'README.txt').is_file() else root/'README.md'
def plain_note(note):
    return '\n'.join(['English','']+['- '+re.sub(r'\[/?b\]','',s) for s in note['en']]+['','Русский','']+['- '+re.sub(r'\[/?b\]','',s) for s in note['ru']])

def versioned_catalog(text):
    raw=text.encode('utf-8');lines=raw.splitlines(keepends=True);replacements={}
    for node in ast.walk(ast.parse(text)):
        values={}
        if isinstance(node,ast.Call) and isinstance(node.func,ast.Name) and node.func.id=='dict':
            values={k.arg:k.value for k in node.keywords}
        elif isinstance(node,ast.Dict):
            values={k.value:v for k,v in zip(node.keys,node.values) if isinstance(k,ast.Constant)}
        mod=values.get('mod');version=values.get('version')
        if isinstance(mod,ast.Constant) and mod.value in NEXT and isinstance(version,ast.Constant):
            start=sum(map(len,lines[:version.lineno-1]))+version.col_offset
            end=sum(map(len,lines[:version.end_lineno-1]))+version.end_col_offset
            replacements[mod.value]=(start,end,json.dumps(NEXT[mod.value]).encode('ascii'))
    assert set(replacements)==set(names),set(names)-set(replacements)
    for start,end,value in sorted(replacements.values(),reverse=True):raw=raw[:start]+value+raw[end:]
    return raw.decode('utf-8')

def metadata():
    # Evaluate the existing complete player guides before changing their versions.
    spec=importlib.util.spec_from_file_location('existing_guides',CANON/'desc/make_desc.py')
    existing=importlib.util.module_from_spec(spec);spec.loader.exec_module(existing)
    notes=read(CANON/'release_notes.json');guides=read(CANON/'desc/qol-20261029.json')
    forks=read(CANON/'fork_catalog.json');history=read(CANON/'changelog.json')
    held_before=json.dumps({'notes':notes.get('AIFixed'),'guide':guides.get('AIFixed')},sort_keys=True)
    before=PROOF/'metadata-before.json'
    if not before.exists():write(before,{'notes':notes,'guides':guides,'forks':forks,'history':history,
                                       'published_ledger_sha256':sha(CANON/'published_releases.json')})
    text=versioned_catalog((CANON/'build.py').read_text('utf-8'))
    for name in names:
        # Exactly baseline +1, including repeat preparation of unpublished candidates.
        note={'version':NEXT[name],'published_version':versions[name],
              **{lang:EXTRA.get(name,{}).get(lang,[])+COMMON[lang] for lang in ('en','ru')}}
        notes[name]=note
        if name not in guides:guides[name]={'ru':existing.T[name][0],'en':existing.T[name][1]}
        if name in forks:
            forks[name]['version']=NEXT[name]
            forks[name]['history'][NEXT[name]]={lang:note[lang] for lang in ('en','ru')}
            if name in EXTRA and name!='ItemMarkers':
                for lang in ('ru','en'):
                    key='player_changes' if 'player_changes' in forks[name] else 'changes'
                    previous=re.sub(r'\[/?b\]','',EXTRA[name][lang][0])
                    if previous in forks[name][key][lang]:forks[name][key][lang].remove(previous)
                    addition=PLAYER.get(name,{}).get(lang,EXTRA[name][lang][0])
                    if addition not in forks[name][key][lang]:forks[name][key][lang].append(addition)
        path=readme_path(name);body=path.read_text('utf-8-sig')
        title=f'Prepared release {NEXT[name]} / Подготовленный выпуск {NEXT[name]} (2026-11-02)'
        if name=='UITweaks':
            if title in body:
                start=body.index(title);end=body.index('ИСТОРИЯ: ВЕРСИЯ 12 / VERSION 12 HISTORY',start)
                body=body[:start]+title+'\n\n'+plain_note(note)+'\n\n'+body[end:]
            else:
                start=body.index('ИЗМЕНЕНИЯ ТЕКУЩЕЙ ВЕРСИИ 12')
                end=body.index('ИСТОРИЯ: ВЕРСИЯ 11',start)
                published12=history[name]['12']
                body=body[:start]+title+'\n\n'+plain_note(note)+'\n\nИСТОРИЯ: ВЕРСИЯ 12 / VERSION 12 HISTORY\n\n'+re.sub(r'\[/?b\]','',published12)+'\n\n'+body[end:]
        else:
            heading=('## ' if path.suffix=='.md' else '')+title
            if heading in body:body=body[:body.index(heading)].rstrip()
            body=body.rstrip()+'\n\n'+heading+'\n\n'+plain_note(note)+'\n'
        path.write_text(body,encoding='utf-8')
    # Add actual new UI behavior to the main features, with current controls/settings.
    for lang in ('en','ru'):
        body=guides['UITweaks'][lang]
        if '[b]Save and load previews' not in body and '[b]Снимки сохранений' not in body:
            bullets=('\n[*][b]Save and load previews[/b] - select an existing slot in Save Game or Load Game to see its screenshot below the dialog. The window opens higher and stays put when changing rows. No image means no preview frame.\n'
                     '[*][b]Clear saved-game screenshots[/b] - successful manual saves, quicksaves and autosaves capture the game with its HUD, excluding save/load and pause menus. Filtered lossless PNG previews reach 1600x900; old JPEG previews remain readable.\n'
                     '[*][b]Save/load window height[/b] - 100% to 160% in MCM, default 120%, applied when the dialog next opens. The image fits without cropping and respects available screen space.\n') if lang=='en' else (
                     '\n[*][b]Снимки сохранений[/b] - выберите существующий слот в меню сохранения или загрузки, чтобы увидеть его снимок под окном. Окно сразу открывается выше и не двигается при смене строки. Без изображения рамка скрыта.\n'
                     '[*][b]Чёткий предпросмотр[/b] - успешные ручные, быстрые и автоматические сохранения получают снимок игры с HUD, без меню сохранения, загрузки и паузы. Качественное уменьшение и PNG без потерь до 1600x900; старые JPEG читаются.\n'
                     '[*][b]Высота окон сохранения и загрузки[/b] - от 100% до 160% в MCM, по умолчанию 120%; применяется при следующем открытии. Снимок вписывается целиком без обрезки с учётом экрана.\n')
            body=body.replace('[/list]',bullets+'[/list]',1)
        # The 12 release already included portrait trading; preserve it in the guide.
        if lang=='en' and 'Sequential squad trading' not in body:
            body=body.replace('[/list]','[*][b]Sequential squad trading[/b] - RMB/Trade on another allied portrait opens that exchange even while an earlier trade remains open; the native approach to distant targets is retained.\n[/list]',1)
        if lang=='ru' and 'Последовательный обмен' not in body:
            body=body.replace('[/list]','[*][b]Последовательный обмен[/b] - ПКМ и «Обмен» по портрету другого союзника открывает следующий обмен, даже если предыдущий ещё открыт; штатный подход к далёкой цели сохранён.\n[/list]',1)
        guides['UITweaks'][lang]=body
    assert held_before==json.dumps({'notes':notes.get('AIFixed'),'guide':guides.get('AIFixed')},sort_keys=True)
    (CANON/'build.py').write_text(text,encoding='utf-8')
    write(CANON/'release_notes.json',notes);write(CANON/'desc/qol-20261029.json',guides);write(CANON/'fork_catalog.json',forks)
    queue=read(HERE/'localization-release-queue.json')
    queue['policy']='Prepare approved public updates in parallel with satellite work. Verify actual published versions, increment once per release; user uploads Nexus first, then agent updates Steam after links. AIFixed is held for its integrated release 2.'
    queue['holds']={'AIFixed':'GateFix/SleepFix integration in progress; release 2 will combine it with the detailed-log fix. Do not package or bump until accepted.'}
    queue['status']='36 approved packages are being prepared locally; AIFixed held; nothing uploaded by this task'
    write(HERE/'localization-release-queue.json',queue)
    write(HERE/'release-candidate-plan-2026-11-02.json',{'status':'local candidates; not uploaded','created_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'input_root':str(INPUT),'held':queue['holds'],'packages':[{'package':n,'verified_steam_version':versions[n],'candidate_version':NEXT[n]} for n in names]})
    print('Metadata and source READMEs prepared for 36; AIFixed unchanged')

def inputs():
    INPUT.mkdir(exist_ok=True)
    manifest=[]
    for name in names:
        row=records[name];src=Path(build.mod_dir(name));dst=INPUT/name
        assert dst.resolve().is_relative_to(INPUT.resolve()) and INPUT.resolve().is_relative_to(WORK.resolve())
        if dst.exists():shutil.rmtree(dst)
        dst.mkdir()
        for folder,dirs,files in os.walk(src):
            dirs[:]=[d for d in dirs if d.lower() not in build.SKIP_DIRS and not d.lower().endswith('-source')]
            for fname in files:
                rel=(Path(folder)/fname).relative_to(src)
                if not build.runtime_file_allowed(name,folder,fname) or build.is_player_ini(name,fname):continue
                if 'locale' in rel.parts and fname.lower().endswith(('.mo','.po')):continue
                if fname.lower().endswith('.ini') and not fname.lower().endswith('.defaults.ini') and not ('SourceData' in rel.parts and fname in {'00_Vanilla.ini','50_OfficialExtensions.ini','90_ModSupport.ini'}):continue
                target=dst/rel;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(build.runtime_source_path(name,folder,fname),target)
        source=Path(module(name));dll=DEV/'build'/row['build_name']/(row['build_name']+'.dll')
        assert dll.is_file();shutil.copy2(dll,dst/dll.name)
        # A release manifest never carries a temporary diagnostic plugin loader.
        write(dst/'RE_Kenshi.json',{'Plugins':[dll.name]})
        for fname in ('README.txt','README.md','LICENSE','ORIGIN.md'):
            if (source/fname).is_file():shutil.copy2(source/fname,dst/fname)
        for license_file in source.iterdir():
            if license_file.is_file() and license_file.name.upper().startswith(('LICENSE','COPYING')):
                shutil.copy2(license_file,dst/license_file.name)
        factory=build.defaults_ini(name)
        if factory:
            base='CharacterInspector' if name=='Character Inspector' else name
            shutil.copy2(factory,dst/(base+'.defaults.ini'))
        catalogs=[]
        for lang in ('en_GB','ru_RU','zh_CN'):
            pos=[p for p in source.rglob('*.po') if 'locale' in p.parts and lang in p.parts and not {'tests','tools','build','.git'}.intersection(p.relative_to(source).parts)]
            assert len(pos)==1,(name,lang,pos)
            target=dst/'locale'/lang/'LC_MESSAGES'/pos[0].name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(pos[0],target)
            subprocess.run([sys.executable,str(ROOT/'tools/po2mo.py'),str(target)],check=True,capture_output=True)
            mo=target.with_suffix('.mo')
            with mo.open('rb') as stream:assert gettext.GNUTranslations(stream)._catalog
            target.unlink();catalogs.append({'path':str(mo.relative_to(dst)),'sha256':sha(mo)})
        # Disabled stage folders receive only the verified candidate DLL/catalogs/README.
        if name in ('Hidden-Faction-Relations','Map-markers'):
            stage=DEV/'stage'/name
            assert src.resolve()==stage.resolve() and not (Path(build.MODS)/name).exists()
            shutil.copy2(dll,stage/dll.name)
            shutil.copy2(readme_path(name),stage/readme_path(name).name)
            for catalog in catalogs:
                target=stage/catalog['path'];target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(dst/catalog['path'],target)
        manifest.append({'package':name,'candidate_version':NEXT[name],'dll':dll.name,'dll_sha256':sha(dll),'catalogs':catalogs})
    write(INPUT/'release-inputs.json',{'packages':manifest,'held':['AIFixed']})
    print('36 clean candidate inputs; disabled HFR/Map stage synchronized; no game DLL/INI changes')

def sync():
    # Copy authoritative sources only. Old ignored/generated descriptions and
    # staged item folders can exist beside the versioned tools; copying those
    # would overwrite freshly generated release output with stale text.
    sources=[p for p in CANON.iterdir() if p.is_file() and p.suffix in {'.py','.json','.md'}]
    sources += [CANON/'desc/make_desc.py',CANON/'desc/qol-20261029.json']
    for path in sources:
        target=WORK/path.relative_to(CANON);target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(path,target)

def package():
    sync()
    env=dict(os.environ,KENSHI_RELEASE_INPUT_ROOT=str(INPUT),PYTHONIOENCODING='utf-8')
    with (PROOF/'prepare-release.log').open('w',encoding='utf-8') as log:
        subprocess.run([sys.executable,str(WORK/'prepare_release.py'),*names],env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
    print('Unified prepare_release completed for 36; no publication')

if __name__=='__main__':
    {'metadata':metadata,'inputs':inputs,'package':package}[sys.argv[1]]()
