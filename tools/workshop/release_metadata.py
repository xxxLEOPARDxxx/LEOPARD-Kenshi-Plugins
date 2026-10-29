"""Maintain complete bilingual version notes without discarding older history."""
from pathlib import Path
import json
import re
import build

HERE=Path(__file__).resolve().parent

def render_note(note):
    blocks=[]
    for lang,title in (('en','English'),('ru','Русский')):
        lines=note.get(lang)
        if not isinstance(lines,list) or not lines or any(not isinstance(x,str) or not x.strip() for x in lines):
            raise ValueError('Missing complete '+lang+' release notes')
        blocks.append('v'+str(note['version'])+' - '+title+'\n'+'\n'.join('- '+re.sub(r'\[/?b\]','',x) for x in lines))
    return '\n\n'.join(blocks)

def sync_notes(items):
    notes=json.loads((HERE/'release_notes.json').read_text(encoding='utf-8'))
    descriptions=json.loads((HERE/'desc/qol-20261029.json').read_text(encoding='utf-8'))
    if set(notes)!=set(descriptions):raise ValueError('Descriptions and release notes list different mods')
    canonical=Path(build.OURS)/'tools/workshop/changelog.json'
    history_path=canonical if canonical.is_file() else HERE/'changelog.json'
    history=json.loads(history_path.read_text(encoding='utf-8'))
    for item in items:
        mod=item['mod'];version=str(item['version'])
        if mod in notes:
            if str(notes[mod]['version'])!=version:raise ValueError('Release-note version mismatch: '+mod)
            published=notes[mod].get('published_version')
            if published is not None and int(version)!=int(published)+1:
                raise ValueError('Prepared release must be published version +1: '+mod)
            if set(descriptions[mod])!={'en','ru'} or not all(descriptions[mod].values()):raise ValueError('Missing translated description: '+mod)
            history.setdefault(mod,{})[version]=render_note(notes[mod])
        elif not history.get(mod,{}).get(version):
            raise ValueError('Missing version changelog: '+mod+' '+version)
    data=json.dumps(history,ensure_ascii=False,indent=2)+'\n'
    (HERE/'changelog.json').write_text(data,encoding='utf-8')
    if canonical.is_file() and canonical.resolve()!=(HERE/'changelog.json').resolve():canonical.write_text(data,encoding='utf-8')
    return notes

if __name__=='__main__':
    import sys
    wanted=set(sys.argv[1:]);sync_notes([item for item in build.ITEMS if not wanted or item['mod'] in wanted])
