import hashlib, io, json, struct, tempfile, unittest, tracemalloc
from unittest.mock import patch
from pathlib import Path
import numpy as np
import gst2
import build_mips

GENERATED=Path(__file__).resolve().parent/'_generated'
GENERATED.mkdir(exist_ok=True)
def scratch(): return tempfile.TemporaryDirectory(dir=GENERATED)

class CacheTests(unittest.TestCase):
    def setUp(self): self.layout=gst2.Layout(base=8,levels=3,block=2)
    def fixture(self,folder,layout=None):
        layout=layout or self.layout
        for z in range(4):
            for x in range(4):
                path=folder/gst2.filename(x,z)
                h=gst2.make_header(x,z,layout,gst2.full_mask(layout))
                with path.open('wb') as f:
                    f.write(h);f.truncate(layout.file_bytes)
                mat=np.zeros((layout.base,layout.base,4),'<f2')
                yy,xx=np.indices(mat.shape[:2]);mat[:,:,0]=(xx+x*layout.base)*3;mat[:,:,1]=(yy+z*layout.base)*3;mat[:,:,2]=17;mat[:,:,3]=99
                norm=np.empty_like(mat);norm[:]=[128,255,128,255]
                height=(yy+z*layout.base+xx+x*layout.base).astype('<f4')
                with path.open('r+b') as f:
                    for p,a in enumerate([mat,norm,height]):f.seek(layout.offset(0,p));f.write(a.tobytes())
    def test_exact_production_header(self):
        l=gst2.Layout();raw=gst2.make_header(3,2,l,0)
        self.assertEqual(len(raw),1024);self.assertEqual(l.file_bytes,1431307264)
        self.assertEqual(struct.unpack_from('<10I',raw,4),(2,1024,0,3,2,4,8192,6,3,2048))
        self.assertEqual(struct.unpack_from('<QQ',raw,128),(1024,536870912))
        self.assertEqual(struct.unpack_from('<3I',raw,56),(10,28,41))
        self.assertEqual(gst2.parse_header(raw,l,l.file_bytes)['tile'],(3,2))
    def test_reject_overflow_overlap_reserved_unknown_and_partial(self):
        l=self.layout;h=bytearray(gst2.make_header(0,0,l,0))
        for at,value,fmt in [(128,2**64-4,'<Q'),(128+48,1024,'<Q'),(56,1,'<I'),(12,3,'<I'),(48,2**63,'<Q')]:
            bad=bytearray(h);struct.pack_into(fmt,bad,at,value)
            with self.assertRaises(ValueError):gst2.parse_header(bad,l,l.file_bytes)
        with self.assertRaises(ValueError):gst2.parse_header(h[:-1],l,l.file_bytes)
        with self.assertRaises(ValueError):gst2.parse_header(h,l,l.file_bytes-1)
        with self.assertRaises(ValueError):gst2.Layout(base=2**31,levels=6,block=2)
    def test_partial_write_is_error(self):
        class Partial(io.BytesIO):
            def write(self,b):return super().write(b[:max(1,len(b)//2)])
        f=Partial();gst2.write_exact(f,b'abcdefgh');self.assertEqual(f.getvalue(),b'abcdefgh')
        class Failed(io.BytesIO):
            def write(self,b):return 0
        with self.assertRaises(OSError):gst2.write_exact(Failed(),b'x')
    def test_semantic_linear_gloss_coverage_normal_and_nan(self):
        m=np.array([[[0,0,0,20],[255,255,255,220]],[[255,0,0,10],[7,7,7,7]]],'<f2')
        n=np.array([[[255,128,128,255],[128,255,128,255]],[[128,128,255,0],[128,255,128,255]]],np.uint8)
        h=np.array([[2,6],[100,np.nan]],'<f4')
        a,b,c=build_mips.semantic_box(m,n,h)
        self.assertEqual(a[0,0].tolist(),[127.5,127.5,127.5,120]);self.assertEqual(int(b[0,0,3]),128)
        self.assertGreater(b[0,0,0],210);self.assertGreater(b[0,0,1],210);self.assertAlmostEqual(float(c[0,0]),4)
        n[:,:,-1]=0;a,b,c=build_mips.semantic_box(m,n,h)
        self.assertTrue(np.all(a==0));self.assertEqual(b[0,0].tolist(),[128,255,128,0]);self.assertEqual(float(c[0,0]),0)
    def test_cancelled_normal_has_finite_up_fallback(self):
        m=np.ones((2,2,4),'<f2')*70;n=np.array([[[255,128,128,255],[0,127,127,255]],[[255,128,128,255],[0,127,127,255]]],np.uint8)
        _,norm,height=build_mips.semantic_box(m,n,np.ones((2,2),'<f4'))
        self.assertEqual(norm[0,0].tolist(),[128,255,128,255]);self.assertTrue(np.isfinite(height).all())
    def test_missing_regions_refuses_manifest(self):
        with scratch() as tmp:
            root=Path(tmp);self.fixture(root)
            with (root/gst2.filename(0,0)).open('r+b') as f:f.seek(48);f.write(b'\0'*8)
            with self.assertRaises(ValueError):build_mips.build_atlas(root,self.layout)
            self.assertFalse((root/'ready.json').exists())
    def test_global_box_crop_equals_all_tiles_every_level_and_hashes(self):
        with scratch() as tmp:
            root=Path(tmp);self.fixture(root);l=self.layout
            global_planes=[]
            for p in range(3):
                dtype=('<f2' if p==0 else '<f4' if p==2 else np.uint8);shape=(l.base*4,l.base*4) if p==2 else (l.base*4,l.base*4,4)
                a=np.empty(shape,dtype=dtype)
                for z in range(4):
                    for x in range(4):
                        a[z*l.base:(z+1)*l.base,x*l.base:(x+1)*l.base]=build_mips.read_plane(root/gst2.filename(x,z),l,0,p)
                global_planes.append(a)
            manifest=build_mips.build_atlas(root,l)
            self.assertEqual(manifest['format'],'GST2');self.assertEqual(len(manifest['tiles']),16)
            for level in range(1,l.levels):
                global_planes=list(build_mips.semantic_box(*global_planes));side=l.base>>level
                for z in range(4):
                    for x in range(4):
                        path=root/gst2.filename(x,z)
                        for p,a in enumerate(global_planes):
                            np.testing.assert_array_equal(build_mips.read_plane(path,l,level,p),a[z*side:(z+1)*side,x*side:(x+1)*side])
            self.assertEqual(json.loads((root/'ready.json').read_text())['tiles'],manifest['tiles'])
            for tile in manifest['tiles']:
                path=root/tile['file'];h=gst2.parse_header(path.read_bytes()[:1024],l,l.file_bytes)
                self.assertTrue(h['complete'])
                with path.open('rb') as f:
                    for d in h['descriptors']:
                        f.seek(d['offset']);self.assertEqual(hashlib.sha256(f.read(d['length'])).hexdigest(),d['sha256'])
            with self.assertRaises(FileExistsError):build_mips.build_atlas(root,l)
    def test_invalid_base_canonicalized_and_no_ready_on_failure(self):
        with scratch() as tmp:
            root=Path(tmp);self.fixture(root);path=root/gst2.filename(0,0);l=self.layout
            with path.open('r+b') as f:f.seek(l.offset(0,2));f.write(struct.pack('<f',float('inf')))
            build_mips.build_atlas(root,l)
            self.assertEqual(float(build_mips.read_plane(path,l,0,2)[0,0]),0)
            self.assertEqual(int(build_mips.read_plane(path,l,0,1)[0,0,3]),0)
    def test_half_material_preserves_hdr_and_no_gamma(self):
        mat=np.array([[[1.25,0,.25,1.5],[1,1,.75,.5]],[[1.25,0,.25,1.5],[1,1,.75,.5]]],'<f2')
        nc=np.empty((2,2,4),np.uint8);nc[:]=[128,255,128,255]
        result,_,_=build_mips.semantic_box(mat,nc,np.ones((2,2),'<f4'))
        self.assertEqual(result.dtype,np.dtype('<f2'));self.assertEqual(result[0,0].tolist(),[1.125,.5,.5,1])
    def test_real_width_stripe_memory_bounded(self):
        tracemalloc.start()
        mat=np.ones((32,8192,4),'<f2')*np.float16(1.125)
        nc=np.empty((32,8192,4),np.uint8);nc[:]=[128,255,128,255]
        height=np.ones((32,8192),'<f4')
        result=build_mips.semantic_box(mat,nc,height)
        peak=tracemalloc.get_traced_memory()[1];tracemalloc.stop()
        self.assertLess(peak,32*1024*1024)
        self.assertEqual(result[0].shape,(16,4096,4))
        print(f'8192-wide semantic stripe traced peak {peak} bytes')
    def test_duplicate_identity_corrupt_hash_and_interrupted_publish(self):
        with scratch() as tmp:
            root=Path(tmp);self.fixture(root);l=self.layout;path=root/gst2.filename(0,0)
            with path.open('r+b') as f:f.seek(16);f.write(struct.pack('<I',1))
            with self.assertRaises(ValueError):build_mips.build_atlas(root,l)
            self.assertFalse((root/'ready.json').exists())
        with scratch() as tmp:
            root=Path(tmp);self.fixture(root);l=self.layout
            with patch('build_mips.os.rename',side_effect=OSError('injected rename failure')):
                with self.assertRaises(OSError):build_mips.build_atlas(root,l)
            self.assertFalse((root/'ready.json').exists());self.assertFalse((root/'.gst2-build.lock').exists())
            path=root/gst2.filename(0,0)
            with path.open('r+b') as f:f.seek(l.offset(0,0));f.write(b'\xff')
            with self.assertRaises(ValueError):build_mips.build_atlas(root,l)
            self.assertFalse((root/'ready.json').exists())
    def test_resume_after_finalize_interruption_and_partial_header_failure(self):
        with scratch() as tmp:
            root=Path(tmp);self.fixture(root);l=self.layout
            original=build_mips.finalize_file;calls=[]
            def interrupted(*args):
                if calls:raise OSError('injected finalize failure')
                calls.append(1);return original(*args)
            with patch('build_mips.finalize_file',side_effect=interrupted):
                with self.assertRaises(OSError):build_mips.build_atlas(root,l)
            self.assertFalse((root/'ready.json').exists());build_mips.build_atlas(root,l)
            self.assertTrue((root/'ready.json').exists())
    def test_nonfinite_material_and_max_finite_height(self):
        mat=np.ones((2,2,4),'<f2')*np.float16(1.125);mat[0,0,0]=np.nan
        nc=np.empty((2,2,4),np.uint8);nc[:]=[128,255,128,255]
        height=np.full((2,2),np.finfo(np.float32).max,'<f4')
        a,b,c=build_mips.semantic_box(mat,nc,height)
        self.assertEqual(a[0,0].tolist(),[1.125]*4);self.assertEqual(int(b[0,0,3]),191);self.assertTrue(np.isfinite(c).all())
    def raw_fixture(self,root):
        self.fixture(root);l=self.layout;rng=np.random.default_rng(321)
        raw=rng.integers(0,256,(l.base*4,l.base*4,4),np.uint8)
        for z in range(4):
            for x in range(4):
                packed=np.zeros((l.base,l.base,8),np.uint8);packed[:,:,:4]=raw[z*l.base:(z+1)*l.base,x*l.base:(x+1)*l.base]
                with (root/gst2.filename(x,z)).open('r+b') as f:
                    f.seek(0);f.write(gst2.make_header(x,z,l,gst2.full_mask(l),raw_packed=True));f.seek(l.offset(0,0));f.write(packed.tobytes())
        return raw
    def scalar_decode(self,raw,x,y):
        rows,cols=raw.shape[:2];f=np.float32;c=raw[y,x].astype(np.float32)/f(255)
        samples=[raw[min(max(y+dy,0),rows-1),min(max(x+dx,0),cols-1)].astype(np.float32)/f(255) for dx,dy in [(1,0),(-1,0),(0,1),(0,-1)]]
        weights=[f(abs(f(a[2]-c[2]))<f(f(30)/f(255))) for a in samples]
        total=f(sum(weights))
        if total==0:weights=[f(1),f(0),f(0),f(0)];total=f(1)
        other=f(f(f(f(weights[0]*samples[0][1])+f(weights[1]*samples[1][1]))+f(weights[2]*samples[2][1]))+f(weights[3]*samples[3][1]))/total
        co=f((other if (x&1)==(y&1) else c[1])-f(.5));cg=f((c[1] if (x&1)==(y&1) else other)-f(.5))
        return np.array([f(f(c[2]+co)-cg),f(c[2]+cg),f(f(c[2]-co)-cg),c[3]],'<f2')
    def test_raw_decode_global_primary_block_and_rolling_stripe_borders(self):
        with scratch() as tmp:
            root=Path(tmp);raw=self.raw_fixture(root);l=self.layout
            expected=np.empty(raw.shape,'<f2')
            for y in range(raw.shape[0]):
                for x in range(raw.shape[1]):expected[y,x]=self.scalar_decode(raw,x,y)
            with patch('build_mips.RAW_STRIPE_ROWS',3):build_mips.build_atlas(root,l)
            for z in range(4):
                for x in range(4):np.testing.assert_array_equal(build_mips.read_plane(root/gst2.filename(x,z),l,0,0),expected[z*l.base:(z+1)*l.base,x*l.base:(x+1)*l.base])
            # Includes actual native2048 boundary and odd global parity without a full tile.
            strip=np.random.default_rng(7).integers(0,256,(5,2054,4),np.uint8)
            padded=np.pad(strip,((1,1),(1,1),(0,0)),mode='edge')
            decoded=build_mips.decode_packed(padded,0,0)
            for y in range(5):
                for x in [0,1,2,2047,2048,2049,2053]:np.testing.assert_array_equal(decoded[y,x],self.scalar_decode(strip,x,y))
    def test_raw_interrupted_conversion_refuses_recapture_and_bad_padding(self):
        with scratch() as tmp:
            root=Path(tmp);self.raw_fixture(root)
            with patch('build_mips.decode_raw_tile',side_effect=OSError('partial conversion')):
                with self.assertRaises(OSError):build_mips.build_atlas(root,self.layout)
            self.assertFalse((root/'ready.json').exists())
            with self.assertRaisesRegex(ValueError,'recapture'):build_mips.build_atlas(root,self.layout)
        with scratch() as tmp:
            root=Path(tmp);self.raw_fixture(root);path=root/gst2.filename(0,0)
            with path.open('r+b') as f:f.seek(self.layout.offset(0,0)+4);f.write(b'\x01')
            with self.assertRaisesRegex(ValueError,'padding'):build_mips.build_atlas(root,self.layout)
            with path.open('rb') as f:f.seek(12);self.assertEqual(struct.unpack('<I',f.read(4))[0],2)
    def test_stale_manifest_temporary_is_preserved(self):
        with scratch() as tmp:
            root=Path(tmp);self.fixture(root);stale=root/'.ready.json.tmp';stale.write_bytes(b'previous interrupted publication')
            with self.assertRaises(FileExistsError):build_mips.build_atlas(root,self.layout)
            self.assertEqual(stale.read_bytes(),b'previous interrupted publication');self.assertFalse((root/'ready.json').exists())
    def test_capture_metadata_pinned_in_ready_manifest(self):
        with scratch() as tmp:
            root=Path(tmp);self.fixture(root);raw=b'{"format":"GSCAP2","version":2,"sources":[]}\n';(root/'capture.json').write_bytes(raw)
            result=build_mips.build_atlas(root,self.layout)
            self.assertEqual(result['captureMetadataSha256'],hashlib.sha256(raw).hexdigest())
            self.assertEqual(json.loads((root/'ready.json').read_text())['captureMetadataSha256'],hashlib.sha256(raw).hexdigest())
    def test_capture_metadata_changed_midbuild_refuses_ready(self):
        with scratch() as tmp:
            root=Path(tmp);self.fixture(root);capture=root/'capture.json';capture.write_text('{"format":"GSCAP2","version":2,"sources":[]}')
            changed=[]
            def progress(_):
                if not changed:capture.write_text('{"format":"GSCAP2","version":2,"sources":["changed"]}');changed.append(True)
            with self.assertRaisesRegex(ValueError,'capture metadata changed'):build_mips.build_atlas(root,self.layout,progress=progress)
            self.assertFalse((root/'ready.json').exists());self.assertFalse((root/'.ready.json.tmp').exists())
    def test_capture_metadata_bounds_and_format_before_modification(self):
        for raw in (b'x'*65537,b'{',b'{"format":"wrong","version":2}',b'{"format":"GSCAP2","version":3}',b'[]'):
            with scratch() as tmp:
                root=Path(tmp);self.fixture(root);(root/'capture.json').write_bytes(raw)
                with self.assertRaises(ValueError):build_mips.build_atlas(root,self.layout)
                self.assertFalse((root/'ready.json').exists())
                with (root/gst2.filename(0,0)).open('rb') as f:f.seek(12);self.assertEqual(f.read(4),b'\0'*4)
    def test_canonicalization_interruption_keeps_invalid_coverage(self):
        with scratch() as tmp:
            root=Path(tmp);self.fixture(root);l=self.layout;path=root/gst2.filename(0,0)
            with path.open('r+b') as f:f.seek(l.offset(0,0));f.write(np.array([np.nan],'<f2').tobytes())
            original=build_mips.write_rows
            def fail_material(file,layout,level,plane,y,array):
                if plane==0:raise OSError('interrupt after invalid coverage persisted')
                return original(file,layout,level,plane,y,array)
            with patch('build_mips.write_rows',side_effect=fail_material):
                with self.assertRaises(OSError):build_mips.build_atlas(root,l)
            self.assertEqual(int(build_mips.read_plane(path,l,0,1)[0,0,3]),0)
            build_mips.build_atlas(root,l)
            self.assertEqual(int(build_mips.read_plane(path,l,0,1)[0,0,3]),0)
    def test_raw_decode_full_width_peak_and_threshold_fallback(self):
        tracemalloc.start()
        edges=np.empty((16,4,8192,4),np.uint8)
        packed=np.zeros((33,8192,8),np.uint8)
        halo=np.zeros((34,8194,4),np.uint8);halo[:,:,2]=100;halo[:,:,1]=70;halo[:,:,3]=221
        result=build_mips.decode_packed(halo,8192,8192)
        peak=tracemalloc.get_traced_memory()[1];tracemalloc.stop()
        self.assertLess(peak,32*1024*1024);self.assertEqual(result.shape,(32,8192,4))
        self.assertEqual(edges.nbytes,2097152);self.assertEqual(packed.shape,(33,8192,8))
        print(f'8192-wide raw decode plus edge cache traced peak {peak} bytes')
        # All four Y differences reject: fallback MUST be right neighbor chroma.
        raw=np.zeros((3,3,4),np.uint8);raw[:,:,2]=255;raw[:,:,1]=10;raw[:,:,3]=255
        raw[1,1,2]=0;raw[1,1,1]=80;raw[1,2,1]=220
        decoded=build_mips.decode_packed(np.pad(raw,((1,1),(1,1),(0,0)),mode='edge'),0,0)
        np.testing.assert_array_equal(decoded[1,1],self.scalar_decode(raw,1,1))
        # Exactly30-byte luma gaps use the native float32 comparison, not an integer approximation.
        for y in range(3):
            for x in range(3):raw[y,x,2]=[100,130,129][x];raw[y,x,1]=[10,180,220][x]
        decoded=build_mips.decode_packed(np.pad(raw,((1,1),(1,1),(0,0)),mode='edge'),0,0)
        for y in range(3):
            for x in range(3):np.testing.assert_array_equal(decoded[y,x],self.scalar_decode(raw,x,y))

if __name__=='__main__':unittest.main(verbosity=2)
