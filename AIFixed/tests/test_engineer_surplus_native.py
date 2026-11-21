from pathlib import Path
import sys,re,struct,hashlib,importlib.util,os,subprocess
sys.dont_write_bytecode=True;sys.path.insert(0,'D:/DEV/Kenshi/tools/_native_python');import pefile
here=Path(__file__).resolve().parent;s=(here.parent/'EngineerSurplusRuntime.h').read_text();p=Path('D:/SteamLibrary/steamapps/common/Kenshi/RE_Kenshi/Kenshi_x64.exe');d=p.read_bytes();pe=pefile.PE(data=d,fast_load=True)
assert hashlib.sha256(d).hexdigest()=='504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e'
pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_BASERELOC']])
relocations=[e.rva for b in getattr(pe,'DIRECTORY_ENTRY_BASERELOC',[]) for e in b.entries if e.type==10]
checks=1
windows={0:pe.get_data(0,pe.OPTIONAL_HEADER.SizeOfHeaders)}
for a,hexes,n in re.findall(r'(?:Bytes|SharedEntry)\(0x([0-9a-f]+),"((?:\\x[0-9a-f]{2})+)",(\d+)\)',s):
 expected=bytes.fromhex(hexes.replace('\\x',''));assert len(expected)==int(n)
 off=pe.get_offset_from_rva(int(a,16));assert d[off:off+len(expected)]==expected,hex(int(a,16));checks+=1
 windows[int(a,16)]=expected
