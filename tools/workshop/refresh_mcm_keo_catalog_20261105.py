"""Add newly published KEO to the live MCM catalog; no content/version update."""
from pathlib import Path
from datetime import datetime,timezone,timedelta
import ctypes,json,re
import publish_keo_20261105 as p
MOD=3813761584
def main():
    receipt=json.loads(p.REPORT.read_text(encoding='utf-8'))
    assert receipt['steam_status']=='published_verified' and receipt['workshop_id']==3834042449
    st=p.Steam();rows={}
    try:
        for lang,label in [('english','[b]Our forks of other authors:[/b]'),('russian','[b]Наши форки других авторов:[/b]')]:
            d=p.query(st,MOD,lang);old=d['description'];lines=old.splitlines()
            assert re.search(r'(?:Version|Версия) 12\b',old),'Only published MCM12 description may be amended'
            matching=[i for i,line in enumerate(lines) if label in line];assert len(matching)==1
            i=matching[0]
            if 'KEO (' in lines[i]:rows[lang]={'already_present':True};continue
            lines[i]=lines[i].rstrip().removesuffix('.')+', KEO (Jakematt2004 / jacobericson).'
            new='\n'.join(lines)
            assert len(new.encode('utf-8'))<7900
            assert all(name not in new for name in ['Global Satellite','Dynamic Factions','XP Overhaul'])
            h=st.StartUpdate(st.ugc,p.u.APPID,MOD)
            assert st.SetLanguage(st.ugc,h,lang.encode()) and st.SetDescription(st.ugc,h,new.encode('utf-8'))
            r=st.wait(st.Submit(st.ugc,h,None),p.u.SubmitItemUpdateResult,3404)
            assert r.result==1 and not r.needs_agreement
            verify=p.query(st,MOD,lang);assert verify['description'].strip()==new.strip()
            rows[lang]={'before':old,'after':new,'verified':True,'version':'12','content_uploaded':False,'changelog_created':False}
    finally:st.Shutdown()
    out={'mod':'ModConfigMenu','operation':'published description catalog only','time_msk':datetime.now(timezone(timedelta(hours=3))).isoformat(),'added':'KEO / 3834042449','languages':rows,'runtime_update':False,'new_version':False}
    path=p.O/'docs/releases/2026-11-05-mcm-keo-catalog-only.json';path.write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print('MCM12 live catalog: KEO added and verified in English/Russian; no runtime/changelog/version change')
if __name__=='__main__':main()
