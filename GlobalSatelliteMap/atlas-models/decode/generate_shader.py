from pathlib import Path
import hashlib,json,subprocess
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
source=HERE.parents[1]/'shaders/GBufferDecode.hlsl'
data=source.read_bytes();assert hashlib.sha256(data).hexdigest()=='5f6804f56ba7fd77514ed9464d16cc57e233a621e860c037baad4a70d93d7401'
(OUT/'GBufferDecode.hlsl').write_bytes(data)
fxc='C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe'
game=Path('D:/SteamLibrary/steamapps/common/Kenshi/data/materials/deferred')
for name,sha in [('distant_town.hlsl','ea54dc4c8361e0b300ce34cefe1a26d927ebb5082942642ca769824ddd75c066'),('distant_town.material','eaa612b91dad500c9f77dc253019363d2dffea5653a43ed660bd0cc5e4450259'),('gbuffer.hlsl','d6a525658f224379269369c9adaa928fc82a72285aed5cffb21d077df4c9c943')]:assert hashlib.sha256((game/name).read_bytes()).hexdigest()==sha
arrays=['#pragma once']
for symbol,target,entry in [('AtlasModelVertexShader','vs_4_0','main_vs'),('AtlasModelPixelShader','ps_4_0','main_fs')]:
 path=OUT/(symbol+'.cso');subprocess.run([fxc,'/nologo','/T',target,'/E',entry,'/O3','/Gec','/D','DX11=1','/I',str(game),'/Fo',str(path),str(game/'distant_town.hlsl')],check=True)
 raw=path.read_bytes();arrays+=['static const unsigned char '+symbol+'[]={',',\n'.join(','.join(str(v) for v in raw[i:i+24]) for i in range(0,len(raw),24)),'};']
(OUT/'NativeModelShaders.h').write_text('\n'.join(arrays)+'\n')
subprocess.run([fxc,'/nologo','/T','cs_5_0','/E','main','/O3','/I',str(OUT),'/Fo',str(OUT/'ModelDecode.cso'),str(HERE/'ModelDecode.hlsl')],check=True)
code=(OUT/'ModelDecode.cso').read_bytes();(OUT/'ModelDecodeShader.h').write_text('#pragma once\nstatic const unsigned char AtlasModelDecodeShader[]={\n'+',\n'.join(','.join(str(v) for v in code[i:i+24]) for i in range(0,len(code),24))+'\n};\n')
dump=subprocess.check_output([fxc,'/dumpbin',str(OUT/'ModelDecode.cso')],text=True);(OUT/'decode-reflection.txt').write_text(dump)
import re
assert 'dcl_constantbuffer CB0[4]' in dump
for slot,name in enumerate(['albedoBuffer','modelNormal','modelRadial','oldMaterial','oldNormal','oldHeight']):assert re.search(r'\b'+name+r'\s+texture.*?\bt'+str(slot)+r'\s+1',dump)
for slot,name in enumerate(['mergedMaterial','mergedNormal','mergedHeight']):assert re.search(r'\b'+name+r'\s+UAV.*?\bu'+str(slot)+r'\s+1',dump)
(OUT/'source-proof.json').write_text(json.dumps({'decoderPath':str(source),'decoderSha256':hashlib.sha256(data).hexdigest(),'computeSha256':hashlib.sha256(code).hexdigest()},indent=2)+'\n')
print('Model decoder exact production chroma helper copied; CSO',hashlib.sha256(code).hexdigest())
