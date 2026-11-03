from pathlib import Path
import sys
sys.dont_write_bytecode=True
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import numpy as np
from build_mips import semantic_box
material=np.array([[[-2,65504,-65504,.25],[4,2,0,.75]],[[-6,4,2,1],[0,8,6,2]]],dtype='<f2')
normal=np.array([[[128,255,128,255]]*2]*2,dtype=np.uint8)
height=np.array([[-30,10],[20,40]],dtype='<f4')
m,n,h=semantic_box(material,normal,height)
assert n[0,0,3]==255 and np.array_equal(m[0,0],material.astype(np.float32).mean(axis=(0,1)).astype('<f2'))
assert h[0,0]==10 and np.array_equal(n[0,0,:3],[128,255,128])
normal[:,:,3]=0
m,n,h=semantic_box(material,normal,height)
assert n[0,0,3]==0 and not m.any() and not h.any()
print('PASS existing GST2 semantic_box full coverage preserves linear signed HDR/gloss mean and height; native A0 would erase coverage')
