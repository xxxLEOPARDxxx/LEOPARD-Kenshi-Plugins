"""Fixed source/data closure for one compact two-SID native material control."""
from pathlib import Path
import hashlib,json,struct,subprocess,re
from PIL import Image,ImageStat
from prepare_sources import prepare_sources
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
prepare_sources(HERE)
GAME=Path('D:/SteamLibrary/steamapps/common/Kenshi');REPO=HERE.parents[2]
FXC=Path('C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe')
case=json.loads((HERE/'case-sources.json').read_text())
def sha(b):return hashlib.sha256(b).hexdigest()
native_pins={'deferred/terrainfp4.hlsl':'050de26aa6082155fe95bf027b5783b86d87ae6cc03381d9c761b731a88327d7','deferred/terrain.hlsl':'7bde0c5f6aa81d542b5c25925b28508a937d5086effeb959a9b0c5b96f11eb07','deferred/gbuffer.hlsl':'d6a525658f224379269369c9adaa928fc82a72285aed5cffb21d077df4c9c943','common/wet.hlsl':'e9042fbf57df1463d014761576701dd3c7e8680f70c89adcb9196ffdb5580d11','common/shadowcaster.hlsl':'dde7939a53c5d9c3f79c97d12bf044fb005f93f3f352175148ead7a841cd12f7'}
for name,pin in native_pins.items():assert sha((GAME/'data/materials'/name).read_bytes())==pin,name
assert case['region']==[3,3] and case['cell']==10 and case['selector']==[18,4,0,0]
assert [b['sid']for b in case['biomes']]==['2743-Newwworld.mod','48690-Newwworld.mod']
def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]
def literal(x):
 s=format(f32(x),'.9g');return s+('' if '.' in s or 'e' in s.lower() else '.0')+'f'
assets={r['sha256']:r for b in case['biomes'] for r in b['roles']}
ordered=sorted(assets.values(),key=lambda a:(a['kind'],a['path']))
assert len(ordered)==12 and [a['kind'] for a in ordered]==[0]*6+[1]*6
lines=['#pragma once','namespace CompactMaterial {','struct Pin{const char*path;unsigned bytes;unsigned char sha[32];};','static const Pin DdsPins[12]={']
for a in ordered:
 raw=(GAME/a['path']).read_bytes();assert sha(raw)==a['sha256'] and len(raw)==a['fileBytes']
 lines.append('{"%s",%du,{%s}},'%(a['path'],len(raw),','.join(str(b)for b in bytes.fromhex(a['sha256']))))
lines+=['};','static const Pin RgbaPins[3]={']
probe=[]
for a in ordered:
 image=Image.open(GAME/a['path']).convert('RGBA')
 for xy in [(0,0),(1023,731),(2047,2047)]:probe.append(image.getpixel(xy))
(OUT/'ProbeExpected.h').write_text('static const unsigned char ProbeExpected[36][4]={'+','.join('{'+','.join(map(str,p))+'}'for p in probe)+'};\n')
probe_source='''Texture2DArray<float4> dif:register(t0);Texture2DArray<float4> nml:register(t1);RWStructuredBuffer<float4> result:register(u0);
[numthreads(1,1,1)]void main(){int2 p[3]={int2(0,0),int2(1023,731),int2(2047,2047)};for(uint i=0;i<12;++i)for(uint j=0;j<3;++j)result[i*3+j]=i<6?dif.Load(int4(p[j],i,0)):nml.Load(int4(p[j],i-6,0));}'''
(OUT/'Probe.hlsl').write_text(probe_source)
subprocess.run([str(FXC),'/nologo','/T','cs_5_0','/E','main','/O3','/Fo',str(OUT/'Probe.cso'),str(OUT/'Probe.hlsl')],check=True)
data=(OUT/'Probe.cso').read_bytes();(OUT/'Probe.h').write_text('static const unsigned char ProbeShader[]={'+','.join(str(x)for x in data)+'};\n')
rgba=[]
means=[]
paint=case['painted']+[dict(path='data/newland/land/blendmap.png',sha256='db1b67238c5639f1791079b092c4d1a4cd66897c5a33093f07c781fa7d7ce907',side=1024)]
for i,a in enumerate(paint):
 raw=(GAME/a['path']).read_bytes();assert sha(raw)==a['sha256']
 image=Image.open(GAME/a['path']);assert image.size==(a['side'],a['side']);data=image.convert('RGBA').tobytes();(OUT/('image%d.rgba'%i)).write_bytes(data)
 lines.append('{"%s",%du,{%s}},'%(a['path'],len(data),','.join(str(b)for b in bytes.fromhex(sha(data)))))
 rgba.append(dict(path=a['path'],encodedSha256=a['sha256'],decodedRgbaSha256=sha(data),side=a['side'],bytes=len(data)))
 means.append(ImageStat.Stat(image.convert('RGBA')).mean)
