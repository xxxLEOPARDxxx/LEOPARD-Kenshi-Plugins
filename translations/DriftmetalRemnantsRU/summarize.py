import json,re,sys
sys.stdout.reconfigure(encoding='utf-8')
from inspect_sources import ROOT,TYPE_NAMES
old={r['string_id']:r for r in json.loads((ROOT/'installed_ru.json').read_text(encoding='utf8'))}
new={r['string_id']:r for r in json.loads((ROOT/'upstream.json').read_text(encoding='utf8'))}
cyr=re.compile('[А-Яа-яЁё]')
for sid,r in new.items():
 if sid not in old: print('ADDED',TYPE_NAMES[r['type']],r['name'],sid,'\n',json.dumps(r,ensure_ascii=False))
print('REMOVED',[(r['name'],sid) for sid,r in old.items() if sid not in new])
for sid,r in old.items():
 if sid not in new:continue
 n=new[sid];ns=dict(n['str']);os=dict(r['str'])
 for k,v in r['str']:
  if k.startswith('_original'):print('ORIGINAL',sid,k,v)
  elif ns.get(k)!=v and not cyr.search(v):print('NONCYR_TEXT_DIFF',sid,r['name'],k,repr(v),'=>',repr(ns.get(k)))
 if r['name']!=n['name'] and not cyr.search(r['name']): print('NONCYR_NAME_DIFF',sid,r['name'],'=>',n['name'])
print('RACES')
for sid,r in new.items():
 if r['type']==7:print(json.dumps(r,ensure_ascii=False))
