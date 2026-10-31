from pathlib import Path
import json

here = Path(__file__).resolve().parent
shaders = here.parents[2] / 'GlobalSatelliteMap/shaders'
helper = (shaders / 'OrthographicPosition.hlsl').read_text(encoding='utf-8')
source = (shaders / 'PositionProbe.hlsl').read_text(encoding='utf-8')
include = '#include "OrthographicPosition.hlsl"'
assert source.count(include) == 1
source = helper + '\n' + source.replace(include, '')
assert len(source) < 16384 and source.isascii()
output = here / 'build/PositionShader.h'
output.parent.mkdir(exist_ok=True)
output.write_text('static const char PositionShaderSource[] =\n' +
                  '\n'.join(json.dumps(line + '\n') for line in source.splitlines()) +
                  ';\n', encoding='ascii')
print('Embedded tested position helper and private pass source')
