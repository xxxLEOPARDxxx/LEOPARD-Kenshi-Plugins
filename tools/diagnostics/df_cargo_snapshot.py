"""Explicit PID/start-time DF physical cargo snapshot. Import/--help are offline."""
import argparse,calendar,datetime,hashlib,json,math,pathlib,re,struct,sys,traceback
CANONICAL=pathlib.Path(r'D:\DEV\Kenshi\ours\tools\diagnostics\df_readonly_shop_probe.py')
CANONICAL_SHA='dcc8b2c71499f3287cd356e3834967e131abc65cc643f94ede367722e5d15c33'
if hashlib.sha256(CANONICAL.read_bytes()).hexdigest()!=CANONICAL_SHA:raise RuntimeError('Audited canonical reader changed')
sys.path.insert(0,str(CANONICAL.parent))
import df_readonly_shop_probe as p

CARGO_PROFILE_SHA='0170d3f7d1af9773380c648bdfd8229f0a82277cb7f6c32ef873595c864f0b76'
TARGET='1-DynamicFactions.mod'
MAX_FACTIONS=2048;MAX_PLATOONS=256;MAX_ACTORS=128;MAX_INVENTORIES_PER_ACTOR=16
MAX_ITEMS=4096;MAX_SECTIONS=128;MAX_TYPES=1024;MAX_QUANTITY=1000000

def utc_filetime(text):
 match=re.fullmatch(r'(\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d)(?:\.(\d{1,7}))?Z',text)
 if not match:raise argparse.ArgumentTypeError('Expected precise UTC YYYY-MM-DDTHH:MM:SS.fffffffZ')
 try:date=datetime.datetime.strptime(match[1],'%Y-%m-%dT%H:%M:%S')
 except ValueError as exc:raise argparse.ArgumentTypeError(str(exc))
 return (calendar.timegm(date.timetuple())+11644473600)*10000000+int((match[2] or '').ljust(7,'0'))

def arguments(argv=None):
 parser=argparse.ArgumentParser(description='Bounded physical cargo/treasury for faction prototype1-DynamicFactions.mod; no PID discovery, game calls or writes.')
 parser.add_argument('--pid',type=int,required=True)
 parser.add_argument('--expected-start-utc',type=utc_filetime,required=True)
 parser.add_argument('--phase',required=True)
 parser.add_argument('--output',type=pathlib.Path,required=True,help='NEW JSON evidence file')
 args=parser.parse_args(argv)
 if not 0<args.pid<=0xffffffff:parser.error('PID must be positive DWORD')
 if not re.fullmatch(r'[A-Za-z0-9_-]{1,48}',args.phase):parser.error('Invalid phase label')
 return args

def floats(raw,offsets):
 result={}
 for name,offset in offsets:
  value=struct.unpack_from('<f',raw,offset)[0]
  result[name]={'value':value if math.isfinite(value) else None,'bits':raw[offset:offset+4].hex(),'status':'observed' if math.isfinite(value) else 'unknown_nonfinite'}
 return result

