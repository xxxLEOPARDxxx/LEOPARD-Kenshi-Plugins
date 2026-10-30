"""Bounded read-only Kenshi Steam1.0.65 shop evidence. Import is offline-safe."""
from __future__ import annotations
import argparse,ctypes,ctypes.wintypes as wt,datetime,hashlib,json,math,os,pathlib,struct,sys,traceback
GAME=pathlib.Path(r"D:\SteamLibrary\steamapps\common\Kenshi")
EXE_HASHES={
 str(GAME/'Kenshi_x64.exe').casefold():'a596ab4e407c67b58599c54ffb32dc1bf2b64510cdebd3fa9359ef05a576aeb1',
 str(GAME/'RE_Kenshi'/'Kenshi_x64.exe').casefold():'504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e'}
PROFILE_HASHES={
 GAME/'RE_Kenshi'/'Kenshi_x64.exe':'504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e',
 GAME/'KenshiLib.dll':'5b6240622d06801dc39896e385efb834b8e1fe2fcdf86067d071ae7a04f6c83b',
 GAME/'RE_Kenshi'/'RVAs'/'Steam_1.0.65.br':'573c395997ed8d93dd837e27f89a440323089edd1ba93f68291631b448f9cbd0'}
TERMS=('торгов','трактир','бармен','barman','bartender','barkeeper','shinobi trader','merchant')
BUDGET=4*1024*1024
OWNERSHIP_BODY=bytes.fromhex('40534883ec3048c7442420feffffff488b41104883b85002000000741a488b050ceb9901488b80a0020000488b8080000000e9b80000004883b958060000007419488b8158060000488b4878488b01ff9098000000')
_reader=None
zone_handles=None

def read(a,n):
 if _reader is None:raise RuntimeError('No verified process reader')
 return _reader.read(a,n)

def merchant_name_matches(name,template=''):
 text=(name+' '+template).casefold()
 return any(term in text for term in TERMS)

def sha256(path):
 digest=hashlib.sha256()
 with pathlib.Path(path).open('rb') as stream:
  for block in iter(lambda:stream.read(1024*1024),b''):digest.update(block)
 return digest.hexdigest()

def verify_executable(path,hasher=sha256):
 path=pathlib.Path(path).resolve()
 expected=EXE_HASHES.get(str(path).casefold())
 if path.name.casefold()!='kenshi_x64.exe' or expected is None:raise ValueError('Refusing process outside the two audited game executable paths')
 actual=hasher(path)
 if actual!=expected:raise ValueError('Refusing unaudited executable SHA256: '+actual)
 return {'path':str(path),'sha256':actual}

def verify_profile():
 result={}
 for path,expected in PROFILE_HASHES.items():
  actual=sha256(path)
  if actual!=expected:raise ValueError('Audited native/SDK profile file changed: '+str(path))
  result[str(path)]=actual
 return result

class ModuleEntry(ctypes.Structure):
 _fields_=[('size',wt.DWORD),('id',wt.DWORD),('pid',wt.DWORD),('g',wt.DWORD),('p',wt.DWORD),('base',ctypes.c_void_p),('sz',wt.DWORD),('h',wt.HMODULE),('name',wt.WCHAR*256),('path',wt.WCHAR*260)]

