"""Inspect the actual three DXBC interfaces and retain the default terrain pin."""
from pathlib import Path
import hashlib,re,subprocess
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated/pipeline'
FXC='C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe'
def dump(name):return subprocess.check_output([FXC,'/nologo','/dumpbin',str(OUT/name)],text=True)
h=dump('TerrainHdr.cso');w=dump('Water.cso');t=dump('Tone.cso')
for shader,cb,tail,offset in [(h,'AtlasUniforms','dispatchWindow',160),(w,'WaterUniforms','parallelView',176),(t,'WaterTone','toneExposure',0)]:
 assert re.search(r'\b'+cb+r'\s+cbuffer.*?\bcb0\s+1',shader)
 assert re.search(r'\b'+tail+r';\s*// Offset:\s*'+str(offset)+r'\s+Size:\s*16',shader),(cb,tail)
 assert re.search(r'\bresult\s+UAV.*?\bu0\s+1',shader)
for slot,name in enumerate(['materialPlane','normalCoveragePlane','heightPlane','ambientMap','irradianceCube','specularityCube']):assert re.search(r'\b'+name+r'\s+texture.*?\bt'+str(slot)+r'\s+1',h),name
for slot,name in enumerate(['ambientSampler','irradianceSampler','specularitySampler']):assert re.search(r'\b'+name+r'\s+sampler.*?\bs'+str(slot)+r'\s+1',h),name
assert re.search(r'\bhdrInput\s+texture.*?\bt0\s+1',t)
assert not re.search(r'//\s+\w+\s+sampler\s+',t)
assert hashlib.sha256((OUT/'TerrainDefault.cso').read_bytes()).hexdigest()=='e9d39a51c87a02b4dffe81b40c41a48e549dbf17fc0cf504d34d0a77b3e485b6'
assert hashlib.sha256((OUT/'Water.cso').read_bytes()).hexdigest()=='9c5b41477ec29d1663dc58fb1242e09ac80f1b7209d90ce1f3055328be13b5d3'
print('PASS HDR176B/6SRV/3sampler; water192B; tone16B/1SRV/noSampler; default terrain and water DXBC unchanged')
