"""Bounded READ-ONLY XP panel probe for one explicitly supplied Kenshi PID.

Never enumerates processes, injects code, invokes game functions or writes game
memory/files. The only output is a local JSON artifact. ABI offsets measured
by probe_layout.cpp with the production VC10/SDK headers. This version accepts
only audited XP candidates, including visual-nudged06b90cf, with matching maps.
"""
from pathlib import Path
import argparse, ctypes, ctypes.wintypes as wt, datetime, hashlib, json
import math, re, struct, sys, time

EXPECTED_SHA = 'd06b90cf19149f160e9fcef28dde56dc1bd5e856b40d9a7388f2606df50ae7e2'
APPROVED_SHAS = {EXPECTED_SHA,'1487b01b6194345ead383a1708565ce5b5c6e2669fc0c472fb1332ed6185ee43','df789027e693578087824a40f58f96dd50374d148aa0622f921052cd0e292cdf','872b5526f1e3fd074a21431414dc7b814d0afbbead05efb3dcbf553078db3c1b'}
BUILD = Path(r'D:\DEV\Kenshi\build\SkillLevelUpInfo')
NAMES = ['Strength','Toughness','Dexterity','Perception','Katanas','Sabres',
 'Hackers','Heavy weapons','Blunt','Polearms','Melee attack','Melee defence',
 'Dodge','Martial arts','Turrets','Crossbows','Precision shooting','Stealth',
 'Lockpicking','Thievery','Assassination','Athletics','Swimming','Medic',
 'Engineering','Robotics','Science','Weapon smith','Armour smith','Crossbow smith',
 'Labouring','Farming','Cooking','Hive medic','Vet','Mass combat','Survival']

def unpack(m, fmt, address):
 return struct.unpack(fmt, m.read(address, struct.calcsize(fmt)))
def q(m, a): return unpack(m, '<Q', a)[0]
def i(m, a): return unpack(m, '<i', a)[0]
def hand(m, a): return list(unpack(m, '<5I', a + 8))
def finite(n): return n if math.isfinite(n) else str(n)

def nodes(m, address, limit):
 """VC10 map object -> bounded nodes in native key order (not UI order)."""
 head, count = q(m, address + 8), q(m, address + 16)
 if count > limit: raise ValueError('map node budget exceeded: %s' % count)
 if not count: return []
 root = q(m, head + 8); stack=[]; seen=set(); result=[]
 while stack or root != head:
  while root != head:
   if root in seen or len(seen) >= limit: raise ValueError('map changed/cycle')
   seen.add(root); stack.append(root); root=q(m,root)
  root=stack.pop();result.append(root);root=q(m,root+16)
 if len(result) != count: raise ValueError('map changed while sampled')
 return result

def string(m, address, maximum=192):
 raw=m.read(address,32);size,capacity=struct.unpack_from('<QQ',raw,16)
 if size > maximum or capacity < size or capacity > 1048576:
  return {'unreadable_string_header': raw.hex()}
 if not size: return ''
 pointer=struct.unpack_from('<Q',raw)[0] if capacity >= 16 else address
 return m.read(pointer,size).decode('utf-8','replace')

def caption(m,address,maximum=256):
 """UString's public UTF16 code-point backing string, bounded VC10 layout."""
 raw=m.read(address,32);size,capacity=struct.unpack_from('<QQ',raw,16)
 if size>maximum or capacity<size or capacity>1048576:return {'unreadable_caption_header':raw.hex()}
 if not size:return ''
 pointer=struct.unpack_from('<Q',raw)[0] if capacity>=8 else address
 return m.read(pointer,size*2).decode('utf-16-le','replace')

def rendered_font(m,address):
 """Direct/client text only; native getter-verified EditText offsets, no calls."""
 chain=[];seen=set();current=address
 try:
  while current and len(chain)<4 and current not in seen:
   seen.add(current);chain.append(current);current=q(m,current+0x3d0)
  for current in reversed(chain):
   text=q(m,current+0x330)
   if not text:continue
   font=q(m,text+0xe0)
   if not font:continue
   height=i(m,text+0xf0)
   if not 0<height<4096:return {'unrecognized_text_height':height}
   return {'widget':hex(current),'text':hex(text),'font_resource':hex(font),'face':string(m,font+8),'caption':caption(m,text+0x70),
    'font_height':height,'region':list(unpack(m,'<4i',text+0x18)),
    'absolute':list(unpack(m,'<2i',text+0x28)),'text_view_size':list(unpack(m,'<2i',text+0x140)),
    'align':i(m,text+0xb4),'view_offset':list(unpack(m,'<2i',text+0x114)),
    'shadow':bool(m.read(text+0x112,1)[0]),'layout_profile':'native EditText/SimpleText getter offsets'}
  return None
 except (OSError,ValueError) as error:return {'font_read_error':str(error)}

