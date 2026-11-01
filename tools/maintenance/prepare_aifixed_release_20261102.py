"""Prepare only the accepted AI Fixed 2 candidate, without install or publication.

The separate historical 36-package proof remains an as-of record. This script
does not regenerate those packages or replace their published-version baselines.
"""
from pathlib import Path
import gettext
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[2]
DEV=ROOT.parent
HERE=Path(__file__).resolve().parent
CANON=ROOT/'tools/workshop'
WORK=DEV/'workshop'
INPUT=WORK/'maintenance-inputs-20261102'
SOURCE=ROOT/'AIFixed'
PROOF=DEV/'build/ReleaseMaintenance20261102'
DLL=DEV/'build/AIFixed/AIFixed.dll'
EXPECTED='3d0b58002aa920f09e33ad9088ef5dfab4b64f960637219cc63988e60c4ed4b8'
sys.path.insert(0,str(CANON))
from release_metadata import render_note


def read(p):return json.loads(p.read_text('utf-8-sig'))
def write(p,value):p.write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()


NOTES={
 'version':'2','published_version':'1',
 'en':[
  '[b]A modular AI collection.[/b] AI Fixed combines our robotic-repair fixes with selected GateFix and SleepFix adaptations by Basusu. Each module can be configured separately; new sleep, robotic-health and gate assistance are off by default.',
  '[b]Rest and robotic care.[/b] Optional automatic sleep treats bandaged biological wounds. Powered repair beds handle robotic health and wear, including prosthetics. Food takes priority when hunger affects the character.',
  '[b]Nearby suitable beds.[/b] A configurable search radius, ownership and free-bed checks keep automatic rest nearby. Your faction beds are used by default; allied beds can be allowed.',
  '[b]Manual orders take priority.[/b] Travel, Stop and queued orders interrupt our rest. Completed rest releases the character to existing jobs. Continuing healing is not cut short by a fixed total timer; pausing does not consume rest or gate waiting time.',
  '[b]Optional own-gate passage.[/b] A stalled movement order can open your gate and resume its destination. Locked-gate opening, closing after passage and restoring the original lock are separate options. Manual gate actions take priority; cancelled assistance may leave the gate open or unlocked.',
  '[b]Optional detailed logging.[/b] Language and translation summaries follow this plugin\'s Detailed log setting, off by default. Loading and format errors remain visible; enabling the log later reports the saved summary once.',
  '[b]A clean update.[/b] English, Russian and Simplified Chinese use one compiled catalog each. Personal settings and saved games are preserved. Thank you, Basusu, for the original mods and open sources.'
 ],
 'ru':[
  '[b]Модульный сборник исправлений ИИ.[/b] AI Fixed объединяет наши исправления ремонта робочастей и адаптации GateFix и SleepFix автора Basusu. Модули настраиваются отдельно; новые сон, ремонт здоровья робочастей и помощь у ворот по умолчанию выключены.',
  '[b]Отдых и уход за робочастями.[/b] Необязательный автоматический сон помогает лечить перевязанные живые раны. Ремкровати с питанием восстанавливают здоровье и износ робочастей, включая протезы. При влиянии голода на персонажа еда получает приоритет.',
  '[b]Подходящие кровати рядом.[/b] Настраиваемый радиус, проверка собственности и свободного места ограничивают поиск отдыха ближайшими подходящими кроватями. По умолчанию используются кровати своей фракции; союзные можно разрешить.',
  '[b]Ручные приказы важнее.[/b] Движение, Stop и очередь приказов прерывают наш отдых. После восстановления персонаж возвращается к прежним работам. Продолжающееся лечение не обрывается общим лимитом времени; пауза не расходует ожидание отдыха и ворот.',
  '[b]Необязательная помощь у своих ворот.[/b] Остановившийся персонаж с приказом движения может открыть свои ворота и продолжить путь. Открытие запертых ворот, закрытие после прохода и возврат исходного замка настраиваются отдельно. Ручные действия важнее; отмена помощи может оставить ворота открытыми или отпертыми.',
  '[b]Необязательный подробный журнал.[/b] Сводки языка и переводов подчиняются настройке подробного журнала этого плагина, выключенной по умолчанию. Ошибки загрузки и формата видны всегда; включение журнала позже выводит сохранённую сводку один раз.',
  '[b]Чистое обновление.[/b] Английский, русский и упрощённый китайский используют по одному скомпилированному каталогу. Личные настройки и сохранения сохраняются. Спасибо Basusu за оригинальные моды и открытые исходники.'
 ]}

