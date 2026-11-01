"""Inspect real DXBC reflection: legacy sampler sN does not guarantee texture tN."""
from pathlib import Path
import re
import subprocess
import sys

fxc = Path('C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe')
shader = Path(sys.argv[1])
dump = subprocess.check_output([str(fxc), '/dumpbin', str(shader)], text=True, errors='replace')
assert 'Resource Bindings:' in dump, 'DXBC reflection table missing'
bindings = {}
for line in dump.splitlines():
    match = re.fullmatch(r'//\s+(\w+)\s+(texture|sampler)\s+\S+\s+(\S+)\s+([ts]\d+)\s+(\d+)\s*', line)
    if match:
        name, kind, dimension, slot, count = match.groups()
        key = (name, kind)
        assert key not in bindings, f'duplicate reflected resource {key}'
        bindings[key] = (dimension, slot, int(count))
expected = {}
for number, name in enumerate(('albedoBuffer', 'normalBuffer', 'depthBuffer', 'ambientMap',
                               'irradianceCube', 'specularityCube')):
    expected[name, 'texture'] = ('2d' if number < 4 else 'cube', f't{number}', 1)
errors = [f'{key}: got {bindings.get(key)}, expected {value}'
          for key, value in expected.items() if bindings.get(key) != value]
assert not errors, '\n'.join(errors)
samplers = {key: value for key, value in bindings.items() if key[1] == 'sampler'}
expected_samplers = {(name, 'sampler'): ('NA', f's{slot}', 1)
                     for name, slot in [('ambientSampler', 3), ('irradianceSampler', 4), ('specularitySampler', 5)]}
assert samplers == expected_samplers, f'unexpected sampler layout: {samplers}'
assert len(bindings) == 9, f'unexpected resource layout: {bindings}'
print('Actual DXBC: six explicit texture slots and three sampler slots verified')
