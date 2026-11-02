"""Pin unchanged native-cliff VS and explicit-region PS; fade remains data-only."""
from pathlib import Path
import hashlib
here=Path(__file__).resolve().parent
expected={'GeographyVertex.hlsl':'6ef9efa2c8027da354f812b3f59bfa055c48edcfa4a2057782bd7416f58d54e9','GeographyTerrain.hlsl':'f9326a91f29e1f1644fc0acbb33bc126bce04590faef2cc7b89dabd1292fd9a1','GeographyVertex.cso':'dbd13838450fa683426bf3df92728fc0c3dbdf2b798c31c6c1004d0a3e3ed238','GeographyTerrain.cso':'4b3b71ccb9c75b79791b9cf3bef5f858ba9c53151a9cfc0d65cd2e7d113fdebe'}
for name,sha in expected.items():assert hashlib.sha256((here/'build'/name).read_bytes()).hexdigest()==sha,('production shader/DXBC changed',name)
source=(here/'CaptureProbe.cpp').read_text();geo=(here/'PrivateGeography.h').read_text()
assert source.count('"GeographicNativeFade",0')==1,'default0 read once per request'
assert geo.count('data->idata,data->fdata')==1 and 'parameters+b*60+28' in geo
print('PASS native-cliff VS pins unchanged/explicit-region PS pins; fade default0/read-once/typedrow7')
