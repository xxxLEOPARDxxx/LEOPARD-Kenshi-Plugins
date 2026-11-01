from pathlib import Path
import re,subprocess,sys
fxc='C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe'
dump=subprocess.check_output([fxc,'/dumpbin',sys.argv[1]],text=True,errors='replace')
bindings={}
for line in dump.splitlines():
    m=re.fullmatch(r'//\s+(\w+)\s+(texture|sampler)\s+\S+\s+(\S+)\s+([ts]\d+)\s+(\d+)\s*',line)
    if m:bindings[m[1],m[2]]=(m[3],m[4],int(m[5]))
names=['diffuseMaps','normalMaps','overlayMap','colourMap','interiorClip','geoBlend','geoSelector','geoDiffuse1']
expected={(name,'texture'):('2darray' if i in (0,1,2,3,7) else '2d','t%d'%i,1) for i,name in enumerate(names)}
expected.update({(name,'sampler'):('NA','s%d'%slot,1) for name,slot in (('Nearest',4),('Linear',2),('Anisotropic',0))})
assert bindings==expected,(bindings,expected)
assert re.search(r'float4 satBiome\[225\]',dump) and 'Size:  3600' in dump
print('PASS actual geographic PS reflection:8exactSRVs/3samplers/225float4table; no implicit extra biome resources')
