"""Authenticate immutable control inputs; writes only this component's fixtures."""
from pathlib import Path
import hashlib,json,struct
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
ROOT=HERE.parents[2];GAME=Path('D:/SteamLibrary/steamapps/common/Kenshi')
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
snapshot=Path('D:/DEV/Kenshi/research/global-satellite-20261031/biome-current-36248/active-biomes.json')
assert sha(snapshot)=='511a8813928114dbf6b2697d5fa46a46e858c46690907cf8493e092c84633d0f'
catalog=ROOT/'tools/diagnostics/satellite-capture-probe/region-catalog/catalog.json';assert sha(catalog)=='6e274871f3a0d894db352306a5fb6b1f004e685a75897cf8e897fc47e53bb454'
c=json.loads(catalog.read_text());records={v['sid']:v for v in json.loads(snapshot.read_text())['records'] if v.get('sid')}
f32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]
def lit(x):
 s=format(f32(x),'.9g');return s+(''if'.'in s or'e'in s.lower()else'.0')+'f'
keys=[['tiling X 1','tiling Y 1','tiling X 2','tiling Y 2'],['tiling X 0','tiling Y 0','tiling X grass','tiling Y grass'],['tiling X dirt','tiling Y dirt','tiling X road','tiling Y road'],['slope min 1','slope min 2','slope min 3','slope min grass'],['slope max 1','slope max 2','slope max 3','slope max grass'],['slope fade 1','slope fade 2','slope fade 3','slope fade grass'],['overlay mult vertical','overlay mult grass','overlay mult dirt','overlay mult road'],[None]*4,['absorbance 0','absorbance 1','absorbance 2','absorbance grass'],['absorbance dirt','absorbance road',None,None]]
lines=['#pragma once','namespace RasterFixture {'];meta=[]
for label,region,cell in [('Two',27,10),('Four',54,3)]:
 p=[0.]*900;d=[0.]*60;r=c['regions'][region]
 for code in r['cells'][cell]['selector']:
  if not code&15:continue
  row=(code&15)-1;sid=c['biomes'][r['biomes'][row]['source']]['sid'];v=records[sid]['float']
  for ri,group in enumerate(keys):
   for j,key in enumerate(group):
    if key:p[row*60+ri*4+j]=f32(f32(v.get(key,0))*f32(.01 if 3<=ri<=5 else 1))
  p[row*60+40]=f32(v['brightness fix']);w=f32(v['distort wavelength']);d[row*4:row*4+4]=[f32(1/w)if w>0 else 0,f32(f32(v['distort amplitude'])*f32(.01)),0,0]
  # Test-only monolithic role indices for the two-SID reference are supplied
  # separately by the already frozen compact fixture, not inferred here.
 lines+=['static const float '+label+'Parameters[900]={'+','.join(map(lit,p))+'};','static const float '+label+'Distortion[60]={'+','.join(map(lit,d))+'};']
 meta.append(dict(label=label,region=region,cell=cell,scope='immutable base snapshot, zero-fade control; no current native winner/weather receipt'))
lines+=['}'];(OUT/'Materials.h').write_text('\n'.join(lines)+'\n')
height=GAME/'data/newland/land/fullmap.tif';h=hashlib.sha256()
with height.open('rb')as f:
 for b in iter(lambda:f.read(65536),b''):h.update(b)
assert height.stat().st_size==536980180 and h.hexdigest()=='6f7d4bc7baaf80dbf8db91e6f367db71f87fe38f053a34108f192584c9637b18'
with height.open('rb')as f:
 for label,x,z,n in [('two',7294,7294,261),('four',14014,12478,133)]:
  data=bytearray()
  for row in range(n):
   f.seek(43686+2*((z+row)*16385+x));b=f.read(n*2);assert len(b)==n*2;data+=b
  (OUT/('height-'+label+'.u16')).write_bytes(data);meta.append(dict(height=label,minX=x,minZ=z,side=n,sha256=hashlib.sha256(data).hexdigest()))
(OUT/'fixtures.json').write_text(json.dumps(meta,indent=2)+'\n');print('PASS exact base material900/distortion60 and two bounded actual height fixtures')
