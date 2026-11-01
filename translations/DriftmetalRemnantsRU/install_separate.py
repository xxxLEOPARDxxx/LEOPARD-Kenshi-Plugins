"""One-time local migration to original + text-only RU + animation patch.

No Workshop edits, recursive deletes, save changes, or process termination.
All staged files are hashed before moving the old local folders into backup.
"""
import hashlib, json, os, pathlib, re, shutil, subprocess, sys
from inspect_sources import ROOT, SOURCES

GAME = pathlib.Path('D:/SteamLibrary/steamapps/common/Kenshi').resolve()
MODS = (GAME/'mods').resolve()
BACKUP = pathlib.Path('D:/DEV/Kenshi/backup/20261101-driftmetal').resolve()
STAGE = (GAME/'_codex_work/driftmetal-separate-20261101').resolve()

def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()

def inventory(folder):
    return {p.relative_to(folder).as_posix(): sha(p)
            for p in folder.rglob('*') if p.is_file()}

def require_closed():
    result = subprocess.run(['powershell.exe','-NoProfile','-Command',
        "if(Get-Process Kenshi* -ErrorAction SilentlyContinue){exit 9}"],
        capture_output=True, check=False)
    if result.returncode:
        raise RuntimeError('Kenshi must be closed before local installation')

def main():
    require_closed()
    assert not STAGE.exists(), 'Installation staging already exists; inspect it first'
    config = GAME/'data/mods.cfg'
    old_config = config.read_bytes()
    # Exact adjacent old entries only; preserve all other bytes and load order.
    pattern = rb'(?m)^Driftmetal Remnants RU\.mod(\r?\n)Driftmetal Remnants Anim Patch\.mod(?=\r?$)'
    matches = list(re.finditer(pattern, old_config))
    assert len(matches)==1 and b'Driftmetal Remnants.mod' not in old_config
    ending = matches[0].group(1)
    replacement = ending.join([b'Driftmetal Remnants.mod',
        b'Driftmetal Remnants Anim Patch.mod', b'Driftmetal Remnants RU.mod'])
    new_config = re.sub(pattern, lambda _: replacement, old_config)
    assert new_config.replace(replacement,matches[0].group(0),1)==old_config
    sources = json.loads((ROOT/'source-manifest.json').read_text(encoding='utf8'))
    for name, path in SOURCES.items():
        assert sha(path)==sources[name]['sha256'], (name,'source changed')
    previous = json.loads((ROOT/'backup-manifest.json').read_text(encoding='utf8'))
    for name,label in [('Driftmetal Remnants RU','installed-merged-ru'),
                       ('Driftmetal Remnants Anim Patch','installed-animation-patch')]:
        old = (MODS/name).resolve()
        assert old.parent==MODS and old.is_dir()
        expected = {p:v['sha256'] for p,v in previous[label]['files'].items()}
        assert inventory(old)==expected, (name,'local files changed since backup')
        assert not (BACKUP/('retired-'+name)).exists()
    assert not (MODS/'Driftmetal Remnants').exists()
    inputs = [('Driftmetal Remnants', SOURCES['upstream'].parent),
        ('Driftmetal Remnants RU',ROOT/'artifacts/separate/Driftmetal Remnants RU'),
        ('Driftmetal Remnants Anim Patch',ROOT/'artifacts/compatibility/Driftmetal Remnants Anim Patch')]
    wanted = {}
    for name,src in inputs:
        wanted[name] = inventory(src)
        shutil.copytree(src,STAGE/name)
        assert inventory(STAGE/name)==wanted[name], (name,'staging mismatch')
    require_closed()
    assert config.read_bytes()==old_config, 'mods.cfg changed during staging'
    (STAGE/'mods.cfg.before').write_bytes(old_config)
    (STAGE/'mods.cfg.after').write_bytes(new_config)
    for name in ['Driftmetal Remnants RU','Driftmetal Remnants Anim Patch']:
        old = (MODS/name).resolve()
        retired = (BACKUP/('retired-'+name)).resolve()
        assert old.parent==MODS and retired.parent==BACKUP
        shutil.move(str(old),str(retired))
    for name,_ in inputs:
        src=(STAGE/name).resolve(); dest=(MODS/name).resolve()
        assert src.parent==STAGE and dest.parent==MODS and not dest.exists()
        shutil.move(str(src),str(dest))
        assert inventory(dest)==wanted[name], (name,'installed mismatch')
    temp=config.with_name('mods.cfg.driftmetal.tmp')
    assert not temp.exists()
    temp.write_bytes(new_config)
    os.replace(temp,config)
    assert config.read_bytes()==new_config
    assert sha(SOURCES['upstream'])==sources['upstream']['sha256']
    report={'variant':'original English + standalone text-only Russian',
        'load_order':[name+'.mod' for name,_ in inputs if name!='Driftmetal Remnants RU']+['Driftmetal Remnants RU.mod'],
        'files':wanted,'config_sha256':sha(config),
        'old_config_sha256':hashlib.sha256(old_config).hexdigest(),
        'workshop_unchanged':True,'old_folders_preserved':True,
        'game_closed_at_install':True}
    (ROOT/'installation.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
    print(json.dumps({k:v for k,v in report.items() if k!='files'},ensure_ascii=False,indent=2))

if __name__=='__main__':
    sys.stdout.reconfigure(encoding='utf8')
    main()
