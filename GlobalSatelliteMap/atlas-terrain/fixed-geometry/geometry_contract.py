"""Owned arithmetic proof of the pinned native topology contract, not a native render.
Reads bounded rows from actual LE16 fullmap; no game, native call or installed writes.
"""
from pathlib import Path
import hashlib, json, math, struct

HERE=Path(__file__).resolve().parent
TIFF=Path('D:/SteamLibrary/steamapps/common/Kenshi/data/newland/land/fullmap.tif')
EXPECTED='6f7d4bc7baaf80dbf8db91e6f367db71f87fe38f053a34108f192584c9637b18'
def f32(x): return struct.unpack('<f',struct.pack('<f',x))[0]
def sha(p):
    h=hashlib.sha256()
    with p.open('rb') as f:
        for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
    return h.hexdigest()
assert sha(TIFF)==EXPECTED
with TIFF.open('rb') as f:
    h=f.read(8);assert h[:4]==b'II*\0';f.seek(struct.unpack('<I',h[4:])[0]);n=struct.unpack('<H',f.read(2))[0]
    tags={}
    for _ in range(n):
        tag,typ,count,val=struct.unpack('<HHII',f.read(12));tags[tag]=(typ,count,val)
assert {tag:tags[tag][2] for tag in (256,257,258,259,273,274,277,278,279)}=={256:16385,257:16385,258:16,259:1,273:43686,274:1,277:1,278:16385,279:536936450}
# Cache one small rectangle including the extra source sample ring, never full TIFF.
x0,z0,w,h=7102,7358,196,134
rows=[];read=0
with TIFF.open('rb') as f:
    for z in range(z0,z0+h):
        f.seek(43686+2*(z*16385+x0));b=f.read(w*2);assert len(b)==w*2
        rows.append(struct.unpack('<'+'H'*w,b));read+=len(b)
scale=f32(f32(9800)/f32(65535))
def height(x,z):return f32(rows[z-z0][x-x0]*scale)
ring=((0,-1),(1,-1),(1,0),(0,1),(-1,1),(-1,0))
def cross(a,b):return tuple(f32(f32(a[(i+1)%3]*b[(i+2)%3])-f32(a[(i+2)%3]*b[(i+1)%3])) for i in range(3))
def normal(x,z,clamp=None):
    center=height(x,z);v=[]
    for dx,dz in ring:
        xx,zz=x+dx,z+dz
        if clamp:xx=min(clamp[2],max(clamp[0],xx));zz=min(clamp[3],max(clamp[1],zz))
        v.append((float(dx*18),f32(height(xx,zz)-center),float(dz*18)))
    total=[0.,0.,0.]
    for i in range(6):
        c=cross(v[i],v[(i+1)%6]);total=[f32(total[k]+c[k]) for k in range(3)]
    length=f32(math.sqrt(f32(f32(f32(total[0]*total[0])+f32(total[1]*total[1]))+f32(total[2]*total[2]))))
    inverse=f32(1/length)
    return tuple(f32(-f32(q*inverse)) for q in total)
def vertex(x,z):return (-147456+x*18,height(x,z),-147456+z*18,*normal(x,z))
def block(x,z,side):return {(i,j):vertex(i,j) for j in range(z,z+side+1) for i in range(x,x+side+1)}
a,b=block(7104,7360,128),block(7152,7360,128)
common=sorted(a.keys()&b.keys());assert all(a[k]==b[k] for k in common)
# Chunk edge copies and four-corner intersections have exactly the same global normal.
chunk_a,chunk_b=block(7104,7360,64),block(7168,7360,64)
edges=sorted(chunk_a.keys()&chunk_b.keys());assert all(chunk_a[k]==chunk_b[k] for k in edges)
max_clamped=max(math.degrees(math.acos(max(-1,min(1,sum(u*v for u,v in zip(normal(x,z),normal(x,z,(7104,7360,7168,7424)))))))) for x,z in edges)
assert max_clamped>0.01
# Native 5AA0 row-pair strip: 65 columns, 64 rows, four degenerate-join indices.
n=65;indices=[]
for z in range(n-1):
    if z:indices += [indices[-1],indices[-1],z*n,z*n]
    for x in range(n):indices += [z*n+x,(z+1)*n+x]
