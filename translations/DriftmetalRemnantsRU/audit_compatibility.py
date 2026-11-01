import collections,json,pathlib,sys
from inspect_sources import ROOT,SOURCES,records,TYPE_NAMES
sys.stdout.reconfigure(encoding='utf8')
GAME=pathlib.Path('D:/SteamLibrary/steamapps/common/Kenshi')
original={r['string_id']:r for r in records(str(SOURCES['upstream']))}
patch=records(str(SOURCES['animation_patch']))
raceids={r['string_id'] for r in original.values() if r['type']==7}|{r['string_id'] for r in patch}
needed={it[0] for r in patch for cat,items in r['refs'] for it in items}
needed|={it[0] for r in original.values() if r['type']==7 for cat,items in r['refs'] if cat=='animation files' for it in items}
out={'race_edits':[],'animation_targets':{},'quest_sources':[]}
for folder in (GAME/'mods').iterdir():
 if not folder.is_dir():continue
 for p in folder.glob('*.mod'):
  data=p.read_bytes()
  if not any(x.encode() in data for x in needed|raceids):continue
  try:rs=records(str(p))
  except BaseException as e:out.setdefault('parse_errors',[]).append([str(p),str(e)]);continue
  for r in rs:
   sid=r['string_id']
   if sid in needed:out['animation_targets'].setdefault(sid,[]).append({'mod':p.name,'name':r['name'],'type':r['type'],'files':r['file']})
   if sid in raceids:out['race_edits'].append({'mod':p.name,'record':r})
(ROOT/'compatibility-audit.json').write_text(json.dumps(out,ensure_ascii=False,indent=2),encoding='utf8')
for x in out['race_edits']:print('race',x['mod'],x['record']['string_id'],x['record']['name'],dict(x['record']['refs']).get('animation files'))
for sid in sorted(needed):print('animation',sid,out['animation_targets'].get(sid,'MISSING'))
print('errors',out.get('parse_errors'))