with (OUT/'ProbeExpected.h').open('a')as f:f.write('static const float PngMeans[3][4]={'+','.join('{'+','.join(literal(x)for x in row)+'}'for row in means)+'};\n')
lines+=['};']
snapshot=Path('D:/DEV/Kenshi/research/global-satellite-20261031/biome-current-36248/active-biomes.json').read_bytes();assert sha(snapshot)=='511a8813928114dbf6b2697d5fa46a46e858c46690907cf8493e092c84633d0f'
records={r['sid']:r for r in json.loads(snapshot)['records'] if r.get('sid')}
keys=[['tiling X 1','tiling Y 1','tiling X 2','tiling Y 2'],['tiling X 0','tiling Y 0','tiling X grass','tiling Y grass'],['tiling X dirt','tiling Y dirt','tiling X road','tiling Y road'],['slope min 1','slope min 2','slope min 3','slope min grass'],['slope max 1','slope max 2','slope max 3','slope max grass'],['slope fade 1','slope fade 2','slope fade 3','slope fade grass'],['overlay mult vertical','overlay mult grass','overlay mult dirt','overlay mult road'],[None]*4,['absorbance 0','absorbance 1','absorbance 2','absorbance grass'],['absorbance dirt','absorbance road',None,None]]
params=[0.]*900;dist=[0.]*60;role_rows=[]
for b in case['biomes']:
 code=case['selector'][b['slot']]&15;row=code-1;r=records[b['sid']];role_rows.append(row)
 for ri,group in enumerate(keys):
  for col,key in enumerate(group):
   if key:params[row*60+ri*4+col]=f32(f32(r['float'][key])*f32(.01 if 3<=ri<=5 else 1))
 params[row*60+40]=f32(r['float']['brightness fix'])
 # Frozen zero-fade isolation, matching the accepted capture profile. Native
 # fade parameters remain an explicit runtime caller input, never inferred.
 for a in b['roles']:
  index=next(i%6 for i,x in enumerate(ordered) if x['sha256']==a['sha256']);params[row*60+44+a['kind']*8+a['role']]=float(index)
 w=f32(r['float']['distort wavelength']);dist[row*4:row*4+4]=[f32(1/w) if w>0 else 0,f32(f32(r['float']['distort amplitude'])*f32(.01)),0,0]
lines+=['static const float PinnedParameters[900]={'+','.join(map(literal,params))+'};','static const float PinnedDistortion[60]={'+','.join(map(literal,dist))+'};','static const unsigned RoleRows[2]={'+','.join(map(str,role_rows))+'};','}']
(OUT/'Case.h').write_text('\n'.join(lines)+'\n')
shaders=[];arrays=['#pragma once','namespace CompactMaterial {']
for name,file,profile,entry,defines,pin in [('Pixel','GeographyTerrain.hlsl','ps_4_0','main_fs',['DX11'],'f9326a91f29e1f1644fc0acbb33bc126bce04590faef2cc7b89dabd1292fd9a1'),('Vertex','GeographyVertex.hlsl','vs_4_0','main_vs',['TEXTURED'],'6ef9efa2c8027da354f812b3f59bfa055c48edcfa4a2057782bd7416f58d54e9')]:
 source=(HERE/'_source'/file).read_bytes();assert sha(source)==pin
 cmd=[str(FXC),'/nologo','/T',profile,'/E',entry,'/O3','/Gec']
 for d in defines:cmd+=['/D',d+'=1']
 cso=OUT/(name+'.cso');subprocess.run(cmd+['/Fo',str(cso),str(HERE/'_source'/file)],check=True)
 data=cso.read_bytes();arrays+=['static const unsigned char '+name+'Shader[]={'+','.join(str(x)for x in data)+'};']
 dump=subprocess.check_output([str(FXC),'/dumpbin',str(cso)],text=True);(OUT/(name+'.reflection.txt')).write_text(dump)
 fields=[]
 for m in re.finditer(r'//\s+(?:float(?:[1-4](?:x[1-4])?)?)\s+(\w+)(?:\[\d+\])?;\s+// Offset:\s*(\d+) Size:\s*(\d+)',dump):fields.append(dict(name=m[1],offset=int(m[2]),bytes=int(m[3])))
 assert fields;cbsize=(max(f['offset']+f['bytes']for f in fields)+15)&~15
 arrays+=['enum{'+name+'ConstantBytes='+str(cbsize)+'};']+['enum{'+name+'_'+f['name']+'='+str(f['offset'])+'};'for f in fields]
 shaders.append(dict(name=name,sourceSha256=pin,sha256=sha(data),bytes=len(data),constantBytes=cbsize,fields=fields))
arrays+=['}'];(OUT/'Shaders.h').write_text('\n'.join(arrays)+'\n')
(OUT/'contract.json').write_text(json.dumps(dict(scope='fixed base-catalog data; not active resource proof',case=case,dds=ordered,rgba=rgba,shaders=shaders,nativePins=native_pins,compressedLevels=9,paintedLevels=[11,12],gpuSourceBytes=83885956),indent=2)+'\n')
print('PASS pinned compact2SID/12DDS/3PNG, unchanged native-derived VS+PS, reflected ABI')
