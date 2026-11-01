"""Offline actual-input coarse/fine SID-weight boundary audit; no game calls."""
from pathlib import Path
from collections import Counter
import argparse,hashlib,json
from PIL import Image

parser=argparse.ArgumentParser()
parser.add_argument('--snapshot',type=Path,default=Path('D:/DEV/Kenshi/research/global-satellite-20261031/biome-current-36248/biome-selection-snapshot.json'))
parser.add_argument('--game',type=Path,default=Path('D:/SteamLibrary/steamapps/common/Kenshi'))
parser.add_argument('--output',type=Path)
args=parser.parse_args();here=Path(__file__).resolve().parent
manifest=json.loads((here/'geography-inputs.json').read_text())
snapshot_bytes=args.snapshot.read_bytes()
assert hashlib.sha256(snapshot_bytes).hexdigest()=='5cb085baa67a7100181daf4edf25bdf3e47f5c162f7ef551fa8eb5910e565d60'
snapshot=json.loads(snapshot_bytes)
blend=args.game/manifest['blend']['path'];blend_bytes=blend.read_bytes()
assert len(blend_bytes)==manifest['blend']['bytes'] and hashlib.sha256(blend_bytes).hexdigest()==manifest['blend']['sha256']
image=Image.open(blend).convert('RGBA');assert image.size==(1024,1024);pixels=image.load()
cells={(c['x'],c['z']):c for c in snapshot['cells'] if 8<=c['x']<16 and 8<=c['z']<16}
assert len(cells)==64 and len(manifest['cells'])==64
sid_codes={b['sid']:i+1 for i,b in enumerate(manifest['biomes'])}

def morton(x,z):
    return sum(((x>>b)&1)<<(2*b)|((z>>b)&1)<<(2*b+1) for b in range(5))

def select(cell,index):
    tree=bytes.fromhex(cell['tree']);mask=tree[index] if (tree[0]&224)>32 else tree[0]
    return tuple((sid,ch) for ch,sid in reversed(list(enumerate(cell['slots']))) if sid and mask&(1<<ch))[:4]

def weights(selection,rgba,total):
    # Exact integers. total255 for pixels,510 for two-pixel bilinear midpoint.
    result={sid:rgba[ch] for sid,ch in selection[1:]}
    result[selection[0][0]]=total-sum(result.values())
    return {sid:value for sid,value in result.items() if value}

root={};fine_weights={};combinations=Counter()
for (cx,cz),cell in cells.items():
    selected=select(cell,1);root[cx,cz]=selected
    entry=manifest['cells'][(cz-8)*8+cx-8]
    assert hashlib.sha256(bytes.fromhex(cell['tree'])).hexdigest()==entry['tree_sha256']
    selector_codes=[sid_codes[sid]|(channel<<4) for sid,channel in selected]
    assert selector_codes+[0]*(4-len(selector_codes))==entry['selector']
    for z in range(32):
        for x in range(32):
            gx,gz=cx*32+x,cz*32+z;leaf=select(cell,1024|morton(x,z));combinations[leaf]+=1
            actual=weights(selected,pixels[gx,gz],255);expected=weights(leaf,pixels[gx,gz],255)
            assert actual==expected,('root/fine mismatch',gx,gz,actual,expected)
            fine_weights[gx,gz]=actual

counts=Counter()
for (x,z),a in fine_weights.items():
    for dx,dz in ((1,0),(0,1)):
        if (x+dx,z+dz) not in fine_weights:continue
        b=fine_weights[x+dx,z+dz]
        coarse=(x//32,z//32)!=((x+dx)//32,(z+dz)//32)
        delta=sum(abs(a.get(sid,0)-b.get(sid,0)) for sid in set(a)|set(b))
        counts[('coarse' if coarse else 'internal')+'_adjacent_pairs']+=1
        counts[('coarse' if coarse else 'internal')+'_source_L1_gt_one']+=delta>255
        if coarse:
            rgba=[pixels[x,z][ch]+pixels[x+dx,z+dz][ch] for ch in range(4)]
            left=weights(root[x//32,z//32],rgba,510)
            right=weights(root[(x+dx)//32,(z+dz)//32],rgba,510)
            assert left==right,('same bilinear RGBA/coarse decoding mismatch',x,z,left,right)

assert len(fine_weights)==65536 and len(combinations)==57 and counts['coarse_adjacent_pairs']==3584
result=dict(scope='offline exact primary1,1 input weights; not full shader/mip/runtime parity',fine_centers=65536,root_leaf_weight_mismatches=0,fine_ordered_combinations=57,coarse_bilinear_boundary_pairs=3584,coarse_decoder_discontinuities=0,source_edges=dict(counts),snapshot_sha256=hashlib.sha256(snapshot_bytes).hexdigest(),blend_sha256=hashlib.sha256(blend_bytes).hexdigest())
output=json.dumps(result,indent=2)+'\n'
if args.output:args.output.write_text(output)
print(output,end='')
