"""Bounded NumPy stripe GST2 semantic mips + final atomic readiness manifest."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import time
import numpy as np
import gst2

OUTPUT_STRIPE_ROWS=16
CANONICAL_STRIPE_ROWS=32
RAW_STRIPE_ROWS=32

def capture_metadata_digest(folder):
    """Optional frozen root-producer identity, not a source-file rehash job."""
    try:
        with (folder/'capture.json').open('rb') as file:raw=file.read(65537)
    except FileNotFoundError:return None
    if len(raw)>65536:raise ValueError('capture metadata exceeds65536-byte bound')
    try:metadata=json.loads(raw)
    except (ValueError,UnicodeError,RecursionError) as error:raise ValueError('invalid capture metadata JSON') from error
    if not isinstance(metadata,dict) or metadata.get('format')!='GSCAP2' or type(metadata.get('version')) is not int or metadata['version']!=2:
        raise ValueError('capture metadata requires GSCAP2 version2')
    return hashlib.sha256(raw).hexdigest()

def decode_packed(halo,global_x,global_z):
    """Native GBufferDecode.hlsl four-neighbor YCoCg, actual BGRA storage.

    halo is original raw BGRA rows/columns including one texel on every side.
    Global coordinates determine checker parity, not stripe/block-local coordinates.
    """
    center=halo[1:-1,1:-1].astype(np.float32)/np.float32(255)
    y=center[:,:,2];g=center[:,:,1]
    weighted=np.zeros_like(y);weight_sum=np.zeros_like(y)
    right_g=None
    for neighbor in (halo[1:-1,2:],halo[1:-1,:-2],halo[2:,1:-1],halo[:-2,1:-1]):
        values=neighbor.astype(np.float32)/np.float32(255)
        weight=(np.abs(values[:,:,2]-y)<np.float32(30)/np.float32(255)).astype(np.float32)
        weighted+=weight*values[:,:,1];weight_sum+=weight
        if right_g is None:right_g=values[:,:,1].copy()
    other=np.divide(weighted,np.maximum(weight_sum,1));other[weight_sum==0]=right_g[weight_sum==0]
    parity=(((np.arange(y.shape[1])+global_x)&1)[None,:]==((np.arange(y.shape[0])+global_z)&1)[:,None])
    co=np.where(parity,other,g)-np.float32(.5);cg=np.where(parity,g,other)-np.float32(.5)
    result=np.empty(center.shape,'<f2')
    result[:,:,0]=y+co-cg;result[:,:,1]=y+cg;result[:,:,2]=y-co-cg;result[:,:,3]=center[:,:,3]
    return result

def read_raw_rows(file,layout,y,rows):
    file.seek(layout.offset(0,0)+y*layout.base*8);raw=file.read(rows*layout.base*8)
    if len(raw)!=rows*layout.base*8:raise OSError('short raw read')
    return np.frombuffer(raw,np.uint8).reshape(rows,layout.base,8)

def collect_original_edges(folder,layout):
    """2MiB for16 production tiles. Validate raw padding before any overwrite."""
    edges={};side=layout.base
    for z in range(4):
        for x in range(4):
            e={name:np.empty((side,4),np.uint8) for name in ('top','bottom','left','right')}
            with (folder/gst2.filename(x,z)).open('rb') as f:
                for y in range(0,side,RAW_STRIPE_ROWS):
                    rows=min(RAW_STRIPE_ROWS,side-y);raw=read_raw_rows(f,layout,y,rows)
                    if raw[:,:,4:].any():raise ValueError('raw packed trailing4-byte padding must be zero')
                    e['left'][y:y+rows]=raw[:,0,:4];e['right'][y:y+rows]=raw[:,-1,:4]
                    if y==0:e['top'][:]=raw[0,:,:4]
                    if y+rows==side:e['bottom'][:]=raw[-1,:,:4]
            edges[(x,z)]=e
    return edges

def decode_raw_tile(path,layout,x,z,edges):
    side=layout.base;carry=None
    with path.open('r+b') as f:
        for y in range(0,side,RAW_STRIPE_ROWS):
            rows=min(RAW_STRIPE_ROWS,side-y)
            raw=read_raw_rows(f,layout,y,rows+int(y+rows<side))
            halo=np.empty((rows+2,side+2,4),np.uint8)
            halo[1:rows+1,1:-1]=raw[:rows,:,:4]
            halo[0,1:-1]=carry if y else edges[(x,max(z-1,0))]['bottom' if z else 'top']
            halo[-1,1:-1]=raw[rows,:,:4] if y+rows<side else edges[(x,min(z+1,3))]['top' if z<3 else 'bottom']
            halo[1:-1,0]=edges[(x-1,z)]['right'][y:y+rows] if x else raw[:rows,0,:4]
            halo[1:-1,-1]=edges[(x+1,z)]['left'][y:y+rows] if x<3 else raw[:rows,-1,:4]
            # Corners are never sampled by the native axial four-neighbor filter.
            halo[0,0]=halo[0,1];halo[0,-1]=halo[0,-2];halo[-1,0]=halo[-1,1];halo[-1,-1]=halo[-1,-2]
            carry=raw[rows-1,:,:4].copy() # preserve ORIGINAL previous row before write.
            decoded=decode_packed(halo,x*side,z*side+y)
            write_rows(f,layout,0,0,y,decoded)
        gst2.flush(f)

def decode_raw_atlas(folder,layout):
    edges=collect_original_edges(folder,layout)
    # In-place overwrite is intentionally non-resumable. Mark every tile before it.
    for z in range(4):
        for x in range(4):
            with (folder/gst2.filename(x,z)).open('r+b') as f:f.seek(12);gst2.write_exact(f,b'\x06\0\0\0');gst2.flush(f)
    for z in range(4):
        for x in range(4):decode_raw_tile(folder/gst2.filename(x,z),layout,x,z,edges)
    # All16 data planes are now decoded/flushed; only now clear RAW/DECODING.
    for z in range(4):
        for x in range(4):
            with (folder/gst2.filename(x,z)).open('r+b') as f:f.seek(12);gst2.write_exact(f,b'\0'*4);gst2.flush(f)

def round_u8(a):return np.clip(np.floor(a+0.5),0,255).astype(np.uint8)

def semantic_box(material,normal_coverage,height):
    """Global aligned 2x2 box, linear bytes; no sRGB/gamma transform.

    Coverage is averaged independently; covered samples with nonfinite height are
    invalid. Material+gloss and height use valid-coverage-weighted means. Normals
    decode in world XYZ, coverage-weight, normalize; cancellation uses world up.
    """
    h,w=height.shape
    if h%2 or w%2 or material.shape!=(h,w,4) or normal_coverage.shape!=(h,w,4):raise ValueError('box dimensions')
    weights=normal_coverage[:,:,3].astype(np.float32)
    weights[(~np.isfinite(height))|(~np.isfinite(material).all(axis=2))]=0
    def total(a):return a.reshape((h//2,2,w//2,2)+a.shape[2:]).sum(axis=(1,3),dtype=a.dtype)
    weight_sum=total(weights)
    denominator=np.maximum(weight_sum,1)
    coverage=round_u8(weight_sum*0.25)
    covered=coverage!=0
    colours=material.astype(np.float32);colours[weights==0]=0
    colours*=weights[:,:,None]
    out_material=(total(colours)/denominator[:,:,None]).astype('<f2');del colours
    out_material[~covered]=0
    normals=normal_coverage[:,:,:3].astype(np.float32)
    normals*=np.float32(2.0/255.0);normals-=1;normals*=weights[:,:,None]
    summed=total(normals);del normals
    length=np.sqrt(np.sum(summed*summed,axis=2))
    valid=covered&(length>weight_sum*np.float32(1e-6))
    out_normals=np.zeros_like(summed);out_normals[:,:,1]=1
    np.divide(summed,length[:,:,None],out=out_normals,where=valid[:,:,None])
    out_nc=np.empty((h//2,w//2,4),np.uint8)
    out_nc[:,:,:3]=round_u8((out_normals+1)*127.5);out_nc[:,:,3]=coverage
    # float64 accumulation prevents overflow for finite float32 extremes.
    height64=height.astype(np.float64);height64[weights==0]=0;height64*=weights
    out_height=(total(height64)/denominator).astype('<f4');out_height[~covered]=0
    return out_material,out_nc,out_height

def read_rows(file,layout,level,plane,y,rows):
    side=layout.side(level);size=rows*side*layout.stride(plane)
    if y<0 or rows<1 or y+rows>side:raise ValueError('row bounds')
    file.seek(layout.offset(level,plane)+y*side*layout.stride(plane));raw=file.read(size)
    if len(raw)!=size:raise OSError('short plane read')
    dtype='<f2' if plane==0 else '<f4' if plane==2 else np.uint8
    shape=(rows,side) if plane==2 else (rows,side,4)
    return np.frombuffer(raw,dtype=dtype).reshape(shape).copy()

def write_rows(file,layout,level,plane,y,array):
    side=layout.side(level);rows=array.shape[0]
    expected=(rows,side) if plane==2 else (rows,side,4)
    if array.shape!=expected or y<0 or y+rows>side:raise ValueError('write row bounds')
    file.seek(layout.offset(level,plane)+y*side*layout.stride(plane))
    gst2.write_exact(file,memoryview(np.ascontiguousarray(array)).cast('B'))

def read_plane(path,layout,level,plane):
    """Small-fixture helper. Production builder never calls this full-plane read."""
    with Path(path).open('rb') as f:return read_rows(f,layout,level,plane,0,layout.side(level))

def canonicalize_base(file,layout):
    for y in range(0,layout.base,CANONICAL_STRIPE_ROWS):
        rows=min(CANONICAL_STRIPE_ROWS,layout.base-y)
        mat=read_rows(file,layout,0,0,y,rows);nc=read_rows(file,layout,0,1,y,rows);height=read_rows(file,layout,0,2,y,rows)
        invalid=(nc[:,:,3]==0)|~np.isfinite(height)|~np.isfinite(mat).all(axis=2)
        if invalid.any():
            mat[invalid]=0;nc[invalid]=[128,255,128,0];height[invalid]=0
            # Persist invalid coverage FIRST. Otherwise a crash after replacing a
            # NaN material by0 could lose the only invalidity marker on resume.
            write_rows(file,layout,0,1,y,nc);gst2.flush(file)
            write_rows(file,layout,0,2,y,height);write_rows(file,layout,0,0,y,mat)
    gst2.flush(file)

def generate_level(file,layout,level):
    side=layout.side(level)
    for y in range(0,side,OUTPUT_STRIPE_ROWS):
        rows=min(OUTPUT_STRIPE_ROWS,side-y)
        source=[read_rows(file,layout,level-1,p,y*2,rows*2) for p in range(3)]
        result=semantic_box(*source)
        for p,a in enumerate(result):write_rows(file,layout,level,p,y,a)
    gst2.flush(file)

def finalize_file(path,layout,x,z):
    with path.open('r+b') as f:
        digests=[gst2.plane_hash(f,layout.offset(l,p),layout.length(l,p)) for l in range(layout.levels) for p in range(3)]
        # Flush the entire digest-bearing header with COMPLETE=0 first.
        header=gst2.make_header(x,z,layout,gst2.full_mask(layout),digests,False)
        f.seek(0);gst2.write_exact(f,header);gst2.flush(f)
        f.seek(12);gst2.write_exact(f,b'\x01\0\0\0');gst2.flush(f)
    with path.open('rb') as f:
        saved=f.read(gst2.HEADER_BYTES)
        parsed=gst2.parse_header(saved,layout,path.stat().st_size)
        if not parsed['complete'] or [d['sha256'] for d in parsed['descriptors']]!=[d.hex() for d in digests]:raise OSError('final header readback mismatch')
        header_hash=hashlib.sha256(saved).hexdigest()
    return dict(x=x,z=z,file=path.name,bytes=layout.file_bytes,headerSha256=header_hash)

def build_atlas(folder,layout=gst2.PRODUCTION,progress=None):
    folder=Path(folder)
    if (folder/'ready.json').exists():raise FileExistsError('ready atlas is immutable; use a new output directory')
    manifest_tmp=folder/'.ready.json.tmp'
    if manifest_tmp.exists():raise FileExistsError('stale manifest temporary requires explicit inspection')
    expected={gst2.filename(x,z) for z in range(4) for x in range(4)}
    if {p.name for p in folder.glob('*.gst')}!=expected:raise ValueError('exactly16 canonical tile files required')
    lock=folder/'.gst2-build.lock'
    fd=os.open(lock,os.O_CREAT|os.O_EXCL|os.O_WRONLY,0o600)
    os.close(fd)
    manifest_owned=False
    try:
        capture_digest=capture_metadata_digest(folder)
        states={};raw_states=[]
        # Validate all16 before modifying any: size, tile identity and complete base ledger.
        for z in range(4):
            for x in range(4):
                path=folder/gst2.filename(x,z)
                with path.open('rb') as f:h=gst2.parse_header(f.read(gst2.HEADER_BYTES),layout,path.stat().st_size)
                if h['tile']!=(x,z) or h['mask']!=gst2.full_mask(layout):raise ValueError('wrong tile/missing base regions')
                if h['complete']:gst2.verify_file(path,layout)
                states[(x,z)]=h['complete']
                raw_states.append(h['raw_packed'])
        if any(raw_states):
            if not all(raw_states):raise ValueError('mixed raw/decoded atlas; full recapture required')
            decode_raw_atlas(folder,layout)
        for z in range(4):
            for x in range(4):
                if not states[(x,z)]:
                    with (folder/gst2.filename(x,z)).open('r+b') as f:canonicalize_base(f,layout)
        # Global alignment guarantees every2x2 footprint lies in one tile at all6levels.
        # Process all cells at each level; runtime bilinear across edges is separate.
        for level in range(1,layout.levels):
            for z in range(4):
                for x in range(4):
                    if not states[(x,z)]:
                        with (folder/gst2.filename(x,z)).open('r+b') as f:generate_level(f,layout,level)
                    if progress:progress(f'mip {level} tile {x},{z}')
        tiles=[]
        for z in range(4):
            for x in range(4):
                path=folder/gst2.filename(x,z)
                if states[(x,z)]:
                    with path.open('rb') as f:header_hash=hashlib.sha256(f.read(1024)).hexdigest()
                    tiles.append(dict(x=x,z=z,file=path.name,bytes=layout.file_bytes,headerSha256=header_hash))
                else:tiles.append(finalize_file(path,layout,x,z))
                if progress:progress(f'verified {x},{z}')
        manifest=dict(format='GST2',version=2,complete=True,grid=4,baseSide=layout.base,levels=layout.levels,planes=list(gst2.PLANES),rowOrder='minZ-first',columnOrder='minX-first',mipFilter='global-aligned-2x2-box-coverage-weighted-v1',tiles=tiles)
        if capture_digest is not None:manifest['captureMetadataSha256']=capture_digest
        encoded=(json.dumps(manifest,sort_keys=True,indent=2)+'\n').encode('utf8')
        if len(encoded)>65536:raise ValueError('manifest cap')
        with manifest_tmp.open('xb') as f:
            manifest_owned=True;gst2.write_exact(f,encoded);gst2.flush(f)
        # Same-directory rename publishes readiness only after all16 complete/hash-verified.
        # Windows same-directory rename is atomic and refuses an existing destination.
        # This tool targets the same Windows runtime as the capture writer.
        if os.name!='nt':raise OSError('atomic no-overwrite publication requires Windows')
        if capture_metadata_digest(folder)!=capture_digest:raise ValueError('capture metadata changed during build; readiness refused')
        os.rename(manifest_tmp,folder/'ready.json')
        return manifest
    finally:
        if manifest_owned and manifest_tmp.exists():manifest_tmp.unlink()
        lock.unlink()

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('directory',type=Path);args=parser.parse_args()
    started=time.monotonic();build_atlas(args.directory,progress=lambda s:print(s,flush=True))
    print(f'GST2 atlas ready; {time.monotonic()-started:.2f}s; exactly16 files, no temporary image tiles')