assert len(indices)==(n*2+4)*(n-1)-4==8572
tri=[]
for k in range(2,len(indices)):
    t=(indices[k-2],indices[k-1],indices[k])
    if len(set(t))==3:tri.append(frozenset(t))
expected=[]
for z in range(n-1):
    for x in range(n-1):
        aa=z*n+x;bb=(z+1)*n+x;cc=aa+1;dd=bb+1
        expected.extend([frozenset((aa,bb,cc)),frozenset((bb,cc,dd))])
assert len(tri)==8192 and set(tri)==set(expected)
# Pixel-centre samples are piecewise planar, not bilinear; opposite diagonal is observably wrong.
bilinear_delta=other_diagonal_delta=0.
for z in range(7400,7464):
    for x in range(7140,7204):
        aa,bb,cc,dd=height(x,z),height(x,z+1),height(x+1,z),height(x+1,z+1)
        for u,v in ((.25,.25),(.75,.25),(.25,.75),(.75,.75)):
            native=aa*(1-u-v)+cc*u+bb*v if u+v<=1 else dd*(u+v-1)+bb*(1-u)+cc*(1-v)
            bilinear=(aa*(1-u)+cc*u)*(1-v)+(bb*(1-u)+dd*u)*v
            opposite=aa*(1-u)+dd*v+cc*(u-v) if u>=v else aa*(1-v)+dd*u+bb*(v-u)
            bilinear_delta=max(bilinear_delta,abs(native-bilinear));other_diagonal_delta=max(other_diagonal_delta,abs(native-opposite))
assert bilinear_delta>.001 and other_diagonal_delta>.001
out={'scope':'offline owned arithmetic contract, not native image/float-bit parity','tiffSha256':EXPECTED,'readBytesAfterFullIdentityHash':read,'sourceWindow':[x0,z0,w,h],'overlapVertices':len(common),'sharedEdgeVertices':len(edges),'clampedNormalNegativeControlMaxDegrees':max_clamped,'indicesPer65Square':len(indices),'trianglesPer65Square':len(tri),'bilinearNegativeControlMaxMeters':bilinear_delta,'oppositeDiagonalNegativeControlMaxMeters':other_diagonal_delta,'sampleRing':ring,'sourceStepMeters':18,'sourceHeightScaleFloat':scale,'outputBasePixelMeters':9}
(HERE/'geometry-contract.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out,indent=2))

# Literal native40B builder packet, not an invented POSITION/normal rearrangement.
def raw_vertex(x,z):
    fine=normal(x,z)
    if x&1 and z&1:neighbors=((x-1,z-1),(x+1,z+1))
    elif x&1:neighbors=((x-1,z),(x+1,z))
    elif z&1:neighbors=((x,z-1),(x,z+1))
    else:neighbors=None
    coarse=fine;target=height(x,z)
    if neighbors:
        n1,n2=(normal(*q) for q in neighbors);v=[f32(n1[k]+n2[k]) for k in range(3)]
        length=f32(math.sqrt(f32(f32(f32(v[0]*v[0])+f32(v[1]*v[1]))+f32(v[2]*v[2]))));inv=f32(1/length)
        coarse=tuple(f32(q*inv) for q in v);target=f32(f32(sum(height(*q) for q in neighbors))*f32(.5))
    return (-147456+x*18,height(x,z),-147456+z*18,*fine,*coarse,target)
(HERE/'height-window.u16').write_bytes(struct.pack('<4I',x0,z0,w,h)+b''.join(struct.pack('<'+'H'*w,*row) for row in rows))
(HERE/'expected-patch.f32').write_bytes(b''.join(struct.pack('<10f',*raw_vertex(x,z)) for z in range(7360,7425) for x in range(7104,7169)))

# Root's first256-square/3m four-patch raster fixture centred at(-13824,-13824).
with TIFF.open('rb') as f:
    pixels=[]
    for z in range(7358,7491):
        f.seek(43686+2*(z*16385+7358));pixels.append(f.read(133*2))
    assert all(len(q)==266 for q in pixels)
(HERE/'raster-window.u16').write_bytes(struct.pack('<4I',7358,7358,133,133)+b''.join(pixels))

(HERE/'expected-indices.u16').write_bytes(struct.pack('<8572H',*indices))
