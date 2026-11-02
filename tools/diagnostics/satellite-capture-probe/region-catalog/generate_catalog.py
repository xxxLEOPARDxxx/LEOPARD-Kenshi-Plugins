"""Fixed64-region generator. Offline reads only; no engine/process API.

All verification completes before any output is written. File hashing streams
at most1MiB at a time; compressed textures are never decoded or pooled in RAM.
"""
from pathlib import Path, PureWindowsPath
import argparse, hashlib, json, math, struct, time

HERE=Path(__file__).resolve().parent
GAME=Path('D:/SteamLibrary/steamapps/common/Kenshi')
SNAPSHOT=Path('D:/DEV/Kenshi/research/global-satellite-20261031/biome-current-36248')
MANIFEST_SHA='69b7a25921afa65cecbeef898bd2ca80eca0eeaaf16237790c8b3477337a4023'
ROLES=('base','slope','vertical','grass','dirt','road')

def require(condition, message):
    if not condition:raise ValueError(message)

def sha(path):
    digest=hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda:stream.read(1024*1024),b''):digest.update(chunk)
    return digest.hexdigest()

def safe_relative(value):
    require(isinstance(value,str) and value and '\\' not in value and ':' not in value and not value.startswith('/'),'noncanonical relative path')
    require(all(part and part not in ('.','..') for part in value.split('/')),'unsafe relative path')
    return value