class NativeAccess:
 def __init__(self):
  if os.name!='nt' or ctypes.sizeof(ctypes.c_void_p)!=8:raise RuntimeError('Windows64-bit Python required')
  k=self.k=ctypes.WinDLL('kernel32',use_last_error=True)
  k.OpenProcess.argtypes=[wt.DWORD,wt.BOOL,wt.DWORD];k.OpenProcess.restype=wt.HANDLE
  k.CloseHandle.argtypes=[wt.HANDLE];k.CloseHandle.restype=wt.BOOL
  k.QueryFullProcessImageNameW.argtypes=[wt.HANDLE,wt.DWORD,wt.LPWSTR,ctypes.POINTER(wt.DWORD)];k.QueryFullProcessImageNameW.restype=wt.BOOL
  k.GetProcessTimes.argtypes=[wt.HANDLE]+[ctypes.POINTER(wt.FILETIME)]*4;k.GetProcessTimes.restype=wt.BOOL
  k.CreateToolhelp32Snapshot.argtypes=[wt.DWORD,wt.DWORD];k.CreateToolhelp32Snapshot.restype=wt.HANDLE
  k.Module32FirstW.argtypes=[wt.HANDLE,ctypes.POINTER(ModuleEntry)];k.Module32FirstW.restype=wt.BOOL
  k.Module32NextW.argtypes=[wt.HANDLE,ctypes.POINTER(ModuleEntry)];k.Module32NextW.restype=wt.BOOL
  k.ReadProcessMemory.argtypes=[wt.HANDLE,ctypes.c_void_p,ctypes.c_void_p,ctypes.c_size_t,ctypes.POINTER(ctypes.c_size_t)];k.ReadProcessMemory.restype=wt.BOOL
 def metadata(self,handle):
  name=ctypes.create_unicode_buffer(32768);length=wt.DWORD(len(name))
  if not self.k.QueryFullProcessImageNameW(handle,0,name,ctypes.byref(length)):raise ctypes.WinError(ctypes.get_last_error())
  times=[wt.FILETIME() for _ in range(4)]
  if not self.k.GetProcessTimes(handle,*[ctypes.byref(t) for t in times]):raise ctypes.WinError(ctypes.get_last_error())
  return str(pathlib.Path(name.value).resolve()),(times[0].dwHighDateTime<<32)|times[0].dwLowDateTime
 def modules(self,pid):
  snapshot=self.k.CreateToolhelp32Snapshot(0x18,pid)
  if snapshot==ctypes.c_void_p(-1).value:raise ctypes.WinError(ctypes.get_last_error())
  try:
   m=ModuleEntry();m.size=ctypes.sizeof(m);modules=[]
   if not self.k.Module32FirstW(snapshot,ctypes.byref(m)):raise ctypes.WinError(ctypes.get_last_error())
   while True:
    if len(modules)>=1024:raise ValueError('module list bound')
    modules.append({'name':m.name,'path':m.path,'base':m.base,'size':m.sz})
    if not self.k.Module32NextW(snapshot,ctypes.byref(m)):
     if ctypes.get_last_error()!=18:raise ctypes.WinError(ctypes.get_last_error())
     break
   return modules
  finally:self.k.CloseHandle(snapshot)
 def open_verified(self,pid):
  # This handle can query metadata only; no VM_READ before path/hash validation.
  query=self.k.OpenProcess(0x1000,False,pid)
  if not query:raise ctypes.WinError(ctypes.get_last_error())
  try:
   path,created=self.metadata(query);identity=verify_executable(path)
   modules=self.modules(pid)
   if modules[0]['name'].casefold()!='kenshi_x64.exe' or str(pathlib.Path(modules[0]['path']).resolve()).casefold()!=path.casefold():raise ValueError('main module path/name mismatch')
   mods={}
   for m in modules:
    key=m['name'].casefold()
    if key in mods:raise ValueError('ambiguous duplicate module name')
    if not m['base'] or m['base']<65536 or m['size']<=0:raise ValueError('invalid module address/size')
    mods[key]=m
   kl=mods.get('kenshilib.dll')
   if not kl or pathlib.Path(kl['path']).resolve()!= (GAME/'KenshiLib.dll').resolve():raise ValueError('KenshiLib module path mismatch')
   if sha256(kl['path'])!=PROFILE_HASHES[GAME/'KenshiLib.dll']:raise ValueError('KenshiLib module hash mismatch')
   handle=self.k.OpenProcess(0x410,False,pid)
   if not handle:raise ctypes.WinError(ctypes.get_last_error())
   try:
    if self.metadata(handle)!=(path,created):raise ValueError('PID identity changed during verification')
    reader=Reader(self,handle)
    return reader,mods,dict(identity,created_filetime=created)
   except BaseException:self.k.CloseHandle(handle);raise
  finally:self.k.CloseHandle(query)

