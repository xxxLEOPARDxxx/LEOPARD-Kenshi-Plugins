"""Canonical local release preparation. Does not upload to Nexus or Steam."""
from pathlib import Path
import os,sys,subprocess,shutil,json
HERE=Path(__file__).resolve().parent
import build
from release_metadata import sync_notes
def main():
    only=set(sys.argv[1:]);items=[i for i in build.ITEMS if not only or i['mod'] in only]
    if only-{i['mod'] for i in items}:raise SystemExit('Unknown mod names: '+str(only-{i['mod'] for i in items}))
    sync_notes(items)
    for item in items:
        mod=item['mod'];dst=build.mod_dir(mod);src=build.source_dir(mod)
        (HERE/'images'/mod).mkdir(parents=True,exist_ok=True)
        if not dst:raise RuntimeError('Installed runtime missing: '+mod)
        if mod=='UITweaks':
            plugins=json.loads((Path(dst)/'RE_Kenshi.json').read_text(encoding='utf-8-sig')).get('Plugins',[])
            if any(Path(name).name.startswith(('LayerProbe','HookCount')) for name in plugins):
                raise RuntimeError('Remove diagnostic loader before preparing public UITweaks package')

        if src:
            for name in ['README.txt','README.md','ORIGIN.md','LICENSE']:
                p=Path(src)/name
                if p.is_file():shutil.copy2(p,Path(dst)/name)
        # build.py deletes only these generated directories. Validate first.
        for child in ['content','screens']:
            target=(HERE/mod/child).resolve()
            if not target.is_relative_to((HERE/mod).resolve()):raise RuntimeError('Unsafe generated-content path')
    names=[i['mod'] for i in items]
    env=dict(os.environ,PYTHONIOENCODING='utf-8')
    for script in ['desc/make_desc.py','build.py','nexus_desc.py','validate_descriptions.py','nexus_pack.py']:
        subprocess.run([sys.executable,str(HERE/script),*names],check=True,env=env)
    return 0
if __name__=='__main__':sys.exit(main())
