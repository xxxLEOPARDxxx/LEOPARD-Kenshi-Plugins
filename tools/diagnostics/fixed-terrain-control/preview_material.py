"""Visualize real control buffers, not a simulated final game screenshot.

Uses the pinned SatelliteDecodeAlbedo formula, then the standard sRGB transfer
for display. No lighting, exposure, sharpening or invented material detail.
Both panels cover precisely the same768m square. Left keeps original9m samples.
"""
from pathlib import Path
import hashlib
import json
import argparse
import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / 'GlobalSatelliteMap/atlas-terrain/compact-material/_generated'
OUT = Path('D:/DEV/Kenshi/research/gsm-highzoom-budget-20261103')

def display(linear):
    assert np.isfinite(linear).all()
    linear = np.clip(linear, 0, 1)
    srgb = np.where(linear <= .0031308, linear * 12.92, 1.055 * linear ** (1 / 2.4) - .055)
    return Image.fromarray(np.rint(srgb * 255).astype(np.uint8))

def decode(raw):
    v = np.frombuffer(raw, np.uint8).reshape(256, 256, 4).astype(np.float32) / np.float32(255)
    a = np.pad(v, ((1, 1), (1, 1), (0, 0)), mode='edge')
    neighbors = np.stack([a[1:-1, 2:], a[1:-1, :-2], a[2:, 1:-1], a[:-2, 1:-1]])
    weights = (np.abs(neighbors[..., 0] - v[..., 0]) < np.float32(30 / 255)).astype(np.float32)
    no_match = weights.sum(axis=0) == 0
    weights[0, no_match] = 1
    other = (weights * neighbors[..., 1]).sum(axis=0) / weights.sum(axis=0)
    y, x = np.indices((256, 256))
    parity = (x & 1) == (y & 1)
    co = np.where(parity, other, v[..., 1]) - np.float32(.5)
    cg = np.where(parity, v[..., 1], other) - np.float32(.5)
    rgb = np.stack([v[..., 0] + co - cg, v[..., 0] + cg, v[..., 0] - co - cg], axis=-1)
    return display(rgb)

parser = argparse.ArgumentParser()
parser.add_argument('--spacing', type=int, choices=[1, 3], default=3)
parser.add_argument('--raw', action='store_true', help='Use new full RGB terrain output, not packed decoding.')
options = parser.parse_args()
spacing = options.spacing
fine_name = 'control' + str(spacing)
if options.raw:
    SOURCE = ROOT / 'GlobalSatelliteMap/atlas-terrain/raw-rgb/_generated'
    buffers = {name: (SOURCE / (name + '-material.rgba16f')).read_bytes() for name in [fine_name, 'control9']}
    assert all(len(b) == 256 * 256 * 8 for b in buffers.values())
    image = lambda data: display(np.frombuffer(data, '<f2').reshape(256, 256, 4)[..., :3].astype(np.float32))
else:
    buffers = {name: (SOURCE / (name + '-gbuffer0.bin')).read_bytes() for name in [fine_name, 'control9']}
    assert all(len(b) == 256 * 256 * 4 for b in buffers.values())
    image = decode
fine = image(buffers[fine_name])
base = image(buffers['control9'])
# Pillow extent uses image-edge coordinates: exact same ground footprint.
edge = 128 - 128 * spacing / 9
base = base.transform((256, 256), Image.Transform.EXTENT, (edge, edge, 256 - edge, 256 - edge), Image.Resampling.NEAREST)
scale = 3
canvas = Image.new('RGB', (1608, 900), '#202225')
draw = ImageDraw.Draw(canvas)
font = ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf', 29)
small = ImageFont.truetype('C:/Windows/Fonts/segoeui.ttf', 23)
for x, image, title in [(24, base, 'База: 9 м на пиксель'), (816, fine, f'Новая отрисовка: {spacing} м на пиксель')]:
    draw.text((x, 12), title, font=font, fill='#eeeeee')
    canvas.paste(image.resize((768, 768), Image.Resampling.NEAREST), (x, 58))
span = spacing * 256
draw.text((24, 840), f'Один участок {span} × {span} м. Реальные текстуры игры; без света, зданий и повышения резкости.', font=small, fill='#dddddd')
OUT.mkdir(exist_ok=True, parents=True)
suffix = '-raw-rgb' if options.raw else ''
path = OUT / f'material-detail-{spacing}m-vs-9m{suffix}.png'
canvas.save(path)
(OUT / f'material-preview-{spacing}m{suffix}.json').write_text(json.dumps({
    'kind': 'native material diagnostic; not final satellite lighting/viewer',
    'source': str(SOURCE), 'bufferSha256': {n: hashlib.sha256(b).hexdigest() for n, b in buffers.items()},
    'center': [-13824, -13824], 'comparedSpanMeters': span,
    'decode': ('Direct native linear RGB16F' if options.raw else 'SatelliteDecodeAlbedo') + '; standard sRGB display; nearest enlargement; no lighting/sharpening',
    'pngSha256': hashlib.sha256(path.read_bytes()).hexdigest(),
}, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
print(path)
