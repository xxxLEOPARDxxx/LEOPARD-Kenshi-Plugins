"""Inspect completed private game captures, without touching the game or atlas.

Reference comparisons describe GPU variation; they do not claim active source
winner, final lighting, whole-world seam freedom or gameplay performance.
"""
from pathlib import Path
import argparse
import hashlib
import json
import struct

import numpy as np

HEADER = struct.Struct('<8s6I5f4I32s32s3I')
PLANE = 256 * 256 * 4
HEIGHT = '4c2943629387364ee0bfd9f630656affd94fdc7671ab0754b5199e8f3109d3f2'
MATERIAL = 'afe79b4152ba35cc364e4f7c92a59771a428d4dec2d63217d442bfba2993f0b9'
HERE = Path(__file__).resolve().parent
REFERENCE = HERE.parents[2] / 'GlobalSatelliteMap/atlas-terrain/compact-material/_generated'


def require(condition, message):
    if not condition:
        raise ValueError(message)


def inspect(folder, pid, token):
    require((folder / 'complete.txt').is_file(), 'Incomplete capture set')
    require(HEADER.size == 144, 'Header layout changed')
    results = []
    epoch = None
    names = ['control1', 'control3', 'control9', 'shift3']
    for index, name in enumerate(names):
        path = folder / f'view-{index}.bin'
        require(path.stat().st_size == HEADER.size + 3 * PLANE, 'Unexpected extent')
        data = path.read_bytes()
        h = HEADER.unpack_from(data)
        spacing, center = [1, 3, 9, 3][index], [-13824, -13824, -13824, -13632][index]
        if epoch is None:
            epoch = h[6]
        require(h[:6] == (b'GFTC0001', 1, 256, index, token, pid), 'Request identity mismatch')
        require(h[6] == epoch and epoch > 0, 'Request epoch mismatch')
        require(h[7:12] == (center, center, spacing, 10000, 20000), 'View mismatch')
        require(h[12:16] == (28, 28, 41, PLANE), 'Format mismatch')
        require(h[16].hex() == HEIGHT and h[17].hex() == MATERIAL, 'Source pins mismatch')
        require(h[18:] == (0, 0, 0), 'Reserved header fields changed')
        planes = [data[144 + i * PLANE:144 + (i + 1) * PLANE] for i in range(3)]
        comparisons = []
        for plane, actual in enumerate(planes):
            reference = (REFERENCE / f'{name}-gbuffer{plane}.bin').read_bytes()
            require(len(reference) == PLANE, 'Reference extent changed')
            dtype = '<f4' if plane == 2 else np.uint8
            a = np.frombuffer(actual, dtype).astype(np.float64)
            b = np.frombuffer(reference, dtype).astype(np.float64)
            require(np.isfinite(a).all() and np.isfinite(b).all(), 'Nonfinite output')
            if plane == 2:
                require(((a > 0) & (a < 1)).all(), 'Uncovered or invalid radial depth')
            if plane == 1:
                require((np.frombuffer(actual, np.uint8).reshape(-1, 4)[:, 3] == 0).all(),
                        'Native normal alpha contract changed')
            difference = np.abs(a - b)
            comparisons.append({'plane': plane, 'maxAbsDifference': float(difference.max()),
                                'meanAbsDifference': float(difference.mean()),
                                'differentComponents': int(np.count_nonzero(difference)),
                                'sha256': hashlib.sha256(actual).hexdigest()})
        results.append({'view': index, 'spacingMeters': spacing, 'center': center,
                        'sha256': hashlib.sha256(data).hexdigest(), 'planes': comparisons})
    return {'scope': 'Completed fixed base-fixture game control; GPU variation versus WARP',
            'directory': str(folder.resolve()), 'pid': pid, 'token': token, 'epoch': epoch,
            'views': results, 'limits': 'Not active material winner, full atlas, lighting or FPS proof'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    parser.add_argument('--pid', required=True, type=int)
    parser.add_argument('--token', required=True, type=int)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    report = inspect(args.directory, args.pid, args.token)
    text = json.dumps(report, ensure_ascii=False, indent=2) + '\n'
    if args.report:
        # Refuse replacing an earlier experiment's evidence.
        with args.report.open('x', encoding='utf-8') as output:
            output.write(text)
    print(text)
