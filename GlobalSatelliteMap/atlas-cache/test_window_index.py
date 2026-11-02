"""Small sparse GST fixtures, all21840 window records, transactional controls."""
import ctypes
import hashlib
import importlib.util
import json
import msvcrt
import os
from pathlib import Path
import struct
import tempfile
import unittest
import gst2

if os.environ.get('GSI_TEST_MODULE'):
    spec=importlib.util.spec_from_file_location('window_index_under_test',os.environ['GSI_TEST_MODULE'])
    index=importlib.util.module_from_spec(spec);spec.loader.exec_module(index)
else:
    import build_window_index as index

HERE=Path(__file__).resolve().parent
OUT=HERE/'_generated/window-index-tests'
OUT.mkdir(parents=True,exist_ok=True)
LAYOUT=gst2.Layout(base=64,levels=6,block=16)
WINDOW=2

def sparse(file,size):
    kernel=ctypes.WinDLL('kernel32',use_last_error=True)
    kernel.DeviceIoControl.argtypes=[ctypes.c_void_p,ctypes.c_uint,ctypes.c_void_p,ctypes.c_uint,ctypes.c_void_p,ctypes.c_uint,ctypes.POINTER(ctypes.c_uint),ctypes.c_void_p]
    done=ctypes.c_uint()
    if not kernel.DeviceIoControl(msvcrt.get_osfhandle(file.fileno()),0x900c4,None,0,None,0,ctypes.byref(done),None):raise ctypes.WinError(ctypes.get_last_error())
    file.truncate(size)

