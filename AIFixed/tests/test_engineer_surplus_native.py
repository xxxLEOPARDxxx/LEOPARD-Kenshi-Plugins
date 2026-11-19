from pathlib import Path
import sys,re,struct,hashlib,importlib.util,os,subprocess
sys.dont_write_bytecode=True;sys.path.insert(0,'D:/DEV/Kenshi/tools/_native_python');import pefile
here=Path(__file__).resolve().parent;s=(here.parent/'EngineerSurplusRuntime.h').read_text();p=Path('D:/SteamLibrary/steamapps/common/Kenshi/RE_Kenshi/Kenshi_x64.exe');d=p.read_bytes();pe=pefile.PE(data=d,fast_load=True)
assert hashlib.sha256(d).hexdigest()=='504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e'
pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_BASERELOC']])
relocations=[e.rva for b in getattr(pe,'DIRECTORY_ENTRY_BASERELOC',[]) for e in b.entries if e.type==10]
checks=1
for a,hexes,n in re.findall(r'(?:Bytes|SharedEntry)\(0x([0-9a-f]+),"((?:\\x[0-9a-f]{2})+)",(\d+)\)',s):
 expected=bytes.fromhex(hexes.replace('\\x',''));assert len(expected)==int(n)
 off=pe.get_offset_from_rva(int(a,16));assert d[off:off+len(expected)]==expected,hex(int(a,16));checks+=1
for a,n,h in re.findall(r'Contract\(0x([0-9a-f]+),0x([0-9a-f]+),0x([0-9a-f]+)ULL\)',s):
 off=pe.get_offset_from_rva(int(a,16));raw=d[off:off+int(n,16)]
 assert not any(int(a,16)<=r<int(a,16)+int(n,16) for r in relocations),'relocated native body cannot use raw hash'
 def fnv(v):
  h=14695981039346656037
  for b in v:h=((h^b)*1099511628211)&((1<<64)-1)
  return h
 assert fnv(raw)==int(h,16);changed=bytearray(raw);changed[len(raw)//2]^=1;assert fnv(changed)!=int(h,16);checks+=2
spec=importlib.util.spec_from_file_location('slots','D:/DEV/Kenshi/tools/slots.py');slots=importlib.util.module_from_spec(spec);spec.loader.exec_module(slots)
for name,at in [('?runGOAP@AITaskSytem',0x50f380),('?score@Tasker',0x60f2f0),('?removeItemDontDestroy_returnsItem@Inventory',0x749e10),('?addItem@InventorySection',0x74bd70),('?copyFrom@ActionsListMgr',0x50c850),('?setCurrentTask@AITaskSytem',0x50cab0),('?choosePermaJob@AITaskSytem',0x50db20)]:
 rows=slots.resolve(name);assert len(rows)==1 and rows[0][1]==at;checks+=1
assert 'normalJobScope!=ai->taskSystemAI' in s and 'from!=reinterpret_cast<ULONG_PTR>(image)+0x50fd28' in s;checks+=1
planner=pe.get_data(0x50f380,0x6af)
# RVA 0x15c10 in the audited LSE build (PE timestamp 0x6af675eb).
wrapper=bytes.fromhex(
 '48895c2408574883ec300f297424204c8bd9bf010000004885d27456488b42704885c0744d'
 '4c8b154cd602004d85d2740e493bc2750981784426010000741a488b0d3ad602004885c974'
 '27483bc17522817844270100007519803dd2bd020000400fb6df740e8bc7f00fc105b8d602'
 '00eb0232db498bcbff1503d702000f28f084db7466803da7bd020000745df30f117424488b'
 '4424480f14f687050cd602000f5ac6ff158f6a010085c0740e0f2f3514bd0100b81f0000'
 '007705b820000000803d6bbd0200000f28c6741e4898488d0ddbd50200f00fc13c81488b'
 '5c24400f287424204883c4305fc3488b5c24400f287424204883c4305fc3')
assert fnv(planner[5:])==0xf27e769eb7fb5a5d
assert fnv(wrapper)==0x953671ec41749d05
assert 'if(!PlannerEntry())return false;' in s, 'Existing LSE planner detour must be supported'
assert 'if(!Contract(0x50cab0,0x170,0x30cde7edcc348bb3ULL))return false;' in s
assert not re.search(r'if\(!Bytes\(0x50f380',s), 'An unconditional strict entry check blocks LSE'
checks+=5

# Exercise the production admission code with the real native body and LSE
# wrapper copied into owned buffers. No hooks or game code are executed.
out=here/'_generated/engineer_surplus'
out.mkdir(parents=True,exist_ok=True)
src=out/'planner_admission.cpp'
production=s[s.index('    bool MatchesCode('):s.index('    bool NativeAdmission()')]
array=lambda data: ','.join(str(x) for x in data)
src.write_text('''#include <Windows.h>
#include <cstring>
static unsigned char game[0x510000], lse[0x16000];
static unsigned char* image=game;
static bool loaded=true;
static HMODULE FindLse(const wchar_t*) { return loaded?reinterpret_cast<HMODULE>(lse):NULL; }
#define GetModuleHandleW FindLse
'''+production+'''
int main() {
    const unsigned char body[]={'''+array(planner)+'''};
    const unsigned char wrapper[]={'''+array(wrapper)+'''};
    memcpy(game+0x50f380,body,sizeof(body));
    memcpy(lse+0x15c10,wrapper,sizeof(wrapper));
    if(!PlannerEntry())return 1;
    unsigned char* entry=game+0x50f380;
    unsigned char* relay=entry-32;
    *entry=0xe9;
    LONG jump=-37;memcpy(entry+1,&jump,sizeof(jump));
    relay[0]=0xff;relay[1]=0x25;
    ULONG_PTR target=reinterpret_cast<ULONG_PTR>(lse+0x15c10);
    memcpy(relay+6,&target,sizeof(target));
    if(!PlannerEntry())return 2;
    entry[0x650]^=1;if(PlannerEntry())return 3;entry[0x650]^=1;
    relay[2]=1;if(PlannerEntry())return 4;relay[2]=0;
    ++target;memcpy(relay+6,&target,sizeof(target));
    if(PlannerEntry())return 5;
    --target;memcpy(relay+6,&target,sizeof(target));
    lse[0x15c25]^=1;if(PlannerEntry())return 6;lse[0x15c25]^=1;
    loaded=false;if(PlannerEntry())return 7;loaded=true;
    if(!PlannerEntry())return 8;
    return 0;
}
''',encoding='utf-8')
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
for opt in ('Od','O2'):
 binary=out/f'planner_admission_{opt}.exe'
 subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT','/'+opt,str(src),
                 '/Fo'+str(out)+'/', '/Fe'+str(binary)],env=env,cwd=out,check=True)
 subprocess.run([str(binary)],check=True)
 checks+=8
print('PASS',checks,'actual Steam export/body/callsite checks and middle-body negative controls; no game execution')
