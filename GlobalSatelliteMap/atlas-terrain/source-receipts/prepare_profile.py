"""Authenticate installed code/SDK and generate private read-only profile pins."""
from pathlib import Path
import sys,hashlib,json,struct
sys.dont_write_bytecode=True;sys.path.insert(0,'D:/DEV/Kenshi/ours/SkillLevelUpInfo/tests/_tools')
import pefile
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
for name,want in json.loads((HERE/'sdk-pins.json').read_text()).items():assert hashlib.sha256(Path(name).read_bytes()).hexdigest()==want,name
DLL=Path('D:/SteamLibrary/steamapps/common/Kenshi/OgreMain_x64.dll');data=DLL.read_bytes();digest=hashlib.sha256(data).hexdigest()
assert digest=='7d3888d21398647519f10a89b1f300d8606f761ad159e6021a797b1b9b3df5f6'
pe=pefile.PE(data=data);exports={s.name.decode():s.address for s in pe.DIRECTORY_ENTRY_EXPORT.symbols if s.name}
windows={}
for rva in [0x27f160,0x282720,0x2869a0,0xd1bb0,0xd1320,0xd2d30,0xce60,0xcd20,0xae00,0xcc90,0x28b930,0x86b0,0x1a1e60]:
 f=next(f.struct for f in pe.DIRECTORY_ENTRY_EXCEPTION if f.struct.BeginAddress<=rva<f.struct.EndAddress);assert f.BeginAddress==rva
 blob=pe.get_data(rva,f.EndAddress-rva);assert len(blob)<=4096;windows[rva]=blob
for rva,count in [(0x286990,8),(0x27b6c0,8),(0x393f0,5),(0x393e0,5),(0x667c0,8)]:windows[rva]=pe.get_data(rva,count)
for block in getattr(pe,'DIRECTORY_ENTRY_BASERELOC',[]):
 for entry in block.entries:
  assert not entry.type or all(not(r<=entry.rva<r+len(b))for r,b in windows.items()),'profile code window contains ASLR relocation'
arr=['#pragma once','namespace SourceReceipts {','struct CodePin{unsigned rva,bytes;BYTE sha[32];};','static const CodePin NativeCodePins[]={']
for rva,b in windows.items():arr+=['{'+str(rva)+','+str(len(b))+',{'+','.join(str(v)for v in hashlib.sha256(b).digest())+'}},']
arr+=['};',f'static const DWORD NativeImageSize={pe.OPTIONAL_HEADER.SizeOfImage};',f'static const DWORD NativeTimestamp={pe.FILE_HEADER.TimeDateStamp};']
for key,prefix in [('GroupExport','?getResourceGroup@ResourceGroupManager'),('SingletonExport','?getSingletonPtr@ResourceGroupManager'),('ListenerExport','?getLoadingListener@ResourceGroupManager')]:
 name=next(n for n in exports if n.startswith(prefix));arr+=['static const char '+key+'[]="'+name+'";']
vptr=exports['??_7FileSystemArchive@Ogre@@6B@'];assert vptr==0x682dc8;targets=[x-pe.OPTIONAL_HEADER.ImageBase for x in struct.unpack('<14Q',pe.get_data(vptr,112))]
arr+=['static const unsigned NativeFsVtable[14]={'+','.join(str(v)for v in targets)+'};','}']
(OUT/'NativePins.h').write_text('\n'.join(arr)+'\n')
sdk=Path('D:/DEV/Kenshi/KenshiLib-src/Include/ogre');boost=Path('D:/DEV/Kenshi/deps/boost_1_60_0/boost/thread/win32')
inputs={str(DLL):digest}
for p in [sdk/n for n in ['OgreResourceGroupManager.h','OgreArchive.h','OgreResource.h','OgreSharedPtr.h','OgrePrerequisites.h','OgreBuildSettings.h','Threading/OgreThreadDefinesBoost.h']]+[boost/n for n in ['recursive_mutex.hpp','basic_recursive_mutex.hpp','basic_timed_mutex.hpp']]:inputs[str(p)]=hashlib.sha256(p.read_bytes()).hexdigest()
(OUT/'profile-contract.json').write_text(json.dumps({'inputPins':inputs,'codeWindows':{hex(r):{'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()} for r,b in windows.items()},'vtableRva':hex(vptr),'vtableEntryRvas':[hex(r)for r in targets],'singletonSlotRva':hex(0x27b6c7+struct.unpack_from('<i',pe.get_data(0x27b6c0,8),3)[0]),'lockABI':{'size':24,'recursion':0,'thread':4,'basic':8,'active':8,'event':16,'lockBit':31,'eventBit':30,'MainAcquire':'SDK try_lock only, no waiting'}},indent=2)+'\n')
print('PASS exact installed module/SDK pins,18 code windows and14 FS vtable slots; no native execution')
