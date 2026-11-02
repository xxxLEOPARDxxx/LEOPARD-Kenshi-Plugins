"""Build GSI1 in one streaming GST2 pass; no images/recapture or GST mutation.

Production CLI is fixed8192/256/6levels. Tests inject64/2 with the same1365
records/cell. A verified, unadvertised windows.gsi may remain after a failed
ready publication; rerunning rechecks all planes and safely adopts identical bytes.
Different preexisting index/control files are never silently overwritten/deleted.
"""
import argparse
from contextlib import ExitStack,contextmanager
import ctypes
from ctypes import wintypes
import hashlib
import json
import msvcrt
import os
from pathlib import Path
import re
import struct
import time
import gst2

HEADER=1024
RECORDS_PER_TILE=1365
RECORDS=21840
RECORD_BYTES=96
BODY_BYTES=RECORDS*RECORD_BYTES
INDEX_BYTES=HEADER+BODY_BYTES
_kernel=ctypes.WinDLL('kernel32',use_last_error=True)
_kernel.CreateFileW.argtypes=[wintypes.LPCWSTR,wintypes.DWORD,wintypes.DWORD,ctypes.c_void_p,wintypes.DWORD,wintypes.DWORD,wintypes.HANDLE]
_kernel.CreateFileW.restype=wintypes.HANDLE
_kernel.CloseHandle.argtypes=[wintypes.HANDLE]
_kernel.GetFinalPathNameByHandleW.argtypes=[wintypes.HANDLE,wintypes.LPWSTR,wintypes.DWORD,wintypes.DWORD]
_kernel.GetFinalPathNameByHandleW.restype=wintypes.DWORD
_kernel.GetDriveTypeW.argtypes=[wintypes.LPCWSTR]
_kernel.GetDriveTypeW.restype=wintypes.UINT
_INVALID=ctypes.c_void_p(-1).value

def checked_path(path,missing_leaf=False):
    path=Path(os.path.abspath(path))
    text=str(path)
    if len(text)>=260 or len(path.drive)!=2 or path.drive[1]!=':' or not path.drive[0].isalpha() or any(ord(c)<32 or ord(c)==127 for c in text) or ':' in text[2:]:
        raise ValueError('canonical local-drive path required')
    if _kernel.GetDriveTypeW(path.anchor) in (0,1,4):raise ValueError('remote/unknown drive refused')
    for part in reversed((path,)+tuple(path.parents)):
        try:attributes=part.lstat().st_file_attributes
        except FileNotFoundError:
            if missing_leaf and part==path:continue
            raise
        if attributes&0x400:raise ValueError('reparse path/ancestor refused: '+str(part))
    return path

def final_path(handle):
    buf=ctypes.create_unicode_buffer(264)
    n=_kernel.GetFinalPathNameByHandleW(handle,buf,len(buf),0)
    if not n or n>=len(buf) or not buf.value.startswith('\\\\?\\'):raise OSError('file physical path unavailable')
    return buf.value[4:]

@contextmanager
def locked_read(path):
    path=checked_path(path)
    h=_kernel.CreateFileW(str(path),0x80000000,1,None,3,0x08200000,None)
    if h==_INVALID:raise ctypes.WinError(ctypes.get_last_error())
    fd=None
    try:
        if final_path(h).casefold()!=str(path).casefold():raise ValueError('physical path alias refused')
        fd=msvcrt.open_osfhandle(h,os.O_RDONLY|os.O_BINARY);h=None
        f=os.fdopen(fd,'rb',buffering=0);fd=None
    except BaseException:
        if fd is not None:os.close(fd)
        if h is not None:_kernel.CloseHandle(h)
        raise
    try:yield f,path,os.fstat(f.fileno())
    finally:f.close()

def stable(file,path,before):
    after=os.fstat(file.fileno())
    if (before.st_size,before.st_mtime_ns,before.st_dev,before.st_ino)!=(after.st_size,after.st_mtime_ns,after.st_dev,after.st_ino):raise ValueError('input changed during index build')
    h=msvcrt.get_osfhandle(file.fileno())
    if final_path(h).casefold()!=str(path).casefold():raise ValueError('input directory identity changed')

def checkpoint(cancel):
    if cancel is not None and cancel():raise InterruptedError('window-index build cancelled')

def read_exact(file,n,cancel=None):
    result=bytearray(n);at=0
    while at<n:
        checkpoint(cancel)
        take=min(65536,n-at)
        got=file.readinto(memoryview(result)[at:at+take])
        if not got or got>take:raise OSError('short index/source read')
        at+=got
    return result

def unique_object(pairs):
    out={}
    for k,v in pairs:
        if k in out:raise ValueError('duplicate JSON field: '+k)
        out[k]=v
    return out

def small_json(file,before):
    if not 0<before.st_size<=65536:raise ValueError('metadata size cap')
    raw=bytes(read_exact(file,before.st_size))
    return raw,json.loads(raw,object_pairs_hook=unique_object)