class Snapshot:
 def __init__(self,modules):
  self.modules=modules;self.watches=[];self.types={};self.methods={};self.items_seen=set();self.actor_seen=set();self.item_count=0
 def watch(self,address,raw):self.watches.append((address,raw))
 def watched(self,address,size):
  raw=p.read(address,size);self.watch(address,raw);return raw
 def ptr(self,address):return struct.unpack('<Q',self.watched(address,8))[0]
 def lektor(self,address,limit):
  raw=self.watched(address,24);count,capacity,pointer=struct.unpack_from('<IIQ',raw,8)
  if count>limit or count>capacity or capacity>10000000:raise ValueError('borrowed lektor bounds',hex(address),count,capacity)
  if not count:return []
  data=self.watched(pointer,count*8);values=list(struct.unpack('<'+'Q'*count,data))
  if 0 in values or len(set(values))!=len(values):raise ValueError('null/duplicate borrowed-list pointer')
  return values
 def gamedata(self,address):
  if not address:raise ValueError('GameData unavailable')
  if address not in self.types:
   if len(self.types)>=MAX_TYPES:raise ValueError('GameData type bound')
   sid=p.s(address+0x58)
   if not sid:raise ValueError('GameData SID unavailable')
   self.types[address]={'pointer':hex(address),'sid':sid,'name':p.s(address+0x28),'type':p.i(address+0x50)}
  return dict(self.types[address])
 def getter(self,object_ptr,actor=False):
  function=p.q(p.q(object_ptr)+0x160)
  if function not in self.methods:
   main=self.modules['kenshi_x64.exe']
   current=function;seen=set();thunks=[]
   for depth in range(5): # Four bounded native E9 thunks plus the final getter.
    if not main['base']<=current<=main['base']+main['size']-8:raise ValueError('unsupported getInventory code module')
    if current in seen:raise ValueError('getInventory thunk cycle')
    seen.add(current);raw=p.read(current,8)
    if raw[0]!=0xe9:break
    thunks.append({'function':hex(current),'code':raw.hex()})
    current=current+5+struct.unpack_from('<i',raw,1)[0]
   else:raise ValueError('getInventory thunk depth bound')
   if raw==bytes.fromhex('488b81e8020000c3'):offset=0x2e8
   elif raw==bytes.fromhex('488b8190020000c3'):offset=0x290
   elif raw[:3] in (b'\x33\xc0\xc3',b'\x31\xc0\xc3'):offset=None
   else:raise ValueError('unknown actual getInventory method; no ABI guess',hex(function),raw.hex())
   self.methods[function]={'function':hex(function),'resolved_function':hex(current),'thunks':thunks,'code':raw.hex(),'offset':offset}
  method=self.methods[function];offset=method['offset']
  if actor and offset!=0x2e8:raise ValueError('Character inventory getter ABI mismatch')
  if not actor and offset not in (None,0x290):raise ValueError('Item inventory getter ABI mismatch')
  return (self.ptr(object_ptr+offset) if offset is not None else 0),dict(method)
 def placements(self,inventory):
  result={};sections=[]
  for section in self.lektor(inventory+0x68,MAX_SECTIONS):
   if self.ptr(section+0xc8)!=inventory:raise ValueError('section parent mismatch')
   raw=self.watched(section+0x40,24);begin,end,capacity=struct.unpack('<QQQ',raw)
   if end<begin or capacity<end or (end-begin)%16 or (end-begin)//16>MAX_ITEMS:raise ValueError('section vector bounds')
   entries=self.watched(begin,end-begin) if end>begin else b''
   name=p.s(section+8);sections.append({'pointer':hex(section),'name':name,'items':len(entries)//16})
   for offset in range(0,len(entries),16):
    item=struct.unpack_from('<Q',entries,offset)[0]
    if not item or item in result:raise ValueError('null/duplicate physical placement')
    x,y,w,h=struct.unpack_from('<HHHH',entries,offset+8)
    result[item]={'section':hex(section),'section_name':name,'x':x,'y':y,'width':w,'height':h}
  return result,sections
 def item(self,item,inventory,placement):
  if item in self.items_seen:raise ValueError('physical item appears twice; no duplicate counting')
  if self.item_count>=MAX_ITEMS:raise ValueError('total physical item bound')
  self.items_seen.add(item);self.item_count+=1
  owner=struct.unpack('<Q',self.watched(item+0x10,8))[0]
  data=struct.unpack('<Q',self.watched(item+0x40,8))[0]
  identity=struct.unpack('<IIIII',self.watched(item+0x60,20))
  raw=self.watched(item+0xc0,0xc0);quantity=struct.unpack_from('<i',raw,0x6c)[0]
  if not 0<quantity<=MAX_QUANTITY:raise ValueError('physical quantity bounds')
  width,height=struct.unpack_from('<ii',raw,0x70)
  if not 0<=width<=4096 or not 0<=height<=4096:raise ValueError('raw item dimension bounds')
  record={'pointer':hex(item),'handle':identity,'game_data':self.gamedata(data),'quantity':quantity,'inventory':hex(inventory),'placement':placement,'raw_width':width,'raw_height':height,'inventory_section_sdk':p.s(item+0xe8),'raw_owner_faction':hex(owner),'proper_owner_hand':struct.unpack_from('<IIIII',raw,0x88),'current_inventory_owner_hand':struct.unpack_from('<IIIII',raw,0xa8),'is_in_inventory':bool(raw[0x18]),'is_equipped':bool(raw[0x69]),'item_function':struct.unpack_from('<i',raw,0x64)[0],'charges':floats(raw,[('full',0x54),('left',0x58)]),'raw_c0_17f':raw.hex()}
  child,proof=self.getter(item);record['child_inventory']=hex(child) if child else None;record['child_inventory_method']=proof
  record['child_inventory_status']='observed_pointer' if child else 'observed_null_return'
  return record,child
 def actor(self,actor,faction,platoon,active):
  out={'pointer':hex(actor),'status':'unknown','cargo_status':'unknown','totals_by_sid':None,'inventories':[],'errors':[]}
  start=len(self.watches)
  try:
   if actor in self.actor_seen or len(self.actor_seen)>=MAX_ACTORS:raise ValueError('duplicate/global actor bound')
   self.actor_seen.add(actor)
   owner=self.ptr(actor+0x10);actual_active=self.ptr(actor+0x658)
   if owner!=faction or actual_active!=active or self.ptr(active+0x78)!=platoon:raise ValueError('actor membership mismatch')
   identity=struct.unpack('<IIIII',self.watched(actor+0x60,20))
   if identity[0]!=1:raise ValueError('non-Character active object')
   data=self.gamedata(self.ptr(actor+0x40))
   if data['type']!=1:raise ValueError('non-Character prototype')
   out.update({'name':p.s(actor+0x18),'handle':identity,'owner_faction':hex(owner),'prototype':data,'position_before':floats(p.read(actor+0x48,12),[('x',0),('y',4),('z',8)])})
   root,proof=self.getter(actor,actor=True);out['inventory_getter']=proof
   if not root:raise ValueError('loaded Character main inventory is unavailable')
   queue=[(root,actor,[])];seen=set();totals={}
   while queue:
    inventory,expected_owner,path=queue.pop(0)
    if inventory in seen or len(seen)>=MAX_INVENTORIES_PER_ACTOR:raise ValueError('inventory graph cycle/shared node/bound')
    seen.add(inventory);inv_owner=self.ptr(inventory+0x88)
    if inv_owner!=expected_owner:raise ValueError('inventory owner differs from actor/container')
    owner_handle=struct.unpack('<IIIII',self.watched(inv_owner+0x60,20))
    entry={'pointer':hex(inventory),'owner':hex(inv_owner),'owner_handle':owner_handle,'container_path':path,'items':[]};out['inventories'].append(entry)
    items=self.lektor(inventory+0x10,MAX_ITEMS);places,sections=self.placements(inventory);entry['sections']=sections
    borrowed=set(items)
    if borrowed-set(places):raise ValueError('borrowed allItems has no physical section placement')
    # Native allItems excludes worn gear. Physical sections remain authoritative,
    # including the worn backpack and the real inventory nested inside it.
    for item in places:
     record,child=self.item(item,inventory,places[item]);entry['items'].append(record)
     if item not in borrowed and not record['is_equipped']:raise ValueError('unlisted physical item is not equipped')
     if any(value['status']!='observed' for value in record['charges'].values()):raise ValueError('nonfinite charge scalar; raw bits retained, full cargo unknown')
     sid=record['game_data']['sid'];totals[sid]=totals.get(sid,0)+record['quantity']
     if child:queue.append((child,item,path+[{'container':hex(item),'handle':record['handle'],'sid':sid}]))
   self.recheck(start)
   out.update({'status':'observed','cargo_status':'observed','totals_by_sid':totals,'position_after':floats(p.read(actor+0x48,12),[('x',0),('y',4),('z',8)])})
  except Exception as exc:out['errors'].append(str(exc))
  return out
 def recheck(self,start=0):
  for address,expected in self.watches[start:]:
   if p.read(address,len(expected))!=expected:raise ValueError('observed cargo/roster/wallet changed',hex(address))
 def collect(self,report):
  kl=self.modules['kenshilib.dll'];exports=p.exps(kl['path'])
  slot=next(kl['base']+rva for name,rva in exports.items() if name.startswith('?ou@@'))
  world=self.ptr(slot);manager=self.ptr(world+0x4a8)
  matches=[]
  for faction in self.lektor(manager,MAX_FACTIONS):
   data=p.q(faction+0x240)
   if data and p.s(data+0x58)==TARGET:matches.append(faction)
  if len(matches)!=1:raise ValueError('unique target faction prototype unavailable',len(matches))
  faction=matches[0];data=self.ptr(faction+0x240);ownership=self.ptr(faction+0x80)
  if not ownership:raise ValueError('faction treasury ownership unavailable')
  money=struct.unpack('<i',self.watched(ownership+0x88,4))[0]
  report['faction']={'pointer':hex(faction),'prototype':self.gamedata(data),'name':p.s(faction+0x1a8),'ownership':hex(ownership),'treasury_before':money,'treasury_after':None,'treasury_status':'unknown'}
  loaded=self.lektor(faction+0x208,MAX_PLATOONS);unloaded=self.lektor(faction+0x220,MAX_PLATOONS)
  report['unloaded_platoons']=[{'pointer':hex(platoon),'handle':p.hand(platoon+0x58),'cargo_status':'unknown_not_loaded'} for platoon in unloaded]
  for platoon in loaded:
   active=self.ptr(platoon+0x1d8);identity=p.hand(platoon+0x58)
   if not active:
    report['unloaded_platoons'].append({'pointer':hex(platoon),'handle':identity,'cargo_status':'unknown_active_pointer_null'});continue
   if self.ptr(active+0x78)!=platoon:raise ValueError('ActivePlatoon.me mismatch')
   members=self.lektor(active+0x50,MAX_ACTORS)
   report['platoons'].append({'pointer':hex(platoon),'handle':identity,'active':hex(active),'loaded_actor_count':len(members)})
   for actor in members:report['actors'].append(self.actor(actor,faction,platoon,active))
  try:
   self.recheck();report['revalidation_status']='stable_observation'
   report['faction']['treasury_after']=p.signed(ownership+0x88)
   report['faction']['treasury_status']='observed' if report['faction']['treasury_after']==money else 'changed_during_observation'
  except Exception as exc:
   report['revalidation_status']='unknown_changed';report['errors'].append(str(exc))
   for actor in report['actors']:
    actor['status']='unknown_changed';actor['cargo_status']='unknown_changed';actor['totals_by_sid']=None
    actor['errors'].append('Final faction revalidation failed; earlier actor totals invalidated')
  good=report['revalidation_status']=='stable_observation' and report['faction']['treasury_status']=='observed' and bool(report['actors']) and not report['unloaded_platoons'] and all(a['cargo_status']=='observed' for a in report['actors'])
  if good:
   totals={}
   for actor in report['actors']:
    for sid,quantity in actor['totals_by_sid'].items():totals[sid]=totals.get(sid,0)+quantity
   report['all_member_totals_by_sid']=totals;report['cargo_status']='observed';report['completed']=True
  else:report['cargo_status']='unknown_or_partial'

def main(argv=None):
 args=arguments(argv)
 if args.output.exists():raise SystemExit('Refusing overwrite of existing phase JSON')
 report={'pid':args.pid,'phase':args.phase,'expected_created_filetime':args.expected_start_utc,'target_prototype_sid':TARGET,'started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'native_calls':False,'process_writes':False,'actors':[],'platoons':[],'unloaded_platoons':[],'all_member_totals_by_sid':None,'cargo_status':'unknown','completed':False,'errors':[],'read_bytes':0,'atomic_snapshot':False}
 reader=None
 try:
  profile_bytes=pathlib.Path(__file__).with_name('df-cargo-native-profile.json').read_bytes()
  if hashlib.sha256(profile_bytes).hexdigest()!=CARGO_PROFILE_SHA:raise ValueError('cargo ABI profile changed')
  profile=json.loads(profile_bytes)
  report['reader_profile']=p.verify_profile();reader,modules,identity=p.NativeAccess().open_verified(args.pid);report['verified_process']=identity
  if identity['created_filetime']!=args.expected_start_utc:raise ValueError('start time mismatch: STOP before RPM')
  p._reader=reader
  if profile['game_image_sha256']!='504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e':raise ValueError('unsupported cargo ABI profile')
  for rva,expected in profile['guards'].items():
   raw=bytes.fromhex(expected)
   if p.read(modules['kenshi_x64.exe']['base']+int(rva,16),len(raw))!=raw:raise ValueError('loaded cargo ABI guard differs:'+rva)
  Snapshot(modules).collect(report)
 except Exception as exc:report['errors'].append(str(exc));report['traceback']=traceback.format_exc()
 finally:
  if reader:report['read_bytes']=reader.total;reader.close()
  p._reader=None;report['finished_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat()
  payload=json.dumps(report,ensure_ascii=True,indent=2,allow_nan=False);args.output.parent.mkdir(parents=True,exist_ok=True)
  with args.output.open('x',encoding='utf-8') as stream:stream.write(payload)
 print(json.dumps({'artifact':str(args.output),'pid':args.pid,'phase':args.phase,'completed':report['completed'],'cargo_status':report['cargo_status'],'actors_observed':len(report['actors']),'unloaded_platoons':len(report['unloaded_platoons']),'faction':report.get('faction'),'errors':report['errors'],'read_bytes':report['read_bytes']},ensure_ascii=True))
 return 0 if report['completed'] else 1

if __name__=='__main__':sys.exit(main())