GUIDE={
 'en':'''[h2]WHAT IT DOES[/h2]

AI Fixed is a growing collection of AI improvements by different authors. Its modules can be enabled separately.

[list]
[*][b]Robotic care.[/b] Rescuers prefer suitable repair beds for wounded skeletons. Robotic parts, including prosthetics, can finish their health and wear repairs before the character returns to work.
[*][b]Optional rest.[/b] Wounded characters can use a nearby ordinary bed after their wounds are treated. Hungry characters give food priority. Manual travel, Stop and queued orders take precedence.
[*][b]Beds nearby.[/b] Choose a search radius and use your own faction's free beds, or optionally allied beds. The bed must suit the treatment, and repair beds need power.
[*][b]Optional own-gate passage.[/b] A character stopped at your closed gate can open it and continue to the selected destination. Opening locked gates, closing after passage and restoring the lock are separate choices.
[*][b]Return to normal work.[/b] Our rest order ends after recovery. Pausing does not spend the waiting time, and ongoing healing can continue until it finishes.
[/list]

[h2]SETTINGS[/h2]

Use Options → MCM → AI Fixed, or AIFixed.ini without MCM. New automatic sleep, robotic-health repair and gate assistance are off by default. Existing robotic wear repair remains enabled. Health thresholds, bed ownership, search radii and gate closing can be adjusted separately. Detailed log is last and off by default. Settings that need a restart are marked in MCM.

[h2]AUTHORS AND SOURCES[/h2]

LEOPARD's robotic-repair fixes are joined by adaptations of [b]Basusu's GateFix and SleepFix[/b]. [b]Thank you, Basusu, for the original mods, open source and investigations![/b] Further modules may include other authors, credited separately.

[b]Original mods:[/b] [url=https://steamcommunity.com/sharedfiles/filedetails/?id=3787399736]GateFix by Basusu[/url], [url=https://steamcommunity.com/sharedfiles/filedetails/?id=3787375792]SleepFix by Basusu[/url].
[b]Original source code:[/b] [url=https://github.com/Basusu0/Basusu-kenshi-plugins]Basusu-kenshi-plugins[/url].
[b]Our source code:[/b] [url=https://github.com/xxxLEOPARDxxx/LEOPARD-Kenshi-Plugins]LEOPARD-Kenshi-Plugins[/url]. GPLv3; LICENSE and UPSTREAM.md are included.

[h2]COMPATIBILITY[/h2]

Do not enable the same mechanics in the original GateFix/SleepFix alongside this collection. Gate Push Guard handles a separate anti-push problem. Other AI mods, custom beds and races may change the result. Manual orders and gate actions take priority; cancelling passage assistance may leave a gate open or unlocked. Food availability and a usable path are still required.
''',
 'ru':'''[h2]ЧТО ДЕЛАЕТ[/h2]

AI Fixed — расширяемый сборник улучшений ИИ разных авторов. Его модули включаются отдельно.

[list]
[*][b]Уход за робочастями.[/b] Спасатели предпочитают подходящие ремкровати для раненых скелетов. Робочасти, включая протезы, могут закончить восстановление здоровья и снятие износа до возвращения персонажа к работе.
[*][b]Необязательный отдых.[/b] Раненые персонажи могут пользоваться обычной кроватью рядом после обработки ран. При голоде еда получает приоритет. Ручное движение, Stop и очередь приказов важнее.
[*][b]Кровати рядом.[/b] Настройте радиус поиска и используйте свободные кровати своей фракции, а по желанию — союзников. Кровать должна подходить для лечения, ремкровати требуется питание.
[*][b]Необязательный проход своих ворот.[/b] Персонаж, остановившийся у своих закрытых ворот, может открыть их и продолжить путь к выбранной точке. Открытие запертых ворот, закрытие после прохода и возврат замка настраиваются отдельно.
[*][b]Возврат к обычным работам.[/b] Наш приказ отдыха завершается после восстановления. Пауза не расходует время ожидания; продолжающееся лечение может идти до завершения.
[/list]

[h2]НАСТРОЙКИ[/h2]

Окно «Настройки» → MCM → AI Fixed либо AIFixed.ini без MCM. Новые автоматический сон, ремонт здоровья робочастей и помощь у ворот по умолчанию выключены. Прежний ремонт износа остаётся включён. Пороги здоровья, собственность кроватей, радиусы поиска и закрытие ворот регулируются отдельно. Подробный журнал последний и по умолчанию выключен. Требующие перезапуска настройки отмечены в MCM.

[h2]АВТОРЫ И ИСХОДНИКИ[/h2]

К исправлениям ремонта робочастей от LEOPARD добавлены адаптации [b]GateFix и SleepFix автора Basusu[/b]. [b]Спасибо Basusu за оригинальные моды, открытые исходники и исследования![/b] В дальнейшем сборник может включать модули других авторов с отдельным указанием авторства.

[b]Оригинальные моды:[/b] [url=https://steamcommunity.com/sharedfiles/filedetails/?id=3787399736]GateFix — Basusu[/url], [url=https://steamcommunity.com/sharedfiles/filedetails/?id=3787375792]SleepFix — Basusu[/url].
[b]Исходники оригинала:[/b] [url=https://github.com/Basusu0/Basusu-kenshi-plugins]Basusu-kenshi-plugins[/url].
[b]Наши исходники:[/b] [url=https://github.com/xxxLEOPARDxxx/LEOPARD-Kenshi-Plugins]LEOPARD-Kenshi-Plugins[/url]. GPLv3; LICENSE и UPSTREAM.md включены.

[h2]СОВМЕСТИМОСТЬ[/h2]

Не включайте одинаковые механики оригинальных GateFix/SleepFix одновременно с этим сборником. Gate Push Guard решает отдельную задачу защиты от выталкивания. Другие моды на ИИ, нестандартные кровати и расы могут менять результат. Ручные приказы и действия у ворот важнее; отмена помощи может оставить ворота открытыми или отпертыми. Доступная еда и свободный путь по-прежнему необходимы.
'''}


