"""Freeze/recheck local terrain inputs; no game, UI or Steam mutations."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import time

GAME = Path('D:/SteamLibrary/steamapps/common/Kenshi')
WORKSHOP = Path('D:/SteamLibrary/steamapps/workshop/content/233860')
ROOT = Path(__file__).resolve().parent
MAX_BYTES = 8 * 1024**3

def digest(path):
    before = path.stat()
    if before.st_file_attributes & 0x400:
        raise ValueError('reparse input: '+str(path))
    value = hashlib.sha256()
    with path.open('rb') as f:
        while True:
            block = f.read(65536)
            if not block:
                break
            value.update(block)
    after = path.stat()
    if (before.st_size, before.st_mtime_ns) != (after.st_size, after.st_mtime_ns):
        raise ValueError('input changed during hashing: '+str(path))
    return value.hexdigest()

def sources():
    cfg = GAME/'data/mods.cfg'
    names = {s.strip().casefold() for s in cfg.read_text('utf-8-sig').splitlines()
             if s.strip() and not s.lstrip().startswith(('#', ';'))}
    if any('/' in s or '\\' in s or not s.endswith('.mod') for s in names):
        raise ValueError('unexpected enabled mod filename')
    files = {cfg, GAME/'data/gamedata.base', GAME/'data/Dialogue.mod',
             GAME/'data/newland/land/fullmap.tif', GAME/'RE_Kenshi/Kenshi_x64.exe'}
    found = set()
    # Retain every matching candidate; never silently guess local/Workshop priority.
    for folder in (GAME/'mods', WORKSHOP):
        for path in folder.rglob('*.mod'):
            key = path.name.casefold()
            if key in names:
                files.add(path)
                found.add(key)
    missing = sorted(names-found)
    if missing:
        raise ValueError('unresolved enabled mod sources: '+repr(missing))
    result = sorted({p.resolve(strict=True) for p in files}, key=lambda p: str(p).casefold())
    if len(result)>512 or sum(p.stat().st_size for p in result)>MAX_BYTES:
        raise ValueError('source count/byte budget exceeded')
    return result

def create(destination):
    if destination.exists():
        raise FileExistsError('identity must be new: '+str(destination))
    files = sources()
    rows = [dict(path=p.as_posix(), sha256=digest(p)) for p in files]
    lookup = {r['path']:r['sha256'] for r in rows}
    engine=(GAME/'RE_Kenshi/Kenshi_x64.exe').resolve().as_posix()
    cfg=(GAME/'data/mods.cfg').resolve().as_posix()
    height=(GAME/'data/newland/land/fullmap.tif').resolve().as_posix()
    data=(ROOT.parent/'tools/diagnostics/satellite-capture-probe/region-catalog/RegionCatalogData.h').read_text()
    manifest=re.search(r'ManifestSha256\[\]="([0-9a-f]{64})"',data).group(1)
    out=dict(format='GSCAP2', version=2, grid=4, baseSide=8192,
             origin=[-147456,-147456], span=294912,
             enginePath=engine,engineSha256=lookup[engine],
             modsCfgPath=cfg,modsCfgSha256=lookup[cfg],
             heightPath=height,heightSha256=lookup[height],
             sourceManifestSha256=manifest,sources=rows)
    encoded=(json.dumps(out,ensure_ascii=True,indent=2)+'\n').encode('utf-8')
    if len(encoded)>65536:
        raise ValueError('capture metadata byte limit exceeded')
    with destination.open('xb') as f:
        f.write(encoded)
    print(f'Frozen {len(rows)} local inputs; {len(encoded)} metadata bytes; SHA256 {hashlib.sha256(encoded).hexdigest()}')

def verify(source):
    data=json.loads(source.read_text('utf-8'))
    assert data['format']=='GSCAP2' and data['version']==2
    current={p.as_posix() for p in sources()}
    assert current=={r['path'] for r in data['sources']},'input source set changed'
    for row in data['sources']:
        if digest(Path(row['path']))!=row['sha256']:
            raise ValueError('captured input changed: '+row['path'])
    print(f'PASS {len(current)} frozen source identities unchanged')

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action',choices=['create','verify'])
    parser.add_argument('file',type=Path)
    args=parser.parse_args();start=time.monotonic()
    (create if args.action=='create' else verify)(args.file)
    print(f'{time.monotonic()-start:.2f}s')
