"""Install the verified LoK update while the game is closed; retain local extras."""
from pathlib import Path
from datetime import datetime, timezone, timedelta
import hashlib, json, os, shutil, subprocess

GAME=Path('D:/SteamLibrary/steamapps/common/Kenshi')
SOURCE=Path('D:/SteamLibrary/steamapps/workshop/content/233860/Legendary of Kenshi')
STAGE=Path('D:/DEV/Kenshi/build/lok-update-20261113')
MOD=GAME/'mods/Legendary of Kenshi'
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def plain(path,root):
    assert path.absolute().is_relative_to(root.absolute()),str(path)
    cursor=path
    while cursor!=root.parent:
        if cursor.exists():assert not cursor.is_symlink() and not cursor.is_junction(),str(cursor)
        cursor=cursor.parent
def closed():
    count=subprocess.check_output(['powershell','-NoProfile','-Command',
        '@(Get-Process Kenshi* -ErrorAction SilentlyContinue).Count'],text=True).strip()
    assert count=='0','Kenshi running: prepared files retained, no installation'
delta=json.loads((STAGE/'delta.json').read_text(encoding='utf-8'))
translation=json.loads((STAGE/'translation-audit.json').read_text(encoding='utf-8'))
assert translation['missing_player_strings']==translation['stale_anchors']==translation['gameplay_overrides']==0
rows=[]
for row in delta['files']:
    if row['after'] is None:continue # keep local backups and unused old asset for third-party references
    path=Path(row['path']);assert not path.is_absolute() and '..' not in path.parts
    source=SOURCE/path;target=MOD/path
    assert sha(source)==row['after']['sha256']
    if row['before']:assert sha(target)==row['before']['sha256']
    else:assert not target.exists()
    rows.append((source,target))
rows += [(STAGE/'Legendary of Kenshi RU.mod',GAME/'mods/Legendary of Kenshi RU/Legendary of Kenshi RU.mod'),
         (STAGE/'NCE-candidate.mod',GAME/'mods/THE_NCE_Patch/THE_NCE_Patch.mod')]
assert len(rows)==9
assert sha(rows[-2][0])==translation['overlay_sha256']
namefix=json.loads((STAGE/'nce-name-fix-audit.json').read_text(encoding='utf-8'))
closed()
now=datetime.now(timezone(timedelta(hours=3)))
backup=Path('D:/DEV/Kenshi/backup')/('LoK-before-update-'+now.strftime('%Y%m%d-%H%M%S'))
backup.mkdir(exist_ok=False)
protected=[GAME/'data/mods.cfg',GAME/'mods/KEO/KEO.dll',GAME/'mods/KEO/KEO.ini']
protected += [GAME/'mods'/name/(name+'.mod') for name in
    ('Legendary of Kenshi All Anim patch','Legendary of Kenshi All Robots Module Patch')]
held={str(p):sha(p) for p in protected}
before={str(p):sha(p) if p.exists() else None for _,p in rows}
for source,target in rows:
    plain(source,STAGE if source.is_relative_to(STAGE) else SOURCE);plain(target,GAME)
    if target.exists():
        saved=backup/target.relative_to(GAME);saved.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(target,saved)
staged=[];written=[]
try:
    for source,target in rows:
        target.parent.mkdir(parents=True,exist_ok=True)
        tmp=target.with_name(target.name+'.lok-update-20261113.tmp')
        assert not tmp.exists()
        shutil.copy2(source,tmp);assert sha(tmp)==sha(source);staged.append(tmp)
    closed()
    for (source,target),tmp in zip(rows,staged):
        os.replace(tmp,target);written.append(target)
        assert sha(target)==sha(source)
    assert all(sha(Path(p))==h for p,h in held.items())
    for relative,row in delta['original_manifests']['downloaded'].items():
        assert sha(MOD/relative)==row['sha256'],relative
except BaseException:
    for target in reversed(written):
        saved=backup/target.relative_to(GAME)
        if saved.exists():shutil.copy2(saved,target)
        elif target.exists():target.unlink()
    raise
finally:
    for tmp in staged:
        if tmp.exists():tmp.unlink()
receipt=dict(status='installed_locally',time_msk=now.isoformat(),backup=str(backup),
    changed_files=[dict(path=str(t.relative_to(GAME)),before_sha256=before[str(t)],after_sha256=sha(t)) for _,t in rows],
    protected_files=held,all_downloaded_files_verified=len(delta['original_manifests']['downloaded']),
    local_extra_files_preserved=[r['path'] for r in delta['files'] if r['after'] is None],
    game_process_count_before_install=0,publication=False)
(STAGE/'installation.json').write_text(json.dumps(receipt,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps({k:receipt[k] for k in ('status','time_msk','backup','all_downloaded_files_verified','local_extra_files_preserved')},ensure_ascii=False,indent=2))
print('Installed9files; KEO test DLL/INI, mods.cfg and companion patches unchanged')
