"""Bounded read-only evidence for the confirmed Lagoon Drone identity only.

No native calls, UI, process writes, or settings changes. Uses the existing
verified read-only process helper and its 4MiB total / 256KiB read bounds.
"""
from pathlib import Path
import argparse, collections, datetime, hashlib, importlib.util, json, struct, time
ap=argparse.ArgumentParser();ap.add_argument('--pid',type=int,required=True);args=ap.parse_args()
helper=Path('D:/DEV/Kenshi/ours/tools/diagnostics/df_readonly_shop_probe.py')
spec=importlib.util.spec_from_file_location('probe',helper);p=importlib.util.module_from_spec(spec);spec.loader.exec_module(p)
out=Path(__file__).resolve().parent/'_generated';out.mkdir(exist_ok=True)
log=Path('D:/SteamLibrary/steamapps/common/Kenshi/RE_Kenshi_log.txt').read_bytes()
lines=log.decode('utf-8',errors='replace').splitlines()
drone_lines=[s for s in lines if 'platform NPC 1/61/2046386688:3/2518066432 ' in s]
report=dict(pid=args.pid,captured_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
    access='verified0x410 read-only',native_calls=False,writes=False,
    log=dict(bytes=len(log),sha256=hashlib.sha256(log).hexdigest(),all_drone_lines=drone_lines,
        all_freed=[s for s in lines if 'AIFixed: freed NPC' in s]),profile=p.verify_profile())
reader=None
try:
    reader,mods,metadata=p.NativeAccess().open_verified(args.pid);p._reader=reader
    report['process']=metadata
    exports=p.exps(mods['kenshilib.dll']['path'])
    world=p.q(mods['kenshilib.dll']['base']+next(r for n,r in exports.items() if n.startswith('?ou@@')))
    nav=p.q(world+0x4b0);report['world']=hex(world);report['navmesh']=hex(nav)
    wanted=(1,61,2046386688,3,2518066432);found=[];seen=set();platoons=0
    for f in p.lektor(p.q(world+0x4a8),2048):
        for pl in p.lektor(f+0x208,2048):
            platoons+=1
            if platoons>4096:raise ValueError('platoon bound')
            active=p.q(pl+0x1d8)
            if not active:continue
            for c in p.lektor(active+0x50,256):
                if c in seen:continue
                seen.add(c)
                if len(seen)>16384:raise ValueError('actor bound')
                if p.hand(c+0x58)==wanted:found.append(c)
        if found:break
    if len(found)!=1:raise ValueError('unique full Drone identity not found')
    c=found[0];movement=p.q(c+0x640);body=p.q(c+0x648)
    if p.q(movement+0x3a8)!=c:raise ValueError('movement owner mismatch')
    hk=p.q(movement+0x320)
    report['actor']=dict(pointer=hex(c),identity=wanted,name=p.s(c+0x18),position=p.pos(c),
        movement=hex(movement),navigation=hex(hk),standing_support=p.hand(movement+0x338))
    report['scan']=dict(platoons=platoons,actors=len(seen))
    report['samples']=[]
    for n in range(16):
        if p.hand(c+0x58)!=wanted or p.q(movement+0x320)!=hk:raise ValueError('actor changed')
        action=p.q(body+0x68);td=p.q(action+0x70) if action else 0
        report['samples'].append(dict(position=p.pos(c),native_position=struct.unpack('<4f',p.read(hk+0x20,16)),
            goal=struct.unpack('<4f',p.read(hk+0x40,16)),radius=struct.unpack('<f',p.read(hk+0x70,4))[0],
            state=p.i(hk+0x88),path=p.i(hk+0x90),face=p.i(hk+0x14),action=p.i(td+0x44) if td else None))
        if n!=15:time.sleep(.05)
    # Retain only bounded current tile/instance evidence; do not follow a world graph.
    hk_world=p.q(nav+0x88);collection=p.q(hk_world+0x20);instances=p.q(collection+0x20)
    tile=report['samples'][0]['face']>>22;record=instances+tile*48
    report['nav_geometry']=dict(havok_world=hex(hk_world),collection=hex(collection),tile=tile,
        nav_dump=p.read(nav,0x300).hex(),hk_world_dump=p.read(hk_world,0x100).hex(),
        collection_dump=p.read(collection,0x80).hex(),tile_record=p.read(record,48).hex())
    instance=p.q(record)
    report['nav_geometry'].update(instance=hex(instance),instance_dump=p.read(instance,0x200).hex())
    report['world_stable']=p.q(mods['kenshilib.dll']['base']+next(r for n,r in exports.items() if n.startswith('?ou@@')))==world
except Exception as error:report['error']=repr(error)
finally:
    if reader:report['read_bytes']=reader.total;reader.close()
    target=out/f'drone-live-{args.pid}.json';target.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    print(str(target));print(json.dumps({k:report.get(k) for k in ('error','read_bytes','world_stable','scan')},ensure_ascii=False))