def metadata():
    baseline=read(PROOF/'aifixed-public-baseline.json')
    assert baseline['description_version']=='1' and baseline['candidate_version']=='2'
    assert sha(DLL)==EXPECTED
    text=(CANON/'build.py').read_text('utf-8')
    prefix='dict(mod="AIFixed",title="AI Fixed (QoL)",nexus=2256,version="'
    at=text.index(prefix)+len(prefix);end=text.index('"',at)
    assert text[at:end] in ('1','2')
    text=text[:at]+'2'+text[end:]
    text=text.replace('Focused repair-bed AI fixes and optional automatic wear repair for skeletons and characters with prosthetics.',
                      'A modular AI collection with robotic care, optional rest and own-gate assistance, configured through MCM.')
    (CANON/'build.py').write_text(text,encoding='utf-8')
    notes=read(CANON/'release_notes.json');notes['AIFixed']=NOTES;write(CANON/'release_notes.json',notes)
    history=read(CANON/'changelog.json');history.setdefault('AIFixed',{})['2']=render_note(NOTES);write(CANON/'changelog.json',history)
    guides=read(CANON/'desc/qol-20261029.json');guides['AIFixed']=GUIDE;write(CANON/'desc/qol-20261029.json',guides)
    catalog=read(CANON/'mcm_catalog.json')
    for row in catalog:
        if row['package']=='AIFixed':
            row['author']='LEOPARD; Basusu (GateFix/SleepFix)'
            row['contributors']=[{'name':'LEOPARD','scope':'Robotic-repair fixes and collection integration'},
                                 {'name':'Basusu','scope':'GateFix/SleepFix adaptations','source_url':'https://github.com/Basusu0/Basusu-kenshi-plugins'}]
            row['source_export'][0]['line']=next(i for i,line in enumerate((SOURCE/'AIFixed.cpp').read_text('utf-8-sig').splitlines(),1) if 'void MCM_Describe(' in line)
    write(CANON/'mcm_catalog.json',catalog)
    print('AI Fixed 2 metadata/credits only; existing 36 versions and histories retained')


