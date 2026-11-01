import json,sys,pathlib
from inspect_sources import ROOT,SOURCES,records
sys.stdout.reconfigure(encoding='utf8')
base={}
for n in ['gamedata.base','rebirth.mod','Newwworld.mod','Dialogue.mod']:
 for r in records('D:/SteamLibrary/steamapps/common/Kenshi/data/'+n):base[r['string_id']]=r
for r in records(str(SOURCES['upstream'])):base[r['string_id']]=r
out={}
for sid,r in base.items():
 if r['type']==21 and sid.endswith('Driftmetal Remnants.mod'):
  d={'name':r['name'],'int':dict(r['int']),'bool':dict(r['bool']),'refs':{cat:[{'sid':t,'name':base.get(t,{}).get('name'),'values':[a,b,c]} for t,a,b,c in items] for cat,items in r['refs']}}
  out[sid]=d
(ROOT/'research-resolved.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
for sid,r in out.items():print(sid,r)
targets={'486-Driftmetal Remnants.mod','686-Driftmetal Remnants.mod','685-Driftmetal Remnants.mod','56146-rebirth.mod','64855-rebirth.mod'}
incoming={t:[{'sid':r['string_id'],'name':r['name'],'category':cat,'values':[a,b,c]} for r in base.values() for cat,items in r['refs'] for x,a,b,c in items if x==t] for t in targets}
(ROOT/'quest-locations-resolved.json').write_text(json.dumps(incoming,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
print('QUEST LOOT INCOMING',json.dumps(incoming,ensure_ascii=False))
