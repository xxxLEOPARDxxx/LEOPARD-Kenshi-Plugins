"""Validate actual profiled DDS identities before allowing the private reference."""
from pathlib import Path
import hashlib,re,struct
root=Path(__file__).resolve().parent
game=Path('D:/SteamLibrary/steamapps/common/Kenshi')
entries=re.findall(r'\{"([^"\n]+)","([a-f0-9]{64})",(\d+)u\}',(root/'PrivateBiomeDds.h').read_text())
assert len(entries)==12 and len({p for p,_,_ in entries})==12
total=0
for index,(name,sha,size) in enumerate(entries):
    path=game/name
    with path.open('rb') as stream:
        header=stream.read(128)
    assert header[:4]==b'DDS ' and struct.unpack_from('<I',header,4)[0]==124
    assert struct.unpack_from('<II',header,12)==(2048,2048)
    assert struct.unpack_from('<I',header,28)[0]==12
    assert header[84:88]==(b'DXT5' if index<6 else b'DXT1')
    assert path.stat().st_size==int(size)<=32*1024*1024
    assert hashlib.sha256(path.read_bytes()).hexdigest()==sha
    total+=int(size)
assert total==50333424 and total<=128*1024*1024
source=(root/'PrivateBiomeReference.h').read_text()
assert 'Ogre::ManualResourceLoader' not in source and 'PrivateLoadImagesName' not in source
assert source.index('ReadDds(path,gameRoot')<source.index('loader.images[layer].load(stream')
assert source.index('Hash(encoded)!=expected->sha256')<source.index('void Begin(')
assert 'AddBytes(decodedTotal,shape.bytes)' in source
assert 'first.getNumMipmaps()-3,first.getFormat(),0x15,NULL' in source
assert 'PrivateArrayTextureVtableRva' in source and '0x2d8' in source
print('12 actual profiled DDS identities/headers and native array-upload source contracts passed')