def widget(m,address):
 if not address: return None
 coord=list(unpack(m,'<4i',address+0x20));parent=q(m,address+0x450)
 return {'address':hex(address),'coord':coord,'bottom':coord[1]+coord[3],
         'parent':hex(parent),'visible':m.read(address+0x46c,1)[0] != 0,'rendered_font':rendered_font(m,address)}

def skin_children(m,address):
 """Own progress skin only: native getSkinWidgetsByName verified vector3f8."""
 begin,end,capacity=unpack(m,'<3Q',address+0x3f8)
 if end<begin or capacity<end or (end-begin)%8 or (end-begin)//8>8:
  raise ValueError('owned progress skin child budget/header exceeded')
 result=[]
 for offset in range(0,end-begin,8):
  child=q(m,begin+offset);item=widget(m,child)
  if item:item['name']=string(m,child+0x428);result.append(item)
 return result

def panel(m,address):
 result={'address':hex(address),'root':widget(m,q(m,address+8)),
         'scrollWin':widget(m,q(m,address+0x50)),'categories':[]}
 parents={}
 for outer in nodes(m,address+0x60,24):
  category=i(m,outer+0x18);rows=[]
  for inner in nodes(m,outer+0x20,128):
   line=q(m,inner+0x40)
   row={'key':string(m,line+0x28),'address':hex(line),'type':i(m,line+0x118),
        'w1':widget(m,q(m,line+8)),'w2':widget(m,q(m,line+16))}
   if row['type']==11:
    row['textBox']=widget(m,q(m,line+0x120))
    row['progress']=widget(m,q(m,line+0x128))
    row['progressText']=widget(m,q(m,line+0x130))
    if isinstance(row['key'],str) and row['key'].startswith('skill_xp_info_slot_'):
     try:row['progress_skin_children']=skin_children(m,q(m,line+0x128))
     except (OSError,ValueError) as error:row['progress_skin_error']=str(error)
   for w in [row['w1'],row['w2']]:
    if w and int(w['parent'],16): parents[w['parent']]=widget(m,int(w['parent'],16))
   rows.append(row)
  result['categories'].append({'category':category,'rows':rows})
 result['parents']=parents
 return result

def binding(m,map_address,reference):
 """Pointer-key map lookup only: never walk every player's indexed stats."""
 if not reference:return None
 head=q(m,map_address+8);node=q(m,head+8);seen=set()
 for _ in range(64):
  if node==head:return None
  if node in seen:raise ValueError('binding map changed/cycle')
  seen.add(node);key=q(m,node+24)
  if key==reference:
   skill,owner,index,token=unpack(m,'<4Q',node+32)
   return {'skill':hex(skill),'owner':hex(owner),'statIndex':index,'token':token}
  node=q(m,node+(0 if reference<key else 16))
 raise ValueError('binding depth budget exceeded')

def selected_state(m,registry,owner,now,recent,index_map):
 entries=nodes(m,registry,512)
 result={'registry_count':len(entries),'selected_owner':owner,'found':False}
 for node in entries:
  if hand(m,node+24)!=owner:continue
  # Node._Myval at24, pair<hand,Snapshot>.second at32 => Snapshot at56.
  snapshot=node+56;raw=m.read(snapshot,1344);skills=[]
  for index,name in enumerate(NAMES):
   value,pending,first,last,live,active,levels,emitted=struct.unpack_from('<ffIIBB2xii',raw,index*28)
   age=(now-last)&0xffffffff
   skill={'index':index,'name':name,'value':finite(value),'pending':finite(pending),
          'first':first,'last':last,'age_ms':age,'live':bool(live),'active':bool(active),
          'recent':bool(active and age<=recent),'levels':levels,'emittedLevels':emitted}
   if active or index in (2,13,21):
    reference=struct.unpack_from('<Q',raw,1040+index*8)[0];skill['reference']=hex(reference)
    if index in (2,13,21):
     skill['raw_reference_value']=finite(unpack(m,'<f',reference)[0]) if reference else None
     b=binding(m,index_map,reference);skill['binding']=b
     skill['binding_matches']=bool(b and int(b['owner'],16)==snapshot and
       int(b['skill'],16)==snapshot+index*28 and b['statIndex']==index and b['token'])
    skills.append(skill)
  result.update({'found':True,'snapshot':hex(snapshot),'live':bool(raw[1336]),
                 'pending':bool(raw[1337]),'skills':skills})
  break
 return result

