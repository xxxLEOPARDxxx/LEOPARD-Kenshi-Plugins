"""Read-only check of complete EN/RU Steam, Nexus and version-note outputs."""
from pathlib import Path
import importlib.util
import json
import re
import sys
import build
from release_metadata import render_note
from publication_sections import forks, mcm_catalog
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
        if '[line]' not in nexus or '[size=5][b]Русский[/b][/size]' not in nexus:raise ValueError('Missing Nexus language separator: '+mod)
        sizes={}
        for lang,index,heading in (('ru',0,'Что изменено в версии '),('en',1,"What's changed in version ")):
            path=stage/('description_'+lang+'.txt');text=path.read_text(encoding='utf-8')
            if '\ufffd' in text or len(text.encode('utf-8'))>build.LIMIT:raise ValueError('Damaged/oversized description: '+str(path))
            balanced(text,path);sizes[lang]=len(text.encode('utf-8'))
            if mod in notes and '[h2]' not in text:raise ValueError('Missing formatted sections: '+str(path))
            body=descriptions.T.get(mod,(None,None))[index]
            if body and body.strip() not in text:raise ValueError('Incomplete generated description: '+str(path))
            if mod in notes:
                note=notes[mod]
                if str(note['version'])!=str(item['version']):raise ValueError('Wrong note version: '+mod)
                if mod not in fork_data and heading+str(item['version']) not in text:raise ValueError('Missing visible version section: '+str(path))
                for line in note[lang]:
                    if line not in text or line not in nexus:raise ValueError('Missing translated change: '+mod+' '+lang+' '+line)
                if history.get(mod,{}).get(str(item['version']))!=render_note(note):raise ValueError('Stale bilingual changelog: '+mod)
            if mod in fork_data:
                fork=fork_data[mod]
                recorded={version for version in history.get(mod,{}) if version.isdigit() and int(version)<=int(fork['version'])}
                if recorded-set(fork['history']):raise ValueError('Fork history omits recorded versions: '+mod)
                headings=('ЧТО ИЗМЕНЕНО В НАШЕМ ФОРКЕ','ИСТОРИЯ ВЕРСИЙ НАШЕГО ФОРКА','Исходники оригинала:','Спасибо ') if lang=='ru' else ('CHANGES IN OUR FORK','OUR FORK VERSION HISTORY','Original source code:','Thank you, ')
                if not all(value in text for value in headings) or fork['author'] not in text:raise ValueError('Missing explicit fork credit/change/history section: '+mod+' '+lang)
                for url in (fork['original_url'],fork['source_url']):
                    if url and (url not in text or url not in nexus):raise ValueError('Missing original-source link: '+mod)
                for version,entry in fork['history'].items():
                    if '[b]v'+version+'[/b]' not in text:raise ValueError('Missing real fork version: '+mod+' v'+version)
                    for line in entry[lang]:
                        if line not in text or line not in nexus:raise ValueError('Missing fork history detail: '+mod+' v'+version+' '+lang)
            if mod=='ModConfigMenu':
                for consumer in mcm_catalog():
                    if consumer['name'] not in text or consumer['name'] not in nexus:raise ValueError('Missing supported MCM page: '+consumer['name']+' '+lang)
        checked.append({'mod':mod,'version':item['version'],'steam_bytes':sizes,'nexus_languages':['en','ru'],'version_notes':mod in notes or mod in fork_data,'fork_history_versions':list(fork_data[mod]['history']) if mod in fork_data else [],'explicit_fork_credit':mod in fork_data})
    return checked

if __name__=='__main__':
    result=validate(set(sys.argv[1:]));print(json.dumps({'descriptions_checked':len(result),'results':result},ensure_ascii=False,indent=2))
