"""Explicit upstream credits, fork-only changes/history and verified MCM support."""
from pathlib import Path
import json
import re

HERE=Path(__file__).resolve().parent

def forks():
    return json.loads((HERE/'fork_catalog.json').read_text(encoding='utf-8'))

def mcm_catalog():
    return json.loads((HERE/'mcm_catalog.json').read_text(encoding='utf-8'))

def section(title,text):return '[h2]'+title+'[/h2]\n\n'+text.strip()+'\n\n'
def bullets(lines):return '[list]\n'+'\n'.join('[*]'+line for line in lines)+'\n[/list]'

def fork_sections(mod,body,lang):
    data=forks()[mod]
    if not data['version']:raise ValueError('No verified fork version: '+mod)
    # The old combined header included inherited functionality. Keep the actual
    # player guide intact; own changes are now rendered from the audited matrix.
    title='ЧТО ДЕЛАЕТ' if lang=='ru' else 'WHAT IT DOES'
    token='[h2]'+title+'[/h2]'
    if token not in body:raise ValueError('Missing original player guide: '+mod)
    guide=body[body.index(token):]
    if mod=='Formations':
        guide=guide.replace('ещё одиннадцать','ещё десять').replace('another eleven','another ten').replace('eleven more','ten more')
        guide=guide.replace('работает с любым списком модов и любым сейвом.','использует штатное следование игры.').replace('works with any mod list and any save.','uses the game\'s native following behavior.')
    if lang=='ru':
        credit='Оригинальный мод: [url='+data['original_url']+']'+data['name']+'[/url]. Автор: [b]'+data['author']+'[/b].\n\n[b]Спасибо '+data['author']+' за оригинальный мод![/b]\n\n'
        source='[b]Исходники оригинала:[/b] '+('[url='+data['source_url']+']'+data['source_url'].replace('https://github.com/','')+'[/url]' if data['source_url'] else data['source_note'][lang]+' [url='+data['original_url']+']Страница оригинала[/url].')
        own=section('ЧТО ИЗМЕНЕНО В НАШЕМ ФОРКЕ',bullets(data['changes'][lang]))
        history_title='ИСТОРИЯ ВЕРСИЙ НАШЕГО ФОРКА'
        first='Это первый выпуск нашего форка. Исправления до публикации входят в v1.'
    else:
        credit='Original mod: [url='+data['original_url']+']'+data['name']+'[/url]. Author: [b]'+data['author']+'[/b].\n\n[b]Thank you, '+data['author']+', for the original mod![/b]\n\n'
        source='[b]Original source code:[/b] '+('[url='+data['source_url']+']'+data['source_url'].replace('https://github.com/','')+'[/url]' if data['source_url'] else data['source_note'][lang]+' [url='+data['original_url']+']Original mod page[/url].')
        own=section('CHANGES IN OUR FORK',bullets(data['changes'][lang]))
        history_title='OUR FORK VERSION HISTORY'
        first='This is our first fork release. Fixes made before publication remain in v1.'
    history=[]
    for version,entry in sorted(data['history'].items(),key=lambda x:tuple(int(n) for n in x[0].split('.')),reverse=True):
        history.append('[b]v'+version+'[/b] — '+' '.join(entry[lang]))
    preface=first+'\n\n' if list(data['history'])==['1'] else ''
    return credit+source+'\n\n'+own+section(history_title,preface+bullets(history))+guide

def supported_section(lang):
    entries=mcm_catalog()
    def names(group,skip_self=False):return ', '.join(e['name'] for e in entries if e['group']==group and (not skip_self or e['id']!='ModConfigMenu'))
    other=', '.join(e['name']+' ('+e['author']+')' for e in entries if e['group']=='other_fork')
    if lang=='ru':
        rows=['[b]Наши плагины:[/b] '+names('own',True)+'.','[b]Наши форки Emkej8:[/b] '+names('emkej_fork')+'.','[b]Наши форки других авторов:[/b] '+other+'.']
        tail='У самого Mod Config Menu есть страница настроек. В список входят подготовленные и отключённые сборки; Organize the Inventory пока собран локально. Kill Counter — локальная сборка: разрешение автора на публикацию не подтверждено. Dynamic Factions поддерживает MCM, но ещё разрабатывается. XP Overhaul (Asur) — отдельная локальная сборка. Эти моды устанавливаются отдельно.'
        return section('ПОДДЕРЖИВАЕМЫЕ ПЛАГИНЫ И ФОРКИ',bullets(rows)+'\n\n'+tail)
    rows=['[b]Our plugins:[/b] '+names('own',True)+'.','[b]Our Emkej8 forks:[/b] '+names('emkej_fork')+'.','[b]Our forks of other authors:[/b] '+other+'.']
    tail='Mod Config Menu also has its own settings page. The list includes prepared and disabled builds; Organize the Inventory is currently a local build. Kill Counter is a local build; author permission to publish is unconfirmed. Dynamic Factions supports MCM but remains in development. XP Overhaul (Asur) is a separate local build. These mods are installed separately.'
    return section('SUPPORTED PLUGINS AND FORKS',bullets(rows)+'\n\n'+tail)

def apply_sections(table,items):
    registered={item['mod']:item for item in items}
    for mod,data in forks().items():
        if data['version'] is None:continue
        if mod not in table or mod not in registered:raise ValueError('Missing fork description/metadata: '+mod)
        if data['version']!=registered[mod]['version']:raise ValueError('Wrong fork release version: '+mod)
        table[mod]=tuple(fork_sections(mod,body,lang) for lang,body in zip(('ru','en'),table[mod]))
    pair=[]
    for lang,body in zip(('ru','en'),table['ModConfigMenu']):
        title='КАКИЕ МОДЫ' if lang=='ru' else 'WHICH MODS'
        marker='[h2]'+title+'[/h2]';start=body.index(marker);end=body.index('[h2]',start+len(marker))
        body=body[:start]+supported_section(lang)+body[end:]
        pair.append(body.replace('Slinkysalmon 03','Slinkysalmon03'))
    table['ModConfigMenu']=tuple(pair)