def map_symbols(path,dll):
 text=path.read_text(encoding='ascii');raw=dll.read_bytes()
 if hashlib.sha256(raw).hexdigest() not in APPROVED_SHAS:raise ValueError('candidate DLL SHA is not an audited XP candidate')
 pe=struct.unpack_from('<I',raw,0x3c)[0];timestamp=struct.unpack_from('<I',raw,pe+8)[0]
 match=re.search(r'Timestamp is ([0-9a-fA-F]+)',text)
 if not match or int(match[1],16)!=timestamp:raise ValueError('MAP timestamp does not match DLL')
 imagebase=int(re.search(r'Preferred load address is ([0-9a-fA-F]+)',text)[1],16)
 found={}
 for line in text.splitlines():
  match=re.match(r'\s*[0-9a-fA-F]+:[0-9a-fA-F]+\s+\?(g_\w+)@\S+\s+([0-9a-fA-F]{16})\b',line)
  if match:found[match[1]]=int(match[2],16)-imagebase
 required=['g_enabled','g_bars','g_maxRows','g_recent','g_loadGeneration','g_seenGeneration',
  'g_panel','g_panelRoot','g_panelCategory','g_panelCharacter','g_seen','g_index','g_slots',
  'g_rowSkill','g_rowValue','g_diagRows','g_diagBottom','g_diagWidth','g_diagParentHeight',
  'g_diagValueRight','g_diagCapacity','g_diagPanelStatus']
 if set(required)-set(found):raise ValueError('MAP symbols missing: '+str(set(required)-set(found)))
 return found,timestamp

def loaded_header(header,disk,base,module_size):
 """Allow only the observed loader ImageBase rewrite to this module's base."""
 pe=struct.unpack_from('<I',disk,0x3c)[0];opt=pe+24;offset=opt+24
 if disk[pe:pe+4]!=b'PE\0\0' or header[pe:pe+4]!=b'PE\0\0':raise ValueError('invalid PE header')
 preferred=struct.unpack_from('<Q',disk,offset)[0];actual=struct.unpack_from('<Q',header,offset)[0]
 size=struct.unpack_from('<I',header,opt+56)[0]
 if size!=module_size:raise ValueError('loaded image size differs from module inventory')
 if actual not in (base,preferred):raise ValueError('loaded ImageBase is neither actual nor preferred module base')
 normalized=bytearray(header);normalized[offset:offset+8]=disk[offset:offset+8]
 if normalized!=disk[:len(header)]:raise ValueError('loaded header differs outside verified ImageBase field')
 return {'imagebase_field_offset':hex(offset),'preferred_base':hex(preferred),'loaded_imagebase':hex(actual),
  'module_base':hex(base),'image_size':size,'changed_offsets':[hex(j) for j in range(len(header)) if header[j]!=disk[j]],
  'header_bytes_verified':len(header)}

