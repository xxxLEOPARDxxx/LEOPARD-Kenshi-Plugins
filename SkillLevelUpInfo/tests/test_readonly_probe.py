"""Offline tests of the native-memory parser; never opens a process."""
from pathlib import Path
import struct, unittest
import readonly_panel_probe as probe

class Memory:
 def __init__(self):self.data={}
 def write(self,a,data):self.data.update({a+i:v for i,v in enumerate(data)})
 def read(self,a,n):return bytes(self.data[a+i] for i in range(n))
 def pack(self,a,fmt,*v):self.write(a,struct.pack(fmt,*v))
 def zero(self,a,n):self.write(a,bytes(n))

class ProbeTests(unittest.TestCase):
 def test_current_dll_and_map_match(self):
  symbols,stamp=probe.map_symbols(probe.BUILD/'SkillLevelUpInfo.map',probe.BUILD/'SkillLevelUpInfo.dll')
  self.assertEqual(symbols['g_panel'],0x22078)
  self.assertEqual(symbols['g_slots'],0x220f8)
  self.assertEqual(stamp,0x6ae48f91)
 def test_vc10_map_keeps_key_order_not_visual_order(self):
  m=Memory();obj,head=0x10000,0x11000
  m.pack(obj+8,'<QQ',head,3);m.pack(head+8,'<Q',0x13000)
  m.pack(0x13000,'<3Q',0x12000,head,0x14000)
  m.pack(0x12000,'<3Q',head,0x13000,head)
  m.pack(0x14000,'<3Q',head,0x13000,head)
  self.assertEqual(probe.nodes(m,obj,3),[0x12000,0x13000,0x14000])
  with self.assertRaisesRegex(ValueError,'budget'):probe.nodes(m,obj,2)
 def test_vc10_string_small_and_heap(self):
  m=Memory();m.zero(0x10000,40);m.write(0x10000,b'Dexterity');m.pack(0x10010,'<QQ',9,15)
  self.assertEqual(probe.string(m,0x10000),'Dexterity')
  value=b'skill_xp_info_slot_1';m.pack(0x10000,'<Q',0x20000);m.pack(0x10010,'<QQ',len(value),31);m.write(0x20000,value)
  self.assertEqual(probe.string(m,0x10000),value.decode())
 def test_real_widget_offsets_and_parent(self):
  m=Memory();m.pack(0x10020,'<4i',9,105,307,22);m.pack(0x10450,'<Q',0x20000);m.write(0x1046c,b'\1')
  w=probe.widget(m,0x10000)
  self.assertEqual((w['bottom'],w['parent'],w['visible']),(127,'0x20000',True))
 def test_owner_snapshot_uses_node_plus56_not64(self):
  m=Memory();obj,head,node=0x10000,0x11000,0x12000;owner=[1,2,3,4,5]
  m.pack(obj+8,'<QQ',head,1);m.pack(head+8,'<Q',node);m.pack(node,'<3Q',head,head,head)
  m.zero(node+24,32);m.pack(node+32,'<5I',*owner)
  snapshot=node+56;m.zero(snapshot,1344);m.write(snapshot+1336,b'\1\1')
  for index,value in [(2,79.929),(13,63.490),(21,86.911)]:
   m.pack(snapshot+index*28,'<ffIIBB2xii',value,.001,1000,2000,1,1,0,0)
   reference=0x20000+index*4;m.pack(snapshot+1040+index*8,'<Q',reference);m.pack(reference,'<f',value)
  # Empty native pointer-key index tree: each inspected skill reports missing binding.
  index=0x30000;index_head=0x31000;m.pack(index+8,'<Q',index_head);m.pack(index_head+8,'<Q',index_head)
  state=probe.selected_state(m,obj,owner,2500,120000,index)
  self.assertTrue(state['found']);self.assertEqual(state['snapshot'],hex(snapshot))
  self.assertEqual([s['name'] for s in state['skills']],['Dexterity','Martial arts','Athletics'])
  self.assertTrue(all(s['active'] and s['recent'] and s['age_ms']==500 for s in state['skills']))
  self.assertTrue(all(not s['binding_matches'] for s in state['skills']))
  self.assertTrue(state['live'] and state['pending'])
 def test_binding_lookup_uses_scalar_key_and_validated_layout(self):
  m=Memory();obj,head,node=0x10000,0x11000,0x12000
  m.pack(obj+8,'<Q',head);m.pack(head+8,'<Q',node);m.pack(node,'<3Q',head,head,head)
  m.pack(node+24,'<Q',0x40000);m.pack(node+32,'<4Q',0x50000,0x60000,13,77)
  self.assertEqual(probe.binding(m,obj,0x40000),{'skill':'0x50000','owner':'0x60000','statIndex':13,'token':77})
  self.assertIsNone(probe.binding(m,obj,0x40004))
 def test_import_has_no_process_side_effects(self):
  # Import above succeeded without calling kernel() or requiring a PID.
  self.assertEqual(len(probe.NAMES),37)
  self.assertEqual(probe.NAMES[2:3]+probe.NAMES[13:14],['Dexterity','Martial arts'])
 def test_only_actual_loader_imagebase_rewrite_is_allowed(self):
  disk=(probe.BUILD/'SkillLevelUpInfo.dll').read_bytes();pe=struct.unpack_from('<I',disk,0x3c)[0];opt=pe+24
  base=0x7ffe7f480000;header=bytearray(disk[:1024]);struct.pack_into('<Q',header,opt+24,base)
  verified=probe.loaded_header(header,disk,base,0x27000)
  self.assertEqual(verified['changed_offsets'],['0x14a','0x14b','0x14c','0x14d'])
  header[pe+8]^=1
  with self.assertRaisesRegex(ValueError,'outside'):probe.loaded_header(header,disk,base,0x27000)
 def test_wrong_loader_base_and_image_size_are_rejected(self):
  disk=(probe.BUILD/'SkillLevelUpInfo.dll').read_bytes();pe=struct.unpack_from('<I',disk,0x3c)[0];opt=pe+24
  header=bytearray(disk[:1024]);struct.pack_into('<Q',header,opt+24,0x12340000)
  with self.assertRaisesRegex(ValueError,'neither actual nor preferred'):probe.loaded_header(header,disk,0x7ffe7f480000,0x27000)
  with self.assertRaisesRegex(ValueError,'size'):probe.loaded_header(disk[:1024],disk,0x7ffe7f480000,0x28000)

if __name__=='__main__':unittest.main()
