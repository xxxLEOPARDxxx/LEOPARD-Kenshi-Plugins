from pathlib import Path
import os, subprocess, sys, hashlib

HERE = Path(__file__).resolve().parent
OUT = HERE / '_generated'
OUT.mkdir(exist_ok=True)
VC = Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
SDK = Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env = os.environ.copy()
env['INCLUDE'] = f'{VC}/include;{SDK}/Include'
env['LIB'] = f'{VC}/lib/amd64;{SDK}/Lib/x64'

def run(name, command):
    p = subprocess.run(command, cwd=HERE, env=env, capture_output=True)
    (OUT / (name + '.log')).write_bytes(p.stdout + p.stderr)
    print((p.stdout + p.stderr).decode('utf-8', 'replace'), end='', flush=True)
    if p.returncode:
        raise SystemExit(p.returncode)

code = []
for name, entry, stage in [('VertexShader', 'vertex', 'vs_5_0'), ('PixelShader', 'pixel', 'ps_5_0')]:
    target = OUT / (name + '.cso')
    run(name, ['C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe', '/nologo', '/T', stage, '/E', entry, '/Fo', str(target), str(HERE / 'geometry.hlsl')])
    code.append('static const unsigned char ' + name + '[]={' + ','.join(map(str, target.read_bytes())) + '};\n')
(OUT / 'shaders.h').write_text(''.join(code), encoding='ascii')
run('build', [str(VC / 'bin/amd64/cl.exe'), '/nologo', '/EHsc', '/O2', '/MT', '/W3', str(HERE / 'control.cpp'), '/Fo' + str(OUT / 'control.obj'), '/Fe' + str(OUT / 'control.exe'), '/link', 'd3d11.lib', 'dxgi.lib'])
fixture = HERE.parents[2] / 'GlobalSatelliteMap/atlas-terrain/fixed-geometry/raster-window.u16'
assert hashlib.sha256(fixture.read_bytes()).hexdigest() == '5faef118b6476b6c87eb52425483872fded07d6481bfefb96cfab5cc76005ed3'
run('control', [str(OUT / 'control.exe'), str(fixture)])
old_edge = fixture.parent / 'height-window.u16'
assert hashlib.sha256(old_edge.read_bytes()).hexdigest() == '9dcdaf8f8d73105861fe88d38cd496ef365678b7b49539935a7d2ea547fad68f'
run('old-atlas-edge', [str(OUT / 'control.exe'), str(old_edge)])

# Genuine compiled/running failures: never count a compiler failure as a kill.
source = (HERE / 'control.cpp').read_text(encoding='utf-8')
source = source.replace('"../../../GlobalSatelliteMap/atlas-terrain/fixed-geometry/FixedTerrainGeometry.h"', '"' + (HERE.parents[2] / 'GlobalSatelliteMap/atlas-terrain/fixed-geometry/FixedTerrainGeometry.h').as_posix() + '"')
source = source.replace('"_generated/shaders.h"', '"' + (OUT / 'shaders.h').as_posix() + '"')
mutations = {
    'missing-patch': ('for(unsigned k=0;k<4;++k){c.p->IASetVertexBuffers', 'for(unsigned k=0;k<3;++k){c.p->IASetVertexBuffers'),
    'wrong-topology': ('D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP', 'D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST'),
    'wrong-stride': ('stride=sizeof(FixedTerrain::Vertex)', 'stride=36'),
    'wrong-index-origin': ('DrawIndexed(FixedTerrain::IndexCount,0,0)', 'DrawIndexed(FixedTerrain::IndexCount,1,0)'),
}
for name, (before, after) in mutations.items():
    assert source.count(before) == 1, name
    path, exe = OUT / (name + '.cpp'), OUT / (name + '.exe')
    path.write_text(source.replace(before, after), encoding='utf-8')
    run(name + '-build', [str(VC / 'bin/amd64/cl.exe'), '/nologo', '/EHsc', '/O2', '/MT', '/W3', str(path), '/Fo' + str(OUT / (name + '.obj')), '/Fe' + str(exe), '/link', 'd3d11.lib', 'dxgi.lib'])
    p = subprocess.run([str(exe), str(fixture)], cwd=HERE, capture_output=True)
    (OUT / (name + '-run.log')).write_bytes(p.stdout + p.stderr)
    assert p.returncode == 1 and b'FAIL native anti-diagonal interpolated height' in p.stderr, (name, p.returncode, p.stdout, p.stderr)
    print('Expected RED:', name, p.stderr.decode('ascii').strip(), flush=True)
for name, replacement in [('constant-normal', 'float3(0,1,0)'), ('wrong-normal-layout', 'normalize(v.normal.xyz)')]:
    shader = (HERE / 'geometry.hlsl').read_text(encoding='ascii')
    assert shader.count('normalize(v.normal.zxy)') == 1
    shader_path = OUT / (name + '.hlsl')
    shader_path.write_text(shader.replace('normalize(v.normal.zxy)', replacement), encoding='ascii')
    bytecode = OUT / (name + '.cso')
    run(name + '-shader', ['C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe', '/nologo', '/T', 'vs_5_0', '/E', 'vertex', '/Fo', str(bytecode), str(shader_path)])
    header = OUT / (name + '.h')
    header.write_text('static const unsigned char VertexShader[]={' + ','.join(map(str, bytecode.read_bytes())) + '};\n' + code[1], encoding='ascii')
    path, exe = OUT / (name + '.cpp'), OUT / (name + '.exe')
    path.write_text(source.replace((OUT / 'shaders.h').as_posix(), header.as_posix()), encoding='utf-8')
    run(name + '-build', [str(VC / 'bin/amd64/cl.exe'), '/nologo', '/EHsc', '/O2', '/MT', '/W3', str(path), '/Fo' + str(OUT / (name + '.obj')), '/Fe' + str(exe), '/link', 'd3d11.lib', 'dxgi.lib'])
    p = subprocess.run([str(exe), str(fixture)], cwd=HERE, capture_output=True)
    (OUT / (name + '-run.log')).write_bytes(p.stdout + p.stderr)
    assert p.returncode == 1 and any(reason in p.stderr for reason in [b'FAIL native morph0 interpolated normal', b'FAIL unit upward geometric normal']), (name, p.returncode, p.stderr)
    print('Expected RED:', name, p.stderr.decode('ascii').strip(), flush=True)
print('PASS six compiled diagnostic mutations rejected; no game/native calls')
