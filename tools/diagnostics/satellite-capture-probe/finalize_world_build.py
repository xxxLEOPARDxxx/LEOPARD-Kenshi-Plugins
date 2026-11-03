"""Inspect or finalize a CLOSED private WorldBuild candidate; never activate it."""
from pathlib import Path
from contextlib import ExitStack
import argparse,hashlib,importlib.util,json,os,re,sys,time
import numpy as np
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[3]
CACHE=ROOT/'GlobalSatelliteMap/atlas-cache'
sys.path.insert(0,str(CACHE))
import gst2
import build_mips
import build_window_index as index
spec=importlib.util.spec_from_file_location('world_build_capture_identity',ROOT/'GlobalSatelliteMap/capture_identity.py')
identity=importlib.util.module_from_spec(spec);spec.loader.exec_module(identity)
MAX_METADATA=65536
MARKER='.world-build-finalizing.json'
DONE='.world-build-finalized.json'
STRIPE_ROWS=32
def require(ok,message):
 if not ok:raise ValueError(message)
def completion(raw,directory,capture_hash):
 require(isinstance(raw,dict),'completion must be an object')
 for key,value in [('format','GSM_WORLD_BUILD_COMPLETION'),('version',1),('expectedWindows',4096),('windows',4096),('error',0),('quarantined',False),('writerJoined',True),('producerJoined',True),('captureSha256',capture_hash)]:
  require(key in raw and type(raw[key]) is type(value) and raw[key]==value,'completion refusal: '+key)
 require(isinstance(raw.get('directory'),str) and Path(raw['directory']).is_absolute() and os.path.normcase(os.path.abspath(raw['directory']))==os.path.normcase(str(directory)),'completion candidate differs')
 index.sha(capture_hash)
 return raw

def metadata(path):
 with index.locked_read(path) as (f,p,before):
  raw,value=index.small_json(f,before);index.stable(f,p,before)
 return raw,value

def capture_metadata(raw,value):
 require(isinstance(value,dict),'capture must be an object')
 for k,v in [('format','GSCAP2'),('version',2),('grid',4),('baseSide',8192),('span',294912)]:
  require(k in value and type(value[k]) is type(v) and value[k]==v,'capture identity: '+k)
 require(value.get('origin')==[-147456,-147456] and all(type(v) is int for v in value['origin']),'capture origin')
 rows=value.get('sources');require(isinstance(rows,list) and 0<len(rows)<=512,'capture source count')
 lookup={}
 for row in rows:
  require(isinstance(row,dict) and set(row)=={'path','sha256'} and isinstance(row['path'],str),'capture source row')
  index.sha(row['sha256']);require(row['path'] not in lookup,'duplicate source row');lookup[row['path']]=row['sha256']
 for name in ('engine','modsCfg','height'):
  path=value.get(name+'Path');pin=value.get(name+'Sha256');index.sha(pin)
  require(isinstance(path,str) and lookup.get(path)==pin,'capture source alias/pin: '+name)
 text=(ROOT/'tools/diagnostics/satellite-capture-probe/region-catalog/RegionCatalogData.h').read_text()
 compiled=re.search(r'ManifestSha256\[\]="([0-9a-f]{64})"',text)
 require(compiled is not None and value.get('sourceManifestSha256')==compiled.group(1),'compiled region catalogue differs')
 return hashlib.sha256(raw).hexdigest()

def scan_base(file,layout):
 """Read <=2MiB source stripe; reject instead of canonicalizing bad base pixels."""
 hashes=[];read_bytes=0
 for plane in range(3):
  file.seek(layout.offset(0,plane));digest=hashlib.sha256()
  stride=layout.stride(plane)
  for y in range(0,layout.base,STRIPE_ROWS):
   count=min(STRIPE_ROWS,layout.base-y)*layout.base
   data=bytes(index.read_exact(file,count*stride));digest.update(data);read_bytes+=len(data)
   if plane==0:require(bool(np.isfinite(np.frombuffer(data,'<f2')).all()),'nonfinite material/gloss base')
   elif plane==1:require(bool((np.frombuffer(data,'u1').reshape(-1,4)[:,3]==255).all()),'base coverage must be exactly255')
   else:require(bool(np.isfinite(np.frombuffer(data,'<f4')).all()),'nonfinite world-height base')
  hashes.append(digest.hexdigest())
 return dict(basePlaneSha256=hashes,semanticPixels=layout.base**2,baseReadBytes=read_bytes)

