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
 def test_committed_bounded_trace_has_native_before_after_and_no_stale_inflight_entry(self):
  m=Memory();ring,head=0x10000,0x20000;m.zero(ring,32*104);m.pack(head,'<I',2)
  m.pack(ring,'<IIIiQQQii7Q',1,1234,5,337,0x30000,0x40000,0x50000,6,0,0x60000,0x70000,0,0,0x80000,0x80000,0x80000)
  # The reserved second entry is still being written (sequence0).
  m.pack(ring+104,'<I',0)
  result=probe.lifecycle_trace(m,ring,head);self.assertEqual(len(result['records']),1)
  row=result['records'][0];self.assertEqual((row['generation'],row['context_mask'],row['progress_after_original']),(337,5,['0x0','0x0']))
  self.assertEqual(row['divider_before'],row['divider_after_draw']);self.assertTrue(result['stable_head'])
 def test_debug_off_trace_does_not_read_uninitialized_ring(self):
  m=Memory();m.pack(0x20000,'<I',0)
  self.assertEqual(probe.lifecycle_trace(m,0x10000,0x20000),{'head':0,'records':[],'stable_head':True})
 def test_owned_native_labels_use_verified_ref_string_arrays(self):
  m=Memory();refs,skins,types,w=0x60000,0x61000,0x62000,0x70000
  m.zero(refs,120);m.zero(skins,200);m.zero(types,200);m.pack(refs+8,'<Q',w)
  for address,text in [(skins,b'PaintedSkin'),(types,b'EditBox')]:
   m.write(address,text);m.pack(address+16,'<2Q',len(text),15)
  m.pack(w+0x20,'<4i',8,131,141,25);m.pack(w+0x30,'<2i',18,1378);m.pack(w+0x450,'<Q',0)
  m.write(w+0x46c,b'\1');m.pack(w+0x330,'<Q',0);m.pack(w+0x3d0,'<Q',0)
  labels=probe.owned_labels(m,refs,skins,types)
  self.assertEqual((labels[0]['skin'],labels[0]['native_type'],labels[0]['widget']['coord']),('PaintedSkin','EditBox',[8,131,141,25]))
 def test_reversible_placement_cache_has_compiler_verified_layout_and_budget(self):
  m=Memory();cache,w=0x80000,0x90000;m.zero(cache,3616);m.pack(cache+3608,'<i',1)
  m.pack(cache+32,'<Q',w);m.pack(cache+48,'<4i',8,147,285,25);m.pack(cache+64,'<4i',8,130,285,25);m.pack(w+0x20,'<4i',8,130,285,25)
  result=probe.placement_cache(m,cache)
  self.assertEqual((result['entries'][0]['original'][1],result['entries'][0]['current'][1]),(147,130))
  m.pack(cache+3608,'<i',65)
  with self.assertRaisesRegex(ValueError,'budget'):probe.placement_cache(m,cache)
 def test_owned_skin_vector_uses_native_skin_children_not_content_children(self):
  m=Memory();bar,storage,child=0x10000,0x20000,0x30000
  m.pack(bar+0x3f8,'<3Q',storage,storage+8,storage+8);m.pack(storage,'<Q',child)
  m.pack(child+0x20,'<4i',11,5,132,17);m.pack(child+0x450,'<Q',bar);m.write(child+0x46c,b'\1')
  m.pack(child+0x30,'<2i',11,5)
  m.pack(child+0x3d0,'<Q',0);m.pack(child+0x330,'<Q',0)
  m.zero(child+0x428,32);m.write(child+0x428,b'TrackPlace');m.pack(child+0x438,'<2Q',10,15)
  result=probe.skin_children(m,bar)
  self.assertEqual((result[0]['name'],result[0]['coord']),('TrackPlace',[11,5,132,17]))
  m.pack(bar+0x3f8,'<3Q',storage,storage+72,storage+72)
  with self.assertRaisesRegex(ValueError,'budget'):probe.skin_children(m,bar)
 def test_current_dll_and_map_match(self):
  symbols,stamp=probe.map_symbols(probe.BUILD/'SkillLevelUpInfo.map',probe.BUILD/'SkillLevelUpInfo.dll')
  self.assertEqual(symbols['g_panel'],0x28ff0)
  self.assertEqual(symbols['g_slots'],0x29108)
  self.assertEqual(symbols['g_panelTrace'],0x282d0)
  self.assertEqual(symbols['g_underlay'],0x27a18)
  self.assertEqual(stamp,0x6ae4f78e)
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
  m=Memory();m.pack(0x10020,'<4i',9,105,307,22);m.pack(0x10450,'<Q',0x20000);m.write(0x1046c,b'\1');m.pack(0x103d0,'<Q',0);m.pack(0x10330,'<Q',0)
  m.pack(0x10030,'<2i',9,105)
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
 def test_composite_font_snapshot_uses_verified_native_getter_offsets(self):
  m=Memory();outer,client,text,font=0x10000,0x20000,0x30000,0x40000
  m.pack(outer+0x20,'<4i',9,109,307,26);m.pack(outer+0x30,'<2i',9,109)
  m.pack(client+0x20,'<4i',0,0,307,26);m.pack(client+0x30,'<2i',9,109)
  m.pack(outer+0x3d0,'<Q',client);m.pack(client+0x3d0,'<Q',0);m.pack(client+0x330,'<Q',text)
  m.pack(text+0xe0,'<Q',font);m.pack(text+0xf0,'<i',25)
  m.pack(text+0x18,'<4i',0,0,307,26);m.pack(text+0x28,'<2i',9,109)
  m.pack(text+0x140,'<2i',100,25);m.pack(text+0xb4,'<i',1);m.pack(text+0x114,'<2i',0,0);m.write(text+0x112,b'\0')
  m.zero(text+0x70,32)
  m.zero(font+8,32);m.write(font+8,b'ActualFont');m.pack(font+24,'<QQ',10,15)
  style=probe.rendered_font(m,outer)
  self.assertEqual((style['face'],style['font_height'],style['widget']),('ActualFont',25,hex(client)))
  self.assertEqual(style['client_chain'][1]['absolute'],[9,109])
  self.assertEqual(style['text_view_size'],[100,25])
 def test_actual_rendered_caption_supports_inline_and_heap_utf16(self):
  m=Memory();m.zero(0x10000,32);m.write(0x10000,'Уворот'.encode('utf-16-le'));m.pack(0x10010,'<QQ',6,7)
  self.assertEqual(probe.caption(m,0x10000),'Уворот')
  text='Боевые искусства';m.pack(0x10000,'<Q',0x20000);m.pack(0x10010,'<QQ',len(text),31);m.write(0x20000,text.encode('utf-16-le'))
  self.assertEqual(probe.caption(m,0x10000),text)
 def test_import_has_no_process_side_effects(self):
  # Import above succeeded without calling kernel() or requiring a PID.
  self.assertEqual(len(probe.NAMES),37)
  self.assertEqual(probe.NAMES[2:3]+probe.NAMES[13:14],['Dexterity','Martial arts'])
 def test_only_actual_loader_imagebase_rewrite_is_allowed(self):
  disk=(probe.BUILD/'SkillLevelUpInfo.dll').read_bytes();pe=struct.unpack_from('<I',disk,0x3c)[0];opt=pe+24
  base=0x7ffe7f480000;header=bytearray(disk[:1024]);struct.pack_into('<Q',header,opt+24,base)
  image_size=struct.unpack_from('<I',disk,opt+56)[0]
  verified=probe.loaded_header(header,disk,base,image_size)
  self.assertEqual(verified['changed_offsets'],[hex(opt+24+n) for n in (2,3,4,5)])
  header[pe+8]^=1
  with self.assertRaisesRegex(ValueError,'outside'):probe.loaded_header(header,disk,base,image_size)
 def test_wrong_loader_base_and_image_size_are_rejected(self):
  disk=(probe.BUILD/'SkillLevelUpInfo.dll').read_bytes();pe=struct.unpack_from('<I',disk,0x3c)[0];opt=pe+24
  header=bytearray(disk[:1024]);struct.pack_into('<Q',header,opt+24,0x12340000)
  image_size=struct.unpack_from('<I',disk,opt+56)[0]
  with self.assertRaisesRegex(ValueError,'neither actual nor preferred'):probe.loaded_header(header,disk,0x7ffe7f480000,image_size)
  with self.assertRaisesRegex(ValueError,'size'):probe.loaded_header(disk[:1024],disk,0x7ffe7f480000,image_size+0x1000)

if __name__=='__main__':unittest.main()
