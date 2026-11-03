#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <cstdio>
#include <cstdlib>
static unsigned errors=0,checks=0;
void ErrorLog(const char*) { ++errors; }
namespace GameTheme { bool fail=false;std::string Hex(const char*,const char* fallback){if(fail)throw 17;return fallback;} }
#include "../GradeRuntime.h"
#define CHECK(x) do { ++checks;if(!(x)){std::printf("FAIL %d %s\n",__LINE__,#x);std::exit(1);} }while(0)
static void Write(const char* path,const char* contents) {
    HANDLE f=CreateFileA(path,GENERIC_WRITE,0,0,CREATE_ALWAYS,0,0);CHECK(f!=INVALID_HANDLE_VALUE);
    DWORD n=0;CHECK(WriteFile(f,contents,static_cast<DWORD>(std::strlen(contents)),&n,0)&&n==std::strlen(contents));CHECK(CloseHandle(f));
}
int main(int argc,char**argv) {
    CHECK(argc==3);IMRuntime::State state;
    IM_ItemView view={sizeof(view),4,0,"backpack.mod","",-1,0};IM_Appearance out={};uint64_t observed=0;
    HMODULE provider=LoadLibraryA(argv[1]);CHECK(provider!=0);
    IM_Query query=reinterpret_cast<IM_Query>(GetProcAddress(provider,"Provider"));CHECK(query!=0);
    IM_Provider p={sizeof(p),"runtime.provider",5,query,0};IM_Handle h=0;
    CHECK(state.Add(&p,&h)==IM_OK);CHECK(FreeLibrary(provider));CHECK(GetModuleHandleA("extensions_provider.dll")!=0);
    CHECK(state.Query(view,out,observed)&&out.rank==4&&out.rgb==0x123456);
    const uint64_t before=state.Revision();CHECK(state.Notify(h)==IM_OK);CHECK(state.Revision()==before+1);
    CHECK(state.Remove(h)==IM_OK);CHECK(GetModuleHandleA("extensions_provider.dll")==0);CHECK(state.Notify(h)==IM_NOT_FOUND);
    for(unsigned fault=1;fault<=2;++fault) {
        provider=LoadLibraryA(argv[1]);CHECK(provider!=0);p.query=reinterpret_cast<IM_Query>(GetProcAddress(provider,"Provider"));p.context=reinterpret_cast<void*>(fault);
        CHECK(state.Add(&p,&h)==IM_OK);CHECK(FreeLibrary(provider));const unsigned prior=errors;
        const uint64_t beforeFault=state.Revision();
        CHECK(!state.Query(view,out,observed));CHECK(errors==prior+1);CHECK(state.Revision()==beforeFault+1);
        CHECK(!state.Query(view,out,observed));CHECK(errors==prior+1);CHECK(state.Revision()==beforeFault+1);
        CHECK(state.Remove(h)==IM_OK);CHECK(GetModuleHandleA("extensions_provider.dll")==0);
    }
    CHECK(CreateDirectoryA(argv[2],0)||GetLastError()==ERROR_ALREADY_EXISTS);state.SetDirectory(argv[2]);
    const std::string file=std::string(argv[2])+"\\test.ini";
    Write(file.c_str(),"[Grade.high]\nRank=4\nColour=#abcdef\n[Rule.test]\nItemID=backpack.mod\nGrade=high\n");state.Reload();
    CHECK(state.Query(view,out,observed)&&out.rgb==0xabcdef);const uint64_t good=state.Revision();
    Write(file.c_str(),"[Grade.high]\nRank=not-a-number\nColour=#abcdef\n");state.Reload();
    CHECK(state.Revision()==good);CHECK(state.Query(view,out,observed)&&out.rgb==0xabcdef);
    Write(file.c_str(),"[Grade.high]\nRank=4\nColour=#fedcba\nPalette=ItemGrade4\n[Rule.test]\nItemID=backpack.mod\nGrade=high\n");
    GameTheme::fail=true;state.ReloadSafely();GameTheme::fail=false;
    CHECK(state.Revision()==good);CHECK(state.Query(view,out,observed)&&out.rgb==0xabcdef);
    CHECK(DeleteFileA(file.c_str()));state.RequestReload();state.ProcessReload();CHECK(!state.Query(view,out,observed));
    CHECK(RemoveDirectoryA(argv[2]));std::printf("%u runtime DLL/config controls passed\n",checks);return 0;
}