def verify_asset(path, entry, kind):
    require(path.is_file() and path.stat().st_size==entry['bytes'],'source missing or changed byte count: '+str(path))
    require(sha(path)==entry['sha256'],'source SHA256 changed: '+str(path))
    with path.open('rb') as f:header=f.read(128)
    if kind=='dds':
        require(entry['fourcc'] in ('DXT5','DXT1'),'unsupported BC source')
        block=16 if entry['fourcc']=='DXT5' else 8
        require(header[:4]==b'DDS ' and (entry['width'],entry['height'],entry['mips'])==(2048,2048,12),'DDS metadata/header shape')
        fields=((4,124),(8,659463),(12,2048),(16,2048),(20,512*512*block),(24,0),(28,12),
                (76,32),(80,4),(108,4198408),(112,0))
        require(len(header)==128 and all(struct.unpack_from('<I',header,at)[0]==want for at,want in fields),'strict DDS header refused')
        require(header[84:88].decode('ascii')==entry['fourcc'],'DDS FourCC differs')
        require(entry['bytes']==128+sum((((2048>>m)+3)//4)**2*block for m in range(12)),'DDS complete payload refused')
    elif kind=='png':
        require(header[:8]==b'\x89PNG\r\n\x1a\n' and header[12:16]==b'IHDR','PNG header refused')
        require(struct.unpack_from('>IIBB',header,16)==(entry['width'],entry['height'],8,6),'RGBA8 PNG shape refused')

def load_pinned_inputs(manifest_path=HERE/'source-manifest.json',game=GAME,snapshot_dir=SNAPSHOT):
    require(sha(manifest_path)==MANIFEST_SHA,'fixed source manifest identity changed')
    manifest=json.loads(manifest_path.read_text())
    require(manifest['version']==1 and len(manifest['dds'])==219 and len(manifest['painted'])==128,'manifest counts/version refused')
    snapshot_file=snapshot_dir/'biome-selection-snapshot.json';active_file=snapshot_dir/'active-biomes.json'
    require(sha(snapshot_file)==manifest['snapshot_sha256'] and sha(active_file)==manifest['active_sha256'],'pinned snapshot/active identity changed')
    seen=set()
    for key,kind in (('dds','dds'),('painted','png'),('identities','raw')):
        for entry in manifest[key]:
            name=safe_relative(entry['path']);require(name not in seen,'duplicate manifest path');seen.add(name)
            verify_asset(game/name,entry,kind)
    snapshot=json.loads(snapshot_file.read_text());active=json.loads(active_file.read_text())
    return dict(manifest=manifest,snapshot=snapshot,active=active,game=game)

def selected(cell,index):
    tree=bytes.fromhex(cell['tree']);mask=tree[index] if tree[0]&224>32 else tree[0]
    return tuple((sid,ch) for ch,sid in reversed(list(enumerate(cell['slots']))) if sid and mask&(1<<ch))[:4]

def morton(x,z):return sum(((x>>b)&1)<<(2*b)|((z>>b)&1)<<(2*b+1) for b in range(5))

def build_catalog(inputs):
    snapshot=inputs['snapshot'];active=inputs['active'];manifest=inputs['manifest']
    require(snapshot['grid']==32 and snapshot['fine']==32 and snapshot['bounds']==[-147456,-147456,294912,294912],'world extent/grid refused')
    require(len(snapshot['cells'])==1024,'coarse cell count refused')
    records={r['sid']:r for r in active['records'] if r.get('sid')}
    cells={}
    for c in snapshot['cells']:
        key=(c['x'],c['z'])
        require(type(key[0]) is int and type(key[1]) is int and 0<=key[0]<32 and 0<=key[1]<32 and key not in cells,'coarse coordinate/duplicate refused')
        require(len(c['slots'])==5 and all(s is None or s in records for s in c['slots']),'unknown source SID/slot shape')
        require(len({s for s in c['slots'] if s})==sum(s is not None for s in c['slots']),'duplicate source SID in cell')
        tree=bytes.fromhex(c['tree'])
        require(len(tree) in (1,2048) and ((tree[0]&224>32)==(len(tree)==2048)),'mask tree shape/mode refused')
        require(selected(c,1),'empty root selection refused');cells[key]=c
    sids=sorted({s for c in cells.values() for s in c['slots'] if s})
    require(len(sids)==57,'fixed global SID count refused')
    sid_index={s:i for i,s in enumerate(sids)}
    assets=[dict(f,format=77 if f['fourcc']=='DXT5' else 71) for f in manifest['dds']]
    require([a['path'] for a in assets]==sorted(a['path'] for a in assets),'asset ordering refused')
    by_path={a['path']:i for i,a in enumerate(assets)}
    canonical={}
    for i,a in enumerate(assets):canonical.setdefault((a['format'],a['sha256']),i)
    biomes=[]
    for sid in sids:
        record=records[sid]
        require(all(math.isfinite(v) for v in record['float'].values()),'nonfinite active parameter')
        require('ground colour' in record['int'] and 'fade distance' in record['float'],'missing typed fade field')
        sources=[]
        for kind,suffix in enumerate(('',' normal')):
            indexes=[]
            for role in ROLES:
                value=record['file'].get('texture '+role+suffix) or record['file'].get('texture base'+suffix)
                require(value,'missing base/role path')
                windows=PureWindowsPath(value)
                require(not windows.is_absolute() and '..' not in windows.parts,'unsafe native source path')
                path=safe_relative('/'.join(windows.parts).lower())
                require(path in by_path,'active biome path not in fixed inventory')
                index=by_path[path];require(assets[index]['format']==(77 if kind==0 else 71),'role format mismatch')
                indexes.append(index)
            sources.append(indexes)
        biomes.append(dict(sid=sid,sources=sources))
    painted=sorted(manifest['painted'],key=lambda p:(p['kind'],p['z'],p['x']))
    require(all((p['kind'],p['x'],p['z'],p['width'],p['height'])==(i//64,i%8,(i%64)//8,1024 if i<64 else 2048,1024 if i<64 else 2048) for i,p in enumerate(painted)),'painted layout/shape refused')
    regions=[]
    for rz in range(8):
        for rx in range(8):
            region_cells=[cells[rx*4+x,rz*4+z] for z in range(4) for x in range(4)]
            used=sorted({sid for c in region_cells for sid,ch in selected(c,1)})
            require(1<=len(used)<=15,'local shader SID capacity exceeded')
            local={s:i+1 for i,s in enumerate(used)}
            pools=[]
            for kind in range(2):
                pool=sorted({canonical[assets[source]['format'],assets[source]['sha256']] for sid in used for source in biomes[sid_index[sid]]['sources'][kind]})
                require(1<=len(pool)<=46,'fixed texture-bank capacity exceeded');pools.append(pool)
            bindings=[]
            for sid in used:
                source=biomes[sid_index[sid]];layers=[]
                for kind in range(2):
                    layers.append([pools[kind].index(canonical[assets[a]['format'],assets[a]['sha256']]) for a in source['sources'][kind]])
                bindings.append(dict(source=sid_index[sid],layers=layers))
            out_cells=[]
            for c in region_cells:
                tree=bytes.fromhex(c['tree']);root=selected(c,1)
                # Validate every native fine node: local shader catalog never
                # discards a biome that the fine selector can actually choose.
                for z in range(32):
                    for x in range(32):require(set(selected(c,1024|morton(x,z)))<=set(root),'fine selection crosses selected root')
                out_cells.append(dict(slots=[sid_index[s]+1 if s else 0 for s in c['slots']],
                    selector=[local[s]|(ch<<4) for s,ch in root]+[0]*(4-len(root)),
                    root_mask=tree[1] if len(tree)==2048 else tree[0],tree_bytes=len(tree),tree_sha256=hashlib.sha256(tree).hexdigest()))
            banks=[]
            for pool in pools:banks.extend(pool+[pool[0]]*(46-len(pool)))
            px,pz=rx//2,rz//2
            paints=[kind*64+(pz*2+layer//2)*8+px*2+layer%2 for kind in range(2) for layer in range(4)]
            selector=[0]*256
            for n,c in enumerate(out_cells):selector[((n//4)*8+n%4)*4:((n//4)*8+n%4)*4+4]=c['selector']
            regions.append(dict(x=rx,z=rz,primary=[px,pz],origin=[-147456+rx*36864,-147456+rz*36864],
                paint_base=[-147456+px*73728,-147456+pz*73728],painted=paints,biomes=bindings,
                diffuse_count=len(pools[0]),normal_count=len(pools[1]),banks=banks,cells=out_cells,selector=selector))
    return dict(version=1,snapshot_sha256=manifest['snapshot_sha256'],active_sha256=manifest['active_sha256'],
        manifest_sha256=MANIFEST_SHA,assets=assets,biomes=biomes,painted=painted,regions=regions,identities=manifest['identities'])

def cpp_data(doc):
    q=lambda s:json.dumps(s,ensure_ascii=True)
    array=lambda values:'{'+','.join(str(v) for v in values)+'}'
    rows=['// Generated by generate_catalog.py; fixed snapshot/assets, not live ownership proof.',
          '#ifndef SATELLITE_REGION_CATALOG_DATA_H','#define SATELLITE_REGION_CATALOG_DATA_H',
          '#include "RegionCatalog.h"','namespace SatelliteRegion {',
          'static const char SnapshotSha256[]='+q(doc['snapshot_sha256'])+';',
          'static const char ActiveSha256[]='+q(doc['active_sha256'])+';',
          'static const char ManifestSha256[]='+q(doc['manifest_sha256'])+';',
          'static const Asset Assets[]={']
    rows += ['{%s,%s,%du,%du},'%(q(a['path']),q(a['sha256']),a['bytes'],a['format']) for a in doc['assets']]
    rows += ['};','static const Biome Biomes[]={']
    rows += ['{%s,{%s,%s}},'%(q(b['sid']),array(b['sources'][0]),array(b['sources'][1])) for b in doc['biomes']]
    rows += ['};','static const Painted PaintedMaps[]={']
    rows += ['{%s,%s,%du,%du,%du,%du,%du},'%(q(p['path']),q(p['sha256']),p['bytes'],p['kind'],p['x'],p['z'],p['width']) for p in doc['painted']]
    rows += ['};','static const Region Regions[]={']
    for r in doc['regions']:
        rows += ['{%du,%du,%du,%du,%d,%d,%d,%d,%du,%du,%du,%s,%s,{'%(r['x'],r['z'],*r['primary'],*r['origin'],*r['paint_base'],len(r['biomes']),r['diffuse_count'],r['normal_count'],array(r['painted']),array(r['banks']))]
        rows += ['{%du,{%s,%s}},'%(b['source'],array(b['layers'][0]),array(b['layers'][1])) for b in r['biomes']]
        rows += ['{65535u,{{0,0,0,0,0,0},{0,0,0,0,0,0}}},']*(15-len(r['biomes']))
        rows += ['},{']
        rows += ['{%s,%s,%du,%du,%s},'%(array(c['slots']),array(c['selector']),c['root_mask'],c['tree_bytes'],q(c['tree_sha256'])) for c in r['cells']]
        rows += ['},'+array(r['selector'])+'},']
    rows += ['};','static const Catalog Data={Assets,219u,Biomes,57u,PaintedMaps,128u,Regions,64u};','}','#endif','']
    return '\n'.join(rows)

def write_outputs(doc,output):
    # Render both complete artifacts before replacing either. All fallible input
    # reads/verification happened before here. No temporary game/render state.
    rendered={'catalog.json':json.dumps(doc,indent=2)+'\n','RegionCatalogData.h':cpp_data(doc)}
    output.mkdir(parents=True,exist_ok=True)
    for name,text in rendered.items():(output/(name+'.tmp')).write_text(text,encoding='ascii')
    for name in rendered:(output/(name+'.tmp')).replace(output/name)

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--manifest',type=Path,default=HERE/'source-manifest.json')
    parser.add_argument('--game',type=Path,default=GAME)
    parser.add_argument('--snapshot-dir',type=Path,default=SNAPSHOT)
    parser.add_argument('--output',type=Path,default=HERE)
    args=parser.parse_args();started=time.perf_counter()
    try:doc=build_catalog(load_pinned_inputs(args.manifest,args.game,args.snapshot_dir));write_outputs(doc,args.output)
    except (ValueError,OSError,KeyError,TypeError) as exc:
        parser.exit(1,'REFUSED: '+str(exc)+'\n')
    print('PASS64 fixed data regions;57 global SIDs;219DDS/128PNG verified;15/24+22/46 unchanged; elapsed %.3fs'%(time.perf_counter()-started))

if __name__=='__main__':main()
