"""Publish owner-authorized Nexus 2271 / KEO v1 with the existing Steam uploader.

No Steam client launch, credentials, unrelated content or private-project publication.
An interrupted submission must be inspected remotely before invoking this again.
"""
from pathlib import Path
from datetime import datetime, timezone, timedelta
import ctypes, hashlib, json, sys, zipfile
W=Path('D:/DEV/Kenshi/workshop');O=Path('D:/DEV/Kenshi/ours')
sys.path.insert(0,str(W))
import steam_upload as u
import personal_changelog as journal
import validate_descriptions
from steam_pipe_upload import Steam
OWNER=76561198098571264
REPORT=O/'docs/releases/2026-11-05-keo-1-publication.json'
def stamp():return datetime.now(timezone(timedelta(hours=3))).isoformat()
def save(s):REPORT.write_text(json.dumps(s,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
def bind(st,name,result,*args):
    f=getattr(st.dll,name);f.restype=result;f.argtypes=list(args);return f
class QueryResult(ctypes.Structure):
    _pack_=8
    _fields_=[('handle',ctypes.c_uint64),('result',ctypes.c_int32),('returned',ctypes.c_uint32),('total',ctypes.c_uint32),('cached',ctypes.c_bool)]
class DetailFront(ctypes.Structure):
    _pack_=8
    _fields_=[('id',ctypes.c_uint64),('result',ctypes.c_int32),('file_type',ctypes.c_int32),('creator_app',ctypes.c_uint32),('consumer_app',ctypes.c_uint32),('title',ctypes.c_char*129),('description',ctypes.c_char*8000),('owner',ctypes.c_uint64),('created',ctypes.c_uint32),('updated',ctypes.c_uint32)]
def query(st,item,language):
    ptr=ctypes.c_void_p;u64=ctypes.c_uint64;u32=ctypes.c_uint32;b=ctypes.c_bool
    create=bind(st,'SteamAPI_ISteamUGC_CreateQueryUGCDetailsRequest',u64,ptr,ctypes.POINTER(u64),u32)
    release=bind(st,'SteamAPI_ISteamUGC_ReleaseQueryUGCRequest',b,ptr,u64)
    h=create(st.ugc,(u64*1)(item),1);assert h and h!=2**64-1
    try:
        assert bind(st,'SteamAPI_ISteamUGC_SetLanguage',b,ptr,u64,ctypes.c_char_p)(st.ugc,h,language.encode())
        assert bind(st,'SteamAPI_ISteamUGC_SetReturnLongDescription',b,ptr,u64,b)(st.ugc,h,True)
        r=st.wait(bind(st,'SteamAPI_ISteamUGC_SendQueryUGCRequest',u64,ptr,u64)(st.ugc,h),QueryResult,3401)
        assert r.result==1 and r.returned==1
        buf=ctypes.create_string_buffer(16384)
        assert bind(st,'SteamAPI_ISteamUGC_GetQueryUGCResult',b,ptr,u64,u32,ptr)(st.ugc,h,0,buf)
        d=DetailFront.from_buffer_copy(buf)
        assert d.id==item and d.owner==OWNER and d.result==1 and d.consumer_app==233860
        return dict(id=d.id,owner=str(d.owner),title=d.title.decode('utf-8'),description=d.description.decode('utf-8'),time_updated=d.updated)
    finally:release(st.ugc,h)
def main():
    validate_descriptions.validate({'KEO'})
    meta=json.loads((W/'KEO/item.json').read_text(encoding='utf-8'))
    assert meta['version']=='1' and meta['nexus']==2271 and meta['title']=='KEO (MCM fork) (QoL)'
    with zipfile.ZipFile(W/'nexus/KEO-1.zip') as z:
        assert z.testzip() is None and len(z.namelist())==11
        for n in z.namelist():assert z.read(n)==(W/'KEO/content'/n.removeprefix('KEO/')).read_bytes()
    state=json.loads(REPORT.read_text(encoding='utf-8')) if REPORT.exists() else {}
    assert state.get('steam_status') not in ('published_verified','submitting'), 'Inspect earlier submission before retry'
    state.update(mod='KEO',version='1',nexus=2271,nexus_status='published_confirmed_by_owner',steam_status='connecting',time_msk=stamp())
    save(state)
    st=Steam()
    try:
        user=bind(st,'SteamAPI_ISteamClient_GetISteamUser',ctypes.c_void_p,ctypes.c_void_p,ctypes.c_int,ctypes.c_int,ctypes.c_char_p)(st.SteamClient(),st.HUser(),st.HPipe(),b'SteamUser019')
        assert user and bind(st,'SteamAPI_ISteamUser_GetSteamID',ctypes.c_uint64,ctypes.c_void_p)(user)==OWNER
        assert bind(st,'SteamAPI_ISteamUtils_GetAppID',ctypes.c_uint32,ctypes.c_void_p)(st.utils)==233860
        # The owner already confirmed Nexus release and its journal entry.
        # Uploading the same release to another store must not duplicate it.
        original_record=journal.record_published
        def record_once(mod,version,here):
            text=journal.JOURNAL.read_text(encoding='utf-8')
            if f'Мод: {mod} | Версия: {version}\n\n' in text:return False
            return original_record(mod,version,here)
        journal.record_published=record_once
        state['steam_status']='submitting';save(state)
        u.upload(st,'KEO',u.change_note('KEO','1'))
        meta=json.loads((W/'KEO/item.json').read_text(encoding='utf-8'));item=int(meta['workshop_id'])
        state.update(workshop_id=item,workshop_url=f'https://steamcommunity.com/sharedfiles/filedetails/?id={item}',steam_status='verifying')
        save(state)
        remote={}
        for language,filename in [('english','description_en.txt'),('russian','description_ru.txt')]:
            d=query(st,item,language)
            assert d['description'].strip()==(W/'KEO'/filename).read_text(encoding='utf-8').strip()
            assert d['title']==meta['title']
            remote[language]=d
        state.update(steam_status='published_verified',verified_msk=stamp(),remote=remote)
        save(state)
        print(json.dumps({k:state[k] for k in ('mod','version','nexus','steam_status','workshop_url')},ensure_ascii=False))
    finally:st.Shutdown()
if __name__=='__main__':main()
