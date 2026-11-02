"""Fresh actual DXBC reflection contracts for native reference and production ABI."""
from pathlib import Path
import re
import subprocess
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated'
FXC='C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe'
def dump(name):return subprocess.check_output([FXC,'/nologo','/dumpbin',str(OUT/name)],text=True)
n=dump('NativeWater.cso');p=dump('AtlasWater.cso')
for name,offset in [('sunColour',0),('envColour',64),('invStrength',100),('distortion',112),('gloss',120),('glow',124),('gameTime',128)]:
    assert re.search(r'\b'+name+r';\s*// Offset:\s*'+str(offset)+r'\b',n),name
for slot,name in enumerate(['colourmap','flowmap','normalmap','reflection','irradianceCube','specularityCube']):
    for typ,reg in [('sampler','s'),('texture','t')]:assert re.search(r'\$'+name+r'\s+'+typ+r'.*?\b'+reg+str(slot)+r'\s+1',n),(name,typ)
for slot,name in enumerate(['terrainHdr','normalCoverage','worldHeight','colourmap','flowmap','normalmap','irradianceCube','specularityCube']):assert re.search(r'\b'+name+r'\s+texture.*?\bt'+str(slot)+r'\s+1',p),name
for slot,name in enumerate(['colourSampler','flowSampler','normalSampler','irradianceSampler','specularitySampler']):assert re.search(r'\b'+name+r'\s+sampler.*?\bs'+str(slot)+r'\s+1',p),name
assert re.search(r'WaterUniforms\s+cbuffer.*?\bcb0\s+1',p)
assert re.search(r'parallelView;\s*// Offset:\s*176\s+Size:\s*16',p)
assert re.search(r'\bresult\s+UAV.*?\bu0\s+1',p)
assert 'sample_d' in p and 'sample_l' in p
print('PASS actual native reference bindings/constants and production192B/8SRV/5sampler/1UAV/gradient+explicitCubeLOD ABI')
