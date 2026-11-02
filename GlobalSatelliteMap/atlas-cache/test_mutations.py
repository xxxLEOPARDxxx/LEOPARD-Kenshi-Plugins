"""Targeted negative controls, in-memory production copies only."""
import io
from pathlib import Path
import sys
import types
import unittest
import build_mips
import test_cache

source=Path(build_mips.__file__).read_text()
mutations=[
 ('independent-primary-clamp',
  "halo[1:-1,0]=edges[(x-1,z)]['right'][y:y+rows] if x else raw[:rows,0,:4]",
  'halo[1:-1,0]=raw[:rows,0,:4]',
  'test_raw_decode_global_primary_block_and_rolling_stripe_borders'),
 ('stripe-local-parity',
  'decoded=decode_packed(halo,x*side,z*side+y)',
  'decoded=decode_packed(halo,0,0)',
  'test_raw_decode_global_primary_block_and_rolling_stripe_borders'),
 ('read-converted-prior-row',
  "halo[0,1:-1]=carry if y else edges[(x,max(z-1,0))]['bottom' if z else 'top']",
  "halo[0,1:-1]=read_raw_rows(f,layout,y-1,1)[0,:,:4] if y else edges[(x,max(z-1,0))]['bottom' if z else 'top']",
  'test_raw_decode_global_primary_block_and_rolling_stripe_borders'),
 ('erase-invalidity-before-coverage',
  'write_rows(file,layout,0,1,y,nc);gst2.flush(file)\n            write_rows(file,layout,0,2,y,height);write_rows(file,layout,0,0,y,mat)',
  'write_rows(file,layout,0,0,y,mat)\n            write_rows(file,layout,0,1,y,nc);write_rows(file,layout,0,2,y,height)',
  'test_canonicalization_interruption_keeps_invalid_coverage'),
 ('clip-native-hdr',
  "out_material=(total(colours)/denominator[:,:,None]).astype('<f2');del colours",
  "out_material=np.clip(total(colours)/denominator[:,:,None],0,1).astype('<f2');del colours",
  'test_half_material_preserves_hdr_and_no_gamma'),
]
for name,old,new,test in mutations:
    assert source.count(old)==1,name
    module=types.ModuleType('build_mips')
    exec(compile(source.replace(old,new),str(Path(build_mips.__file__))+' mutant '+name,'exec'),module.__dict__)
    previous=sys.modules['build_mips'];sys.modules['build_mips']=module;test_cache.build_mips=module
    try:
        result=unittest.TextTestRunner(stream=io.StringIO()).run(test_cache.CacheTests(test))
        assert result.failures and not result.errors, (name,'mutant was not rejected by the intended assertion')
        print(name+': correctly rejected')
    finally:sys.modules['build_mips']=previous;test_cache.build_mips=previous
print('GST2: five negative controls rejected; production files unchanged')
