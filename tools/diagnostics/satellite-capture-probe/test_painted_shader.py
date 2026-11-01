"""Actual generated private PS reflection, not an assumed declaration order."""
from pathlib import Path
import re,subprocess,sys
fxc='C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe'
dump=subprocess.check_output([fxc,'/dumpbin',sys.argv[1]],text=True,errors='replace')
bindings={}
for line in dump.splitlines():
    match=re.fullmatch(r'//\s+(\w+)\s+(texture|sampler)\s+\S+\s+(\S+)\s+([ts]\d+)\s+(\d+)\s*',line)
    if match:
        name,kind,dimension,slot,count=match.groups()
        assert (name,kind) not in bindings
        bindings[name,kind]=(dimension,slot,int(count))
expected={(name,'texture'):('2darray' if slot<4 else '2d',f't{slot}',1)
    for slot,name in enumerate(('diffuseMaps','normalMaps','overlayMap','colourMap','interiorClip'))}
expected.update({(name,'sampler'):('NA',f's{slot}',1)
    for name,slot in [('Nearest',4),('Linear',2),('Anisotropic',0)]})
assert bindings==expected,(bindings,expected)
print('Actual painted PS DXBC: exact5texture/3sampler bindings,4array2D inputs verified')
