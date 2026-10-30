"""Audited ca0/a63b candidates. Bounded renderer geometry; Root supplies PID.

No process enumeration, native calls, injection, writes or screenshots. The
base probe preserves its8MB/256KB budgets and exact loaded-header/MAP gate.
"""
from pathlib import Path
import hashlib,struct,sys
import readonly_panel_probe as base

CA0='ca0f3e41a5f3bae2119885598ee464474df24e164e10df01493dea4dcc5df642'
CURRENT='a63b8462542a0cf5263fc9b50e8a72b81be02290981665922db408fcdadf80f3'
base.EXPECTED_SHA=CURRENT;base.APPROVED_SHAS={CURRENT,CA0}
ENGINE=Path(r'D:/SteamLibrary/steamapps/common/Kenshi/MyGUIEngine_x64.dll')

def renderer_profile():
 """Verify native exported getters/setter; no live function invocation."""
 sys.path.insert(0,str(Path(__file__).parent/'_tools'))
 import pefile
 pe=pefile.PE(str(ENGINE));exports={e.name:e.address for e in pe.DIRECTORY_ENTRY_EXPORT.symbols if e.name}
 def body(part,length):
  r=next(v for k,v in exports.items()if part in k)
  for _ in range(4):
   if pe.get_data(r,1)!=b'\xe9':break
   r+=5+struct.unpack('<i',pe.get_data(r+1,4))[0]
  return pe.get_data(r,length)
 assert body(b'getSubWidgetMain@SkinItem',8)==bytes.fromhex('48 8b 81 b0 00 00 00 c3')
 assert body(b'getSubWidgetText@SkinItem',8)==bytes.fromhex('48 8b 81 a8 00 00 00 c3')
 assert body(b'_getTextureName@SkinItem',8)==bytes.fromhex('48 8d 81 d8 00 00 00 c3')
 uv=body(b'_setUVSet@SubSkin',140)
 assert bytes.fromhex('f3 0f 10 41 50')in uv and bytes.fromhex('f3 44 0f 11 51 50')in uv
 return {'engine_sha256':hashlib.sha256(ENGINE.read_bytes()).hexdigest(),'skin_item_base':0x288,
         'widget_main':0x338,'widget_texture_name':0x360,'subskin_uv':0x50,'subskin_coord':0x18}

def skin_main(m,address):
 main=base.q(m,address+0x338)
 if not main:return None
 uv=list(base.unpack(m,'<4f',main+0x50))
 valid=all(-1<=v<=2 for v in uv)and uv[2]>=uv[0]and uv[3]>=uv[1]
 return {'main':hex(main),'coord':list(base.unpack(m,'<4i',main+0x18)),
         'uv':uv,'uv_plausible':valid,'texture':base.string(m,address+0x360),
         'profile':'current native SkinItem getter + SubSkin UV setter; main expected SubSkin by XML'}

original_panel=base.panel
PROFILE=None
def panel(m,address):
 result=original_panel(m,address)
 result['renderer_geometry_profile']=PROFILE
 chain=[];seen=set();current=base.q(m,address+8)
 while current and current not in seen and len(chain)<6:
  seen.add(current);node=base.widget(m,current);node['skin_main']=skin_main(m,current);chain.append(node)
  current=base.q(m,current+0x450)
 result['background_ancestors']=chain
 for category in result['categories']:
  for row in category['rows']:
   for child in row.get('progress_skin_children',[]):child['skin_main']=skin_main(m,int(child['address'],16))
 return result

if __name__=='__main__':
 if '--out'not in sys.argv:raise SystemExit('An explicit NEW --out JSON path is required')
 output=Path(sys.argv[sys.argv.index('--out')+1])
 if output.exists():raise SystemExit('Refusing to overwrite existing evidence: '+str(output))
 PROFILE=renderer_profile();print('Verified installed renderer getter profile:',PROFILE)
 base.panel=panel
 try:sys.exit(base.main())
 except (OSError,ValueError)as error:print(str(error),file=sys.stderr);sys.exit(2)