def fixture(folder):
    capture=b'{"format":"GSCAP2","version":2}\n'
    (folder/'capture.json').write_bytes(capture)
    tiles=[];oracle={}
    for z in range(4):
        for x in range(4):
            tile=z*4+x;hashes=[];path=folder/gst2.filename(x,z)
            with path.open('w+b') as f:
                sparse(f,LAYOUT.file_bytes)
                for level in range(6):
                    for plane in range(3):
                        raw=bytes((tile*11+level*17+plane*23+i//LAYOUT.stride(plane)+3*(i%LAYOUT.stride(plane)))&255 for i in range(LAYOUT.length(level,plane)))
                        oracle[tile,level,plane]=raw;hashes.append(hashlib.sha256(raw).digest())
                        f.seek(LAYOUT.offset(level,plane));f.write(raw)
                h=gst2.make_header(x,z,LAYOUT,gst2.full_mask(LAYOUT),hashes,True)
                f.seek(0);f.write(h)
            tiles.append(dict(x=x,z=z,file=path.name,bytes=LAYOUT.file_bytes,headerSha256=hashlib.sha256(h).hexdigest()))
    ready=dict(format='GST2',version=2,complete=True,grid=4,baseSide=64,levels=6,planes=list(gst2.PLANES),rowOrder='minZ-first',columnOrder='minX-first',mipFilter='global-aligned-2x2-box-coverage-weighted-v1',captureMetadataSha256=hashlib.sha256(capture).hexdigest(),tiles=tiles)
    (folder/'ready.json').write_text(json.dumps(ready)+'\n')
    return oracle

class IndexTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory(prefix='gsi-fixture-',dir=OUT)
        self.folder=Path(self.tmp.name)
        self.assertTrue(self.folder.resolve().is_relative_to(OUT.resolve()))
        self.oracle=fixture(self.folder)
        self.ready=(self.folder/'ready.json').read_bytes()
        self.input_hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in self.folder.glob('*.gst')}
    def tearDown(self):self.tmp.cleanup()
    def build(self,**kw):return index.build_index(self.folder,LAYOUT,WINDOW,**kw)
    def inputs_unchanged(self):
        self.assertEqual(self.input_hashes,{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in self.folder.glob('*.gst')})
    def test_all_records_exact_and_one_pass(self):
        stats=self.build();raw=(self.folder/'windows.gsi').read_bytes()
        self.assertEqual(len(raw),2097664)
        self.assertEqual(struct.unpack_from('<4s8I',raw),(b'GSI1',1,1024,2,4,64,6,3,21840))
        self.assertFalse(any(raw[36:64]));self.assertFalse(any(raw[608:1024]))
        ready=json.loads((self.folder/'ready.json').read_bytes())
        self.assertEqual(ready['windowIndexSha256'],hashlib.sha256(raw).hexdigest())
        self.assertEqual(raw[64:96],bytes.fromhex(ready['captureMetadataSha256']))
        for tile,row in enumerate(ready['tiles']):self.assertEqual(raw[96+tile*32:128+tile*32],bytes.fromhex(row['headerSha256']))
        records=0
        # Independent crop oracle: full small planes, not builder's row-digest code.
        for tile in range(16):
            for level in range(6):
                side=LAYOUT.side(level);n=side//WINDOW
                for z in range(n):
                    for x in range(n):
                        for p in range(3):
                            stride=LAYOUT.stride(p);source=self.oracle[tile,level,p]
                            crop=b''.join(source[((z*WINDOW+y)*side+x*WINDOW)*stride:((z*WINDOW+y)*side+(x+1)*WINDOW)*stride] for y in range(WINDOW))
                            at=1024+records*96+p*32
                            self.assertEqual(raw[at:at+32],hashlib.sha256(crop).digest())
                        records+=1
        self.assertEqual(records,21840)
        self.assertEqual(stats['planeReadBytes'],16*(LAYOUT.file_bytes-1024))
        self.assertEqual(stats['wholePlanesVerified'],288)
        self.assertLessEqual(stats['maxSourceReadBytes'],65536)
        self.inputs_unchanged()
        self.assertFalse((self.folder/'.gsi-build.lock').exists())
    def test_whole_plane_corruption_refuses_ready_pin(self):
        p=self.folder/'tile_0_0.gst'
        with p.open('r+b') as f:f.seek(LAYOUT.offset(0,0)+13);f.write(b'\x91')
        with self.assertRaisesRegex(ValueError,'whole-plane SHA256 mismatch'):self.build()
        self.assertEqual((self.folder/'ready.json').read_bytes(),self.ready)
        self.assertFalse((self.folder/'windows.gsi').exists())
    def test_wrong_header_capture_and_missing_tile(self):
        (self.folder/'capture.json').write_bytes(b'{"format":"GSCAP2","version":2,"changed":true}')
        with self.assertRaises(ValueError):self.build()
        self.assertEqual((self.folder/'ready.json').read_bytes(),self.ready)
        (self.folder/'tile_3_3.gst').unlink()
        with self.assertRaises(ValueError):self.build()
    def test_stale_controls_and_different_old_index_preserved(self):
        p=self.folder/'.windows.gsi.tmp';p.write_bytes(b'previous temporary')
        with self.assertRaises(FileExistsError):self.build()
        self.assertEqual(p.read_bytes(),b'previous temporary');p.unlink()
        p=self.folder/'windows.gsi';p.write_bytes(b'old index')
        with self.assertRaises(FileExistsError):self.build()
        self.assertEqual(p.read_bytes(),b'old index')
        self.assertEqual((self.folder/'ready.json').read_bytes(),self.ready);self.inputs_unchanged()
    def test_cancel_cleans_only_owned_controls(self):
        cancelled=[False]
        def observer(kind,row):
            if kind=='row':cancelled[0]=True
        with self.assertRaises(InterruptedError):self.build(cancel=lambda:cancelled[0],observer=observer)
        self.assertEqual((self.folder/'ready.json').read_bytes(),self.ready)
        self.assertFalse((self.folder/'windows.gsi').exists())
        self.assertFalse((self.folder/'.gsi-build.lock').exists());self.inputs_unchanged()
    def test_real_file_sharing_denies_input_writer(self):
        checked=[False]
        def observer(kind,row):
            if kind=='row' and not checked[0]:
                with self.assertRaises(PermissionError):
                    with (self.folder/'tile_3_3.gst').open('r+b'):pass
                with self.assertRaises(PermissionError):(self.folder/'capture.json').write_bytes(b'changed')
                checked[0]=True
        self.build(observer=observer);self.assertTrue(checked[0]);self.inputs_unchanged()
    def test_atomic_ready_failure_and_safe_retry(self):
        def fail(kind,row):
            if kind=='before_ready_replace':raise OSError('injected ready publication failure')
        with self.assertRaisesRegex(OSError,'injected'):self.build(observer=fail)
        self.assertEqual((self.folder/'ready.json').read_bytes(),self.ready)
        # Complete verified index is unadvertised; no old input/index was deleted.
        before=(self.folder/'windows.gsi').read_bytes()
        self.assertFalse((self.folder/'.ready.windows.tmp').exists())
        stats=self.build();self.assertEqual(stats['wholePlanesVerified'],288)
        self.assertEqual((self.folder/'windows.gsi').read_bytes(),before)
        again=self.build();self.assertFalse(again['published']);self.assertEqual(again['wholePlanesVerified'],288)
        self.inputs_unchanged()
    def test_production_defaults_and_bad_lattice(self):
        self.assertEqual(gst2.PRODUCTION.base,8192)
        with self.assertRaises(ValueError):index.build_index(self.folder,LAYOUT,4)
        with self.assertRaises(ValueError):index.build_index(self.folder) # No small-layout CLI bypass.

if __name__=='__main__':unittest.main(verbosity=2)
