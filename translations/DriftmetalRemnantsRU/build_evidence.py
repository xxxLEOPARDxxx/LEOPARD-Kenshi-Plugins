import collections,json,pathlib,re,sys
from inspect_sources import ROOT,SOURCES,records,TYPE_NAMES
sys.stdout.reconfigure(encoding='utf8')
rs=records(str(SOURCES['upstream']));byid={r['string_id']:r for r in rs};old=records(str(SOURCES['installed_ru']))
def refs_to(sid,records_=rs):
 return [{'sid':r['string_id'],'name':r['name'],'type':TYPE_NAMES[r['type']],'category':cat,'value':it[1:]} for r in records_ for cat,items in r['refs'] for it in items if it[0]==sid]
def display(r):return {k:v for k,v in r.items() if k not in ['instances','_end','vec3','vec4']}
out={'walkers':[],'research':[],'items':[],'retired_B_incoming_old':refs_to('492-Driftmetal Remnants.mod',old),'retired_B_incoming_new':refs_to('492-Driftmetal Remnants.mod'),'humanoid_races':[]}
def sid(n):return str(n)+'-Driftmetal Remnants.mod'
for number,record,pack,dialog,item,check,join in [(40,35,412,414,428,408,411),(22,43,500,484,429,443,445),(87,44,501,485,430,478,481)]:
 walker=byid[sid(record)]
 parents=refs_to(sid(record));squads=[x['sid'] for x in parents if x['category']=='animals']
 out['walkers'].append({'number':number,'record':display(walker),'incoming':parents,'squad_records':[display(byid[x]) for x in squads if x in byid],'squad_incoming':{x:refs_to(x) for x in squads},'dialogue_package':display(byid[sid(dialog)]),'item_condition':display(byid[sid(check)]),'join_effect':display(byid[sid(join)]),'item_sid':sid(item)})
for r in rs:
 if r['type']==21:out['research'].append(display(r))
 if r['string_id'] in [sid(n) for n in [428,429,430]]:out['items'].append({'record':display(r),'incoming':refs_to(r['string_id'])})
 if r['type']==7:out['humanoid_races'].append({'sid':r['string_id'],'name':r['name'],'incoming':refs_to(r['string_id']),'refs':r['refs']})
(ROOT/'walker-evidence.json').write_text(json.dumps(out,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
for w in out['walkers']:
 print('WALKER',w['number'],w['incoming'])
 print('SPAWN',w['squad_incoming'])
 print('INVENTORY',[(it[0],byid.get(it[0],{}).get('name'),byid.get(it[0],{}).get('refs')) for cat,items in w['record']['refs'] if cat=='inventory' for it in items])
print('QUESTITEMS')
for i in out['items']:print(i['record']['name'],i['incoming'])
print('OLD B',out['retired_B_incoming_old']);print('NEW B',out['retired_B_incoming_new'])
for x in out['humanoid_races']:print('RACE',x)
