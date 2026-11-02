"""Test-only native pre-GBuffer material oracle; production source is untouched."""
from pathlib import Path
import hashlib,json,subprocess
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';GAME=Path('D:/SteamLibrary/steamapps/common/Kenshi');FXC=Path('C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe')
raw=(GAME/'data/materials/deferred/objects.hlsl').read_bytes();assert hashlib.sha256(raw).hexdigest()=='e8511d4bf9d5cf17b7b4dadec33bc6b6a52a9c4b37353f3c206c349c65e70cb8'
# Observe the computed native diffuse BEFORE any GBuffer writer. No production
# RGB source/helper, packed decoder, surface formula copy or quantization oracle.
source=raw.decode('ascii');marker='\t// Write to GBuffer';assert source.count(marker)==1
oracle=source[:source.index(marker)]+'\tINITIALISE_OUTPUT(buffer);\n\tbuffer.buf0 = diffuse;\n}\n'
path=OUT/'NativePrewriteRgbOracle.hlsl';path.write_text(oracle,encoding='ascii');arrays=['#pragma once'];report=[]
for kind,defines in [(43,['DX11','DUST','DUAL_TEXTURE','COLOURING']),(48,['DX11','DUST','TRANSPARENCY'])]:
 cso=OUT/f'PrewriteRgbOracle{kind}.cso';args=[str(FXC),'/nologo','/T','ps_4_0','/E','main_ps','/O3']
 for d in defines:args+=['/D',d+'=1']
 subprocess.run(args+['/I',str(GAME/'data/materials/common'),'/I',str(GAME/'data/materials/deferred'),'/Fo',str(cso),str(path)],check=True)
 data=cso.read_bytes();arrays+=['static const unsigned char PrewriteRgbOracle%d[]={'%kind,','.join(str(b) for b in data),'};'];report.append(dict(kind=kind,sha256=hashlib.sha256(data).hexdigest()))
(OUT/'PrewriteRgbOracle.h').write_text('\n'.join(arrays)+'\n');(OUT/'prewrite-rgb-oracle.json').write_text(json.dumps(dict(sourceSha256=hashlib.sha256(oracle.encode('ascii')).hexdigest(),shaders=report),indent=2)+'\n')
