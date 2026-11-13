"""Compile actual LoadSettings against a temporary INI; no installed writes."""
from pathlib import Path
import os
import subprocess

here = Path(__file__).resolve().parent
root = here.parent
source = (root / 'AIFixed.cpp').read_text(encoding='utf-8-sig')
start = source.index('    bool g_enabled =')
globals_code = source[start:source.index('    std::string IniFileName()', start)]
start = source.index('    void LoadSettings()')
opening = source.index('{', start)
end, depth = opening + 1, 1
while depth:
    depth += (source[end] == '{') - (source[end] == '}')
    end += 1
fixture = (here / 'delay_settings_fixture.cpp').read_text(encoding='utf-8')
fixture = fixture.replace('// @GLOBALS@', globals_code).replace('// @LOAD@', source[start:end])
out = here / '_generated'
out.mkdir(exist_ok=True)
cpp = out / 'delay_settings.cpp'
cpp.write_text(fixture, encoding='utf-8')
vc = Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
sdk = Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env = os.environ.copy()
env['PATH'] = str(vc / 'bin/amd64') + ';' + env['PATH']
env['INCLUDE'] = str(vc / 'include') + ';' + str(sdk / 'Include')
env['LIB'] = str(vc / 'lib/amd64') + ';' + str(sdk / 'Lib/x64')
exe = out / 'delay_settings.exe'
subprocess.run([str(vc / 'bin/amd64/cl.exe'), '/nologo', '/EHsc', '/MD', '/Od',
                '/I' + str(root), str(cpp), '/Fe' + str(exe),
                '/Fo' + str(out / 'delay_settings.obj')], cwd=out, env=env, check=True)
subprocess.run([str(exe)], cwd=out, check=True)
