"""Offline tests only: importing/running this file never opens any process."""
import contextlib,io,pathlib,runpy,struct,unittest
from unittest import mock
import df_readonly_shop_probe as probe

class FakeMemory:
 def __init__(self):self.bytes={}
 def put(self,address,data):
  for offset,value in enumerate(data):self.bytes[address+offset]=value
 def u64(self,address,value):self.put(address,struct.pack('<Q',value))
 def u32(self,address,value):self.put(address,struct.pack('<I',value))
 def read(self,address,count):
  try:return bytes(self.bytes[address+i] for i in range(count))
  except KeyError:raise OSError('unreadable fake memory')
 def string(self,address,value):
  value=value.encode('utf-8')
  if len(value)>15:raise ValueError('fake SSO only')
  self.put(address,value+b'\0'*(16-len(value))+struct.pack('<QQQ',len(value),15,0))

class ProbeTests(unittest.TestCase):
 def setUp(self):self.memory=FakeMemory();self.patch=mock.patch.object(probe,'_reader',self.memory);self.patch.start()
 def tearDown(self):self.patch.stop()
 def test_russian_confirmed_barman_name(self):
  self.assertTrue(probe.merchant_name_matches('Трактирщик'))
  self.assertTrue(probe.merchant_name_matches('Торговец скрытых'))
 def test_english_names(self):
  for name in ('Barman','Bartender','Barkeeper','Shinobi Trader'):self.assertTrue(probe.merchant_name_matches(name))
  self.assertFalse(probe.merchant_name_matches('Игути'))
 def test_template_name_is_considered(self):self.assertTrue(probe.merchant_name_matches('Someone','Barkeeper'))
 def test_pid_required(self):
  with contextlib.redirect_stderr(io.StringIO()),self.assertRaises(SystemExit):probe.parse_args(['--output','x.json'])
 def test_output_required(self):
  with contextlib.redirect_stderr(io.StringIO()),self.assertRaises(SystemExit):probe.parse_args(['--pid','123'])
 def test_any_explicit_positive_pid_supported(self):self.assertEqual(probe.parse_args(['--pid','123','--output','x.json']).pid,123)
 def test_pid_zero_rejected(self):
  with contextlib.redirect_stderr(io.StringIO()),self.assertRaises(SystemExit):probe.parse_args(['--pid','0','--output','x.json'])
 def test_unrelated_process_path_rejected_before_hash(self):
  hasher=mock.Mock()
  with self.assertRaises(ValueError):probe.verify_executable(probe.GAME/'unrelated.exe',hasher)
  hasher.assert_not_called()
 def test_same_name_outside_game_rejected(self):
  with self.assertRaises(ValueError):probe.verify_executable(pathlib.Path('C:/Windows/Kenshi_x64.exe'),lambda _:next(iter(probe.EXE_HASHES.values())))
 def test_known_path_and_hash(self):
  path=probe.GAME/'Kenshi_x64.exe';expected=probe.EXE_HASHES[str(path).casefold()]
  self.assertEqual(probe.verify_executable(path,lambda _:expected)['sha256'],expected)
 def test_unknown_hash_rejected(self):
  with self.assertRaises(ValueError):probe.verify_executable(probe.GAME/'Kenshi_x64.exe',lambda _:'0'*64)
 def ownership_memory(self):
  c,p,ap,owner,vt,fn=0x10000,0x20000,0x30000,0x40000,0x50000,0x60000
  for address,value in [(c+0x10,owner),(owner+0x250,0),(c+0x658,ap),(ap+0x78,p),(p,vt),(vt+0x98,fn)]:self.memory.u64(address,value)
  self.memory.put(fn,bytes.fromhex('488d8148010000c3')+b'\xcc'*8)
  return c,p,ap,owner,fn
 def test_verified_npc_home_ownership_is_platoon_ownership(self):
  c,p,ap,_,_=self.ownership_memory();self.assertEqual(probe.npc_ownership(c,p,ap),p+0x148)
 def test_player_ownership_not_guessed(self):
  c,p,ap,owner,_=self.ownership_memory();self.memory.u64(owner+0x250,0x70000)
  with self.assertRaises(ValueError):probe.npc_ownership(c,p,ap)
 def test_character_platoon_mismatch_not_guessed(self):
  c,p,ap,_,_=self.ownership_memory();self.memory.u64(c+0x658,0x70000)
  with self.assertRaises(ValueError):probe.npc_ownership(c,p,ap)
 def test_unknown_virtual_ownership_getter_not_guessed(self):
  c,p,ap,_,fn=self.ownership_memory();self.memory.put(fn,b'\xcc'*16)
  with self.assertRaises(ValueError):probe.npc_ownership(c,p,ap)
 def test_unreadable_stock_not_zero(self):
  with self.assertRaises(OSError):probe.section_items(0x10000)
 def test_invalid_vector_not_zero(self):
  self.memory.put(0x10040,struct.pack('<QQQ',0x20000,0x1ffff,0x20000))
  with self.assertRaises(ValueError):probe.section_items(0x10000)
 def test_empty_valid_vector(self):
  self.memory.put(0x10040,struct.pack('<QQQ',0,0,0));self.assertEqual(probe.section_items(0x10000),[])
 def test_null_item_not_zero(self):
  self.memory.put(0x10040,struct.pack('<QQQ',0x20000,0x20010,0x20010));self.memory.put(0x20000,b'\0'*16)
  with self.assertRaises(ValueError):probe.section_items(0x10000)
 def test_missing_item_data_not_zero(self):
  self.memory.put(0x10040,struct.pack('<QQQ',0x20000,0x20010,0x20010));self.memory.put(0x20000,struct.pack('<QQ',0x30000,0));self.memory.u64(0x30040,0)
  with self.assertRaises(ValueError):probe.section_items(0x10000)
 def test_physical_ore_count_is_exact_sid(self):
  self.assertEqual(probe.ore_sum([{'sid':'1866-gamedata.base','quantity':3},{'sid':'1866-another.mod','quantity':9}]),3)
 def test_native_int_overflow_not_counted(self):
  with self.assertRaises(ValueError):probe.ore_sum([{'sid':'1866-gamedata.base','quantity':2147483648}])
 def test_bad_string_not_empty(self):
  self.memory.put(0x10000,b'\0'*16+struct.pack('<QQQ',20,15,0))
  with self.assertRaises(ValueError):probe.s(0x10000)
 def test_reader_budget_checked_before_api(self):
  native=mock.Mock();reader=probe.Reader(native,1)
  with self.assertRaises(ValueError):reader.read(0x10000,probe.BUDGET+1)
  native.k.ReadProcessMemory.assert_not_called()
 def test_import_does_not_load_native_api(self):
  with mock.patch.object(probe.ctypes,'WinDLL',side_effect=AssertionError('native API on import')) as native:
   namespace=runpy.run_path(probe.__file__,run_name='offline_probe_import_test')
  self.assertTrue(callable(namespace['main']));native.assert_not_called()
 def test_help_does_not_load_native_api(self):
  with mock.patch.object(probe,'NativeAccess') as native,contextlib.redirect_stdout(io.StringIO()),self.assertRaises(SystemExit) as error:probe.main(['--help'])
  self.assertEqual(error.exception.code,0);native.assert_not_called()
 def test_duplicate_item_pointer_not_double_counted(self):
  self.memory.put(0x10040,struct.pack('<QQQ',0x20000,0x20020,0x20020));self.memory.put(0x20000,struct.pack('<QQQQ',0x30000,0,0x30000,0))
  with mock.patch.object(probe,'data',return_value={'sid':'1866-gamedata.base'}):
   self.memory.u32(0x3012c,1)
   with self.assertRaises(ValueError):probe.section_items(0x10000)
 def native_access_fake(self,path=None):
  native=probe.NativeAccess.__new__(probe.NativeAccess);native.k=mock.Mock()
  native.k.OpenProcess.side_effect=[111,222]
  path=str((probe.GAME/'Kenshi_x64.exe').resolve()) if path is None else path
  native.metadata=mock.Mock(return_value=(path,123456))
  native.modules=mock.Mock(return_value=[
   {'name':'Kenshi_x64.exe','path':path,'base':0x140000000,'size':0x3000000},
   {'name':'KenshiLib.dll','path':str(probe.GAME/'KenshiLib.dll'),'base':0x180000000,'size':0x100000}])
  return native
 def test_unrelated_pid_gets_metadata_only_no_vm_read(self):
  native=self.native_access_fake('C:/Windows/unrelated.exe')
  with self.assertRaises(ValueError):native.open_verified(123)
  native.k.OpenProcess.assert_called_once_with(0x1000,False,123)
  native.modules.assert_not_called();native.k.ReadProcessMemory.assert_not_called()
  native.k.CloseHandle.assert_called_once_with(111)
 def test_bad_hash_gets_metadata_only_no_vm_read(self):
  native=self.native_access_fake()
  with mock.patch.object(probe,'verify_executable',side_effect=ValueError('hash mismatch')),self.assertRaises(ValueError):native.open_verified(123)
  native.k.OpenProcess.assert_called_once_with(0x1000,False,123)
  native.modules.assert_not_called();native.k.ReadProcessMemory.assert_not_called()
 def test_verified_process_only_then_gets_read_handle(self):
  native=self.native_access_fake()
  with mock.patch.object(probe,'verify_executable',return_value={'sha256':'verified'}),mock.patch.object(probe,'sha256',return_value=probe.PROFILE_HASHES[probe.GAME/'KenshiLib.dll']):
   reader,_,_=native.open_verified(123)
  self.assertEqual(native.k.OpenProcess.call_args_list,[mock.call(0x1000,False,123),mock.call(0x410,False,123)])
  native.k.ReadProcessMemory.assert_not_called();reader.close()
  self.assertEqual(native.k.CloseHandle.call_args_list,[mock.call(111),mock.call(222)])
 def test_pid_reuse_rejected_before_memory_read(self):
  native=self.native_access_fake();path=native.metadata.return_value[0]
  native.metadata.side_effect=[(path,123456),(path,999999)]
  with mock.patch.object(probe,'verify_executable',return_value={'sha256':'verified'}),mock.patch.object(probe,'sha256',return_value=probe.PROFILE_HASHES[probe.GAME/'KenshiLib.dll']),self.assertRaises(ValueError):native.open_verified(123)
  native.k.ReadProcessMemory.assert_not_called()
  self.assertEqual(native.k.CloseHandle.call_args_list,[mock.call(222),mock.call(111)])

if __name__=='__main__':unittest.main()
