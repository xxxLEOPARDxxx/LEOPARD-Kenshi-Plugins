import json,re,sys
from inspect_sources import ROOT,TYPE_NAMES
sys.stdout.reconfigure(encoding='utf8')
old={r['string_id']:r for r in json.loads((ROOT/'installed_ru.json').read_text(encoding='utf8'))}
new={r['string_id']:r for r in json.loads((ROOT/'upstream.json').read_text(encoding='utf8'))}
rows=[]
for sid,r in new.items():
 o=old.get(sid,{});os=dict(o.get('str',[]));ns=dict(r['str'])
 if r['name']!=o.get('name'):rows.append([sid,TYPE_NAMES[r['type']],'name',r['name'],o.get('name','')])
 for k,v in r['str']:
  if not v or k.startswith('_original'):continue
  if re.search(r'text\d+$',k) or k in ['description','Description','difficulty','style','building category','inventory content name','company description']:
   rows.append([sid,TYPE_NAMES[r['type']],k,v,os.get(k,'')])
(ROOT/'text-review.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2),encoding='utf8')
lo=int(sys.argv[1]) if len(sys.argv)>1 else 0; hi=int(sys.argv[2]) if len(sys.argv)>2 else len(rows)
print('TOTAL',len(rows))
for i,row in enumerate(rows[lo:hi],lo):print(i,' | '.join(row))
