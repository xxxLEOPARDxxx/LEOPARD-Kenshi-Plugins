"""Read authenticated base-terrain triangles at saved platform actor positions.

This never queries navmesh, collision, clearance or walkability. Zone edits and
native landing validation remain the game's responsibility.
"""
from pathlib import Path
import hashlib
import json
import math
import struct

SOURCE = Path('D:/SteamLibrary/steamapps/common/Kenshi/data/newland/land/fullmap.tif')
SHA256 = '6f7d4bc7baaf80dbf8db91e6f367db71f87fe38f053a34108f192584c9637b18'
HERE = Path(__file__).resolve().parent


def ground_at(stream, x, z):
    # Independently audited native U16 source and anti-diagonal triangles;
    # profile: WorldHeightSource.h and check_native_world_frames.py.
    sx, sz = (x + 147456) / 18, (z + 147456) / 18
    ix, iz = math.floor(sx), math.floor(sz)
    assert 0 <= ix < 16384 and 0 <= iz < 16384
    u, v = sx - ix, sz - iz
    scale = struct.unpack('<f', struct.pack('<f', 9800 / 65535))[0]

    def sample(dx, dz):
        stream.seek(43686 + 2 * ((iz + dz) * 16385 + ix + dx))
        return struct.unpack('<H', stream.read(2))[0] * scale

    y00, y10, y01, y11 = sample(0, 0), sample(1, 0), sample(0, 1), sample(1, 1)
    if u + v <= 1:
        return y00 + u * (y10 - y00) + v * (y01 - y00)
    return y11 + (1 - u) * (y01 - y11) + (1 - v) * (y10 - y11)


if __name__ == '__main__':
    assert SOURCE.stat().st_size == 536980180
    digest = hashlib.sha256()
    with SOURCE.open('rb') as stream:
        for block in iter(lambda: stream.read(65536), b''):
            digest.update(block)
    assert digest.hexdigest() == SHA256
    saved = json.loads((HERE / 'fixtures/platform_saved_20261113.json').read_text('utf-8'))
    actors = []
    seen = set()
    with SOURCE.open('rb') as stream:
        for actor in saved['observations']:
            identity = tuple(actor['identity'])
            if identity in seen:
                continue
            seen.add(identity)
            x, y, z = actor['position']
            ground = ground_at(stream, x, z)
            actors.append(dict(name=actor['name'], identity=identity,
                position=actor['position'], base_ground=ground, drop_metres=(y - ground) / 10))
    result = dict(evidence_note='Authenticated base terrain only; not live navmesh, '
                  'zone terrain edits, physical support, clearance or walkability proof.',
                  source=str(SOURCE), sha256=SHA256, actors=actors)
    out = HERE / '_generated/platform_terrain_20261113.json'
    out.parent.mkdir(exist_ok=True)
    out.write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    for actor in actors:
        print(actor['identity'], actor['base_ground'], actor['drop_metres'])
