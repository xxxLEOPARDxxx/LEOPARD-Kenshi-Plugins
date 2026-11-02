"""Actual production PS uniform reflection and extracted binding expression."""
from pathlib import Path
import hashlib,re,subprocess
HERE=Path(__file__).resolve().parent;PROBE=HERE.parent
fxc='C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe'
dump=subprocess.check_output([fxc,'/dumpbin',str(PROBE/'build/GeographyTerrain.cso')],text=True)
assert re.search(r'float4 geoSelectorRegion;\s*// Offset:\s+3840 Size:\s+16',dump)
assert re.search(r'float4 satBiome\[225\];\s*// Offset:\s+240 Size:\s+3600',dump)
assert hashlib.sha256((PROBE/'build/GeographyVertex.cso').read_bytes()).hexdigest()=='dbd13838450fa683426bf3df92728fc0c3dbdf2b798c31c6c1004d0a3e3ed238'
binding=(PROBE/'PrivateGeography.h').read_text();line=next(line.strip() for line in binding.splitlines() if 'fp->setNamedConstant("geoSelectorRegion"' in line)
assert 'selectedRegion->worldX' in line and 'selectedRegion->worldZ' in line and ',4,4)' in line and '(-73728,-73728,8,8)' in line
assert 'selectedRegion=NULL;' in binding and 'if(uncertain||active||selectedRegion||!OnMain()' in binding
print('PASS actual DXBC newfloat4@3840/225rows unchanged;nativeVS exact;explicit legacy8/regional4 origin bindings and region-latch cleanup')