class Reader:
 def __init__(self,native,handle):self.native=native;self.handle=handle;self.total=0
 def read(self,a,n):
  if n<0 or n>262144 or self.total+n>BUDGET:raise ValueError('read budget')
  if not n:return b''
  if a<65536 or a+n>0x800000000000:raise ValueError('invalid bounded user address')
  self.total+=n;buf=ctypes.create_string_buffer(n);got=ctypes.c_size_t()
  if not self.native.k.ReadProcessMemory(self.handle,ctypes.c_void_p(a),buf,n,ctypes.byref(got)):raise ctypes.WinError(ctypes.get_last_error())
  if got.value!=n:raise OSError('short ReadProcessMemory')
  return buf.raw
 def close(self):
  if self.handle:self.native.k.CloseHandle(self.handle);self.handle=None

def q(a):return struct.unpack('<Q',read(a,8))[0]
def i(a):return struct.unpack('<I',read(a,4))[0]
def pos(a):return struct.unpack('<fff',read(a+0x48,12))
def s(a):
 raw=read(a,40);size,cap=struct.unpack_from('<QQ',raw,16)
 if size>2048 or cap>1048576 or size>cap:raise ValueError('invalid bounded std::string',hex(a),size,cap)
 ptr=struct.unpack_from('<Q',raw,0)[0] if cap>=16 else a
 return read(ptr,size).decode('utf-8') if size else ''

def lektor(a,limit=4096):
 raw=read(a,24);n,cap,p=struct.unpack_from('<IIQ',raw,8)
 if n>limit or n>cap or cap>10000000:raise ValueError('lektor',hex(a),n,cap,hex(p))
 return list(struct.unpack('<'+'Q'*n,read(p,n*8))) if n else []
def exps(p):
 raw=pathlib.Path(p).read_bytes();pe=struct.unpack_from('<I',raw,0x3c)[0];opt=pe+24;sec=opt+struct.unpack_from('<H',raw,pe+20)[0];sections=[struct.unpack_from('<8sIIII',raw,sec+40*j) for j in range(struct.unpack_from('<H',raw,pe+6)[0])]
 def off(v):
  for _,vs,va,rs,rp in sections:
   if va<=v<va+max(vs,rs):return rp+v-va
  return v
 er=off(struct.unpack_from('<I',raw,opt+112)[0]);_,_,_,_,_,_,nf,nn,funcs,names,ords=struct.unpack_from('<IIHHIIIIIII',raw,er);d={}
 for j in range(nn):
  z=off(struct.unpack_from('<I',raw,off(names)+4*j)[0]);name=raw[z:raw.index(b'\0',z)].decode('ascii');ord=struct.unpack_from('<H',raw,off(ords)+2*j)[0];d[name]=struct.unpack_from('<I',raw,off(funcs)+4*ord)[0]
 return d
def hand(a):return struct.unpack('<IIIII',read(a+8,20))
def signed(a):return struct.unpack('<i',read(a,4))[0]
def data(o):
 d=q(o+0x40)
 return {'sid':s(d+0x58),'template_name':s(d+0x28),'type':i(d+0x50)} if d else {}
def get_inventory(o):
 # Decode only verified simple public getter bodies. This is not execution.
 fn=q(q(o)+0x160);seen=set()
 for _ in range(5):
  if fn in seen:raise ValueError('getter jump cycle')
  seen.add(fn);b=read(fn,16)
  if b[0]==0xe9:fn=fn+5+struct.unpack_from('<i',b,1)[0];continue
  if b[:2]==b'\xff\x25':fn=q(fn+6+struct.unpack_from('<i',b,2)[0]);continue
  if b[:3]==b'\x48\x8b\x81' and b[7]==0xc3:
   offset=struct.unpack_from('<i',b,3)[0]
   if offset not in (0x430,0x2e8,0x290,0xc8):raise ValueError('unknown inventory member',hex(offset))
   return q(o+offset),{'function':hex(fn),'field':hex(offset),'code':b.hex()}
  if b[:3] in (b'\x33\xc0\xc3',b'\x31\xc0\xc3'):return 0,{'function':hex(fn),'returns_null':True,'code':b.hex()}
  raise ValueError('unsupported native inventory getter',hex(fn),b.hex())
 raise ValueError('getter jump limit')
