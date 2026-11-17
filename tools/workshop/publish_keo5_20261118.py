"""Publish only verified KEO5 after the owner's Nexus confirmation.

Existing receipts prohibit blind retries; inspect Steam first if interrupted.
Uses the existing logged-in client IPC, never launches the client or the game.
"""
from pathlib import Path
import ctypes, hashlib, json, subprocess, sys

TOOLS=Path('D:/DEV/Kenshi/ours/tools/workshop')
OURS=TOOLS.parents[1]
WORK=Path('D:/DEV/Kenshi/workshop')
REPORT=OURS/'docs/releases/2026-11-18-keo5-publication.json'
sys.path.insert(0,str(TOOLS))
import publish_keo_20261105 as p
from publish_loading_20261117 import BoundedSteam, verified_query

def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def load(path): return json.loads(path.read_text(encoding='utf-8-sig'))
def save(state): REPORT.write_text(json.dumps(state,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

def main():
 if REPORT.exists(): raise RuntimeError('Receipt exists; inspect remote state before any retry')
 subprocess.run([sys.executable,str(TOOLS/'verify_keo5_compatibility.py')],check=True,stdout=subprocess.DEVNULL)
 p.validate_descriptions.validate({'KEO'})
 meta=load(WORK/'KEO/item.json')
 assert (meta['version'],meta['nexus'],meta['workshop_id'])==('5',2271,3834042449)
 assert meta['title']=='KEO (MCM fork) (QoL)'
 archive=WORK/'nexus/KEO-5.zip';dll=WORK/'KEO/content/KEO.dll'
 assert sha(archive)=='5461d23d4d4cca8bc7ae30d7a65aac4bca5bec03117c3a0f3d98d3126e4ec9a1'
 assert sha(dll)=='94a9593f34bfaafcebbb8e337a3d932076766ef7b9f93828d4e860e0b9e261c0'
 note=p.u.change_note('KEO','5');assert 'English'in note and 'Русский'in note
 protected={str(f):sha(f) for f in [Path(p.u.KENSHI)/'mods/KEO/KEO.ini',Path(p.u.KENSHI)/'mods/KEO/KEO.dll',Path(p.u.KENSHI)/'data/mods.cfg']}
 before={str(folder):{'ledger':load(folder/'published_releases.json'),'catalog':load(folder/'mcm_catalog.json')} for folder in [WORK,TOOLS]}
 assert all(row['ledger']['mods']['KEO']['version']=='4' for row in before.values())
 state=dict(mod='KEO',version='5',nexus=2271,workshop_id=3834042449,
  nexus_status='published_confirmed_by_owner',confirmed_msk=p.stamp(),steam_status='preflight',
  archive_sha256=sha(archive),dll_sha256=sha(dll),bilingual_change_note_sha256=hashlib.sha256(note.encode()).hexdigest(),
  protected_before=protected)
 save(state)
 # Nexus is confirmed, so this is a published release even before Steam upload.
 # The ordinary uploader's same-version journal call is idempotent.
 subprocess.run([sys.executable,str(WORK/'personal_changelog.py'),'--published','--mod','KEO','--version','5','--text',p.journal.russian(note)],check=True)
 st=None
 try:
  st=BoundedSteam()
  user=p.bind(st,'SteamAPI_ISteamClient_GetISteamUser',ctypes.c_void_p,ctypes.c_void_p,ctypes.c_int,ctypes.c_int,ctypes.c_char_p)(st.SteamClient(),st.HUser(),st.HPipe(),b'SteamUser019')
  assert user and p.bind(st,'SteamAPI_ISteamUser_GetSteamID',ctypes.c_uint64,ctypes.c_void_p)(user)==p.OWNER
  assert p.bind(st,'SteamAPI_ISteamUtils_GetAppID',ctypes.c_uint32,ctypes.c_void_p)(st.utils)==233860
  state['remote_before']=p.query(st,3834042449,'english');save(state)
  state['steam_status']='submitting';save(state)
  p.u.upload(st,'KEO',note)
  state['steam_status']='submitted';state['remote_after']={};save(state)
  for language,filename in [('english','description_en.txt'),('russian','description_ru.txt')]:
   title=meta.get('title_ru',meta['title']) if language=='russian' else meta['title']
   state['remote_after'][language]=verified_query(st,3834042449,language,(WORK/'KEO'/filename).read_text(encoding='utf-8'),title)
   save(state)
  state.update(steam_status='published_verified',verified_msk=p.stamp());save(state)
  for folder in [WORK,TOOLS]:
   path=folder/'published_releases.json';ledger=load(path)
   assert ledger['mods']['KEO']['version']=='4'
   ledger['date']=state['verified_msk'][:10]
   ledger['mods']['KEO'].update(version='5',nexus=2271,workshop_id=3834042449,
    archive_sha256=state['archive_sha256'],dll_sha256=state['dll_sha256'],published_msk=state['verified_msk'],
    bilingual_change_note_sha256=state['bilingual_change_note_sha256'])
   assert {k:v for k,v in ledger['mods'].items()if k!='KEO'}=={k:v for k,v in before[str(folder)]['ledger']['mods'].items()if k!='KEO'}
   path.write_text(json.dumps(ledger,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
   path=folder/'mcm_catalog.json';catalog=load(path)
   for row in catalog:
    if row['package']=='KEO':
     row['published_version']='5'
     if 'nexus_published_version'in row: row['nexus_published_version']='5'
   assert [row for row in catalog if row['package']!='KEO']==[row for row in before[str(folder)]['catalog']if row['package']!='KEO']
   path.write_text(json.dumps(catalog,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
  assert p.journal.JOURNAL.read_text(encoding='utf-8').count('Мод: KEO | Версия: 5\n\n')==1
  assert all(sha(Path(name))==digest for name,digest in protected.items())
  state['status']='publication_ledger_and_one_journal_verified';save(state)
  print('KEO5 published and verified: EN/RU, one change note, ledger, one journal; installation/preferences unchanged',flush=True)
 finally:
  if st is not None: st.Shutdown()

if __name__=='__main__': main()
