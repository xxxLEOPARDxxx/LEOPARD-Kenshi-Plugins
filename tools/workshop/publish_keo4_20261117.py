"""Publish only the verified KEO4 after the owner's Nexus confirmation.

Uses the existing logged-in Steam client IPC and uploader. An existing receipt
must be inspected before retry: an interrupted submission may have succeeded.
"""
from pathlib import Path
import ctypes,hashlib,json,os,subprocess,sys
HERE=Path(__file__).resolve().parent
WORK=Path('D:/DEV/Kenshi/workshop')
OURS=HERE.parents[1]
REPORT=OURS/'docs/releases/2026-11-17-keo4-publication.json'
sys.path.insert(0,str(HERE))
import publish_keo_20261105 as p
from publish_loading_20261117 import BoundedSteam,verified_query

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def save(data):REPORT.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

def main():
 if REPORT.exists():raise RuntimeError('Receipt exists; inspect remote state before any retry')
 subprocess.run([sys.executable,str(HERE/'verify_keo4_navmesh.py')],check=True,stdout=subprocess.DEVNULL)
 p.validate_descriptions.validate({'KEO'})
 meta=json.loads((WORK/'KEO/item.json').read_text(encoding='utf-8'))
 assert (meta['version'],meta['nexus'],meta['workshop_id'])==('4',2271,3834042449)
 assert meta['title']=='KEO (MCM fork) (QoL)'
 archive=WORK/'nexus/KEO-4.zip';dll=WORK/'KEO/content/KEO.dll'
 assert sha(archive)=='11c0249c080979afa49667f086ba16e2f1b8d945d0f5946bb998d17707dea358'
 assert sha(dll)=='137bd0a9398f1eadfea261d84ea58ba1deeac3f1fde6f63abe28259fe94375d0'
 note=p.u.change_note('KEO','4');assert 'English' in note and 'Русский' in note
 protected={str(f):sha(f) for f in (Path(p.u.KENSHI)/'mods/KEO/KEO.ini',Path(p.u.KENSHI)/'data/mods.cfg')}
 state=dict(mod='KEO',version='4',nexus=2271,workshop_id=3834042449,
  nexus_status='published_confirmed_by_owner',confirmed_msk=p.stamp(),steam_status='preflight',
  archive_sha256=sha(archive),dll_sha256=sha(dll),bilingual_change_note_sha256=hashlib.sha256(note.encode()).hexdigest(),
  protected_before=protected)
 save(state)
 # The Nexus publication is confirmed. The normal uploader records the same
 # note after Steam; the journal writer deduplicates identical mod/version/text.
 subprocess.run([sys.executable,str(WORK/'personal_changelog.py'),'--published','--mod','KEO','--version','4','--text',p.journal.russian(note)],check=True)
 st=None
 try:
  st=BoundedSteam()
  user=p.bind(st,'SteamAPI_ISteamClient_GetISteamUser',ctypes.c_void_p,ctypes.c_void_p,ctypes.c_int,ctypes.c_int,ctypes.c_char_p)(st.SteamClient(),st.HUser(),st.HPipe(),b'SteamUser019')
  assert user and p.bind(st,'SteamAPI_ISteamUser_GetSteamID',ctypes.c_uint64,ctypes.c_void_p)(user)==p.OWNER
  assert p.bind(st,'SteamAPI_ISteamUtils_GetAppID',ctypes.c_uint32,ctypes.c_void_p)(st.utils)==233860
  state['remote_before']=p.query(st,3834042449,'english');save(state)
  state['steam_status']='submitting';save(state)
  p.u.upload(st,'KEO',note)
  state['steam_status']='submitted';save(state)
  state['remote_after']={}
  for lang,file in [('english','description_en.txt'),('russian','description_ru.txt')]:
   title=meta.get('title_ru',meta['title']) if lang=='russian' else meta['title']
   state['remote_after'][lang]=verified_query(st,3834042449,lang,(WORK/'KEO'/file).read_text(encoding='utf-8'),title)
   save(state)
  state.update(steam_status='published_verified',verified_msk=p.stamp());save(state)
  for folder in (WORK,HERE):
   path=folder/'published_releases.json';ledger=json.loads(path.read_text(encoding='utf-8'))
   assert ledger['mods']['KEO']['version']=='3'
   ledger['date']=state['verified_msk'][:10]
   ledger['mods']['KEO'].update(version='4',nexus=2271,workshop_id=3834042449,
    archive_sha256=state['archive_sha256'],dll_sha256=state['dll_sha256'],published_msk=state['verified_msk'],
    bilingual_change_note_sha256=state['bilingual_change_note_sha256'])
   path.write_text(json.dumps(ledger,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
   path=folder/'mcm_catalog.json';catalog=json.loads(path.read_text(encoding='utf-8'))
   for row in catalog:
    if row['package']=='KEO':
     row['published_version']='4'
     if 'nexus_published_version' in row:row['nexus_published_version']='4'
   path.write_text(json.dumps(catalog,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
  assert p.journal.JOURNAL.read_text(encoding='utf-8').count('Мод: KEO | Версия: 4\n\n')==1
  assert all(sha(Path(n))==digest for n,digest in protected.items())
  state['status']='publication_ledger_and_one_journal_verified';save(state)
  print('KEO4 published and verified: EN/RU, one change note, ledger, one journal; personal preferences unchanged',flush=True)
 finally:
  if st is not None:st.Shutdown()

if __name__=='__main__':main()