def resolve_building(identity):
 typ,container,container_serial,index,serial=identity
 if typ!=0:return 0
 maximum=i(zone_handles+0x10)
 if maximum>1048576 or container>=maximum:raise ValueError('zone container index')
 dummy=q(q(zone_handles+8)+container*8)
 if not dummy:return 0
 if hand(dummy+0x58)[2]!=container_serial:raise ValueError('zone container serial mismatch')
 zone=q(dummy+0x78)
 if not zone:return 0
 handles=q(zone+0x98)
 if not handles:return 0
 maximum=i(handles+0x10)
 if maximum>1048576 or index>=maximum:raise ValueError('object index')
 obj=q(q(handles+8)+index*8)
 if not obj:return 0
 if hand(obj+0x58)!=identity:raise ValueError('object identity mismatch; no speculative handleShift')
 return obj
def interior_buildings(interior):
 # Native 54a220: owned hand map, then raw building lektor only if no result.
 count=q(interior+0x108);out=[]
 if count>2048:raise ValueError('interior map bound')
 if count:
  first=q(interior+0x100);buckets=q(interior+0x120)
  if first>1048576:raise ValueError('interior bucket bound')
  node=q(buckets+first*8);seen=set()
  while node:
   if node in seen or len(seen)>=count:raise ValueError('interior map cycle/count')
   seen.add(node);identity=hand(node+0x10)
   if identity[0]==0:
    building=resolve_building(identity)
    if building:out.append(building)
   node=q(node)
  if len(seen)!=count:raise ValueError('interior map count changed')
 fallback=not out
 if fallback:
  out=[b for b in lektor(interior+0x80,2048) if data(b).get('type')==0]
 return list(dict.fromkeys(out)),{'owned_map_count':count,'used_raw_fallback':fallback}
def section_items(section):
 raw=read(section+0x40,24);begin,end,capacity=struct.unpack('<QQQ',raw)
 if end<begin or capacity<end or (end-begin)%16 or (end-begin)//16>4096:raise ValueError('section vector invalid')
 count=(end-begin)//16;result=[];seen=set()
 entries=read(begin,count*16) if count else b''
 for j in range(count):
  item=struct.unpack_from('<Q',entries,j*16)[0]
  if not item:raise ValueError('null section item')
  if item in seen:raise ValueError('duplicate item pointer in canonical section')
  seen.add(item)
  entry=data(item)
  if not entry.get('sid'):raise ValueError('physical item data/SID unavailable')
  entry.update({'address':hex(item),'quantity':signed(item+0x12c)})
  if entry['quantity']<=0 or entry['quantity']>1000000:raise ValueError('bad physical item quantity')
  result.append(entry)
 return result
def ore_sum(items):
 total=sum(v['quantity'] for v in items if v.get('sid')=='1866-gamedata.base')
 if total<0 or total>2147483647:raise ValueError('ore count overflow')
 return total
def section_snapshot(section):
 parent=q(section+0xc8);items=section_items(section)
 return {'section':hex(section),'name':s(section+8),'parent_inventory':hex(parent),'enabled':bool(read(section+0xd0,1)[0]),'sid1866_units':ore_sum(items),'items':items}

def npc_ownership(c,p,ap):
 """Mirror only the verified non-player, existing-platoon native branch."""
 owner=q(c+0x10)
 if not owner or q(owner+0x250):raise ValueError('Character::getOwnerships player/unknown-owner branch unsupported')
 if q(c+0x658)!=ap or q(ap+0x78)!=p:raise ValueError('Character/platoon ownership relation changed')
 fn=q(q(p)+0x98);seen=set()
 for _ in range(5):
  if fn in seen:raise ValueError('ownership getter jump cycle')
  seen.add(fn);body=read(fn,16)
  if body[0]==0xe9:fn=fn+5+struct.unpack_from('<i',body,1)[0];continue
  if body[:2]==b'\xff\x25':fn=q(fn+6+struct.unpack_from('<i',body,2)[0]);continue
  if body[:8]==bytes.fromhex('488d8148010000c3'):return p+0x148
  raise ValueError('unsupported actual Platoon ownership getter',hex(fn),body.hex())
 raise ValueError('ownership getter jump bound')

