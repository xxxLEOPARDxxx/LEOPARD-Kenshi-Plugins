import json,sys,re
from inspect_sources import ROOT,TYPE_NAMES
sys.stdout.reconfigure(encoding='utf8')
recs=json.loads((ROOT/'upstream.json').read_text(encoding='utf8'));byid={r['string_id']:r for r in recs}
need=set()
for r in recs:
 if (r['type'] in [18,73] and re.search('walker',r['name'],re.I)) or (r['type'] in [76,21,4]):need.add(r['string_id'])
def compact(r):
 return {k:v for k,v in r.items() if v and k not in ['instances','_end','id','change','file','vec3','vec4']}
def expand(sid,depth):
 if sid not in byid:return
 r=byid[sid];print(json.dumps(compact(r),ensure_ascii=False))
 if depth:
  for cat,items in r['refs']:
   if r['type'] in [18,73,19] or cat in ['dialogue','dialogue package','dialogue package player','dialogue package AI']:
    for it in items:expand(it[0],depth-1)
for sid in sorted(need):expand(sid,5)
