"""Refresh publication text only; leave DLLs/content/screens and versions alone."""
from pathlib import Path
import json
import subprocess
import sys
import build

HERE=Path(__file__).resolve().parent

def main():
    names=set(sys.argv[1:])
    if not names:raise ValueError('Specify the mods whose descriptions should be refreshed')
    items=[item for item in build.ITEMS if item['mod'] in names]
    if names-{item['mod'] for item in items}:raise ValueError('Unknown mod')
    subprocess.run([sys.executable,str(HERE/'desc/make_desc.py'),*sorted(names)],check=True)
    for item in items:
        stage=HERE/item['mod'];stage.mkdir(exist_ok=True)
        ru,en=build.render_descriptions(item)
        if max(len(ru.encode('utf-8')),len(en.encode('utf-8')))>build.LIMIT:raise ValueError('Steam description exceeds limit: '+item['mod'])
        for lang,text in (('ru',ru),('en',en)):(stage/('description_'+lang+'.txt')).write_text(text,encoding='utf-8')
        path=stage/'item.json'
        if path.exists():
            meta=json.loads(path.read_text(encoding='utf-8'))
            if meta['version']!=item['version']:raise ValueError('Do not change versions during text refresh')
            meta['description_bytes']=max(len(ru.encode('utf-8')),len(en.encode('utf-8')))
            path.write_text(json.dumps(meta,ensure_ascii=False,indent=1)+'\n',encoding='utf-8')
    for script in ('nexus_desc.py','validate_descriptions.py'):
        subprocess.run([sys.executable,str(HERE/script),*sorted(names)],check=True)

if __name__=='__main__':main()
