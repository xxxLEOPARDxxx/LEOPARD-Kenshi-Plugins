"""Synthetic SDK-layout fixtures only; no live process API."""
import contextlib,io,json,pathlib,struct,tempfile,unittest
from unittest import mock
import df_cargo_snapshot as c

class Memory:
 def __init__(self):self.data={};self.total=0;self.calls=0;self.heap=0x9000000;self.mutate=None
 def put(self,a,b):
  for i,v in enumerate(b):self.data[a+i]=v
 def zero(self,a,n):self.put(a,b'\0'*n)
 def read(self,a,n):
  self.calls+=1;self.total+=n
  if n>262144 or self.total>4*1024*1024:raise ValueError('fixture read budget')
  if self.mutate:
   value=self.mutate(a,n)
   if value is not None:return value
  try:return bytes(self.data[a+i] for i in range(n))
  except KeyError:raise OSError('fixture unreadable')
 def q(self,a,v):self.put(a,struct.pack('<Q',v))
 def i(self,a,v):self.put(a,struct.pack('<i',v))
 def alloc(self,raw):
  a=self.heap;self.heap+=len(raw)+64;self.put(a,raw);return a
 def string(self,a,text):
  raw=text.encode('utf-8');size=len(raw)
  content=raw+b'\0'*(16-size) if size<=15 else struct.pack('<QQ',self.alloc(raw),0)
  self.put(a,content+struct.pack('<QQQ',size,15 if size<=15 else size,0))
 def handle(self,a,typ,index):self.put(a,struct.pack('<QIIIII',0,typ,77,88,index,99)+b'\0'*4)
 def lektor(self,a,values):self.put(a,b'\0'*8+struct.pack('<IIQ',len(values),len(values),self.alloc(struct.pack('<'+'Q'*len(values),*values)) if values else 0))
 def gamedata(self,a,sid,typ,name='fixture'):
  self.zero(a,0x80);self.string(a+0x28,name);self.i(a+0x50,typ);self.string(a+0x58,sid)

