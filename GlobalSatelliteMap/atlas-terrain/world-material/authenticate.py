"""Authenticate existing immutable snapshots and real case assets; write only here."""
from pathlib import Path
import hashlib,json
import math
from PIL import Image
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
PROBE=HERE.parents[2]/'tools/diagnostics/satellite-capture-probe';CAT=PROBE/'region-catalog';GAME=Path('D:/SteamLibrary/steamapps/common/Kenshi');SNAP=Path('D:/DEV/Kenshi/research/global-satellite-20261031/biome-current-36248')
def sha(p):
 h=hashlib.sha256()
 with p.open('rb')as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return h.hexdigest()
pins={CAT/'catalog.json':'6e274871f3a0d894db352306a5fb6b1f004e685a75897cf8e897fc47e53bb454',CAT/'RegionCatalogData.h':'bd85da4b86a5e13bafd3ab9793c9a5f7ac3047488e57c813e430ade5d40f426f',CAT/'source-manifest.json':'69b7a25921afa65cecbeef898bd2ca80eca0eeaaf16237790c8b3477337a4023',SNAP/'active-biomes.json':'511a8813928114dbf6b2697d5fa46a46e858c46690907cf8493e092c84633d0f',SNAP/'biome-selection-snapshot.json':'5cb085baa67a7100181daf4edf25bdf3e47f5c162f7ef551fa8eb5910e565d60'}
for p,h in pins.items():assert sha(p)==h,p
validator=CAT/'RegionCatalog.h'
assert sha(validator)=='b62a97f0ad4fe4c01966eb43a672e4df9298ce51c566ab2db8931f4b90f5aefb'
pins[validator]=sha(validator)
COMPACT=HERE.parent/'compact-material'
for p,h in [(COMPACT/'_source/GeographyTerrain.hlsl','f9326a91f29e1f1644fc0acbb33bc126bce04590faef2cc7b89dabd1292fd9a1'),(COMPACT/'_source/GeographyVertex.hlsl','6ef9efa2c8027da354f812b3f59bfa055c48edcfa4a2057782bd7416f58d54e9'),(COMPACT/'_generated/contract.json','afe79b4152ba35cc364e4f7c92a59771a428d4dec2d63217d442bfba2993f0b9')]:
 assert sha(p)==h,p
 pins[p]=h
c=json.loads((CAT/'catalog.json').read_text());a=c['assets'];plans=[];expected=[]
for ri,r in enumerate(c['regions']):
 for ci,cell in enumerate(r['cells']):
  codes=[v for v in cell['selector'] if v&15];ids=[r['biomes'][(v&15)-1]['source'] for v in codes];assert 1<=len(ids)<=4
  paths=sorted(set(x for sid in ids for k in range(2)for x in c['biomes'][sid]['sources'][k]))
  pools=[sorted(set(a[x]['sha256'] for sid in ids for x in c['biomes'][sid]['sources'][k]))for k in range(2)]
  dds=len(pools[0])*5592384+len(pools[1])*2796192
  plan=dict(region=[ri%8,ri//8],regionIndex=ri,cell=ci,nativeSelector=cell['selector'],nativeChannels=[v>>4 for v in codes],nativeSidOrder=[c['biomes'][sid]['sid'] for sid in ids],sources=ids,sourceAssets=paths,diffuseLayers=len(pools[0]),normalLayers=len(pools[1]),ddsBytes=dds,monoSourceBytes=dds+33554500,stageSourceBytes=88080260)
  plans.append(plan);expected.append('{%du,{%s},{%s},%du,%du,%du},'%(len(ids),','.join(map(str,ids+[0]*(4-len(ids)))),','.join(map(str,[v>>4 for v in codes]+[0]*(4-len(ids)))),len(pools[0]),len(pools[1]),len(paths)))
(OUT/'Expected.h').write_text('struct ExpectedCell{unsigned count,ids[4],channels[4],diffuse,normal,paths;};\nstatic const ExpectedCell expected[1024]={\n'+'\n'.join(expected)+'\n};\n')
cases=[plans[27*16+10],plans[54*16+3]];checked={}
def verify(p,h,size=None):
 if size is not None:assert p.stat().st_size==size,p
 assert sha(p)==h,p
 checked[str(p)]=dict(sha256=h,bytes=p.stat().st_size)
for case in cases:
 for index in case['sourceAssets']:
  v=a[index];verify(GAME/v['path'],v['sha256'],v['bytes'])
 for k in range(2):
  v=c['painted'][k*64+case['regionIndex']];verify(GAME/v['path'],v['sha256'],v['bytes'])
# Also authenticate the actual adjacent painted pair for the worst-cell region edge.
for k in range(2):
 v=c['painted'][k*64+55];verify(GAME/v['path'],v['sha256'],v['bytes'])
for v in c['identities']:
 if v['name'] in ['blend','deferred/terrainfp4.hlsl','deferred/terrain.hlsl','deferred/gbuffer.hlsl']:verify(GAME/v['path'],v['sha256'],v['bytes'])
verify(GAME/'data/materials/common/wet.hlsl','e9042fbf57df1463d014761576701dd3c7e8680f70c89adcb9196ffdb5580d11')
# Current atlas metadata is observed, never used as a winner/geometry-generation substitute.
atlas={}
for name in ['capture.json','ready.json']:
 p=GAME/'mods/GlobalSatelliteMap/captures'/name;v=json.loads(p.read_text());atlas[name]=dict(bytes=p.stat().st_size,sha256=sha(p),keys=sorted(v.keys()))
def bilinear(image,world,rx,rz):
 side=image.width;x=(world[0]-(-147456+rx*36864-18))/36900*side-.5;y=(world[1]-(-147456+rz*36864-18))/36900*side-.5
 ix,iy=math.floor(x),math.floor(y);u,v=x-ix,y-iy
 return [sum(image.getpixel(((ix+dx)%side,(iy+dy)%side))[k]*(u if dx else 1-u)*(v if dy else 1-v)for dx in range(2)for dy in range(2))for k in range(4)]
edge=[]
for kind in range(2):
 samples=[]
 for rx in [6,7]:
  p=c['painted'][kind*64+6*8+rx]
  with Image.open(GAME/p['path'])as opened:
   image=opened.convert('RGBA');samples.append([bilinear(image,(110592,73728+(i+.5)*9216/16),rx,6)for i in range(16)])
 delta=max(abs(x-y)for arow,brow in zip(*samples)for x,y in zip(arow,brow));edge.append(dict(kind=kind,left=[6,6],right=[7,6],worldX=110592,zRange=[73728,82944],samples=16,maxSameWorldRawPaintByteDifference=delta,claim='source sampling measurement; no forced equality or renderer-seam acceptance'))
report=dict(scope='authenticated offline base-snapshot CPU plans, NOT active native winner proof',pins={str(p):h for p,h in pins.items()},checkedRealAssets=checked,cases=cases,allCellCount=len(plans),fourSidCells=sum(len(p['sources'])==4 for p in plans),worst=max(plans,key=lambda p:p['ddsBytes']),allPlans=plans,paintedEdge=edge,observedActiveAtlas=atlas,limits=dict(cap=201326592,sourceDdsLayer9mips=[5592384,2796192],onePaintedPairFullMips=27962024,globalBlendFullMips=5592404,selectorInterior=72,oneFixed6x6SidBank=50331456,twoRgba32fPingPongSets=4194304))
(OUT/'authenticated-plans.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS authenticated',len(checked),'actual case source files;',len(plans),'cell plans;',report['fourSidCells'],'four-SID cells;worstDDS',report['worst']['ddsBytes'],'bytes')
