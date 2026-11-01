"""Check actual guarded PNG files; source guards do not invoke Ogre."""
from pathlib import Path
from PIL import Image
import hashlib,json,re,struct
here=Path(__file__).resolve().parent
game=Path('D:/SteamLibrary/steamapps/common/Kenshi')
rows=json.loads((here/'painted-inputs.json').read_text())
header=(here/'PrivatePaintedInputs.h').read_text()
assert len(rows)==8 and len({r['path'] for r in rows})==8
total=0
for index,row in enumerate(rows):
    kind,layer=divmod(index,4)
    assert (row['kind'],row['x'],row['z'])==(kind,2+layer%2,2+layer//2)
    assert row['side']==(1024 if kind==0 else 2048)
    data=(game/row['path']).read_bytes()
    assert len(data)==row['bytes'] and hashlib.sha256(data).hexdigest()==row['sha256']
    assert data[:8]==b'\x89PNG\r\n\x1a\n' and data[12:16]==b'IHDR'
    assert struct.unpack_from('>II',data,16)==(row['side'],row['side'])
    assert data[24:26]==bytes((8,6)), 'native fixture requires8-bit RGBA PNG'
    entry='{%du,%du,%du,%du,%du,"%s","%s"}'%(kind,row['x'],row['z'],row['side'],row['bytes'],row['path'],row['sha256'])
    assert entry in header
    with Image.open(game/row['path']) as image:
        assert image.mode=='RGBA' and list(image.getchannel('A').getextrema())==row['alpha_extrema']
    total+=len(data)
assert total==45799579 and total<128*1024*1024
source=(here/'PrivatePaintedReference.h').read_text()
assert source.index('PrivateBiomeReference::Hash(encoded)!=input.sha256')<source.index('image.load(stream,"png")')
assert source.index('ImageShape(kind,')<source.index('createManual(Name(kind)')
# GPU descriptor/filtering and native completion ordering are exercised by the
# compiled production seams in run_gpu_mip_tests/run_painted_mip_tests, rather
# than requiring the obsolete mip0 native array-writer implementation text.
assert 'setMaterialCallbacks' not in source and 'ManualResourceLoader' not in source
assert 'remove(ProgramName())' in source and 'cache.Owns(0,program.get())' in source
print('8 actual RGBA PNG identities/alpha,45799579encoded bytes; pre-allocation decode ordering passed')
