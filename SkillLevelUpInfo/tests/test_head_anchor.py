"""Compile the real notification path against narrow native-boundary fakes.

Uses the SDK Ogre vector and actual Ogre DLL; character bones/labels are fakes.
The native tracking composition was read from live Steam ScreenLabel::update.
"""
from pathlib import Path
import os
import re
import subprocess

here = Path(__file__).resolve().parent
mod = here.parent
game = Path(r'D:\SteamLibrary\steamapps\common\Kenshi')
vc = Path(r'C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC')
sdk = Path(r'C:\Program Files\Microsoft SDKs\Windows\v7.1')
kl = Path(r'D:\DEV\Kenshi\KenshiLib-src\Include')

common = (here / 'head_boundary_fakes.h').read_text(encoding='utf-8')
if (mod / 'HeadAnchor.h').exists():
    common += '\n#include "HeadAnchor.h"\n'

source = (mod / 'SkillLevelUpInfo.cpp').read_text(encoding='utf-8-sig')
source = source[source.index('namespace\n{'):source.index('extern "C" __declspec(dllexport) void MCM_Describe')]
a = source.index('    void LoadSettings()')
b = source.index('{', a) + 1
depth = 1
while depth:
    depth += (source[b] == '{') - (source[b] == '}')
    b += 1
source = source[:a] + source[b:]
stats = sorted(set(re.findall(r'\bSTAT_\w+', source)))
defines = '\n'.join('#define ' + s + ' ' + str(i) for i, s in enumerate(stats))
tests = (here / 'head_anchor_fixture.cpp').read_text(encoding='utf-8')
out = here / '_generated'
out.mkdir(exist_ok=True)
cpp, exe = out / 'head_anchor.cpp', out / 'head_anchor.exe'
cpp.write_text(common + '\n' + defines + '\n' + source + '\n' + tests, encoding='utf-8')
env = os.environ.copy()
env['PATH'] = str(vc / 'bin/amd64') + ';' + str(game) + ';' + env['PATH']
env['INCLUDE'] = ';'.join(map(str, [vc / 'include', sdk / 'Include', kl, kl / 'ogre', mod, Path(r'D:\DEV\Kenshi\deps\boost_1_60_0')]))
env['LIB'] = ';'.join(map(str, [vc / 'lib/amd64', sdk / 'Lib/x64', Path(r'D:\DEV\Kenshi\deps\KenshiLib\Libraries')]))
subprocess.run([str(vc / 'bin/amd64/cl.exe'), '/nologo', '/EHsc', '/MD', '/O2',
                str(cpp), '/Fo' + str(out) + '\\', '/Fe' + str(exe), 'OgreMain_x64.lib'],
               cwd=out, env=env, check=True)
subprocess.run([str(exe)], cwd=game, env=env, check=True)
