"""Offline renderer parser/profile/layout checks; never opens any process."""
from pathlib import Path
import os,subprocess,unittest
from test_readonly_probe import Memory
import readonly_underlay_probe as probe

class UnderlayTests(unittest.TestCase):
 def test_current_native_getters_and_uv_setter(self):
  self.assertEqual(probe.renderer_profile()['widget_main'],0x338)
 def test_empty_skin_does_not_follow_unknown_fields(self):
  m=Memory();m.pack(0x10338,'<Q',0)
  self.assertIsNone(probe.skin_main(m,0x10000))
 def test_single_quad_geometry_and_uv_are_exact_not_a_colour_guess(self):
  m=Memory();widget,main=0x10000,0x20000
  m.pack(widget+0x338,'<Q',main);m.pack(main+0x18,'<4i',0,0,320,220)
  m.pack(main+0x50,'<4f',0,907/1080,240/1920,1072/1080)
  m.zero(widget+0x360,32);m.write(widget+0x360,b'Kenshi_UI.png');m.pack(widget+0x370,'<2Q',13,15)
  data=probe.skin_main(m,widget)
  self.assertEqual((data['coord'],data['texture'],data['uv_plausible']),([0,0,320,220],'Kenshi_UI.png',True))
 def test_sdk_widget_skin_base_without_gui_objects_or_calls(self):
  here=Path(__file__).resolve().parent;vc=Path(r'C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC');sdk=Path(r'C:\Program Files\Microsoft SDKs\Windows\v7.1');kl=Path(r'D:\DEV\Kenshi\KenshiLib-src\Include')
  env=os.environ.copy();env['INCLUDE']=';'.join(map(str,[vc/'include',sdk/'Include',kl,kl/'ogre']))
  env['LIB']=';'.join(map(str,[vc/'lib/amd64',sdk/'Lib/x64']))
  out=here/'_generated';exe=out/'underlay_layout.exe'
  subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2',str(here/'underlay_layout.cpp'),'/Fo'+str(out)+'\\','/Fe'+str(exe)],env=env,check=True)
  subprocess.run([str(exe)],env=env,check=True)

if __name__=='__main__':unittest.main()