def inspect_open(file,path,before,layout,semantic):
 raw=bytes(index.read_exact(file,1024));parsed=gst2.parse_header(raw,layout,before.st_size)
 expected='tile_%d_%d.gst.partial'%parsed['tile']
 require(path.name==expected,'partial filename/coordinates differ')
 require(not parsed['complete'] and not parsed['raw_packed'],'only flags0 private base partials accepted')
 row=dict(file=path.name,tile=list(parsed['tile']),bytes=before.st_size,mask=parsed['mask'],headerSha256=hashlib.sha256(raw).hexdigest(),fileIdentity=[before.st_dev,before.st_ino],mtimeNs=before.st_mtime_ns)
 if semantic:
  require(parsed['mask']==gst2.full_mask(layout),'base block ledger incomplete')
  row.update(scan_base(file,layout))
 index.stable(file,path,before)
 return row

def inspect_file(path,layout=gst2.PRODUCTION,semantic=False):
 with index.locked_read(path) as (file,path,before):return inspect_open(file,path,before,layout,semantic)

def candidate_path(directory,allowed_parent=None):
 directory=index.checked_path(directory);require(directory.is_dir(),'candidate directory required')
 if allowed_parent is not None:
  parent=index.checked_path(allowed_parent);require(parent.is_dir() and directory.parent==parent and directory!=parent,'candidate must be an immediate child of explicitly allowed parent')
 return directory

def expected_files():return {'tile_%d_%d.gst.partial'%(x,z) for z in range(4) for x in range(4)}

def inspect_directory(directory,layout=gst2.PRODUCTION):
 directory=candidate_path(directory)
 tiles=[inspect_file(p,layout) for p in sorted(directory.iterdir()) if p.name.endswith('.gst.partial')]
 return dict(directory=str(directory),ready=False,note='Read-only private partial inspection; no complete-atlas claim',tiles=tiles)

def preflight(directory,allowed_parent,layout=gst2.PRODUCTION,stack=None):
 directory=candidate_path(directory,allowed_parent)
 require({p.name for p in directory.iterdir()}==expected_files(),'candidate must contain exactly16 partials and no stale/foreign/finalized files')
 if stack is None:
  with ExitStack() as owned:return [r[3] for r in preflight(directory,allowed_parent,layout,owned)]
 rows=[]
 for z in range(4):
  for x in range(4):
   file,path,before=stack.enter_context(index.locked_read(directory/f'tile_{x}_{z}.gst.partial'))
   row=inspect_open(file,path,before,layout,True);rows.append((file,path,before,row))
 return rows

def write_new(path,raw):
 index.checked_path(path,missing_leaf=True)
 with path.open('xb') as file:gst2.write_exact(file,raw);gst2.flush(file)

def encode(value):return (json.dumps(value,sort_keys=True,indent=2)+'\n').encode('utf-8')

def finalize(directory,allowed_parent,capture_receipt,completion_receipt):
 """Offline only. Candidate is never moved to the viewer's captures directory."""
 return _finalize(directory,allowed_parent,capture_receipt,completion_receipt,gst2.PRODUCTION,identity.verify)

