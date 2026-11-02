"""Rebuild private shader snapshots locally; do not redistribute game shaders.

The original game includes are read by the existing authenticated generators.
Only a private mirror below this component's ignored output tree is written.
No game/probe build output or installed file is touched.
"""
from pathlib import Path
import hashlib
import importlib.util
import json
import shutil
import subprocess
import sys

PINS = {
    'GeographyTerrain.hlsl': 'f9326a91f29e1f1644fc0acbb33bc126bce04590faef2cc7b89dabd1292fd9a1',
    'GeographyVertex.hlsl': '6ef9efa2c8027da354f812b3f59bfa055c48edcfa4a2057782bd7416f58d54e9',
}

def prepare_sources(component=None, force=False):
    component = Path(component or Path(__file__).resolve().parent).resolve()
    repo = component.parents[2]
    probe = repo / 'tools/diagnostics/satellite-capture-probe'
    destination = component / '_source'
    sources = [probe / name for name in [
        'embed_painted_shader.py', 'embed_geography_shader.py',
        'geography_vertex_source.py', 'GeographyDiffuse.hlsl',
        'GeographySampling.hlsl', 'GeographyCliff.hlsl',
    ]] + [repo / 'GlobalSatelliteMap/shaders/PaintedMap.hlsl']
    recipe = {p.relative_to(repo).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}
    mirror = component / '_generated/source-build'
    ledger = mirror / 'recipe.json'
    valid = all((destination / n).is_file() and hashlib.sha256((destination / n).read_bytes()).hexdigest() == h for n, h in PINS.items())
    try:
        cached_recipe = json.loads(ledger.read_text())
    except (OSError, ValueError):
        cached_recipe = None
    if not force and valid and cached_recipe == recipe:
        return destination
    mirror.mkdir(parents=True, exist_ok=True)
    for p in sources[:6]:
        shutil.copyfile(p, mirror / p.name)
    spec = importlib.util.spec_from_file_location('fixed_material_painted_generator', mirror / 'embed_painted_shader.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.HERE = mirror
    module.SHADERS = repo / 'GlobalSatelliteMap/shaders'
    module.main()
    subprocess.run([sys.executable, str(mirror / 'embed_geography_shader.py')], check=True, cwd=mirror)
    for name, pin in PINS.items():
        data = (mirror / 'build' / name).read_bytes()
        if len(data) > 32768 or hashlib.sha256(data).hexdigest() != pin:
            raise RuntimeError('Private shader recipe differs: ' + name)
    destination.mkdir(exist_ok=True)
    for name in PINS:
        shutil.copyfile(mirror / 'build' / name, destination / name)
    ledger.write_text(json.dumps(recipe, indent=2) + '\n', encoding='utf-8')
    return destination

if __name__ == '__main__':
    print('PASS reconstructed private shader snapshots:', prepare_sources(force='--force' in sys.argv))