def sha(value):
    if not isinstance(value,str) or re.fullmatch('[0-9a-f]{64}',value) is None:raise ValueError('SHA256 field')
    return value

def ready_tiles(ready,layout):
    if ready.get('format')!='GST2' or ready.get('version')!=2 or ready.get('complete') is not True or ready.get('grid')!=4 or ready.get('baseSide')!=layout.base or ready.get('levels')!=6 or ready.get('planes')!=list(gst2.PLANES) or ready.get('rowOrder')!='minZ-first' or ready.get('columnOrder')!='minX-first' or ready.get('mipFilter')!='global-aligned-2x2-box-coverage-weighted-v1':raise ValueError('ready identity/dimensions')
    sha(ready.get('captureMetadataSha256'))
    if 'windowIndexSha256' in ready:sha(ready['windowIndexSha256'])
    rows=ready.get('tiles')
    if not isinstance(rows,list) or len(rows)!=16:raise ValueError('exactly16 ready tiles required')
    out=[None]*16
    for row in rows:
        x,z=row.get('x'),row.get('z')
        if type(x)is not int or type(z)is not int or not 0<=x<4 or not 0<=z<4 or out[z*4+x]is not None or row.get('bytes')!=layout.file_bytes or row.get('file')!=gst2.filename(x,z):raise ValueError('ready tile identity/duplicate')
        sha(row.get('headerSha256'));out[z*4+x]=row
    return out

