"""Exercise actual module/window validator on owned sparse image, no game call."""
from pathlib import Path
import os,subprocess
base=Path(__file__).resolve().parents[1]
out=base/'build/origin-profile-review';out.mkdir(exist_ok=True)
text=(base/'GsmNativeLight.h').read_text();a=text.index('inline bool Module(');b=text.index('inline bool Engine(',a);body=text[a:b]
code=r'''
#include <Windows.h>
#include <cstring>
#include <cstdio>
struct GsmCodeWindow{DWORD rva;BYTE code[24];};
namespace NativeLocalLights{struct FunctionPin{DWORD rva;const char*name;BYTE bytes[24];};}
#include "PROFILE"
namespace GsmNative{BODY}
static unsigned checks=0;static void C(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}}
int main(){try{
 const DWORD size=0x232c000;SYSTEM_INFO si;GetSystemInfo(&si);SIZE_T page=si.dwPageSize;
 BYTE*p=static_cast<BYTE*>(VirtualAlloc(NULL,size,MEM_RESERVE,PAGE_NOACCESS));C(p!=NULL,"owned image reservation");
 C(VirtualAlloc(p,page,MEM_COMMIT,PAGE_READWRITE)!=NULL,"owned header page");
 IMAGE_DOS_HEADER*d=reinterpret_cast<IMAGE_DOS_HEADER*>(p);d->e_magic=IMAGE_DOS_SIGNATURE;d->e_lfanew=0x80;
 IMAGE_NT_HEADERS64*n=reinterpret_cast<IMAGE_NT_HEADERS64*>(p+0x80);n->Signature=IMAGE_NT_SIGNATURE;n->FileHeader.Machine=IMAGE_FILE_MACHINE_AMD64;n->FileHeader.TimeDateStamp=0x65d604d7;n->OptionalHeader.SizeOfImage=size;
 const unsigned count=sizeof(TownWindows)/sizeof(TownWindows[0]);
 for(unsigned i=0;i<count;++i){C(VirtualAlloc(p+(TownWindows[i].rva&~(page-1)),page,MEM_COMMIT,PAGE_READWRITE)!=NULL,"owned evidence page");std::memcpy(p+TownWindows[i].rva,TownWindows[i].code,24);}
 C(GsmNative::Module(reinterpret_cast<HMODULE>(p),0x65d604d7,size,TownWindows,count),"all native passive evidence accepted");
 const BYTE kepDetour[5]={0xe9,0xbb,0xa0,0xf1,0xff};std::memcpy(p+0xd5fd0,kepDetour,5);
 C(GsmNative::Module(reinterpret_cast<HMODULE>(p),0x65d604d7,size,TownWindows,count),"unused constructor entry detour does not invalidate unchanged passive tail");
 for(unsigned i=0;i<count;++i){p[TownWindows[i].rva]^=1;C(!GsmNative::Module(reinterpret_cast<HMODULE>(p),0x65d604d7,size,TownWindows,count),"every passive window still required");p[TownWindows[i].rva]^=1;}
 p[0xd5fd5]^=1;C(!GsmNative::Module(reinterpret_cast<HMODULE>(p),0x65d604d7,size,TownWindows,count),"constructor state/entity tail corruption refused");p[0xd5fd5]^=1;
 n->FileHeader.TimeDateStamp^=1;C(!GsmNative::Module(reinterpret_cast<HMODULE>(p),0x65d604d7,size,TownWindows,count),"module identity retained");n->FileHeader.TimeDateStamp^=1;
 C(VirtualFree(p,0,MEM_RELEASE)!=FALSE,"owned image released");std::printf("Origin passive profile PASS %u checks\n",checks);return 0;
}catch(...){return 1;}}
'''.replace('PROFILE',(base/'atlas-lights/OriginProofProfile.h').as_posix()).replace('BODY',body)
cpp=out/'profile.cpp';cpp.write_text(code)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/W3','/Fe'+str(out/'profile.exe'),'/Fo'+str(out/'profile.obj'),str(cpp)],env=env,check=True)
subprocess.run([str(out/'profile.exe')],check=True)
