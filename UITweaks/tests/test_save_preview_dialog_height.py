"""Verify installed Save/Load alignment and native MyGUI row viewport growth."""
from pathlib import Path
import os
import subprocess
import xml.etree.ElementTree as ET

here = Path(__file__).resolve().parent
game = Path('D:/SteamLibrary/steamapps/common/Kenshi')
fixtures = []
for skin, folder in [('vanilla', game/'data/gui/layout'), ('Dark UI Advanced', game/'mods/Dark UI Advanced/gui/layout')]:
    for menu in ('Save', 'Load'):
        root = ET.parse(folder/f'Kenshi_{menu}GamePanel.layout').getroot().find('Widget')
        listed = root.find("Widget[@name='GamesList']")
        assert listed is not None and listed.attrib['align'] == 'Stretch', (skin, menu)
        parts = [float(v) for v in listed.attrib['position_real'].split()]
        fixtures.append((skin + ' ' + menu, int(parts[1]*325), int(parts[3]*325)))

base = (here/'native_size_test.cpp').read_text(encoding='utf-8').split('int main(', 1)[0]
code = r'''
#include "../../SavePreviewPolicy.h"
int main(){
 HMODULE lib=LoadLibraryA("D:\\SteamLibrary\\steamapps\\common\\Kenshi\\MyGUIEngine_x64.dll");assert(lib);
 nativeSize=(SetSize)GetProcAddress(lib,"?setSize@Widget@MyGUI@@UEAAXAEBU?$TSize@H@types@2@@Z");assert(nativeSize);
 cropVtable[2]=(void*)ObservedSize;delegateVtable[2]=(void*)Changed;
'''
for name, top, height in fixtures:
    code += f'''
 {{WidgetFixture root,list;Init(root,680,325,false);Init(list,438,{height},true);
 Field<void*>(list.bytes,0x40)=root.bytes+8;Field<int>(list.bytes,0x24)={top};
 Field<int>(list.bytes,0x468)=0x1E; // actual Stretch from this installed layout
 void* children[1]={{list.bytes}};Field<void*>(root.bytes,0x3d8)=children;Field<void*>(root.bytes,0x3e0)=children+1;
 SavePreviewPolicy::DialogHeightState state;int taller=state.Next(325,false,120,680,1920,1080,true);assert(taller==390);
 Size grown={{680,taller}};nativeSize(root.bytes+8,grown);
 assert(Field<int>(list.bytes,0x2c)=={height}+65&&Field<int>(list.bytes,0x24)=={top});
 assert(state.Next(390,true,120,680,1920,1080,true)==390);
 int restored=state.Next(390,true,100,680,1920,1080,true);Size normal={{680,restored}};nativeSize(root.bytes+8,normal);
 assert(Field<int>(list.bytes,0x2c)=={height});
 puts("PASS {name}: actual native list viewport +65px, stable reopen, setting100 restores native size");}}
'''
code += 'return 0;}\n'
out = here/'_generated'
out.mkdir(exist_ok=True)
cpp = out/'save_preview_dialog_height.cpp'
cpp.write_text(base+code, encoding='utf-8')
vc = Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
sdk = Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env = os.environ.copy()
env['INCLUDE'] = f'{vc}/include;{sdk}/Include'
env['LIB'] = f'{vc}/lib/amd64;{sdk}/Lib/x64'
env['PATH'] = f'{vc}/bin/amd64;' + env['PATH']
exe = out/'save_preview_dialog_height.exe'
subprocess.run([str(vc/'bin/amd64/cl.exe'), '/nologo', '/EHsc', '/MD', '/O2', str(cpp), '/Fe'+str(exe), '/Fo'+str(out/'save_preview_dialog_height.obj')], env=env, check=True)
subprocess.run([str(exe)], env=env, cwd=game, check=True)
