import json,sys
from inspect_sources import ROOT
sys.stdout.reconfigure(encoding='utf8')
rs=json.loads((ROOT/'upstream.json').read_text(encoding='utf8'))
for r in rs:
 if any(r['string_id'].startswith(x+'-') for x in sys.argv[1:]):
  print(json.dumps({k:v for k,v in r.items() if k not in ['_end','instances','vec3','vec4','file'] and (v or k in ['id','change'])},ensure_ascii=False))
