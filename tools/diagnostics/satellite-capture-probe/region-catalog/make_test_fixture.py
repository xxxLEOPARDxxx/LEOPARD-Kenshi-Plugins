"""Independent pinned raw source fixture for C++ tests (not generated catalogs)."""
from pathlib import Path
import hashlib,json,struct
from PIL import Image
HERE=Path(__file__).resolve().parent
SNAP=Path('D:/DEV/Kenshi/research/global-satellite-20261031/biome-current-36248/biome-selection-snapshot.json')
BLEND=Path('D:/SteamLibrary/steamapps/common/Kenshi/data/newland/land/blendmap.png')
assert hashlib.sha256(SNAP.read_bytes()).hexdigest()=='5cb085baa67a7100181daf4edf25bdf3e47f5c162f7ef551fa8eb5910e565d60'
assert hashlib.sha256(BLEND.read_bytes()).hexdigest()=='db1b67238c5639f1791079b092c4d1a4cd66897c5a33093f07c781fa7d7ce907'
snapshot=json.loads(SNAP.read_text());sids=sorted({s for c in snapshot['cells'] for s in c['slots'] if s})
assert len(sids)==57
out=bytearray(b'RGCFIX1\0'+struct.pack('<H',len(sids)))
for sid in sids:
    raw=sid.encode('ascii');out.extend(struct.pack('<H',len(raw))+raw)
for cell in sorted(snapshot['cells'],key=lambda c:(c['z'],c['x'])):
    tree=bytes.fromhex(cell['tree']);out.extend(bytes(sids.index(sid)+1 if sid else 0 for sid in cell['slots']))
    out.extend(struct.pack('<H',len(tree))+tree+hashlib.sha256(tree).hexdigest().encode('ascii'))
image=Image.open(BLEND);assert image.mode=='RGBA' and image.size==(1024,1024)
out.extend(image.tobytes());(HERE/'build').mkdir(exist_ok=True);(HERE/'build/source.fixture').write_bytes(out)
(HERE/'build/blend.rgba').write_bytes(image.tobytes())
print('PASS independent native-source fixture:',len(out),'bytes; no catalog/generator helper used')
