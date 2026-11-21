"""Exercise the shared startup batch and both production call sites offline."""
from pathlib import Path
import os
import subprocess
import sys

tests = Path(__file__).resolve().parent
root = tests.parents[1]
for plugin, before, after, members in (
    ('UITweaks', 'NewTask_hook', 'EditUpdate_hook', (
        'MoveWaypoint_hook', 'PlayerMove_hook', 'CharMoveOrder_hook',
        'SetPositionPoint_hook', 'SetPositionXY_hook', 'SetSizePoint_hook',
        'SetRealSize_hook', 'SetCraftingBench_hook',
    )),
    ('AIFixed', 'InstallEngineers();', 'SetBedMode_hook', (
        'FindOwnedFreeBed_hook', 'GetOutHealed_hook', 'GuiUpdate_hook', 'Drop_hook',
    )),
):
    text = (root / plugin / (plugin + '.cpp')).read_text(encoding='utf-8-sig')
    startup = text.split('__declspec(dllexport) void startPlugin()', 1)[1]
    assert startup.count('StartupHooks::Install(') == 1, plugin + ': expected one startup batch'
    begin = startup.index('StartupHooks::Hook hooks[]')
    commit = startup.index('StartupHooks::Install(')
    assert startup.index(before) < begin < commit < startup.index(after), plugin
    prepared = startup[begin:commit]
    positions = [prepared.index(member + ',') for member in members]
    assert positions == sorted(positions), plugin + ': hooks reordered'
    assert prepared.count('StartupHooks::Hook(') + prepared.count('HookByExport(') + prepared.count('HookMyGui(') == len(members), plugin
    assert 'KenshiLib::AddHook(' not in prepared, plugin
assert 'g_debug||(g_lifecycleReady&&g_newHooksReady)' in startup[startup.index(before):begin], 'AI Fixed drop admission changed'

vc = Path(r'C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC')
sdk = Path(r'C:\Program Files\Microsoft SDKs\Windows\v7.1')
out = root.parent / 'build' / 'startup-hook-batch-20261121'
out.mkdir(parents=True, exist_ok=True)
env = os.environ.copy()
env['INCLUDE'] = ';'.join(map(str, [vc/'include', sdk/'Include']))
env['LIB'] = ';'.join(map(str, [vc/'lib/amd64', sdk/'Lib/x64']))
env['PATH'] = str(vc/'bin/amd64') + ';' + env['PATH']
for opt in ('Od', 'O2'):
    exe = out / ('hook-batch-' + opt + '.exe')
    command = [str(vc/'bin/amd64/cl.exe'), '/nologo', '/EHsc', '/MD', '/W4', '/'+opt,
               str(tests/'startup_hook_batch_test.cpp'), '/Fo'+str(exe.with_suffix('.obj')),
               '/Fe'+str(exe)]
    subprocess.run(command, cwd=out, env=env, check=True)
    subprocess.run([str(exe)], cwd=out, env=env, check=True)


game = Path(os.environ.get('KENSHI_ROOT', r'D:\SteamLibrary\steamapps\common\Kenshi'))
native = out/'hook-batch-native.exe'
subprocess.run([str(vc/'bin/amd64/cl.exe'), '/nologo', '/EHsc', '/MD', '/W4', '/O2',
                str(tests/'startup_hook_batch_native.cpp'), '/Fo'+str(native.with_suffix('.obj')),
                '/Fe'+str(native)], cwd=out, env=env, check=True)
for _ in range(5):
    for mode in ('immediate', 'batch'):
        subprocess.run([str(native), str(game/'KenshiLib.dll'), mode], cwd=out, env=env, check=True)


unsupported = root.parent/'deps/KenshiLib_0.5.1/KenshiLib.dll'
if unsupported.is_file():
    subprocess.run([str(native), str(unsupported), 'fallback'], cwd=out, env=env, check=True)
