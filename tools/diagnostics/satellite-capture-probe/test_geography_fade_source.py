"""Data-only fade must preserve actual accepted production shader bytes."""
from pathlib import Path
import hashlib
here=Path(__file__).resolve().parent
expected={'GeographyVertex.hlsl':'6ef9efa2c8027da354f812b3f59bfa055c48edcfa4a2057782bd7416f58d54e9','GeographyTerrain.hlsl':'3cde7fa65e27b76092bb14fd4636172b3f979ac4db830e677c800433ea0d1d98','GeographyVertex.cso':'dbd13838450fa683426bf3df92728fc0c3dbdf2b798c31c6c1004d0a3e3ed238','GeographyTerrain.cso':'57f7d3ddfabcde2fae7ee39715c5f03c3ac4ebaaefa921c61f381a7cb5b9a2b4'}
for name,sha in expected.items():assert hashlib.sha256((here/'build'/name).read_bytes()).hexdigest()==sha,('production shader/DXBC changed',name)
source=(here/'CaptureProbe.cpp').read_text();geo=(here/'PrivateGeography.h').read_text()
assert source.count('"GeographicNativeFade",0')==1,'default0 read once per request'
assert geo.count('data->idata,data->fdata')==1 and 'parameters+b*60+28' in geo
print('PASS data-only fade: accepted VS/PS source and DXBC hashes unchanged; default0/read-once/typedrow7')
