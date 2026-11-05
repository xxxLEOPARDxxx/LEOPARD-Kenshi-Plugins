"""Verify prepared KEO binary, export corresponding source, record local readiness."""
from pathlib import Path
import datetime,gettext,hashlib,json,subprocess,sys,zipfile
R=Path('D:/DEV/Kenshi/forks/KEO');O=Path('D:/DEV/Kenshi/ours');W=Path('D:/DEV/Kenshi/workshop')
D=Path('D:/DEV/Kenshi/research/keo-release-20261105')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
commit=subprocess.check_output(['git','-C',str(R),'rev-parse','HEAD'],text=True).strip()
assert commit=='d0de2d19f3ba88d9315dcfafa361507fe89ab288'
assert not subprocess.check_output(['git','-C',str(R),'diff','7609b76',commit,'--','src','tools','locale','KEO.ini'])
source=W/'nexus/KEO-1-source.zip';temporary=source.with_suffix('.zip.tmp')
subprocess.run(['git','-C',str(R),'archive','--format=zip','--prefix=KEO-source/','-o',str(temporary),commit],check=True)
with zipfile.ZipFile(temporary) as z:
    assert z.testzip() is None
    names=z.namelist();files=[n for n in names if not n.endswith('/')]
    assert len(names)==len(set(names))
    assert not any(Path(n).suffix.lower() in ('.exe','.dll','.obj','.pdb','.log') for n in files)
    assert all('..' not in Path(n).parts and not Path(n).is_absolute() for n in names)
    for n in ('README.md','README_UPSTREAM.md','BUILD_RELEASE.md','LICENSE','build_step4_noenv.bat','src/game/gog_profile_data.inc','leopard/steel_jobs/ORIGIN.md','leopard/carried_weight/ORIGIN.md'):
        assert 'KEO-source/'+n in files
    source_count=len(files)
temporary.replace(source)
binary=W/'nexus/KEO-1.zip'
with zipfile.ZipFile(binary) as z:
    assert z.testzip() is None
    names=z.namelist();assert len(names)==len(set(names))==11
    assert json.loads(z.read('KEO/RE_Kenshi.json'))=={'Plugins':['KEO.dll']}
    assert hashlib.sha256(z.read('KEO/KEO.dll')).hexdigest()=='ea56647bb504a5287ee2589b815d9b1ac1cef6e760d6921048420908ede86751'
    assert not any(n.lower().endswith(('.exe','.po','.log','.pdb')) or '99_useroverride' in n.lower() or Path(n).name in ('KEO.ini','KEOCompat.ini','mod-config.json') for n in names)
    catalogs=[n for n in names if n.endswith('.mo')];assert len(catalogs)==3
    langs={n.split('/locale/')[1].split('/')[0] for n in catalogs};assert langs=={'en_GB','ru_RU','zh_CN'}
    for n in names:
        assert z.read(n)==(W/'KEO/content'/n.removeprefix('KEO/')).read_bytes()
    readme=z.read('KEO/README.md')
    assert readme==(R/'README.md').read_bytes()
    for text in (readme, (W/'nexus/KEO_description.txt').read_bytes()):
        assert all(token not in text for token in (b'DynamicFactions',b'Dynamic Factions',b'Global Satellite',b'GlobalSatellite',b'XP_Overhaul'))

sys.path.insert(0,'D:/DEV/Kenshi/tools')
from qol_binary_check import PE
pe=PE(W/'KEO/content/KEO.dll');assert {'?startPlugin@@YAXXZ','MCM_Describe'}<=pe.exports()
sys.path.insert(0,'D:/DEV/Kenshi/tools/_release_python')
from PIL import Image
art={}
for name,p in [('steam',W/'steam-covers/KEO/000_LEOPARD_Steam_Cover.png'),('nexus',W/'images/KEO/000_LEOPARD_Nexus_Cover.png')]:
    with Image.open(p) as image: size=image.size
    assert (size[0]==size[1]) if name=='steam' else (size[0]>size[1])
    art[name]=dict(path=str(p),pixels=size,sha256=sha(p))
preview=next((W/'KEO').glob('preview.*'));assert preview.stat().st_size<=1000*1024
with Image.open(preview) as image:assert image.width==image.height
receipt=dict(version='1',status='prepared_not_published',time_msk=datetime.datetime.now(datetime.timezone(datetime.timedelta(hours=3))).isoformat(),binary_code_commit='7609b76e125ab0709a804325fc7e631064875eeb',source_archive_commit=commit,code_change='No new runtime changes: release packaging of the already built and locally accepted readiness fix; current pathfinding investigation explicitly deferred by owner.',archives=[dict(path=str(p),bytes=p.stat().st_size,sha256=sha(p),crc='passed') for p in (binary,source)],source_files=source_count,runtime_files=11,art=art,screenshots='Two generated covers ready; no gameplay screenshots supplied for this fork.',gog='Earlier test candidate accepted by tester; unified current code verified against pinned native Steam/GOG contracts, broad gameplay not tested.',publication='Nexus by owner first; Steam content prepared only. No publication and no ALL_CHANGELOG_MODS entry.')
dest=O/'docs/releases/2026-11-05-keo-1-prepared.json';dest.parent.mkdir(parents=True,exist_ok=True)
for p in (dest,D/'final-verification.json'):p.write_text(json.dumps(receipt,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
patch=subprocess.check_output(['git','-C',str(R),'format-patch','-1',commit,'--stdout'])
(O/'tools/maintenance/keo/release-guide-d0de2d1-20261105.patch').write_bytes(patch)
print(json.dumps(receipt,ensure_ascii=False,indent=2))
