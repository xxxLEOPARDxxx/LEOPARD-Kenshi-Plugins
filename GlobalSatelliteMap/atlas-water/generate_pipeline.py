"""Build the three fixed water-pipeline shaders from installed pinned assets.

generate() returns paths; it writes only atlas-water/_generated/pipeline.
Root's module generator owns integrating these arrays into the main DLL.
"""
from pathlib import Path
import hashlib
import json
import subprocess
import generate_water

HERE=Path(__file__).resolve().parent
OUT=HERE/'_generated/pipeline'
FXC=Path('C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe')
MATERIALS=Path('D:/SteamLibrary/steamapps/common/Kenshi/data/materials')
SHADERS=(
 ('AtlasTerrainHdrShader','TerrainHdr.cso',HERE.parent/'atlas-view/AtlasRelight.hlsl',['/D','DX11=1','/D','ATLAS_LINEAR_HDR=1']),
 ('AtlasWaterMaterialShader','Water.cso',HERE/'AtlasWater.hlsl',[]),
 ('AtlasWaterToneShader','Tone.cso',HERE/'ToneWater.hlsl',['/D','DX11=1']),
)

def generate():
    native=generate_water.generate()
    OUT.mkdir(parents=True,exist_ok=True)
    lines=['#pragma once','// Generated locally from pinned installed native assets.']
    records=[]
    for symbol,name,source,defines in SHADERS:
        target=OUT/name
        subprocess.run([str(FXC),'/nologo','/T','cs_5_0','/E','main','/O3','/Gec',*defines,
            '/I',str(MATERIALS/'common'),'/I',str(MATERIALS/'post'),'/Fo'+str(target),str(source)],check=True)
        code=target.read_bytes()
        lines+=['static const unsigned char '+symbol+'[]={',
            ',\n'.join(','.join(str(b) for b in code[i:i+24]) for i in range(0,len(code),24)),'};']
        records.append(dict(symbol=symbol,file=name,bytes=len(code),sha256=hashlib.sha256(code).hexdigest()))
    header=OUT/'WaterPipelineShaders.h'
    header.write_text('\n'.join(lines)+'\n')
    metadata=dict(privateGpuBytes=3408256,fixedWindowSide=256,waterScratchSide=512,
        sourceWaterSha256=native['waterSha256'],sourceLightingSha256=native['lightingSha256'],shaders=records)
    (OUT/'pipeline-shaders.json').write_text(json.dumps(metadata,indent=2)+'\n')
    return dict(header=str(header),metadata=metadata)

if __name__=='__main__':print(json.dumps(generate(),indent=2))
