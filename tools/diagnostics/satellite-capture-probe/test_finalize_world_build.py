"""Compact controls of real finalizer helpers; no production render/finalization."""
from pathlib import Path
import hashlib,importlib.util,json,os,struct,tempfile,unittest
import numpy as np
HERE=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('finalize_under_test',HERE/'finalize_world_build.py');tool=importlib.util.module_from_spec(spec);spec.loader.exec_module(tool)

def receipt(directory,pin):
 return dict(format='GSM_WORLD_BUILD_COMPLETION',version=1,directory=str(directory),expectedWindows=4096,windows=4096,error=0,quarantined=False,writerJoined=True,producerJoined=True,captureSha256=pin)

class FinalizeTests(unittest.TestCase):
 def setUp(self):
  self.temporary=tempfile.TemporaryDirectory(prefix='gsm-finalize-',dir=HERE/'_generated')
  self.root=Path(self.temporary.name);self.candidate=self.root/'candidate';self.candidate.mkdir()
  self.layout=tool.gst2.Layout(base=64,levels=6,block=16)
 def tearDown(self):self.temporary.cleanup()
 def fill(self,x=0,z=0):
  path=self.candidate/f'tile_{x}_{z}.gst.partial';layout=self.layout
  with path.open('xb') as f:
   f.write(tool.gst2.make_header(x,z,layout,tool.gst2.full_mask(layout)));f.truncate(layout.file_bytes)
   for plane in range(3):
    f.seek(layout.offset(0,plane))
    if plane==0:a=np.full((layout.base,layout.base,4),.5,'<f2');a[:,:,0]=-2;a[:,:,1]=8
    elif plane==1:a=np.full((layout.base,layout.base,4),128,'u1');a[:,:,3]=255
    else:a=np.full((layout.base,layout.base),123.25,'<f4')
    f.write(a.tobytes())
  return path
 def test_completion_requires_explicit_success(self):
  pin='a'*64;r=receipt(self.candidate,pin)
  self.assertEqual(tool.completion(r,self.candidate,pin),r)
  for key,value in [('windows',4),('expectedWindows',4),('error',1),('error',False),('quarantined',0),('writerJoined',1),('producerJoined',False),('version',True),('captureSha256','b'*64),('directory',str(self.root))]:
   bad=dict(r);bad[key]=value
   with self.subTest(key=key,value=value),self.assertRaises(ValueError):tool.completion(bad,self.candidate,pin)
  for key in r:
   bad=dict(r);del bad[key]
   with self.subTest(missing=key),self.assertRaises(ValueError):tool.completion(bad,self.candidate,pin)
 def test_actual_header_and_hdr_semantics(self):
  path=self.fill();row=tool.inspect_file(path,self.layout,semantic=True)
  self.assertEqual(row['tile'],[0,0]);self.assertEqual(row['mask'],tool.gst2.full_mask(self.layout));self.assertEqual(row['semanticPixels'],4096)
  self.assertEqual(row['baseReadBytes'],4096*16)
 def test_semantic_refuses_holes_and_nonfinite_without_changes(self):
  path=self.fill();original=path.read_bytes()
  for plane,relative,value in [(0,0,struct.pack('<H',0x7c00)),(1,3,b'\0'),(2,0,struct.pack('<f',float('nan')))]:
   changed=bytearray(original);at=self.layout.offset(0,plane)+relative;changed[at:at+len(value)]=value;path.write_bytes(changed)
   with self.subTest(plane=plane),self.assertRaises(ValueError):tool.inspect_file(path,self.layout,semantic=True)
   self.assertEqual(path.read_bytes(),changed)
  path.write_bytes(original)
 def test_all16_preflight_before_any_rename(self):
  for z in range(4):
   for x in range(4):self.fill(x,z)
  rows=tool.preflight(self.candidate,self.root,self.layout)
  self.assertEqual(len(rows),16);self.assertEqual(len(list(self.candidate.glob('*.partial'))),16)
  final=self.candidate/'tile_3_3.gst.partial';raw=bytearray(final.read_bytes());struct.pack_into('<I',raw,12,2);final.write_bytes(raw)
  with self.assertRaises(ValueError):tool.preflight(self.candidate,self.root,self.layout)
  self.assertEqual(len(list(self.candidate.glob('*.partial'))),16);self.assertFalse(list(self.candidate.glob('*.gst')))
 def test_foreign_stale_and_mixed_state_refuse(self):
  for name in ['ready.json','.world-build-finalizing.json','windows.gsi','tile_0_0.gst','foreign.txt']:
   p=self.candidate/name;p.write_bytes(b'x')
   with self.subTest(name=name),self.assertRaises(ValueError):tool.preflight(self.candidate,self.root,self.layout)
   self.assertEqual(p.read_bytes(),b'x');p.unlink()
  with self.assertRaises(ValueError):tool.preflight(self.candidate,self.candidate,self.layout)
 def test_duplicate_json_and_alias_refuse(self):
  p=self.root/'receipt.json';p.write_bytes(b'{"version":1,"version":2}')
  with self.assertRaises(ValueError):tool.metadata(p)
  p.write_bytes(b'x'*65537)
  with self.assertRaises(ValueError):tool.metadata(p)
 def test_partial_inspection_never_completes(self):
  p=self.fill();raw=bytearray(p.read_bytes());struct.pack_into('<Q',raw,48,0);p.write_bytes(raw)
  result=tool.inspect_directory(self.candidate,self.layout)
  self.assertEqual(len(result['tiles']),1);self.assertFalse(result['ready']);self.assertEqual(result['tiles'][0]['mask'],0)
 def test_compact_real_finalization_pipeline_and_restart_refusal(self):
  for z in range(4):
   for x in range(4):self.fill(x,z)
  compiled=(tool.ROOT/'tools/diagnostics/satellite-capture-probe/region-catalog/RegionCatalogData.h').read_text()
  catalogue=tool.re.search(r'ManifestSha256\[\]="([0-9a-f]{64})"',compiled).group(1)
  capture=dict(format='GSCAP2',version=2,grid=4,baseSide=8192,span=294912,origin=[-147456,-147456],sourceManifestSha256=catalogue)
  sources=[]
  for name in ('engine','modsCfg','height'):
   path=self.root/(name+'.source');path.write_bytes(name.encode());pin=hashlib.sha256(path.read_bytes()).hexdigest();capture[name+'Path']=str(path);capture[name+'Sha256']=pin;sources.append(dict(path=str(path),sha256=pin))
  capture['sources']=sources;raw=tool.encode(capture);cp=self.root/'capture.receipt.json';cp.write_bytes(raw)
  closed=self.root/'closed.json';closed.write_bytes(tool.encode(receipt(self.candidate,hashlib.sha256(raw).hexdigest())))
  def verify(path):
   self.assertEqual(path,cp)
   for row in capture['sources']:self.assertEqual(hashlib.sha256(Path(row['path']).read_bytes()).hexdigest(),row['sha256'])
  done=tool._finalize(self.candidate,self.root,cp,closed,self.layout,verify)
  self.assertFalse(done['activated']);self.assertEqual(len(list(self.candidate.glob('*.gst'))),16);self.assertFalse(list(self.candidate.glob('*.partial')))
  self.assertEqual((self.candidate/'capture.json').read_bytes(),raw)
  for path in self.candidate.glob('*.gst'):self.assertTrue(tool.gst2.verify_file(path,self.layout)['complete'])
  with self.assertRaises(ValueError):tool._finalize(self.candidate,self.root,cp,closed,self.layout,verify)

if __name__=='__main__':
 (HERE/'_generated').mkdir(exist_ok=True);unittest.main()