def capture(report,mods):
 global zone_handles
 kl=mods['kenshilib.dll'];exports=exps(kl['path'])
 globals_now={n:q(kl['base']+rva) for n,rva in exports.items() if n.startswith('?ou@@') or n.startswith('?shou@@')}
 world=next(v for n,v in globals_now.items() if n.startswith('?ou@@'))
 shared=next(v for n,v in globals_now.items() if n.startswith('?shou@@'))
 base=mods['kenshi_x64.exe']['base']
 # Verify audited native code, in addition to the disk hash. Never execute it.
 body=read(base+0x791d70,24)
 if body!=bytes.fromhex('83790800750f488bd1488d0dd0119a01e9a7b88aff33c0c3'):raise ValueError('native hand::getBuilding code differs')
 if read(base+0x794b00,len(OWNERSHIP_BODY))!=OWNERSHIP_BODY:raise ValueError('native Character::getOwnerships code differs')
 zone_handles=base+0x791d70+16+struct.unpack_from('<i',body,12)[0]
 report['native_handle_resolver']={'locator_rva':'0x791d70','locator_code':body.hex(),'zone_container_global':hex(zone_handles),'zone_container_global_rva':hex(zone_handles-base),'full_identity_validation':True}
 factions=lektor(q(world+0x4a8),2048)
 towns=lektor(q(shared+8)+0x50,4096)
 hubs=[t for t in towns if data(t).get('sid')=='18919-Newwworld.mod']
 if len(hubs)!=1:raise ValueError('unique Hub town unavailable; no stock conclusion')
 hub=hubs[0];home=pos(hub);report['hub']={'address':hex(hub),'position':home}
 for f in factions:
  gd=q(f+0x240)
  if gd and s(gd+0x58)=='1-DynamicFactions.mod':report['df']={'address':hex(f),'money':signed(q(f+0x80)+0x88),'name':s(f+0x1a8)}
 seen_people=set();platoon_count=0;candidates=[]
 for f in factions:
  for p in lektor(f+0x208,2048):
   platoon_count+=1
   if platoon_count>4096:raise ValueError('platoon scan bound')
   ap=q(p+0x1d8)
   if not ap:continue
   for c in lektor(ap+0x50,256):
    if c in seen_people:continue
    seen_people.add(c)
    if len(seen_people)>16384:raise ValueError('actor scan bound')
    if math.dist(pos(c),home)>3000:continue
    name=s(c+0x18);gd=data(c)
    if gd.get('type')==1 and merchant_name_matches(name,gd.get('template_name','')):candidates.append((c,p,ap,f,name,gd))
 report['scan']={'factions':len(factions),'platoons':platoon_count,'unique_actors':len(seen_people),'radius_hub':3000,'merchant_name_candidates':len(candidates)}
 if not candidates:report['merchant_scope_note']='No matching loaded actors within Hub3000m; no stock conclusion.'
 for c,p,ap,f,name,gd in candidates:
  entry={'address':hex(c),'identity':hand(c+0x58),'name':name,'data':gd,'position':pos(c),'faction_name':s(f+0x1a8),'platoon':hex(p),'active_platoon':hex(ap),'physical_stock':[],'stock_status':'unknown','sid1866_in_canonical_stock':None,'stable_observation':False}
  report['merchants'].append(entry)
  try:
   ownership=npc_ownership(c,p,ap);entry['ownership']=hex(ownership)
   entry['ownership_proof']='Non-player Character.platoon658 -> me78 -> verified virtual98 lea+148 getter'
   entry['purse_before']=signed(ownership+0x88)
   identity=hand(ownership+0x38);entry['home_building_handle']=identity
   b=resolve_building(identity);entry['home_building']=hex(b)
   if not b:
    entry['unknown_reason']='Home unavailable: native backpack-stock fallback deliberately not inspected'
    entry['purse_after']=signed(ownership+0x88)
    continue
   entry['home_data']=data(b);interior=q(b+0x1f0);entry['interior']=hex(interior)
   roots=[]
   if interior:roots,entry['interior_enumeration']=interior_buildings(interior)
   seen_inventories=set();seen_stock_items=set()
   for root in roots:
    inv,proof=get_inventory(root)
    if not inv or inv in seen_inventories:continue
    seen_inventories.add(inv);sections=lektor(inv+0x68,128)
    if not sections:raise ValueError('native stock inventory has no first section')
    stock=section_snapshot(sections[0])
    if int(stock['parent_inventory'],16)!=inv:raise ValueError('canonical section parent mismatch')
    for item in stock['items']:
     if item['address'] in seen_stock_items:raise ValueError('same physical item appears in multiple stock sections')
     seen_stock_items.add(item['address'])
    stock.update({'building':hex(root),'building_data':data(root),'native_inventory_getter':proof})
    entry['physical_stock'].append(stock)
   # This count comes only after complete canonical enumeration. Unknown never becomes0.
   entry['sid1866_in_canonical_stock']=sum(v['sid1866_units'] for v in entry['physical_stock'])
   entry['purse_after']=signed(ownership+0x88)
   entry['canonical_ore_recheck']=[ore_sum(section_items(int(v['section'],16))) for v in entry['physical_stock']]
   identity_stable=hand(c+0x58)==tuple(entry['identity']) and npc_ownership(c,p,ap)==ownership and hand(ownership+0x38)==identity
   entry['stable_observation']=identity_stable and entry['purse_before']==entry['purse_after'] and entry['canonical_ore_recheck']==[v['sid1866_units'] for v in entry['physical_stock']]
   entry['stock_status']='observed' if entry['stable_observation'] else 'changed_during_observation'
  except Exception as exc:
   entry['stock_status']='unknown';entry['sid1866_in_canonical_stock']=None;entry['error']=str(exc)
 # A changed world makes the whole batch unsuitable for attributing observations.
 current={n:q(kl['base']+exports[n]) for n in globals_now}
 report['world_globals_stable']=current==globals_now
 if not report['world_globals_stable']:raise ValueError('world globals changed during sample')

