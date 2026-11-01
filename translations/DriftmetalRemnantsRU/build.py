"""Build staged Driftmetal RU products. Never installs or changes mods.cfg."""
import collections,copy,hashlib,json,pathlib,re,shutil,struct,sys,zipfile
from inspect_sources import ROOT,SOURCES,records,serialize,header
sys.stdout.reconfigure(encoding='utf8')
CYR=re.compile('[А-Яа-яЁё]')
SECTIONS=('bool','float','int','vec3','vec4','str','file','refs','inst')
OUT=ROOT/'artifacts'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def jsonout(name,value): (ROOT/name).write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
def pstr(s):
 b=s.encode('utf8');return struct.pack('<i',len(b))+b
def make_header(source,recs,description=None,dependencies=None,references=None,author=None):
 data=source.read_bytes();src=records(str(source));body=b''.join(map(serialize,src));start=len(data)-len(body);h=header(data,start)
 assert data[start:]==body
 body=data[8:12]+b''.join(pstr(v if v is not None else h[k]) for k,v in [('author',author),('description',description),('dependencies',dependencies),('references',references)])+bytes.fromhex(h['tail_hex'])+struct.pack('<i',len(recs))
 return struct.pack('<ii',17,len(body)-8)+body
def write_mod(path,recs,source,**kwargs):
 path.parent.mkdir(parents=True,exist_ok=True)
 data=make_header(source,recs,**kwargs)+b''.join(map(serialize,recs));path.write_bytes(data)
 back=records(str(path));assert len(back)==len(recs)
 assert data[-sum(len(serialize(r)) for r in back):]==b''.join(map(serialize,back))
 return path
def empty_edit(r):
 return dict(type=r['type'],id=0,name=r['name'],string_id=r['string_id'],change=49,**{s:[] for s in SECTIONS})
def main():
 manifest=json.loads((ROOT/'source-manifest.json').read_text(encoding='utf8'))
 for name,path in SOURCES.items():assert sha(path)==manifest[name]['sha256'],(name,'source changed; re-review before building')
 upstream=records(str(SOURCES['upstream']));old={r['string_id']:r for r in records(str(SOURCES['installed_ru']))};byid={r['string_id']:r for r in upstream};edits=[]
 translated=copy.deepcopy(upstream);updates=json.loads((ROOT/'update_ru.json').read_text(encoding='utf8'));catalog=[];preserved=[];revised=[]
 for r in translated:
  sid=r['string_id'];o=old.get(sid,{});en=byid[sid];os=dict(o.get('str',[]));es=dict(en['str']);dest=dict(r['str']);name=en['name']
  if o.get('name')!=name and CYR.search(o.get('name','')):name=o['name'];preserved.append([sid,'name',name])
  for k,v in os.items():
   if k in es and v!=es[k] and CYR.search(v):dest[k]=v;preserved.append([sid,k,v])
  for k,v in updates.get(sid,{}).items():
   assert k=='name' or k in es,(sid,k,'unknown translation field')
   prev=name if k=='name' else dest[k]
   if prev!=v:revised.append({'sid':sid,'field':k,'before':prev,'after':v,'was_previous_translation':CYR.search(prev) is not None})
   if k=='name':name=v
   else:dest[k]=v
  r['name']=name;r['str']=[(k,dest[k]) for k,_ in r['str']]
  edit=empty_edit(en)
  if name!=en['name']:
   edit['name']=name;edit['change']=51;edit['str'].append(('_original_name',en['name']));catalog.append({'sid':sid,'field':'name','en':en['name'],'ru':name})
  for k,v in r['str']:
   if v!=es[k]:
    edit['str'].extend([(k,v),('_original_'+k,es[k])]);catalog.append({'sid':sid,'field':k,'en':es[k],'ru':v})
  if edit['str']:edits.append(edit)
 jsonout('translation-catalog.json',catalog);jsonout('translation-revisions.json',revised)
 # Full merged variant intentionally keeps upstream folder/mod filename, preserving all asset paths and SIDs.
 fullfolder=OUT/'merged'/'Driftmetal Remnants'
 shutil.copytree(SOURCES['upstream'].parent,fullfolder,dirs_exist_ok=True)
 merged=write_mod(fullfolder/'Driftmetal Remnants.mod',translated,SOURCES['upstream'])
 overlay=write_mod(OUT/'separate'/'Driftmetal Remnants RU'/'Driftmetal Remnants RU.mod',edits,SOURCES['animation_patch'],author='xxxLEOPARDxxx (translation update); original translation retained',description='Russian translation for Driftmetal: Remnants by PARA. Text only. Load after Driftmetal Remnants and animation patches.\r\nРусский перевод Driftmetal: Remnants. Только текст. Загружать после оригинала и анимационных патчей.',dependencies='Driftmetal Remnants.mod',references='')
 # Retain the exact eight animation references proven to exist in this local collection.
 animrefs=copy.deepcopy(dict(records(str(SOURCES['animation_patch']))[0]['refs'])['animation files'])
 audit=json.loads((ROOT/'compatibility-audit.json').read_text(encoding='utf8'))
 anim=[]
 for r in upstream:
  if r['type']==7 and r['string_id']!='28-Driftmetal Remnants.mod':
   e=empty_edit(r);refs=copy.deepcopy(animrefs)
   for target,*_ in dict(r['refs']).get('animation files',[]):
    if target not in audit['animation_targets']:
     assert target=='14-CBT Leisure Objects and Furniture.mod',('unreviewed missing animation',target)
     refs.append((target,2147483647,2147483647,2147483647))
   e['refs']=[('animation files',refs)];anim.append(e)
 animation=write_mod(OUT/'compatibility'/'Driftmetal Remnants Anim Patch'/'Driftmetal Remnants Anim Patch.mod',anim,SOURCES['animation_patch'],author='Local compatibility update',description='Driftmetal humanoid animation compatibility: K, worker, E and KX. Requires the installed animation collection. No gameplay fields.',dependencies='Driftmetal Remnants.mod',references='AnimationOverhaul.mod,DigCombatAnimation.mod,Great Anims Mod.mod,GROUNDED2 Combat Animations.mod,Immersive non-combat animations.mod,More Combat Animation.mod')
 jsonout('build-result.json',{'products':[{'path':str(p.relative_to(ROOT)),'sha256':sha(p),'size':p.stat().st_size} for p in [merged,overlay,animation]],'upstream_records':len(upstream),'overlay_records':len(edits),'fields':len(catalog),'preserved_fields':len(preserved),'revisions':len(revised),'animation_races':len(anim)})
 print((ROOT/'build-result.json').read_text(encoding='utf8'))
if __name__=='__main__':main()