def sample(m,base,symbols,now):
 a={name:base+rva for name,rva in symbols.items()};owner=hand(m,a['g_panelCharacter'])
 native=q(m,a['g_panel']);category=i(m,a['g_panelCategory'])
 generation=[i(m,a['g_loadGeneration']),i(m,a['g_seenGeneration'])]
 recent=i(m,a['g_recent']);slots=list(unpack(m,'<6i',a['g_slots']))
 status=q(m,a['g_diagPanelStatus'])
 result={'tick':now,'owner':owner,'panel':hex(native),'category':category,
  'panelRoot':widget(m,q(m,a['g_panelRoot']+8)),
  'enabled':bool(m.read(a['g_enabled'],1)[0]),'bars':bool(m.read(a['g_bars'],1)[0]),
  'maxRows':i(m,a['g_maxRows']),'recent_ms':recent,'generation':generation,
  'slots':{'ids':slots[:5],'count':slots[5]},
  'rowSkill':list(unpack(m,'<5i',a['g_rowSkill'])),
  'rowValue':[finite(v) for v in unpack(m,'<5f',a['g_rowValue'])],
  'diagnostics':{name:i(m,a[name]) for name in ['g_diagRows','g_diagBottom','g_diagWidth',
    'g_diagParentHeight','g_diagValueRight','g_diagCapacity']}}
 if 'g_divider' in a:result['divider']=widget(m,q(m,a['g_divider']+8))
 if 'g_nativeProfile' in a:result['font_profile_verified']=bool(m.read(a['g_nativeProfile'],1)[0])
 for key in ('g_nameMeasure','g_valueMeasure'):
  if key in a:
   try:
    result[key]={'source':hex(q(m,a[key]+8)),'rendered_source':hex(q(m,a[key]+32)),
     'text':hex(q(m,a[key]+48)),'font_resource':hex(q(m,a[key]+56)),'caption':caption(m,a[key]+64),
     'face':string(m,a[key]+128),'fontHeight_width_height_align_textHeight':list(unpack(m,'<5i',a[key]+168)),
     'shadow':bool(m.read(a[key]+188,1)[0]),'word_wrap':bool(m.read(a[key]+189,1)[0]),'rendered_word_wrap':bool(m.read(a[key]+190,1)[0])}
   except (OSError,ValueError) as error:result[key]={'cache_read_error':str(error)}
 if status:
  result['diagnostics']['status']=m.read(status,96).split(b'\0',1)[0].decode('ascii','replace')
 result['selected_state']=selected_state(m,a['g_seen'],owner,now,recent,a['g_index'])
 if native:result['native_panel']=panel(m,native)
 result['consistent_identity']=(native==q(m,a['g_panel']) and category==i(m,a['g_panelCategory']) and
  owner==hand(m,a['g_panelCharacter']) and generation==[i(m,a['g_loadGeneration']),i(m,a['g_seenGeneration'])])
 return result

class ProcessMemory:
 def __init__(self,k,handle):self.k=k;self.handle=handle;self.total=0;self.sample_bytes=0
 def read(self,address,size):
  if not 65536<address<0x7fffffffffff or not 0<size<=8192:raise ValueError('invalid read range')
  if self.total+size>8*1024*1024 or self.sample_bytes+size>256*1024:raise ValueError('read budget exhausted')
  self.total+=size;self.sample_bytes+=size;buffer=ctypes.create_string_buffer(size);got=ctypes.c_size_t()
  if not self.k.ReadProcessMemory(self.handle,ctypes.c_void_p(address),buffer,size,ctypes.byref(got)) or got.value!=size:
   raise OSError('ReadProcessMemory unavailable at '+hex(address))
  return buffer.raw

def kernel():
 k=ctypes.WinDLL('kernel32',use_last_error=True)
 k.OpenProcess.argtypes=[wt.DWORD,wt.BOOL,wt.DWORD];k.OpenProcess.restype=wt.HANDLE
 k.CloseHandle.argtypes=[wt.HANDLE]
 k.ReadProcessMemory.argtypes=[wt.HANDLE,ctypes.c_void_p,ctypes.c_void_p,ctypes.c_size_t,ctypes.POINTER(ctypes.c_size_t)]
 k.CreateToolhelp32Snapshot.argtypes=[wt.DWORD,wt.DWORD];k.CreateToolhelp32Snapshot.restype=wt.HANDLE
 k.GetTickCount.restype=wt.DWORD
 return k

