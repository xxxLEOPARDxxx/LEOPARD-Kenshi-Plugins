"""Offline canonical RGBA pins; only this module's ignored outputs are written."""
from pathlib import Path
import hashlib,json,struct
from PIL import Image,ImageStat
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True);GAME=Path('D:/SteamLibrary/steamapps/common/Kenshi');CAT=HERE.parents[2]/'tools/diagnostics/satellite-capture-probe/region-catalog/catalog.json'
def sha(p):
 h=hashlib.sha256()
 with p.open('rb')as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return h.hexdigest()
assert sha(CAT)=='6e274871f3a0d894db352306a5fb6b1f004e685a75897cf8e897fc47e53bb454'
c=json.loads(CAT.read_text());entries=c['painted']+[next(v for v in c['identities']if v['name']=='blend')];pins=[];lines=['#pragma once','namespace WorldBank {','struct RgbaPin{unsigned bytes;unsigned char sha[32];const char*path;unsigned encodedBytes;const char*encodedSha;};','static const RgbaPin RgbaPins[129]={']
for i,v in enumerate(entries):
 p=GAME/v['path'];assert p.stat().st_size==v['bytes'] and sha(p)==v['sha256'],p
 with p.open('rb')as f:h=f.read(33)
 side=1024 if i<64 or i==128 else 2048
 assert h[:8]==b'\x89PNG\r\n\x1a\n'and h[12:16]==b'IHDR'and struct.unpack('>IIBB',h[16:26])==(side,side,8,6)
 with Image.open(p)as opened:
  image=opened.convert('RGBA');raw=image.tobytes();means=ImageStat.Stat(image).mean
 digest=hashlib.sha256(raw).hexdigest();pins.append(dict(index=i,path=v['path'],encodedBytes=v['bytes'],encodedSha256=v['sha256'],decodedBytes=len(raw),decodedSha256=digest,mean=means))
 lines.append('{%du,{%s},%s,%du,%s},'%(len(raw),','.join(map(str,bytes.fromhex(digest))),json.dumps(v['path']),v['bytes'],json.dumps(v['sha256'])))
 if i in [27,54,91,118,128]:(OUT/('rgba-%d.bin'%i)).write_bytes(raw)
 if i%32==0:print('Authenticated RGBA',i,'/129',flush=True)
lines+=['};','}'];(OUT/'DecodedPins.h').write_text('\n'.join(lines)+'\n');(OUT/'decoded-pins.json').write_text(json.dumps(dict(scope='canonical base RGBA source profiles; not current native winner proof',catalogSha256=sha(CAT),pins=pins),indent=2)+'\n');print('PASS129 encoded/decoded RGBA pins; only five bounded fixture blobs saved')
