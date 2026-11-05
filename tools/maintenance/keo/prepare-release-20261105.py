"""Prepare KEO v1 locally; never publish or mutate live player settings."""
from pathlib import Path
import datetime, gettext, hashlib, json, os, shutil, subprocess, sys

ROOT=Path('D:/DEV/Kenshi')
SRC=ROOT/'forks/KEO'
OWN=ROOT/'ours'
W=ROOT/'workshop'
D=ROOT/'research/keo-release-20261105'
GAME=Path('D:/SteamLibrary/steamapps/common/Kenshi')
C=W/'release-inputs/KEO'
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def read(p): return json.loads(p.read_text(encoding='utf-8'))
def write(p,v): p.write_text(json.dumps(v,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

head=subprocess.check_output(['git','-C',str(SRC),'rev-parse','HEAD'],text=True).strip()
assert head in ('7609b76e125ab0709a804325fc7e631064875eeb','d0de2d19f3ba88d9315dcfafa361507fe89ab288')
dll=SRC/'build/KEO_step4_prod/KEO.dll'
assert sha(dll)=='ea56647bb504a5287ee2589b815d9b1ac1cef6e760d6921048420908ede86751'
protected=[GAME/'mods/KEO/KEO.ini',GAME/'mods/KEO/KEOCompat.ini',GAME/'mods/KEO/RE_Kenshi.json',GAME/'data/mods.cfg']
before={str(p):sha(p) for p in protected}

# The original README remains available in corresponding sources. Runtime gets
# a release guide, not the development notebook and old performance promises.
upstream=subprocess.check_output(['git','-C',str(SRC),'show','origin/main:README.md'])
(SRC/'README_UPSTREAM.md').write_bytes(upstream)
shutil.copy2(D/'README.md',SRC/'README.md')
C.mkdir(parents=True,exist_ok=True)
for a,b in [(dll,C/'KEO.dll'),(GAME/'mods/KEO/KEO.mod',C/'KEO.mod'),(SRC/'LICENSE',C/'LICENSE'),(SRC/'README.md',C/'README.md')]:shutil.copy2(a,b)
write(C/'RE_Kenshi.json',{'Plugins':['KEO.dll']})
for name,source in [('ORIGIN_Steel_jobs.md',SRC/'leopard/steel_jobs/ORIGIN.md'),('ORIGIN_Steel_weight.md',SRC/'leopard/carried_weight/ORIGIN.md')]:
    text=source.read_text(encoding='utf-8')
    # Retain provenance and restrictions; old task bookkeeping is not a guide.
    if name=='ORIGIN_Steel_jobs.md':
        text=text[:text.index('Host controls and static review')]+'The cyclic route and its ordinary save/load were checked in Steam gameplay. Broader production-delivery and GOG gameplay testing remains incomplete. Static review does not establish FPS or leak freedom.\n'
    else:
        text=text[:text.index('Focused controls:')]+'The feature is optional and off by default. Host checks do not replace gameplay testing.\n'
    (C/name).write_text(text,encoding='utf-8')
for lang in ('en_GB','ru_RU','zh_CN'):
    p=SRC/'build/KEO_step4_prod/locale'/lang/'LC_MESSAGES/keo.mo'
    if not p.is_file():
        matches=list((SRC/'build/KEO_step4_prod/locale'/lang).rglob('*.mo'))
        assert len(matches)==1; p=matches[0]
    with p.open('rb') as stream: catalog=gettext.GNUTranslations(stream)._catalog
    assert len([k for k in catalog if k])>=68
    dst=C/'locale'/lang/'LC_MESSAGES'/p.name;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,dst)

bodies=read(D/'description_bodies.json');notes=read(D/'release_notes.json');fork=read(D/'fork_entry.json')
fork['author']='Jakematt2004 (jacobericson)'
for root in (OWN/'tools/workshop',W):
    for name,value in [('desc/qol-20261029.json',bodies),('release_notes.json',notes),('fork_catalog.json',fork)]:
        path=root/name;table=read(path);table['KEO']=value;write(path,table)
    path=root/'mcm_catalog.json';table=read(path);entries=[e for e in table if e['package']=='KEO'];assert len(entries)==1
    entries[0].update(release_authorized=True,public=False,status='release_candidate',group='other_fork',author=fork['author'])
    write(path,table)

env=dict(os.environ,KENSHI_RELEASE_INPUT_ROOT=str(W/'release-inputs'),PYTHONUTF8='1',PYTHONIOENCODING='utf-8')
subprocess.run([sys.executable,'-X','utf8','-B',str(W/'prepare_release.py'),'KEO'],env=env,check=True)
assert before=={str(p):sha(p) for p in protected},'Live settings changed during packaging'
write(D/'preparation.json',dict(status='prepared_not_published',version='1',time_msk=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=3))).isoformat(),binary_source_commit='7609b76e125ab0709a804325fc7e631064875eeb',dll_sha256=sha(dll),live_configs_preserved=before,steam=read(W/'KEO/item.json'),runtime='Steam city-reload and cyclic-route/save-load accepted by owner; earlier isolated GOG build accepted by tester, broad gameplay testing incomplete',tests='Native Steam/GOG weight31; jobs108 factory+RTTI contracts; GOG2676 profile/114 resolver plus rejection mutant; no new gameplay code in this packaging'))
print('KEO v1 prepared, live settings preserved, no publication')
