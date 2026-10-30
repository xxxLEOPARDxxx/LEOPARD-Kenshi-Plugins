"""Primary XML/native profile proof. Never opens or invokes the game."""
from pathlib import Path
import xml.etree.ElementTree as ET
import json,hashlib,sys,struct
here=Path(__file__).resolve().parent
game=Path(r'D:/SteamLibrary/steamapps/common/Kenshi')
report={'templates':[]}
for folder in [game/'data',game/'mods/Dark UI Advanced']:
 path=folder/'gui/templates/kenshi_templates.xml';tree=ET.parse(path).getroot()
 resource=lambda name:next(r for r in tree.findall('Resource')if r.get('name')==name)
 root=resource('Kenshi_ProgressBarFill').find('Widget');assert root.get('position')=='0 0 150 20'
 children=root.findall('Widget');track=next(w for w in children if w.get('name')=='TrackPlace')
 assert track.get('position')=='11 3 129 13' and track.get('align')=='Stretch'
 frame=next(w for w in children if w.get('skin')=='Kenshi_ProgressBarSkin');assert frame.get('position')=='0 0 150 20' and frame.get('align')=='Stretch'
 label=resource('Kenshi_PaintedWordWrapEmpty').find('Widget');client=next(w for w in label.findall('Widget')if w.get('name')=='Client')
 assert client.get('align')=='Stretch' and client.get('position').split()[:2]==['0','0']
 flat=resource('Kenshi_GenericTextBoxFlat').find('Widget')
 report['templates'].append({'path':str(path),'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'flat_skin':flat.get('skin'),'native_label_skin':client.get('skin')})
assert report['templates'][0]['flat_skin']=='Kenshi_GenericTextBoxFlatSkin'
assert report['templates'][1]['flat_skin']=='Kenshi_TextBoxEmptySkin'
print('PASS actual vanilla/Dark XML distinguishes framed generic label from full native client; complete progress frame/track stay together')
profiles=json.loads((here/'ui_profiles.json').read_text(encoding='utf-8'))['profiles']
for p in profiles:
 total=len(p['row_tops'])*p['row_height'];capacity=min(2,(p['parent_height']-total-1)//p['row_height'])
 assert capacity==p['expected_xp_capacity']
 spare=p['parent_height']-total-capacity*p['row_height']-1
 top=min(p['row_tops'][0],spare)
 assert top>=0 and top+total+1+capacity*p['row_height']<=p['parent_height']
 assert p['bar_height']+2==p['row_height']
 if 'observed_old_xp'in p:assert p['bar_height']==p['observed_old_xp']['bar_height']
assert profiles[0]['observed_old_xp']['name_region'][3]==3 and profiles[0]['name_font_height']==25
report['measured_profiles']=profiles
print('PASS actual181/185px5/6-row profiles fit2/1 complete native-font XP rows with no widget/font/bar-height shrink')
sys.path.insert(0,str(here/'_tools'))
import pefile,capstone
engine=game/'MyGUIEngine_x64.dll';pe=pefile.PE(str(engine));names=[e.name for e in pe.DIRECTORY_ENTRY_EXPORT.symbols if e.name]
for method in [b'createWidgetT@Widget',b'setNeedKeyFocus@WidgetInput',b'setInheritsPick@WidgetInput',b'setEditWordWrap@EditBox',b'setEditStatic@EditBox']:
 assert any(method in name for name in names),method
r=next(e.address for e in pe.DIRECTORY_ENTRY_EXPORT.symbols if e.name and b'getSkinWidgetsByName@Widget'in e.name)
for _ in range(4):
 if pe.get_data(r,1)!=b'\xe9':break
 r+=5+struct.unpack('<i',pe.get_data(r+1,4))[0]
body=[(i.mnemonic,i.op_str)for i in capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64).disasm(pe.get_data(r,128),pe.OPTIONAL_HEADER.ImageBase+r)]
assert any('[rcx + 0x3f8]'in op for _,op in body)and any('[rcx + 0x400]'in op for _,op in body)
report['engine']={'path':str(engine),'sha256':hashlib.sha256(engine.read_bytes()).hexdigest(),'readonly_skin_vector_rva':hex(r)}
print('PASS installed native factory/input APIs and read-only skin-vector layout verified; no game calls')
out=here/'_generated';out.mkdir(exist_ok=True);(out/'progress_skin_proof.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