def module(k,pid):
 class Entry(ctypes.Structure):
  _fields_=[('size',wt.DWORD),('id',wt.DWORD),('pid',wt.DWORD),('g',wt.DWORD),('p',wt.DWORD),
   ('base',ctypes.c_void_p),('bytes',wt.DWORD),('h',wt.HMODULE),('name',wt.WCHAR*256),('path',wt.WCHAR*260)]
 k.Module32FirstW.argtypes=[wt.HANDLE,ctypes.POINTER(Entry)]
 k.Module32NextW.argtypes=[wt.HANDLE,ctypes.POINTER(Entry)]
 handle=k.CreateToolhelp32Snapshot(0x18,pid)
 if handle==ctypes.c_void_p(-1).value:raise OSError('specified PID unavailable')
 try:
  entry=Entry();entry.size=ctypes.sizeof(entry);ok=k.Module32FirstW(handle,ctypes.byref(entry))
  while ok:
   if entry.name.lower()=='skilllevelupinfo.dll':return {'base':entry.base,'bytes':entry.bytes,'path':entry.path}
   ok=k.Module32NextW(handle,ctypes.byref(entry))
 finally:k.CloseHandle(handle)
 raise OSError('specified PID or its XP module unavailable; no other PID searched')

def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--pid',type=int,required=True)
 parser.add_argument('--seconds',type=float,default=10)
 parser.add_argument('--interval',type=float,default=.25)
 parser.add_argument('--dll',type=Path,default=BUILD/'SkillLevelUpInfo.dll')
 parser.add_argument('--map',type=Path,default=BUILD/'SkillLevelUpInfo.map')
 parser.add_argument('--out',type=Path)
 args=parser.parse_args()
 if not 0<args.pid<=0xffffffff or not 0<args.seconds<=30 or not .1<=args.interval<=2:parser.error('PID in[1,2^32-1], seconds(0,30], interval[.1,2] required')
 symbols,timestamp=map_symbols(args.map,args.dll);candidate_sha=hashlib.sha256(args.dll.read_bytes()).hexdigest();k=kernel()
 report={'pid':args.pid,'expected_sha256':candidate_sha,'map':str(args.map),'map_timestamp':timestamp,
  'utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'samples':[],
  'limitations':'Read-only polling can miss panel/category switches between samples; no atomic game lock is taken.'}
 handle=None;memory=None;seen=set()
 try:
  loaded=module(k,args.pid);report['module']=loaded
  if hashlib.sha256(Path(loaded['path']).read_bytes()).hexdigest()!=candidate_sha:raise ValueError('loaded module disk SHA differs from supplied audited candidate')
  handle=k.OpenProcess(0x410,False,args.pid)
  if not handle:raise OSError('specified PID unavailable; no other PID searched')
  memory=ProcessMemory(k,handle);header=memory.read(loaded['base'],1024);disk=args.dll.read_bytes()
  report['loaded_header_verification']=loaded_header(header,disk,loaded['base'],loaded['bytes'])
  deadline=time.monotonic()+args.seconds
  while time.monotonic()<deadline and len(report['samples'])<120:
   memory.sample_bytes=0
   try:
    row=sample(memory,loaded['base'],symbols,k.GetTickCount());report['samples'].append(row)
    seen.add((row['panel'],row['category'],tuple(row['owner'])))
   except ValueError as e:
    report['samples'].append({'tick':k.GetTickCount(),'sampling_error':str(e)})
    if 'budget' in str(e):break
   time.sleep(min(args.interval,max(0,deadline-time.monotonic())))
 except (OSError,ValueError) as e:report['stopped']=str(e)
 finally:
  if handle:k.CloseHandle(handle)
 report['read_bytes']=memory.total if memory else 0
 report['distinct_panel_category_owners']=[{'panel':p,'category':c,'owner':list(o)} for p,c,o in sorted(seen)]
 output=args.out or Path(__file__).parent/'_generated'/('runtime-%s-%s.json'%(args.pid,datetime.datetime.now().strftime('%H%M%S')))
 output.parent.mkdir(parents=True,exist_ok=True);output.write_text(json.dumps(report,indent=2,ensure_ascii=False),encoding='utf-8')
 successful=sum('sampling_error' not in row for row in report['samples'])
 print(json.dumps({'output':str(output),'samples':len(report['samples']),'successful_samples':successful,'read_bytes':report['read_bytes'],
  'distinct_panel_category_owners':len(seen),'stopped':report.get('stopped')},ensure_ascii=False))
 return 0 if successful else 2

if __name__=='__main__':
 try:sys.exit(main())
 except (OSError,ValueError) as error:print(str(error),file=sys.stderr);sys.exit(2)
