"""Independent semantic verification: original data + text overlay == merged build."""
import collections,copy,hashlib,json,pathlib,re,struct,sys,zipfile
from inspect_sources import ROOT,SOURCES,records,fields,serialize,header
sys.stdout.reconfigure(encoding='utf8')
SECTIONS=('bool','float','int','vec3','vec4','str','file','refs','inst')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def read(p):
 data=p.read_bytes();rs=records(str(p));body=b''.join(map(serialize,rs));start=len(data)-len(body)
 assert data[start:]==body
 assert struct.unpack_from('<i',data,4)[0]==start-16
 assert struct.unpack_from('<i',data,start-4)[0]==len(rs)
 return {r['string_id']:r for r in rs},header(data,start)
def main():
 result=json.loads((ROOT/'build-result.json').read_text(encoding='utf8'));paths=[ROOT/x['path'] for x in result['products']]
 orig,_=read(SOURCES['upstream']);full,_=read(paths[0]);overlay,oh=read(paths[1]);anim,ah=read(paths[2]);old,_=read(SOURCES['installed_ru'])
 assert set(orig)==set(full)
 catalog=json.loads((ROOT/'translation-catalog.json').read_text(encoding='utf8'));cat={(x['sid'],x['field']):x for x in catalog}
 for sid,o in orig.items():
  f=full[sid]
  assert all(o[s]==f[s] for s in SECTIONS if s!='str'),(sid,'non-text change')
  assert (o['type'],o['id'],o['change'])==(f['type'],f['id'],f['change'])
  expected=copy.deepcopy(o)
  if sid in overlay:
   e=overlay[sid];assert e['type']==o['type'] and e['id']==0
   assert all(not e[s] for s in SECTIONS if s!='str'),(sid,'overlay contains non-text')
   es=dict(e['str']);fs=dict(o['str'])
   for k,v in e['str']:
    if k.startswith('_original_'):
     real=k[len('_original_'):];assert v==(o['name'] if real=='name' else dict(o['str'])[real]),(sid,k,'stale source anchor')
    else:assert k in fs;fs[k]=v
   if '_original_name' in es:expected['name']=e['name'];assert e['change']==51
   else:assert e['name']==o['name'] and e['change']==49
   expected['str']=[(k,fs[k]) for k,_ in o['str']]
  assert expected['name']==f['name'] and expected['str']==f['str'],(sid,'full vs overlay mismatch')
 assert oh['dependencies']=='Driftmetal Remnants.mod' and oh['references']==''
 assert ah['dependencies']=='Driftmetal Remnants.mod'
 expected_races={sid for sid,r in orig.items() if r['type']==7 and sid!='28-Driftmetal Remnants.mod'}
 assert set(anim)==expected_races
 audit=json.loads((ROOT/'compatibility-audit.json').read_text(encoding='utf8'))
 for sid,r in anim.items():
  assert r['change']==49 and r['name']==orig[sid]['name']
  assert all(not r[s] for s in SECTIONS if s!='refs')
  for catname,items in r['refs']:
   assert catname=='animation files' and len(items)==(9 if sid=='1135-Driftmetal Remnants.mod' else 8)
   for target,*v in items:
    if target=='14-CBT Leisure Objects and Furniture.mod':assert sid=='1135-Driftmetal Remnants.mod' and target not in audit['animation_targets'] and v==[2147483647]*3
    else:assert target in audit['animation_targets'] and v==[0,0,0]
 revised=json.loads((ROOT/'translation-revisions.json').read_text(encoding='utf8'));allow={(x['sid'],x['field']) for x in revised if x['was_previous_translation']}
 preserved=0;removed=[]
 for sid,r in old.items():
  vals=[('name',r['name'])]+r['str']
  for k,v in vals:
   if not re.search('[А-Яа-яЁё]',v):continue
   if sid not in orig:removed.append([sid,k,v]);continue
   en=orig[sid]['name'] if k=='name' else dict(orig[sid]['str']).get(k)
   if v==en:continue
   assert (sid,k) in cat,(sid,k,'lost previous translation')
   if (sid,k) not in allow:assert cat[sid,k]['ru']==v,(sid,k,'modified previous translation');preserved+=1
 # Ensure every actual upstream asset remains byte-identical in the full product.
 assets=0
 for p in SOURCES['upstream'].parent.rglob('*'):
  if p.is_file() and p.suffix!='.mod':
   target=paths[0].parent/p.relative_to(SOURCES['upstream'].parent)
   assert sha(target)==sha(p),str(p);assets+=1
 report={'passed':True,'upstream_records':len(orig),'merged_records':len(full),'overlay_records':len(overlay),'translation_fields':len(cat),'preserved_previous_fields':preserved,'reviewed_previous_field_corrections':len(allow),'retired_previous_fields':removed,'animation_races':len(anim),'animation_refs_per_race':8,'byte_identical_upstream_assets':assets,'installed_unchanged':{name:sha(p)==json.loads((ROOT/'source-manifest.json').read_text(encoding='utf8'))[name]['sha256'] for name,p in SOURCES.items()},'products':result['products']}
 assert all(report['installed_unchanged'].values())
 (ROOT/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
 print(json.dumps(report,ensure_ascii=False,indent=2))
if __name__=='__main__':main()