class Fixture:
 base=0x140000000;faction=0x1000000;platoon=0x2000000;active=0x2010000;actor=0x3000000
 main=0x4000000;baginv=0x4010000;ore=0x6000000;bag=0x6010000;ore2=0x6020000
 def __init__(self):
  m=self.mem=Memory();self.modules={'kenshi_x64.exe':{'base':self.base,'size':0x3000000},'kenshilib.dll':{'base':0x160000000,'path':'offline.dll'}}
  vtchar=self.base+0x900000;vtbag=vtchar+0x1000;vtitem=vtchar+0x2000
  for vt,rva,code in [(vtchar,0x5e1450,'488b81e8020000c3'),(vtbag,0x76b320,'488b8190020000c3'),(vtitem,0xd2240,'33c0c3cccccccccc')]:m.q(vt+0x160,self.base+rva);m.put(self.base+rva,bytes.fromhex(code))
  m.zero(self.actor,0x680);m.q(self.actor,vtchar);m.q(self.actor+0x10,self.faction);m.q(self.actor+0x658,self.active);m.q(self.active+0x78,self.platoon);m.q(self.actor+0x2e8,self.main);m.handle(self.actor+0x58,1,11);m.string(self.actor+0x18,'Actor')
  m.gamedata(0x7000000,'actor-prototype.mod',1);m.q(self.actor+0x40,0x7000000)
  m.gamedata(0x7010000,'1866-gamedata.base',4,'Ore');m.gamedata(0x7020000,'bag-data.mod',46,'Bag')
  for pointer,data,typ,qty,vt,parent in [(self.ore,0x7010000,4,3,vtitem,self.actor),(self.bag,0x7020000,46,1,vtbag,self.actor),(self.ore2,0x7010000,4,5,vtitem,self.bag)]:
   m.zero(pointer,0x300);m.q(pointer,vt);m.q(pointer+0x10,self.faction);m.q(pointer+0x40,data);m.handle(pointer+0x58,typ,pointer//0x10000);m.i(pointer+0x12c,qty);m.i(pointer+0x130,2);m.i(pointer+0x134,3);m.put(pointer+0x114,struct.pack('<ff',1.,.5));m.string(pointer+0xe8,'main');m.handle(pointer+0x140,11,0);m.handle(pointer+0x160,1 if parent==self.actor else 46,parent//0x10000)
  m.q(self.bag+0x290,self.baginv);self.inventory(self.main,self.actor,[self.ore,self.bag]);self.inventory(self.baginv,self.bag,[self.ore2])
 def inventory(self,inventory,owner,items,placements=None):
  m=self.mem;section=inventory+0x1000;m.q(inventory+0x88,owner);m.lektor(inventory+0x10,items);m.lektor(inventory+0x68,[section]);m.q(section+0xc8,inventory);m.string(section+8,'main')
  raw=b''.join(struct.pack('<QHHHH',item,0,0,2,3) for item in (items if placements is None else placements));begin=m.alloc(raw);m.put(section+0x40,struct.pack('<QQQ',begin,begin+len(raw),begin+len(raw)))
 def world(self,unloaded=False):
  m=self.mem;world,manager,ownership=0x100000,0x200000,0x1100000
  m.q(0x160000020,world);m.q(world+0x4a8,manager);m.lektor(manager,[self.faction]);m.gamedata(0x7030000,c.TARGET,10);m.q(self.faction+0x240,0x7030000);m.q(self.faction+0x80,ownership);m.i(ownership+0x88,8123);m.string(self.faction+0x1a8,'Arbitrary current name');m.lektor(self.faction+0x208,[self.platoon]);m.lektor(self.faction+0x220,[]);m.q(self.platoon+0x1d8,0 if unloaded else self.active);m.handle(self.platoon+0x58,41,33);m.lektor(self.active+0x50,[self.actor])
 def capture(self):
  snap=c.Snapshot(self.modules)
  with mock.patch.object(c.p,'_reader',self.mem):return snap.actor(self.actor,self.faction,self.platoon,self.active)

class Tests(unittest.TestCase):
 def test_final_revalidation_invalidates_actor_totals(self):
  f=Fixture();f.world();reads=[]
  def mutate(a,n):
   if a==f.actor+0x10 and n==8:
    reads.append(1)
    if len(reads)>=3:return struct.pack('<Q',0xdeadbeef)
  f.mem.mutate=mutate;out={'actors':[],'platoons':[],'unloaded_platoons':[],'errors':[],'completed':False}
  with mock.patch.object(c.p,'_reader',f.mem),mock.patch.object(c.p,'exps',return_value={'?ou@@3PEAVGameWorld@@EA':0x20}):c.Snapshot(f.modules).collect(out)
  self.assertFalse(out['completed']);self.assertIsNone(out['actors'][0]['totals_by_sid']);self.assertNotEqual(out['actors'][0]['cargo_status'],'observed')
 def test_four_thunks_allowed_five_rejected(self):
  for count,expected in [(4,'observed'),(5,'unknown')]:
   f=Fixture();vtable=struct.unpack('<Q',f.mem.read(f.actor,8))[0];first=f.base+0x50000;f.mem.q(vtable+0x160,first)
   for index in range(count):
    address=first+index*16;target=f.base+0x5e1450 if index+1==count else address+16
    f.mem.put(address,b'\xe9'+struct.pack('<i',target-address-5)+b'\xcc'*3)
   self.assertEqual(f.capture()['cargo_status'],expected)
 def test_equipped_bag_missing_from_borrowed_list_still_includes_contents(self):
  f=Fixture();f.mem.lektor(f.main+0x10,[f.ore]);f.mem.put(f.bag+0x129,b'\x01')
  out=f.capture();self.assertEqual(out['cargo_status'],'observed');self.assertEqual(out['totals_by_sid']['1866-gamedata.base'],8)
 def test_unlisted_unequipped_placement_is_not_silently_accepted(self):
  f=Fixture();f.mem.lektor(f.main+0x10,[f.ore])
  self.assertIsNone(f.capture()['totals_by_sid'])
 def test_borrowed_item_without_physical_placement_is_unknown(self):
  f=Fixture();f.inventory(f.main,f.actor,[f.ore,f.bag],placements=[f.bag])
  self.assertIsNone(f.capture()['totals_by_sid'])
 def test_live_character_linker_thunk_to_verified_getter(self):
  f=Fixture();vt=f.mem.read(f.actor,8);vtable=struct.unpack('<Q',vt)[0]
  thunk=f.base+0x47b13;target=f.base+0x5e1450
  f.mem.q(vtable+0x160,thunk)
  f.mem.put(thunk,b'\xe9'+struct.pack('<i',target-thunk-5)+b'\xcc'*3)
  out=f.capture();self.assertEqual(out['cargo_status'],'observed');self.assertEqual(out['totals_by_sid']['1866-gamedata.base'],8)
 def test_container_linker_thunk_to_verified_getter(self):
  f=Fixture();vtable=struct.unpack('<Q',f.mem.read(f.bag,8))[0]
  thunk=f.base+0x40100;target=f.base+0x76b320
  f.mem.q(vtable+0x160,thunk);f.mem.put(thunk,b'\xe9'+struct.pack('<i',target-thunk-5)+b'\xcc'*3)
  out=f.capture();self.assertEqual(out['cargo_status'],'observed');self.assertEqual(out['totals_by_sid']['1866-gamedata.base'],8)
 def test_thunk_loop_refused(self):
  f=Fixture();address=f.base+0x5e1450;f.mem.put(address,b'\xe9'+struct.pack('<i',-5)+b'\xcc'*3)
  self.assertIsNone(f.capture()['totals_by_sid'])
 def test_thunk_outside_game_refused_before_destination_read(self):
  f=Fixture();address=f.base+0x5e1450;target=f.base+0x4000000
  f.mem.put(address,b'\xe9'+struct.pack('<i',target-address-5)+b'\xcc'*3)
  attempts=[]
  def observe(a,n):
   if a==target:attempts.append(a)
  f.mem.mutate=observe;self.assertIsNone(f.capture()['totals_by_sid']);self.assertEqual(attempts,[])
 def test_thunk_unknown_destination_not_assumed_inventory(self):
  f=Fixture();address=f.base+0x5e1450;target=f.base+0x50000
  f.mem.put(address,b'\xe9'+struct.pack('<i',target-address-5)+b'\xcc'*3);f.mem.put(target,b'\xcc'*8)
  self.assertIsNone(f.capture()['totals_by_sid'])
 def test_nested_real_cargo_and_full_handles(self):
  out=Fixture().capture();self.assertEqual(out['cargo_status'],'observed');self.assertEqual(out['totals_by_sid'],{'1866-gamedata.base':8,'bag-data.mod':1});self.assertEqual(len(out['inventories']),2);self.assertEqual(out['handle'],(1,77,88,11,99));self.assertEqual(out['inventories'][1]['items'][0]['placement']['width'],2)
 def test_duplicate_item_not_double_counted(self):
  f=Fixture();f.inventory(f.baginv,f.bag,[f.ore]);out=f.capture();self.assertEqual(out['cargo_status'],'unknown');self.assertIsNone(out['totals_by_sid'])
 def test_cycle_not_empty(self):
  f=Fixture();f.mem.q(f.bag+0x290,f.main);out=f.capture();self.assertEqual(out['cargo_status'],'unknown');self.assertIsNone(out['totals_by_sid'])
 def test_missing_placement_not_omitted(self):
  f=Fixture();f.inventory(f.main,f.actor,[f.ore,f.bag],placements=[f.bag]);self.assertIsNone(f.capture()['totals_by_sid'])
 def test_unknown_virtual_abi_not_empty(self):
  f=Fixture();f.mem.put(f.base+0x76b320,b'\xcc'*8);out=f.capture();self.assertEqual(out['cargo_status'],'unknown')
 def test_negative_quantity_unknown(self):
  f=Fixture();f.mem.i(f.ore+0x12c,-1);self.assertIsNone(f.capture()['totals_by_sid'])
 def test_nonfinite_charges_explicit_unknown(self):
  f=Fixture();f.mem.put(f.ore+0x118,struct.pack('<f',float('nan')));out=f.capture();self.assertEqual(out['cargo_status'],'unknown');self.assertIsNone(out['inventories'][0]['items'][0]['charges']['left']['value'])
 def test_foreign_membership_unknown(self):
  f=Fixture();f.mem.q(f.actor+0x10,0xdeadbeef);self.assertEqual(f.capture()['status'],'unknown')
 def test_mutated_cargo_invalidates_count(self):
  f=Fixture();reads=[]
  def mutate(a,n):
   if a==f.ore+0xc0:
    reads.append(1)
    if len(reads)>1:
     raw=bytearray(f.mem.data[a+i] for i in range(n));struct.pack_into('<i',raw,0x6c,4);return bytes(raw)
  f.mem.mutate=mutate;self.assertIsNone(f.capture()['totals_by_sid'])
 def test_oversized_borrowed_list_refused_before_array_read(self):
  m=Memory();m.put(0x10000,b'\0'*8+struct.pack('<IIQ',4097,4097,0x20000))
  with mock.patch.object(c.p,'_reader',m),self.assertRaises(ValueError):c.Snapshot({}).lektor(0x10000,4096)
  self.assertEqual(m.calls,1)
 def test_global_item_bound_not_silent_truncation(self):
  with mock.patch.object(c,'MAX_ITEMS',1):out=Fixture().capture()
  self.assertIsNone(out['totals_by_sid'])
 def test_dynamic_faction_name_balance_roster(self):
  f=Fixture();f.world();out={'actors':[],'platoons':[],'unloaded_platoons':[],'errors':[]}
  with mock.patch.object(c.p,'_reader',f.mem),mock.patch.object(c.p,'exps',return_value={'?ou@@3PEAVGameWorld@@EA':0x20}):c.Snapshot(f.modules).collect(out)
  self.assertTrue(out['completed']);self.assertEqual(out['faction']['name'],'Arbitrary current name');self.assertEqual(out['faction']['treasury_before'],8123);self.assertEqual(len(out['actors']),1)
 def test_unloaded_member_not_zero_cargo(self):
  f=Fixture();f.world(unloaded=True);out={'actors':[],'platoons':[],'unloaded_platoons':[],'errors':[],'all_member_totals_by_sid':None,'completed':False}
  with mock.patch.object(c.p,'_reader',f.mem),mock.patch.object(c.p,'exps',return_value={'?ou@@3PEAVGameWorld@@EA':0x20}):c.Snapshot(f.modules).collect(out)
  self.assertFalse(out['completed']);self.assertIsNone(out['all_member_totals_by_sid']);self.assertEqual(out['faction']['treasury_status'],'observed')
 def test_precise_utc_and_required_arguments(self):
  self.assertEqual(c.utc_filetime('2026-10-30T14:30:22.9911587Z'),134378442229911587)
  with contextlib.redirect_stderr(io.StringIO()),self.assertRaises(SystemExit):c.arguments(['--pid','123','--phase','before_save','--output','new.json'])
 def test_help_no_native_access(self):
  with mock.patch.object(c.p,'NativeAccess') as access,contextlib.redirect_stdout(io.StringIO()),self.assertRaises(SystemExit):c.main(['--help'])
  access.assert_not_called()
 def test_changed_abi_profile_refused_before_process_access(self):
  with tempfile.TemporaryDirectory() as directory,mock.patch.object(c.p,'NativeAccess') as access,mock.patch.object(pathlib.Path,'read_bytes',return_value=b'changed profile'),contextlib.redirect_stdout(io.StringIO()):
   target=pathlib.Path(directory)/'new.json';self.assertEqual(c.main(['--pid','123','--expected-start-utc','2026-10-30T14:30:22Z','--phase','before_save','--output',str(target)]),1);self.assertEqual(json.loads(target.read_text())['read_bytes'],0)
  access.assert_not_called()
 def test_wrong_start_no_rpm(self):
  access=mock.Mock();reader=mock.Mock(total=0);access.open_verified.return_value=(reader,{}, {'created_filetime':1})
  with tempfile.TemporaryDirectory() as directory,mock.patch.object(c.p,'NativeAccess',return_value=access),mock.patch.object(c.p,'verify_profile',return_value={}),contextlib.redirect_stdout(io.StringIO()):
   target=pathlib.Path(directory)/'new.json';self.assertEqual(c.main(['--pid','123','--expected-start-utc','2026-10-30T14:30:22Z','--phase','before_save','--output',str(target)]),1);out=json.loads(target.read_text());self.assertEqual(out['read_bytes'],0)
  reader.read.assert_not_called();reader.close.assert_called_once()
 def test_existing_json_preserved_before_native_access(self):
  with tempfile.TemporaryDirectory() as directory,mock.patch.object(c.p,'NativeAccess') as access:
   target=pathlib.Path(directory)/'old.json';target.write_text('keep')
   with self.assertRaises(SystemExit):c.main(['--pid','123','--expected-start-utc','2026-10-30T14:30:22Z','--phase','after_load','--output',str(target)])
   self.assertEqual(target.read_text(),'keep');access.assert_not_called()

if __name__=='__main__':unittest.main()
