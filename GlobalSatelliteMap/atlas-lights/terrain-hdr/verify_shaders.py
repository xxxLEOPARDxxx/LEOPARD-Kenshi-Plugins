from pathlib import Path
import re,subprocess
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated'
FXC='C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe'
def dump(n):return subprocess.check_output([FXC,'/nologo','/dumpbin',str(OUT/n)],text=True)
for kind in ['Point','Spot']:
 d=dump('Native'+kind+'.cso');(OUT/(kind+'-reflection.txt')).write_text(d)
 for name,offset in [('diffuseColour',0),('falloff',32),('position',48),('power',60)]:
  assert re.search(r'\b'+name+r';\s*// Offset:\s*'+str(offset)+r'\b',d),(kind,name)
 fields=[('farClip',64),('offset',80),('viewport',96),('farCorner',112),('farCorner2',128),('viewMatrix',144)] if kind=='Point' else [('direction',64),('spot',80),('farClip',92),('offset',96),('viewport',112),('farCorner',128),('farCorner2',144),('viewMatrix',160)]
 for name,offset in fields:assert re.search(r'\b'+name+r';\s*// Offset:\s*'+str(offset)+r'\b',d),(kind,name)
 for slot in range(3):
  for typ,reg in [('sampler','s'),('texture','t')]:assert re.search(r'\$gBuf'+str(slot)+r'\s+'+typ+r'.*?\b'+reg+str(slot)+r'\s+1',d)
p=dump('LocalLights.cso');(OUT/'production-reflection.txt').write_text(p)
assert re.search(r'LocalValues\s+cbuffer.*?\bcb0\s+1',p)
assert re.search(r'lights\[32\];\s*// Offset:\s*80\s+Size:\s*2048',p)
assert 'dcl_constantbuffer CB0[133], dynamicIndexed' in p
for slot,name in enumerate(['terrainHdr','material','normalCoverage','absoluteHeight']):assert re.search(r'\b'+name+r'\s+texture.*?\bt'+str(slot)+r'\s+1',p),name
assert re.search(r'\bresult\s+UAV.*?\bu0\s+1',p)
print('PASS actual native pixel bindings and production2128B/32light/4SRV/1UAV shader ABI')
