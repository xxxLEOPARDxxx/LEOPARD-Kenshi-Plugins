"""Canonical local release preparation. Does not upload to Nexus or Steam."""
from pathlib import Path
import os,sys,subprocess,shutil,json,zipfile
HERE=Path(__file__).resolve().parent
import build
from release_metadata import sync_notes
def pack_ranged_damage_xp_source(source, destination):
    source = Path(source)
    project = source.parent
    sdk = project.parent / 'KenshiLib-src'
    archive = Path(destination) / 'RangedDamageXp-source.zip'
    temporary = archive.with_suffix('.tmp')
    with zipfile.ZipFile(temporary, 'w', zipfile.ZIP_DEFLATED) as z:
        paths = [source / name for name in (
            'RangedDamageXp.cpp', 'BoltTrails.cpp', 'BoltTrails.h', 'BoltTrailHistory.h',
            'BoltTrailRibbon.h', 'RangedDamageXp.ini', 'LICENSE', 'ORIGIN.md', 'BUILD.md',
            'README.txt', 'build.ps1', 'tests/bolt_trail_history.cpp',
        )]
        for language in ('en_GB', 'ru_RU', 'zh_CN'):
            paths.append(source / 'locale' / language / 'LC_MESSAGES/ranged_damage_xp.po')
        for path in paths:
            z.write(path, 'RangedDamageXp/' + path.relative_to(source).as_posix())
        for name in ('IniDefaults.h', 'Localization.h', 'LocalizationLog.h', 'ModConfigMenu.h'):
            z.write(project / 'shared' / name, 'shared/' + name)
        z.write(sdk / 'LICENSE', 'KenshiLib/LICENSE')
        for path in sorted((sdk / 'Include').rglob('*')):
            if path.is_file() and (path.suffix in {'.h', '.inl', '.MIT'} or path.name == 'AUTHORS'):
                z.write(path, 'KenshiLib/' + path.relative_to(sdk).as_posix())
    with zipfile.ZipFile(temporary) as z:
        if z.testzip() is not None:
            raise RuntimeError('Ranged Damage Xp source archive CRC failed')
    os.replace(temporary, archive)


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
            if mod=='RangedDamageXp':
                pack_ranged_damage_xp_source(src, dst)
            if mod=='ItemMarkers':
                sdk=Path(dst)/'sdk';sdk.mkdir(exist_ok=True)
                shutil.copy2(Path(src).parent/'shared'/'ItemMarkersApi.h',sdk/'ItemMarkersApi.h')
                for name in ['README.txt','GradeProviderExample.cpp']:
                    shutil.copy2(Path(src)/'sdk'/name,sdk/name)
                grades=Path(dst)/'grades';grades.mkdir(exist_ok=True)
                # Only our disabled sample; never overwrite/delete player profiles.
                shutil.copy2(Path(src)/'grades'/'backpack-example.ini.disabled',grades/'backpack-example.ini.disabled')
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
