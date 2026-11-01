"""Read-only check of complete EN/RU Steam, Nexus and version-note outputs."""
from pathlib import Path
import importlib.util
import json
import re
import sys
import build
from release_metadata import render_note
from publication_sections import forks, mcm_catalog, OUR_REPOSITORY_URL
from mcm_inventory import verify_catalog

HERE=Path(__file__).resolve().parent

def balanced(text,path):
    if re.search(r'\[/?h\s+2\]',text):raise ValueError('Malformed heading tag: '+str(path))
    stack=[]
    for match in re.finditer(r'\[(/?)(h2|b|i|list|code|url|size)(?:=[^\]]*)?\]',text):
        closing,tag=match.groups()
        if closing:
            if not stack or stack.pop()!=tag:raise ValueError('Unbalanced BBCode: '+str(path))
        else:stack.append(tag)
    if stack:raise ValueError('Unclosed BBCode: '+str(path))

def player_feature_claims(mod,text,path):
    if mod=='ModConfigMenu':
        required=('https://github.com/xxxLEOPARDxxx/LEOPARD-Kenshi-Plugins',
                  'https://github.com/xxxLEOPARDxxx/LEOPARD-Kenshi-Forks')
        if not all(url in text for url in required):raise ValueError('MCM must retain both central source repositories: '+str(path))
    elif OUR_REPOSITORY_URL.search(text):
        raise ValueError('Our repository link belongs only in the MCM description: '+str(path))
    # User, 2026-10-31: this vanilla behavior is not a UI Tweaks feature.
    # README/version history can document the investigation separately.
    if mod=='UITweaks' and re.search(r'Sequential squad trading|Последовательный обмен',text,re.I):
        raise ValueError('Vanilla squad trading advertised as a plugin feature: '+str(path))
    if mod=='UITweaks':
        if re.search(r'\bPNG\b|lossless|1600\s*[x×]\s*900|\bpixels?\b|без потерь|пиксел',text,re.I):
            raise ValueError('Save-preview implementation details in player guide: '+str(path))
        for heading,preview,height in (('WHAT IT DOES','Save previews.','Save list height.'),
                                        ('ЧТО ДЕЛАЕТ','Превью сохранений.','Высота списка сохранений.')):
            if heading in text and (text.count('[b]'+preview+'[/b]')!=1 or text.count('[b]'+height+'[/b]')!=1):
                raise ValueError('Expected one concise preview feature and one list-height feature: '+str(path))

def validate(names):
    notes=json.loads((HERE/'release_notes.json').read_text(encoding='utf-8'))
    history=json.loads((HERE/'changelog.json').read_text(encoding='utf-8'))
    # Import the generator without executing its CLI writer.
    spec=importlib.util.spec_from_file_location('player_description_source',HERE/'desc/make_desc.py')
    descriptions=importlib.util.module_from_spec(spec);spec.loader.exec_module(descriptions)
    checked=[]
    fork_data=forks()
    support=mcm_catalog()
    verify_catalog(support)
    plugin_notes={item['mod'] for item in build.ITEMS if item['mod'] in notes and item.get('plugin',True)}
    absent=plugin_notes-{entry['package'] for entry in support}
    if absent:raise ValueError('Current plugin releases missing from MCM catalog: '+', '.join(sorted(absent)))
    for item in build.ITEMS:
        mod=item['mod']
        if names and mod not in names:continue
        stage=HERE/mod
        if not stage.exists():continue
        nexus_path=HERE/'nexus'/(mod+'_description.txt')
        nexus=nexus_path.read_text(encoding='utf-8')
        balanced(nexus,nexus_path)
        player_feature_claims(mod,nexus,nexus_path)
        if '[line]' not in nexus or '[size=5][b]Русский[/b][/size]' not in nexus:raise ValueError('Missing Nexus language separator: '+mod)
        sizes={}
        for lang,index,heading in (('ru',0,'Что изменено в версии '),('en',1,"What's changed in version ")):
            path=stage/('description_'+lang+'.txt');text=path.read_text(encoding='utf-8')
            if '\ufffd' in text or len(text.encode('utf-8'))>build.LIMIT:raise ValueError('Damaged/oversized description: '+str(path))
            balanced(text,path);sizes[lang]=len(text.encode('utf-8'))
            player_feature_claims(mod,text,path)
            forbidden=('Что изменено в версии ',"What's changed in version ",'ИСТОРИЯ ВЕРСИЙ НАШЕГО ФОРКА','OUR FORK VERSION HISTORY')
            if any(value in text or value in nexus for value in forbidden):raise ValueError('Release changelog duplicated in player description: '+mod+' '+lang)
            if mod in notes and '[h2]' not in text:raise ValueError('Missing formatted sections: '+str(path))
            body=descriptions.T.get(mod,(None,None))[index]
            if body and body.strip() not in text:raise ValueError('Incomplete generated description: '+str(path))
            if mod in notes:
                note=notes[mod]
                if str(note['version'])!=str(item['version']):raise ValueError('Wrong note version: '+mod)
                if history.get(mod,{}).get(str(item['version']))!=render_note(note):raise ValueError('Stale bilingual changelog: '+mod)
            if mod in fork_data:
                fork=fork_data[mod]
                recorded={version for version in history.get(mod,{}) if version.isdigit() and int(version)<=int(fork['version'])}
                if recorded-set(fork['history']):raise ValueError('Fork history omits recorded versions: '+mod)
                headings=('ЧТО ИЗМЕНЕНО В НАШЕМ ФОРКЕ','Исходники оригинала:','Спасибо ') if lang=='ru' else ('CHANGES IN OUR FORK','Original source code:','Thank you, ')
                if not all(value in text for value in headings) or fork['author'] not in text:raise ValueError('Missing explicit fork credit/change/history section: '+mod+' '+lang)
                for url in (fork['original_url'],fork['source_url']):
                    if url and (url not in text or url not in nexus):raise ValueError('Missing original-source link: '+mod)
            if mod=='ModConfigMenu':
                for consumer in mcm_catalog():
                    if consumer['name'] not in text or consumer['name'] not in nexus:raise ValueError('Missing supported MCM page: '+consumer['name']+' '+lang)
            if mod=='AIFixed' and int(item['version'])>=2:
                credits=('Basusu','3787399736','3787375792','https://github.com/Basusu0/Basusu-kenshi-plugins','GPLv3')
                if not all(value in text and value in nexus for value in credits):raise ValueError('Missing AI collection author/original/source credit: '+lang)
                if ('Спасибо Basusu' if lang=='ru' else 'Thank you, Basusu') not in text:raise ValueError('Missing explicit AI collection gratitude: '+lang)
                if ('Исходники оригинала:' if lang=='ru' else 'Original source code:') not in text:raise ValueError('Missing separately labelled AI original sources: '+lang)
        checked.append({'mod':mod,'version':item['version'],'steam_bytes':sizes,'nexus_languages':['en','ru'],'separate_version_notes':mod in notes or mod in fork_data,'fork_history_versions':list(fork_data[mod]['history']) if mod in fork_data else [],'explicit_fork_credit':mod in fork_data})
    return checked

if __name__=='__main__':
    result=validate(set(sys.argv[1:]));print(json.dumps({'descriptions_checked':len(result),'results':result},ensure_ascii=False,indent=2))
