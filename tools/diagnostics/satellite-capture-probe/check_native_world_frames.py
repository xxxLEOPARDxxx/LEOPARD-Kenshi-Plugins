"""Read-only checks of the private GWF exports; never advertises an atlas."""
from pathlib import Path
import argparse
import hashlib
import json
import struct

import numpy as np

HEADER = struct.Struct('<8sII4Q7f3I3I32s32s3I')


def read_frame(path):
    data = Path(path).read_bytes()
    h = HEADER.unpack_from(data)
    assert HEADER.size == 176 and h[:3] == (b'GWSF0001', 1, 512)
    assert h[14:17] == (10, 28, 41), h[14:17]
    assert h[17:20] == (2097152, 1048576, 1048576), h[17:20]
    assert len(data) == HEADER.size + sum(h[17:20])
    material = np.frombuffer(data, '<f2', 512 * 512 * 4, 176).reshape(512, 512, 4)
    normal = np.frombuffer(data, 'u1', 512 * 512 * 4, 176 + h[17]).reshape(512, 512, 4)
    height = np.frombuffer(data, '<f4', 512 * 512, 176 + h[17] + h[18]).reshape(512, 512)
    assert np.isfinite(material).all() and np.isfinite(height).all()
    assert not normal[:, :, 3].any(), 'GWF reserved alpha is not GST coverage'
    return h, data, material.astype(np.float64), normal, height


def main():
    p = argparse.ArgumentParser(__doc__)
    p.add_argument('first', type=Path)
    p.add_argument('shifted', type=Path)
    p.add_argument('--repeat', action='append', type=Path, default=[])
    p.add_argument('--height-source', type=Path, required=True)
    args = p.parse_args()
    a, raw_a, rgb_a, n_a, y_a = read_frame(args.first)
    b, raw_b, rgb_b, n_b, y_b = read_frame(args.shifted)
    assert a[9] == b[9] == 1.0, 'Only this 1m native proof is covered'
    assert b[7] - a[7] == b[8] - a[8] == 256.0
    assert a[20:22] == b[20:22], 'Different source/material parameter hash'
    for path in args.repeat:
        _, data, *_ = read_frame(path)
        assert data[176:] == raw_a[176:], str(path)

    # Independently read the authenticated native U16 height source and its
    # anti-diagonal triangles; do not use the renderer's radial-depth output.
    source = args.height_source
    digest = hashlib.sha256()
    with source.open('rb') as f:
        for chunk in iter(lambda: f.read(65536), b''):
            digest.update(chunk)
    assert digest.digest() == a[20] and digest.hexdigest() == (
        '6f7d4bc7baaf80dbf8db91e6f367db71f87fe38f053a34108f192584c9637b18')
    assert source.stat().st_size == 536980180
    grid = np.memmap(source, mode='r', dtype='<u2', offset=43686, shape=(16385, 16385))
    scale = float(np.float32(9800) / np.float32(65535))
    xx = (a[7] - 256 + np.arange(512) + .5 + 147456) / 18
    zz = (a[8] - 256 + np.arange(512) + .5 + 147456) / 18
    x, z = np.meshgrid(xx, zz)
    ix, iz = np.floor(x).astype(int), np.floor(z).astype(int)
    u, v = x - ix, z - iz
    y00 = grid[iz, ix].astype(float) * scale
    y10 = grid[iz, ix + 1].astype(float) * scale
    y01 = grid[iz + 1, ix].astype(float) * scale
    y11 = grid[iz + 1, ix + 1].astype(float) * scale
    expected = np.where(u + v <= 1, y00 + u * (y10 - y00) + v * (y01 - y00),
                        y11 + (1 - u) * (y01 - y11) + (1 - v) * (y10 - y11))
    error = float(np.max(np.abs(expected - y_a)))
    assert error < .001, error
    assert np.array_equal(y_a[256:, 256:], y_b[:256, :256])
    ra, rb = rgb_a[256:, 256:], rgb_b[:256, :256]
    gamma_a = np.rint(np.clip(ra[:, :, :3], 0, 1) ** (1 / 2.2) * 255).astype(int)
    gamma_b = np.rint(np.clip(rb[:, :, :3], 0, 1) ** (1 / 2.2) * 255).astype(int)
    normal_delta = np.abs(n_a[256:, 256:].astype(int) - n_b[:256, :256].astype(int))
    print(json.dumps(dict(
        scope='Actual native private 512x512@1m frames; not whole-atlas acceptance',
        firstSha256=hashlib.sha256(raw_a).hexdigest(),
        shiftedSha256=hashlib.sha256(raw_b).hexdigest(),
        repeatPayloadsBitExact=len(args.repeat),
        independentHeightMaxErrorMetres=error,
        overlapHeightPixelsBitExact=65536,
        overlapRgbLinearMax=float(np.max(np.abs(ra[:, :, :3] - rb[:, :, :3]))),
        overlapGlossMax=float(np.max(np.abs(ra[:, :, 3] - rb[:, :, 3]))),
        overlapGamma8Max=int(np.max(np.abs(gamma_a - gamma_b))),
        overlapGamma8ChangedPixels=int(np.any(gamma_a != gamma_b, axis=2).sum()),
        overlapNormalMaxLsb=int(normal_delta.max()),
        overlapNormalChangedComponents=int(np.count_nonzero(normal_delta))), indent=2))


if __name__ == '__main__':
    main()