def inputs():
    assert sha(DLL)==EXPECTED
    dst=INPUT/'AIFixed'
    assert dst.resolve().is_relative_to(INPUT.resolve()) and INPUT.resolve().is_relative_to(WORK.resolve())
    if dst.exists():shutil.rmtree(dst)
    dst.mkdir()
    shutil.copy2(DLL,dst/'AIFixed.dll')
    shutil.copy2(Path('D:/SteamLibrary/steamapps/common/Kenshi/mods/AIFixed/AIFixed.mod'),dst/'AIFixed.mod')
    write(dst/'RE_Kenshi.json',{'Plugins':['AIFixed.dll']})
    for name in ('README.txt','LICENSE','UPSTREAM.md'):
        assert (SOURCE/name).is_file();shutil.copy2(SOURCE/name,dst/name)
    assert 'Prepared release 2 / Подготовленный выпуск 2' in (dst/'README.txt').read_text('utf-8-sig')
    shutil.copy2(SOURCE/'AIFixed.ini',dst/'AIFixed.defaults.ini')
    catalogs=[]
    for lang in ('en_GB','ru_RU','zh_CN'):
        src=SOURCE/'locale'/lang/'LC_MESSAGES/ai_fixed.po'
        target=dst/'locale'/lang/'LC_MESSAGES/ai_fixed.po';target.parent.mkdir(parents=True);shutil.copy2(src,target)
        subprocess.run([sys.executable,str(ROOT/'tools/po2mo.py'),str(target)],check=True,capture_output=True)
        mo=target.with_suffix('.mo')
        with mo.open('rb') as stream:assert gettext.GNUTranslations(stream)._catalog
        target.unlink();catalogs.append({'path':str(mo.relative_to(dst)),'sha256':sha(mo)})
    manifest=read(INPUT/'release-inputs.json')
    manifest['packages']=[r for r in manifest['packages'] if r['package']!='AIFixed']+[
        {'package':'AIFixed','candidate_version':'2','dll':'AIFixed.dll','dll_sha256':EXPECTED,'catalogs':catalogs}]
    manifest['held']=[]
    manifest['runtime_acceptance_pending']=['AIFixed']
    write(INPUT/'release-inputs.json',manifest)
    print('AI Fixed clean input: only candidate, factory defaults, README/license/provenance and three MO catalogs')


def sync():
    for p in CANON.iterdir():
        if p.is_file() and p.suffix in {'.py','.json','.md'}:shutil.copy2(p,WORK/p.name)
    for rel in ('desc/make_desc.py','desc/qol-20261029.json'):shutil.copy2(CANON/rel,WORK/rel)


def package():
    sync()
    env=dict(os.environ,KENSHI_RELEASE_INPUT_ROOT=str(INPUT),PYTHONIOENCODING='utf-8')
    result=subprocess.run([sys.executable,str(WORK/'prepare_release.py'),'AIFixed'],env=env,
                          stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,encoding='utf-8')
    (PROOF/'prepare-aifixed-2.log').write_text(result.stdout,encoding='utf-8')
    print(result.stdout);result.check_returncode()
    print('AI Fixed 2 local package prepared; no installation or publication')


if __name__=='__main__':
    {'metadata':metadata,'inputs':inputs,'package':package}[sys.argv[1]]()