def _finalize(directory,allowed_parent,capture_receipt,completion_receipt,layout,verify_sources):
 """Same production phases; compact layout/source verifier injected by tests only."""
 require(os.name=='nt','Windows sharing/no-replace contract required')
 directory=candidate_path(directory,allowed_parent)
 capture_receipt=index.checked_path(capture_receipt);completion_receipt=index.checked_path(completion_receipt)
 require(capture_receipt.parent!=directory and completion_receipt.parent!=directory,'receipts must be outside fresh candidate')
 capture_raw,capture=metadata(capture_receipt);capture_hash=capture_metadata(capture_raw,capture)
 completion_raw,closed=metadata(completion_receipt);completion(closed,directory,capture_hash)
 verify_sources(capture_receipt)
 with ExitStack() as stack:
  rows=preflight(directory,allowed_parent,layout,stack=stack)
  # Keep every input sharing-locked through all preflight and receipt checks.
  verify_sources(capture_receipt)
  require(metadata(capture_receipt)[0]==capture_raw and metadata(completion_receipt)[0]==completion_raw,'receipt changed during preflight')
  for f,p,b,r in rows:
   index.stable(f,p,b);f.seek(0);require(hashlib.sha256(index.read_exact(f,1024)).hexdigest()==r['headerSha256'],'partial header changed')
  marker=dict(format='GSM_WORLD_BUILD_FINALIZING',version=1,directory=str(directory),captureSha256=capture_hash,completionSha256=hashlib.sha256(completion_raw).hexdigest(),tiles=[r[3] for r in rows])
  write_new(directory/MARKER,encode(marker))
 # Locks must close before same-directory renames. Recheck paths/identity before
 # each no-replace rename. This is a trusted local CLOSED-candidate workflow,
 # not a claim of atomic live publication or hostile namespace-race immunity.
 for _,path,before,row in rows:
  candidate_path(directory,allowed_parent)
  with index.locked_read(path) as (f,p,b):
   require([b.st_dev,b.st_ino]==row['fileIdentity'] and b.st_size==row['bytes'] and b.st_mtime_ns==row['mtimeNs'],'partial changed before rename')
   require(hashlib.sha256(index.read_exact(f,1024)).hexdigest()==row['headerSha256'],'partial header changed before rename')
  dest=path.with_suffix('');index.checked_path(dest,missing_leaf=True);require(not dest.exists(),'rename destination exists');os.rename(path,dest)
 write_new(directory/'capture.json',capture_raw)
 build_mips.build_atlas(directory,layout,progress=lambda n:print(str(n),flush=True))
 result=index.build_index(directory,layout,window=layout.base//32,observer=lambda kind,value:print(f'{kind}: {value}',flush=True) if kind=='tile_complete' else None)
 ready_raw,ready=metadata(directory/'ready.json')
 require(ready.get('captureMetadataSha256')==capture_hash and ready.get('windowIndexSha256')==result['windowIndexSha256'],'ready pins missing/different')
 with index.locked_read(directory/'windows.gsi') as (f,p,b):
  require(b.st_size==index.INDEX_BYTES and index.hash_stream(f,b.st_size,None)==result['windowIndexSha256'],'final index digest differs');index.stable(f,p,b)
 done=dict(format='GSM_WORLD_BUILD_FINALIZED',version=1,captureSha256=capture_hash,windowIndexSha256=result['windowIndexSha256'],readySha256=hashlib.sha256(ready_raw).hexdigest(),windows=4096,activated=False)
 write_new(directory/DONE,encode(done));return done
def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('action',choices=['inspect','finalize']);parser.add_argument('directory',type=Path)
 parser.add_argument('--allowed-parent',type=Path);parser.add_argument('--capture-receipt',type=Path);parser.add_argument('--completion-receipt',type=Path)
 args=parser.parse_args();started=time.monotonic()
 if args.action=='inspect':result=inspect_directory(args.directory)
 else:
  if not all((args.allowed_parent,args.capture_receipt,args.completion_receipt)):parser.error('finalize requires --allowed-parent, --capture-receipt and --completion-receipt')
  result=finalize(args.directory,args.allowed_parent,args.capture_receipt,args.completion_receipt)
 print(json.dumps(result,sort_keys=True,indent=2));print('elapsedSeconds=%.3f'%(time.monotonic()-started))
if __name__=='__main__':main()