def record(tile,level,x,z,layout,window):
    n=layout.side(level)//window
    return tile*RECORDS_PER_TILE+sum((layout.side(l)//window)**2 for l in range(level))+z*n+x

def index_header(capture_digest,tile_rows,layout,window):
    h=bytearray(HEADER)
    struct.pack_into('<4s8I',h,0,b'GSI1',1,HEADER,window,4,layout.base,6,3,RECORDS)
    h[64:96]=bytes.fromhex(capture_digest)
    for i,row in enumerate(tile_rows):h[96+i*32:128+i*32]=bytes.fromhex(row['headerSha256'])
    return h

def hash_stream(file,n,cancel):
    h=hashlib.sha256();left=n
    while left:
        data=read_exact(file,min(65536,left),cancel);h.update(data);left-=len(data)
    checkpoint(cancel)
    return h.hexdigest()

def build_index(folder,layout=gst2.PRODUCTION,window=256,cancel=None,observer=None):
    if os.name!='nt':raise OSError('Windows atomic/sharing contract required')
    if layout.grid!=4 or layout.levels!=6 or layout.base//window!=32 or layout.side(5)!=window:raise ValueError('index lattice must be32windows..1window across sixlevels')
    folder=checked_path(folder)
    if not folder.is_dir():raise ValueError('atlas directory required')
    final=folder/'windows.gsi';temp=folder/'.windows.gsi.tmp';ready_temp=folder/'.ready.windows.tmp';lock=folder/'.gsi-build.lock'
    for p in (temp,ready_temp):
        if p.exists():raise FileExistsError('stale index control file requires inspection: '+str(p))
    if {p.name for p in folder.glob('*.gst')}!={gst2.filename(x,z) for z in range(4) for x in range(4)}:raise ValueError('exactly16 canonical GST files required')
    fd=os.open(lock,os.O_CREAT|os.O_EXCL|os.O_WRONLY,0o600);os.close(fd)
    temp_owned=ready_owned=False
    stats={'planeReadBytes':0,'wholePlanesVerified':0,'windows':RECORDS,'maxSourceReadBytes':0,'indexBytes':INDEX_BYTES,'published':False}
    try:
        with ExitStack() as stack:
            ready_file,ready_path,ready_stat=stack.enter_context(locked_read(folder/'ready.json'))
            ready_raw,ready=small_json(ready_file,ready_stat);tiles=ready_tiles(ready,layout)
            capture_file,capture_path,capture_stat=stack.enter_context(locked_read(folder/'capture.json'))
            capture_raw,capture=small_json(capture_file,capture_stat)
            if capture.get('format')!='GSCAP2' or capture.get('version')!=2 or hashlib.sha256(capture_raw).hexdigest()!=ready['captureMetadataSha256']:raise ValueError('capture metadata identity/pin differs')
            handles=[]
            for tile,row in enumerate(tiles):
                checkpoint(cancel)
                f,path,before=stack.enter_context(locked_read(folder/row['file']))
                raw=bytes(read_exact(f,1024,cancel));parsed=gst2.parse_header(raw,layout,before.st_size)
                if not parsed['complete'] or parsed['tile']!=(tile%4,tile//4) or hashlib.sha256(raw).hexdigest()!=row['headerSha256']:raise ValueError('GST complete header identity/pin differs')
                handles.append((f,path,before,raw,parsed))
            body=bytearray(BODY_BYTES)
            for tile,(f,path,before,raw,parsed) in enumerate(handles):
                for level in range(6):
                    side=layout.side(level);across=side//window
                    for plane in range(3):
                        d=parsed['descriptors'][level*3+plane];f.seek(d['offset']);whole=hashlib.sha256();stride=layout.stride(plane);part=window*stride;pitch=side*stride
                        for wz in range(across):
                            digests=[hashlib.sha256() for _ in range(across)]
                            for y in range(window):
                                row_data=read_exact(f,pitch,cancel);whole.update(row_data)
                                stats['planeReadBytes']+=pitch;stats['maxSourceReadBytes']=max(stats['maxSourceReadBytes'],pitch)
                                view=memoryview(row_data)
                                for wx,h in enumerate(digests):h.update(view[wx*part:(wx+1)*part])
                                if observer:observer('row',(tile,level,plane,wz*window+y,stats['planeReadBytes']))
                            for wx,h in enumerate(digests):
                                at=record(tile,level,wx,wz,layout,window)*96+plane*32;body[at:at+32]=h.digest()
                        if whole.hexdigest()!=d['sha256']:raise ValueError('whole-plane SHA256 mismatch: '+path.name+f' level{level} plane{plane}')
                        stats['wholePlanesVerified']+=1
                if observer:observer('tile_complete',(tile,stats['planeReadBytes']))
            h=index_header(ready['captureMetadataSha256'],tiles,layout,window)
            digest=hashlib.sha256();digest.update(h);digest.update(body);index_sha=digest.hexdigest()
            with temp.open('xb') as f:
                temp_owned=True;gst2.write_exact(f,h);gst2.write_exact(f,body);gst2.flush(f)
            with locked_read(temp) as (f,path,before):
                if before.st_size!=INDEX_BYTES or hash_stream(f,before.st_size,cancel)!=index_sha:raise OSError('index write readback mismatch')
                stable(f,path,before)
            if observer:observer('before_publish',stats)
            checkpoint(cancel)
            for f,path,before,raw,_ in handles:
                stable(f,path,before);f.seek(0)
                if bytes(read_exact(f,1024,cancel))!=raw:raise ValueError('GST header changed during index build')
            stable(capture_file,capture_path,capture_stat);capture_file.seek(0)
            if bytes(read_exact(capture_file,capture_stat.st_size,cancel))!=capture_raw:raise ValueError('capture changed during index build')
            stable(ready_file,ready_path,ready_stat);ready_file.seek(0)
            if bytes(read_exact(ready_file,ready_stat.st_size,cancel))!=ready_raw:raise ValueError('ready changed during index build')
            # Existing verified deterministic index is reused; a DIFFERENT file
            # is never overwritten. New source identities require a new atlas dir.
            if final.exists():
                f,path,before=stack.enter_context(locked_read(final))
                if before.st_size!=INDEX_BYTES or hash_stream(f,before.st_size,cancel)!=index_sha:raise FileExistsError('different existing windows.gsi requires inspection')
                stable(f,path,before);temp.unlink();temp_owned=False
            else:
                os.rename(temp,final);temp_owned=False
                f,path,before=stack.enter_context(locked_read(final))
                if before.st_size!=INDEX_BYTES or hash_stream(f,before.st_size,cancel)!=index_sha:raise OSError('published index readback mismatch')
                stable(f,path,before)
            new_ready=dict(ready);new_ready['windowIndexSha256']=index_sha
            encoded=(json.dumps(new_ready,sort_keys=True,indent=2)+'\n').encode('utf-8')
            if len(encoded)>65536:raise ValueError('updated ready exceeds metadata cap')
            if ready.get('windowIndexSha256')==index_sha:
                stats['windowIndexSha256']=index_sha;return stats
            with ready_temp.open('xb') as f:
                ready_owned=True;gst2.write_exact(f,encoded);gst2.flush(f)
            checkpoint(cancel)
            if observer:observer('before_ready_replace',stats)
            # All source handles remain locked. Only our ready handle is closed
            # immediately before the single atomic metadata replacement.
            ready_file.close();checked_path(folder)
            os.replace(ready_temp,ready_path);ready_owned=False;stats['published']=True
            stats['windowIndexSha256']=index_sha;return stats
    finally:
        if temp_owned and temp.exists():temp.unlink()
        if ready_owned and ready_temp.exists():ready_temp.unlink()
        lock.unlink()

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory',type=Path)
    args=parser.parse_args();start=time.monotonic()
    def progress(kind,value):
        if kind=='tile_complete':print(f'Verified tile{value[0]+1}/16; source bytes{value[1]}',flush=True)
    result=build_index(args.directory,observer=progress)
    print(json.dumps(result,sort_keys=True),flush=True)
    print(f'GSI1 verified/pinned; elapsed{time.monotonic()-start:.2f}s; no GST/image changes',flush=True)