for a,n,h in re.findall(r'Contract\(0x([0-9a-f]+),0x([0-9a-f]+),0x([0-9a-f]+)ULL\)',s):
 off=pe.get_offset_from_rva(int(a,16));raw=d[off:off+int(n,16)]
 assert not any(int(a,16)<=r<int(a,16)+int(n,16) for r in relocations),'relocated native body cannot use raw hash'
 def fnv(v):
  h=14695981039346656037
  for b in v:h=((h^b)*1099511628211)&((1<<64)-1)
  return h
 assert fnv(raw)==int(h,16);changed=bytearray(raw);changed[len(raw)//2]^=1;assert fnv(changed)!=int(h,16);checks+=2
 windows[int(a,16)]=raw
spec=importlib.util.spec_from_file_location('slots','D:/DEV/Kenshi/tools/slots.py');slots=importlib.util.module_from_spec(spec);spec.loader.exec_module(slots)
for name,at in [('?runGOAP@AITaskSytem',0x50f380),('?score@Tasker',0x60f2f0),('?_NV_removeItemDontDestroy_returnsItem@Inventory',0x749e10),('?_NV_addItem@InventorySection',0x74bd70),('?copyFrom@ActionsListMgr',0x50c850),('?setCurrentTask@AITaskSytem',0x50cab0),('?choosePermaJob@AITaskSytem',0x50db20)]:
 rows=slots.resolve(name);assert len(rows)==1 and rows[0][1]==at;checks+=1
assert 'normalJobScope!=ai->taskSystemAI' in s and 'from!=reinterpret_cast<ULONG_PTR>(image)+0x50fd28' in s;checks+=1
# RVA 0x15c10 in the audited LSE build (PE timestamp 0x6af675eb).
planner_wrapper=bytes.fromhex(
 '48895c2408574883ec300f297424204c8bd9bf010000004885d27456488b42704885c0744d'
 '4c8b154cd602004d85d2740e493bc2750981784426010000741a488b0d3ad602004885c974'
 '27483bc17522817844270100007519803dd2bd020000400fb6df740e8bc7f00fc105b8d602'
 '00eb0232db498bcbff1503d702000f28f084db7466803da7bd020000745df30f117424488b'
 '4424480f14f687050cd602000f5ac6ff158f6a010085c0740e0f2f3514bd0100b81f0000'
 '007705b820000000803d6bbd0200000f28c6741e4898488d0ddbd50200f00fc13c81488b'
 '5c24400f287424204883c4305fc3488b5c24400f287424204883c4305fc3')
# The same jobs hook in LSE's startup-batch build.
planner_batch_wrapper=bytes.fromhex(
 '48895c2408574883ec300f297424204c8bd9bf010000004885d27456488b42704885c0744d4c8b'
 '15dc7b02004d85d2740e493bc2750981784426010000741a488b0dca7b02004885c97427483bc1'
 '7522817844270100007519803d226a020000400fb6df740e8bc7f00fc105487c0200eb0232db49'
 '8bcbff15937c02000f28f084db7466803df769020000745df30f117424488b4424480f14f68705'
 '9c7b02000f5ac6ff153f3a010085c0740e0f2f3594840100b81f0000007705b820000000803dbb'
 '690200000f28c6741e4898488d0d6b7b0200f00fc13c81488b5c24400f287424204883c4305fc3'
 '488b5c24400f287424204883c4305fc3')
assert fnv(planner_batch_wrapper)==0x8fce3c6c38a760fd
# RVA 0x5cec0 in KEP 0.17.3 (PE timestamp 0x6a9acdf7), the furnace-extension
# hook. Its non-furnace path tail-jumps to the saved original with all arguments.
storage_wrapper=bytes.fromhex(
 '48895c2418488974242041544883ec30488b02488bd9488bca4d8be0488bf2ff90f002000083f80e'
 '0f85fe00000083be58040000000f84f1000000488b9e600400008b8e5804000048897c2448488d0c'
 'cb33ff483bd90f84b800000048896c2440488b1341b801000000488b4a18488b5210488b01ff5008'
 '84c074660f1f4000488b13498bcc488b5210ff15c0680400488b0b41b801000000488b4918488bd0'
 '488be84c8b0941ff511041b80100000084c07419488b13ffc7488b4a18488b5210488b01ff500884'
 'c075b5eb15498b042441b101488bd5498bccc644242000ff50108b9658040000488b8e6004000048'
 '83c308488d14d1483bda0f8561ffffff488b6c244085ff7e0b33d2488bceff15f46704008bc7488b'
 '7c2448488b5c2450488b7424584883c430415cc34d8bc4488bd6488bcb488b5c2450488b74245848'
 '83c430415c48ff2524c20800')
assert fnv(planner_wrapper)==0x953671ec41749d05
assert fnv(storage_wrapper)==0xdead890ede93144a
assert 'if(!PlannerEntry())return false;' in s, 'Existing LSE planner detour must be supported'
assert 'if(!Contract(0x50cab0,0x170,0x30cde7edcc348bb3ULL))return false;' in s
assert not re.search(r'if\(!Bytes\(0x50f380',s), 'An unconditional strict entry check blocks LSE'
assert '__declspec(thread) char storageList' not in s, 'Native storage query buffer must not outlive the search'
assert 'NativePointerList<Building> candidates;' in s, 'Storage search needs scoped ownership'
assert 'EngineerMaterialsRuntime::SelectedProject(tasks->body,tasks->character)' in s, 'Surplus gate must prefer the committed construction destination'
checks+=5

# Exercise the whole production admission path with native windows and both
# audited wrappers in owned buffers. No hooks or game code are executed.
out=here/'_generated/engineer_surplus'
out.mkdir(parents=True,exist_ok=True)
src=out/'surplus_admission.cpp'
production=s[s.index('    bool Bytes('):s.index('    void Init(')]
array=lambda data: ','.join(str(x) for x in data)
copy_windows=[]
for rva,data in sorted(windows.items()):
 copy_windows.append(f'static const unsigned char bytes_{rva:x}[]={{{array(data)}}};'
                     f'memcpy(game+{rva},bytes_{rva:x},sizeof(bytes_{rva:x}));')
for rva in (0x16be0f0,0x16be100):
 assert rva in relocations
 rebased=struct.unpack('<Q',pe.get_data(rva,8))[0]-pe.OPTIONAL_HEADER.ImageBase
 copy_windows.append(f'*reinterpret_cast<ULONG_PTR*>(game+{rva})='
                     f'reinterpret_cast<ULONG_PTR>(game+{rebased});')
src.write_text('''#include <Windows.h>
#include <cstring>
#include <cstdio>
#include <cwchar>
static unsigned char game[0x232c000], lse[0x16e00], kep[0x5d100];
static unsigned char* image=game;
static bool lseLoaded=true, kepLoaded=true;
static bool StartupGateClosed(){return false;}
static bool (*admit)()=&StartupGateClosed;
static bool (*epoch)()=&StartupGateClosed;
static bool (*allowed)()=&StartupGateClosed;
static HMODULE FixtureGame(const char*) { return reinterpret_cast<HMODULE>(game); }
static HMODULE FixturePlugin(const wchar_t* name) {
    if(!std::wcscmp(name,L"Loot-Scoot-Execute.dll")&&lseLoaded)return reinterpret_cast<HMODULE>(lse);
    if(!std::wcscmp(name,L"KenshiExtensionPlugin.dll")&&kepLoaded)return reinterpret_cast<HMODULE>(kep);
    return NULL;
}
struct Tasker { static unsigned score(){return 0x60f2f0;} };
struct AITaskSytem {
    static unsigned runGOAP(){return 0x50f380;}
    static unsigned setCurrentTask(){return 0x50cab0;}
    static unsigned choosePermaJob(){return 0x50db20;}
};
struct Inventory { static unsigned _NV_removeItemDontDestroy_returnsItem(){return 0x749e10;} };
struct InventorySection { static unsigned _NV_addItem(){return 0x74bd70;} };
namespace KenshiLib {
    intptr_t GetRealAddress(unsigned(*slot)()) { return reinterpret_cast<intptr_t>(game+slot()); }
}
#define GetModuleHandleA FixtureGame
#define GetModuleHandleW FixturePlugin
'''+production+'''
void HookEntry(unsigned char* entry, const unsigned char* target) {
    unsigned char* relay=entry-32;
    *entry=0xe9;
    LONG jump=-37;memcpy(entry+1,&jump,sizeof(jump));
    const unsigned char absolute[]={0xff,0x25,0,0,0,0};
    memcpy(relay,absolute,sizeof(absolute));
    memcpy(relay+6,&target,sizeof(target));
}
bool Check(const char* label,bool expected) {
    if(NativeAdmission()==expected)return true;
    std::printf("FAIL %s\\n",label);return false;
}
int main() {
    '''+'\n    '.join(copy_windows)+'''
    static const unsigned char planner[]={'''+array(planner_wrapper)+'''};
    static const unsigned char storage[]={'''+array(storage_wrapper)+'''};
    memcpy(lse+0x15c10,planner,sizeof(planner));
    static const unsigned char plannerBatch[]={'''+array(planner_batch_wrapper)+'''};
    memcpy(lse+0x16d00,plannerBatch,sizeof(plannerBatch));
    memcpy(kep+0x5cec0,storage,sizeof(storage));
    bool okay=Check("vanilla native contracts",true);
    game[0x60f2f0]=0xe9; // KEO's already-shared score entry.
    HookEntry(game+0x50f380,lse+0x15c10);
    okay=Check("LSE planner with shared score",true)&&okay;
    HookEntry(game+0x50f380,lse+0x16d00);
    okay=Check("LSE startup-batch planner",true)&&okay;
    lse[0x16d25]^=1;okay=Check("modified LSE startup-batch wrapper",false)&&okay;lse[0x16d25]^=1;
    HookEntry(game+0x50f380,lse+0x15c10);
    game[0x50f380+0x650]^=1;
    okay=Check("modified planner body",false)&&okay;
    game[0x50f380+0x650]^=1;
    game[0x50f380-30]=1;okay=Check("foreign planner relay encoding",false)&&okay;
    HookEntry(game+0x50f380,lse+0x15c11);okay=Check("foreign planner relay target",false)&&okay;
    HookEntry(game+0x50f380,lse+0x15c10);
    lse[0x15c25]^=1;okay=Check("modified LSE wrapper",false)&&okay;lse[0x15c25]^=1;
    lseLoaded=false;okay=Check("LSE module absent",false)&&okay;lseLoaded=true;
    unsigned char* entry=game+0x340eb0;
    HookEntry(entry,kep+0x5cec0);
    okay=Check("KEP furnace extension and LSE planner",true)&&okay;
    entry[0x201]^=1;okay=Check("modified storage body",false)&&okay;entry[0x201]^=1;
    entry[-30]=1;okay=Check("foreign storage relay encoding",false)&&okay;entry[-30]=0;
    HookEntry(entry,kep+0x5cec1);okay=Check("foreign storage relay target",false)&&okay;
    HookEntry(entry,kep+0x5cec0);
    kep[0x5cf4a]^=1;okay=Check("modified KEP wrapper",false)&&okay;kep[0x5cf4a]^=1;
    kepLoaded=false;okay=Check("KEP module absent",false)&&okay;kepLoaded=true;
    okay=Check("restored known chain",true)&&okay;
    memcpy(game+0x50f380,bytes_50f380,5);
    okay=Check("KEP furnace extension without LSE",true)&&okay;
    game[0x50cab0]=0xe9;
    okay=Check("unrecognized replacement of setCurrentTask",false)&&okay;
    return okay?0:1;
}
''',encoding='utf-8')
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
for opt in ('Od','O2'):
 binary=out/f'surplus_admission_{opt}.exe'
 subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT','/'+opt,str(src),
                 '/Fo'+str(out)+'/', '/Fe'+str(binary)],env=env,cwd=out,check=True)
 subprocess.run([str(binary)],check=True)
 checks+=16
print('PASS',checks,'native-window, export-table and KEP/LSE startup checks; no game execution')