def parse_args(argv=None):
 parser=argparse.ArgumentParser(description='Read-only bounded physical shop/purse snapshot for an explicitly supplied current Kenshi PID. No process discovery.')
 parser.add_argument('--pid',required=True,type=int,help='Verified current Kenshi process ID; no default and no process discovery')
 parser.add_argument('--output',required=True,type=pathlib.Path,help='New local JSON evidence file (existing files are refused)')
 args=parser.parse_args(argv)
 if not 0<args.pid<=0xffffffff:parser.error('--pid must be a positive DWORD')
 return args

def main(argv=None):
 global _reader
 args=parse_args(argv)
 if args.output.exists():raise SystemExit('Refusing to overwrite existing evidence: '+str(args.output))
 report={'pid':args.pid,'started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'access':'Metadata query0x1000, then verified QUERY_INFORMATION|VM_READ0x410','native_calls':False,'process_writes':False,'df':None,'errors':[],'merchants':[],'read_bytes':0,'complete':False}
 reader=None
 try:
  report['profile_files']=verify_profile()
  reader,mods,identity=NativeAccess().open_verified(args.pid);_reader=reader
  report['verified_process']=identity
  report['verified_modules']={n:mods[n] for n in ('kenshi_x64.exe','kenshilib.dll')}
  capture(report,mods)
  report['complete']=bool(report['merchants']) and not report['errors'] and all(v['stock_status']=='observed' for v in report['merchants'])
 except Exception as exc:
  report['errors'].append(str(exc));report['traceback']=traceback.format_exc()
 finally:
  if reader:report['read_bytes']=reader.total;reader.close()
  _reader=None;report['finished_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat()
  args.output.parent.mkdir(parents=True,exist_ok=True)
  with args.output.open('x',encoding='utf-8') as stream:json.dump(report,stream,ensure_ascii=True,indent=2)
 print(json.dumps({'artifact':str(args.output),'pid':args.pid,'complete':report['complete'],'errors':report['errors'],'df':report['df'],'scan':report.get('scan'),'merchants':[{k:v.get(k) for k in ('name','stock_status','sid1866_in_canonical_stock','purse_before','purse_after','stable_observation','unknown_reason','error')} for v in report['merchants']],'read_bytes':report['read_bytes']},ensure_ascii=True))
 return 0 if report['complete'] else 1

if __name__=='__main__':sys.exit(main())
