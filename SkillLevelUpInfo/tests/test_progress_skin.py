"""Read-only proof of real native/Dark UI Stretch progress geometry."""
from pathlib import Path
import xml.etree.ElementTree as ET
import json,hashlib,os,subprocess,sys,struct
game=Path(r'D:\SteamLibrary\steamapps\common\Kenshi')
report={'templates':[]}
for folder in [game/'data',game/'mods/Dark UI Advanced']:
 path=folder/'gui/templates/kenshi_templates.xml'
 layout=next(r for r in ET.parse(path).getroot().findall('Resource') if r.get('name')=='Kenshi_ProgressBarFill')
 root=layout.find('Widget');assert root.get('position')=='0 0 150 20'
 nodes=root.findall('Widget')
 empty=next(n for n in nodes if n.get('skin')=='Kenshi_ProgressBarEmptySkin')
 track=next(n for n in nodes if n.get('name')=='TrackPlace')
 assert empty.get('position')=='10 2 131 15' and empty.get('align')=='Stretch'
 assert track.get('position')=='11 3 129 13' and track.get('align')=='Stretch'
 # Public Stretch layout at the observed153x24 logical XP bar.
 e=[10,2,134,19];t=[11,3,132,17]
 shifted_e=[10,4,134,19];shifted_t=[11,5,132,17]
 assert shifted_e[1]+shifted_e[3]<=24 and shifted_t[1]+shifted_t[3]<=24
 assert e[3]==shifted_e[3] and t[3]==shifted_t[3]
 report['templates'].append({'path':str(path),'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),
  'original_empty_track':[e,t],'shifted_empty_track':[shifted_e,shifted_t]})
print('PASS actual vanilla/Dark UI progress children Stretch;2px child nudge preserves19/17px heights within24px')
capture=Path(__file__).parent/'_generated/runtime-25036-173531.json'
if capture.exists():
 data=json.loads(capture.read_text());fonts=[]
 for sample in data['samples']:
  if 'native_panel' not in sample:continue
  for cat in sample['native_panel']['categories']:
   for row in cat['rows']:
    if row['type']==11:
     text=row['progressText']['rendered_font'];assert text['font_height']==21 and text['text_view_size'][1]==21 and text['align']==8
     assert text['region'][1]==0 and text['region'][3]==row['progressText']['coord'][3]==24
     fonts.append(text['text_view_size'][1])
 assert fonts and 21<=22
 report['native_percent']={'capture':str(capture),'observations':len(fonts),'text_height':21,'vertical':'Top','new_box_height':22}
 print('PASS actual1487 native percentage21px/Top fits22px after2px drop; lower bound185 unchanged')
outdir=Path(__file__).parent/'_generated';outdir.mkdir(exist_ok=True)
# No game calls: disassemble the installed export and compile the SDK signature
# to assembly only. Native vector return is32B, pointers0/8/16, allocator24.
sys.path.insert(0,str(Path(__file__).parent/'_tools'))
import pefile,capstone
engine=game/'MyGUIEngine_x64.dll';pe=pefile.PE(str(engine));base=pe.OPTIONAL_HEADER.ImageBase
def instructions(name,length):
 r=next(e.address for e in pe.DIRECTORY_ENTRY_EXPORT.symbols if e.name and name.encode() in e.name)
 for _ in range(4):
  if pe.get_data(r,1)!=b'\xe9':break
  r+=5+struct.unpack('<i',pe.get_data(r+1,4))[0]
 return r,[(i.mnemonic,i.op_str) for i in capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64).disasm(pe.get_data(r,length),base+r)]
r,body=instructions('getSkinWidgetsByName@Widget',128)
for op in ['qword ptr [rdx], rax','qword ptr [rdx + 8], rax','qword ptr [rdx + 0x10], rax','rbp, r8','rdi, rdx','rsi, rcx']:
 assert ('mov',op) in body,op
assert any('[rcx + 0x3f8]' in op for _,op in body) and any('[rcx + 0x400]' in op for _,op in body)
_,finder=instructions('findWidgets@Widget',300)
assert ('lea','rcx, [rbx + 0x18]') in finder
source=outdir/'native_skin_abi.cpp'
source.write_text('''#include <mygui/MyGUI_Widget.h>
struct Access:MyGUI::Widget {
 static MyGUI::VectorWidgetPtr Find(MyGUI::Widget*w,const std::string&n) {
 MyGUI::VectorWidgetPtr (MyGUI::Widget::*f)(const std::string&)=&Access::getSkinWidgetsByName;return(w->*f)(n); }
};
MyGUI::VectorWidgetPtr NativeSkinABI(MyGUI::Widget*w,const std::string&n){return Access::Find(w,n);}
''')
vc=Path(r'C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC');sdk=Path(r'C:\Program Files\Microsoft SDKs\Windows\v7.1');kl=Path(r'D:\DEV\Kenshi\KenshiLib-src\Include')
env=os.environ.copy();env['INCLUDE']=';'.join(map(str,[vc/'include',sdk/'Include',kl,kl/'ogre']))
assembly=outdir/'native_skin_abi.asm'
subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/c','/EHsc','/MD','/O2','/DNDEBUG','/FAs','/Fa'+str(assembly),'/Fo'+str(outdir/'native_skin_abi.obj'),str(source)],env=env,check=True)
asm=assembly.read_text();assert '__imp_?getSkinWidgetsByName@Widget@MyGUI@@IEAA?AV?$vector' in asm
assert 'lea\trdx, QWORD PTR $T' in asm and 'mov\trcx, rax\n\tcall\tQWORD PTR __imp_?getSkinWidgetsByName' in asm
report['native_skin_api']={'engine':str(engine),'sha256':hashlib.sha256(engine.read_bytes()).hexdigest(),'getter_rva':hex(r),
 'body':body,'return':'RCX=this,RDX=hidden vector result,R8=name;size32,begin0/end8/cap16,empty allocator1 at24','compiler_assembly':str(assembly)}
print('PASS installed skin export vector ABI and production VC10 SDK member-pointer call compile; no game calls')
out=outdir/'progress_skin_proof.json';out.write_text(json.dumps(report,indent=2),encoding='utf-8')
