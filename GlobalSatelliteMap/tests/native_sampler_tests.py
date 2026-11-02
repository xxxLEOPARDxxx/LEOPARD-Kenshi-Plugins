"""Execute ONLY the fixed native pure filter converter in private test memory.
No DLL loading/DllMain, game calls, source-file modification, or live process access.
"""
import ctypes,hashlib,itertools
from pathlib import Path
from test_pe_contract import PE
p=PE(Path('D:/SteamLibrary/steamapps/common/Kenshi/RenderSystem_Direct3D11_x64.dll'))
assert hashlib.sha256(p.data).hexdigest()=='5f224882ded5b6d3f68f14d6bb4e1762267bbb602b7b9733bd153663d1541bb9'
assert p.data[p.offset(0x34632):p.offset(0x34640)]==bytes.fromhex('c7433400000000c74338ffff7f7f')
k=ctypes.WinDLL('kernel32',use_last_error=True)
k.VirtualAlloc.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_uint,ctypes.c_uint];k.VirtualAlloc.restype=ctypes.c_void_p
k.VirtualProtect.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_uint,ctypes.POINTER(ctypes.c_uint)]
k.VirtualFree.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_uint]
size=p.u32(p.opt+56);base=k.VirtualAlloc(None,size,0x3000,4);assert base
try:
 for rva,_,raw,n in p.sections:ctypes.memmove(base+rva,p.data[raw:raw+n],n)
 old=ctypes.c_uint();assert k.VirtualProtect(base,size,0x20,ctypes.byref(old))
 native=ctypes.CFUNCTYPE(ctypes.c_uint,ctypes.c_uint,ctypes.c_uint,ctypes.c_uint,ctypes.c_bool)(base+0x28b70)
 count=0
 for a,b,c,comparison in itertools.product(range(4),range(4),range(4),(False,True)):
  expected=0x55 if 3 in (a,b,c) else ((0x10 if a==2 else 0)|(4 if b==2 else 0)|(1 if c==2 else 0))
  if comparison:expected|=0x80
  assert native(a,b,c,comparison)==expected,(a,b,c,comparison)
  count+=1
 print(f'Native sampler PASS {count} exact machine-code cases; native MaxLOD=FLT_MAX including FO_NONE')
finally:assert k.VirtualFree(base,0,0x8000)
