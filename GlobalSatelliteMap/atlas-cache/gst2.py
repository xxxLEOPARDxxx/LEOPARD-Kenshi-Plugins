"""Fixed little-endian GST2 v2 cache ABI. No rendering or game access."""
import hashlib
import os
import struct
from dataclasses import dataclass

HEADER_BYTES=1024
MAGIC=b'GST2'
PLANES=('materialRGBA16FloatLE-linearRGB-glossA','normalCoverageRGBA8-worldXYZ-coverageA','worldHeightR32floatLE')
FORMATS=(10,28,41)

@dataclass(frozen=True)
class Layout:
    base:int=8192
    levels:int=6
    block:int=2048
    grid:int=4
    def __post_init__(self):
        if self.grid!=4 or not(2<=self.base<=8192) or self.base&(self.base-1) or not(1<=self.levels<=6) or self.base>>(self.levels-1)<1:
            raise ValueError('unsupported dimensions')
        if self.block<1 or self.block>self.base or self.base%self.block or self.block&(self.block-1) or 3*(self.base//self.block)**2>64:
            raise ValueError('unsupported block lattice')
    def side(self,level):
        if not 0<=level<self.levels:raise ValueError('level')
        return self.base>>level
    def stride(self,plane):
        if not 0<=plane<3:raise ValueError('plane')
        return 8 if plane==0 else 4
    def length(self,level,plane):return self.side(level)**2*self.stride(plane)
    def offset(self,level,plane):
        if not 0<=plane<3:raise ValueError('plane')
        return HEADER_BYTES+sum(self.length(l,p) for l in range(level) for p in range(3))+sum(self.length(level,p) for p in range(plane))
    @property
    def file_bytes(self):return HEADER_BYTES+sum(self.length(l,p) for l in range(self.levels) for p in range(3))

PRODUCTION=Layout()
def full_mask(layout=PRODUCTION):return (1<<(3*(layout.base//layout.block)**2))-1
def filename(x,z):return f'tile_{x}_{z}.gst'
def make_header(x,z,layout=PRODUCTION,mask=0,hashes=None,complete=False,raw_packed=False):
    if not(0<=x<layout.grid and 0<=z<layout.grid) or mask&~full_mask(layout):raise ValueError('tile/ledger')
    hashes=hashes or [b'\0'*32]*(layout.levels*3)
    if len(hashes)!=layout.levels*3 or any(len(h)!=32 for h in hashes):raise ValueError('hash count')
    if complete and (raw_packed or mask!=full_mask(layout) or any(h==b'\0'*32 for h in hashes)):raise ValueError('incomplete finalize')
    h=bytearray(HEADER_BYTES)
    struct.pack_into('<4s10I',h,0,MAGIC,2,HEADER_BYTES,2 if raw_packed else int(complete),x,z,layout.grid,layout.base,layout.levels,3,layout.block)
    struct.pack_into('<Q',h,48,mask)
    struct.pack_into('<3I',h,56,*FORMATS)
    for l in range(layout.levels):
        for p in range(3):struct.pack_into('<QQ32s',h,128+(l*3+p)*48,layout.offset(l,p),layout.length(l,p),hashes[l*3+p])
    return bytes(h)

def parse_header(raw,layout=PRODUCTION,file_bytes=None):
    if len(raw)!=HEADER_BYTES:raise ValueError('partial header')
    magic,version,size,flags,x,z,grid,base,levels,planes,block=struct.unpack_from('<4s10I',raw)
    if flags&4:raise ValueError('interrupted in-place decode; full atlas recapture required')
    if magic!=MAGIC or version!=2 or size!=HEADER_BYTES or flags not in (0,1,2):raise ValueError('header identity/flags')
    if (grid,base,levels,planes,block)!=(layout.grid,layout.base,layout.levels,3,layout.block) or x>=grid or z>=grid:raise ValueError('header dimensions/tile')
    if struct.unpack_from('<3I',raw,56)!=FORMATS:raise ValueError('plane formats')
    if any(raw[44:48]) or any(raw[68:128]) or any(raw[128+levels*3*48:]):raise ValueError('reserved bytes')
    mask=struct.unpack_from('<Q',raw,48)[0]
    if mask&~full_mask(layout) or flags==1 and mask!=full_mask(layout):raise ValueError('ledger')
    if file_bytes is not None and file_bytes!=layout.file_bytes:raise ValueError('truncated/extra file bytes')
    descriptors=[]
    for l in range(levels):
        for p in range(3):
            off,length,digest=struct.unpack_from('<QQ32s',raw,128+(l*3+p)*48)
            # Exact canonical offsets also reject overlap, wrap, holes and aliases.
            if off!=layout.offset(l,p) or length!=layout.length(l,p) or off>layout.file_bytes or length>layout.file_bytes-off:raise ValueError('descriptor range')
            if flags==1 and digest==b'\0'*32:raise ValueError('missing digest')
            descriptors.append(dict(offset=off,length=length,sha256=digest.hex()))
    return dict(tile=(x,z),complete=flags==1,raw_packed=flags==2,mask=mask,descriptors=descriptors)

def write_exact(file,data):
    view=memoryview(data).cast('B')
    while view:
        n=file.write(view)
        if n is None or n<=0 or n>len(view):raise OSError('short/failed write')
        view=view[n:]

def flush(file):file.flush();os.fsync(file.fileno())

def plane_hash(file,offset,length):
    file.seek(offset);digest=hashlib.sha256();left=length
    while left:
        data=file.read(min(left,1024*1024))
        if not data:raise OSError('truncated plane')
        digest.update(data);left-=len(data)
    return digest.digest()

def verify_file(path,layout=PRODUCTION):
    with path.open('rb') as f:
        h=parse_header(f.read(HEADER_BYTES),layout,path.stat().st_size)
        if not h['complete']:raise ValueError('cache not complete')
        for d in h['descriptors']:
            if plane_hash(f,d['offset'],d['length']).hex()!=d['sha256']:raise ValueError('plane SHA256 mismatch')
        return h
