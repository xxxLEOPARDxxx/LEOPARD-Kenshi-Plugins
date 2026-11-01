import collections, hashlib, json, pathlib, re, struct, sys
sys.path.insert(0, 'D:/DEV/Kenshi/tools')
from moddiff import records, fields, TYPE_NAMES
from leopard_difficulty import serialize
ROOT=pathlib.Path(__file__).parent
SOURCES={
 'upstream': pathlib.Path('D:/SteamLibrary/steamapps/workshop/content/233860/Driftmetal Remnants/Driftmetal Remnants.mod'),
 # The old translation/patch are immutable inputs, even after installing the update.
 'installed_ru': pathlib.Path('D:/DEV/Kenshi/backup/20261101-driftmetal/installed-merged-ru/Driftmetal Remnants RU.mod'),
 'animation_patch': pathlib.Path('D:/DEV/Kenshi/backup/20261101-driftmetal/installed-animation-patch/Driftmetal Remnants Anim Patch.mod'),
}
def header(data,start):
 p=12; out={}
 for key in ['author','description','dependencies','references']:
  n=struct.unpack_from('<i',data,p)[0];p+=4
  out[key]=data[p:p+n].decode('utf-8');p+=n
 out['tail_hex']=data[p:start-4].hex();out['start']=start
 out['v']=struct.unpack_from('<i',data)[0]
 return out
def main():
 allrecs={}; manifest={}
 for name,path in SOURCES.items():
  data=path.read_bytes();recs=records(str(path));body=b''.join(map(serialize,recs));start=len(data)-len(body)
  assert data[start:]==body,(name,'roundtrip')
  manifest[name]={'path':str(path),'sha256':hashlib.sha256(data).hexdigest(),'size':len(data),'records':len(recs),'header':header(data,start)}
  allrecs[name]={r['string_id']:r for r in recs}
  (ROOT/(name+'.json')).write_text(json.dumps(recs,ensure_ascii=False,indent=2),encoding='utf-8')
  print(name,len(recs),collections.Counter(TYPE_NAMES.get(r['type']) for r in recs)); print(manifest[name]['header'])
 old=allrecs['installed_ru'];new=allrecs['upstream']; changes=[]
 for sid in new.keys()|old.keys():
  o,n=old.get(sid),new.get(sid)
  if o is None or n is None:changes.append({'sid':sid,'kind':'added' if o is None else 'removed','name':(n or o)['name'],'type':TYPE_NAMES.get((n or o)['type'])});continue
  fo,fn=fields(o),fields(n);diff=[{'field':list(k),'old':fo.get(k),'new':fn.get(k)} for k in sorted(fo.keys()|fn.keys()) if fo.get(k)!=fn.get(k)]
  if o['name']!=n['name']:diff.insert(0,{'field':['name'],'old':o['name'],'new':n['name']})
  if diff:changes.append({'sid':sid,'kind':'changed','name':n['name'],'type':TYPE_NAMES.get(n['type']),'fields':diff})
 (ROOT/'source-manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2),encoding='utf-8')
 (ROOT/'installed-vs-upstream.json').write_text(json.dumps(changes,ensure_ascii=False,indent=2),encoding='utf-8')
 print('diff',collections.Counter(c['kind'] for c in changes)); print('fields',collections.Counter(tuple(d['field'][:2]) for c in changes for d in c.get('fields',[])))
if __name__=='__main__':main()
