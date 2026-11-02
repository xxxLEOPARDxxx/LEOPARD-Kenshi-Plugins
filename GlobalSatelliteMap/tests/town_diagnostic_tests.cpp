#include <Windows.h>
#include <cstdio>
#include <cstring>
#include "../GsmTownDiagnostic.h"
static unsigned checks=0;
static void Check(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}}
struct Window {SIZE_T rva;const char*hex;};
static Window windows[]={
{0xb85f0,"488d4150c3cccccccccccccccccccccc83f9057734488d15"},
{0x8679bd,"48897b08488bc3488b5c24504883c4305fc3cccccccccccc"},
{0x927e08,"397958766348897424408bf74c896424480f1f8000000000"},
{0x927e20,"488b4560488b0c06488b01ff5058493bc57528488b45608b"},
{0x253a6,"e935bd9200e950cb0a00e96bea8b00e946b47900e9510424"}};
static unsigned Hex(char c){return c<='9'?c-'0':c-'a'+10;}
static void PutPointer(BYTE*p,SIZE_T at,const void*v){std::memcpy(p+at,&v,sizeof(v));}
static void PutCount(BYTE*p,unsigned v){std::memcpy(p+0x58,&v,4);}
static BYTE*mutationList=NULL,*mutationKing=NULL,*mutationProxy=NULL,*mutationTown=NULL,*mutationEngine=NULL,*mutationAlternative=NULL,*guarded=NULL;
static unsigned mutation=0,fired=0;
static LONG CALLBACK ChangeDuringRead(EXCEPTION_POINTERS*e){
    if(!mutation||e->ExceptionRecord->ExceptionCode!=STATUS_GUARD_PAGE_VIOLATION)return EXCEPTION_CONTINUE_SEARCH;
    const ULONG_PTR at=e->ExceptionRecord->ExceptionInformation[1];
    if(at<reinterpret_cast<ULONG_PTR>(guarded)||at>=reinterpret_cast<ULONG_PTR>(guarded)+4096)return EXCEPTION_CONTINUE_SEARCH;
    unsigned kind=mutation;mutation=0;++fired;
    if(kind==1)PutCount(mutationList,0);
    if(kind==2)PutPointer(mutationList,0x60,NULL);
    if(kind==3)PutPointer(mutationKing,8,mutationAlternative);
    if(kind==4)PutPointer(mutationProxy,0,NULL);
    if(kind==5)PutPointer(mutationProxy,16,NULL);
    if(kind==6)PutPointer(mutationTown,0x4f8,NULL);
    if(kind==7)PutPointer(mutationTown,0,mutationEngine+0x1733fa8);
    if(kind==8){unsigned s=2;std::memcpy(mutationProxy+8,&s,4);}
    return EXCEPTION_CONTINUE_EXECUTION;
}
int main(){try{
    SYSTEM_INFO si;GetSystemInfo(&si);const SIZE_T page=si.dwPageSize;
    BYTE*engine=static_cast<BYTE*>(VirtualAlloc(NULL,0x1736000,MEM_RESERVE,PAGE_NOACCESS));
    Check(engine!=NULL,"owned sparse engine reservation");
    for(unsigned i=0;i<5;++i){Check(VirtualAlloc(engine+(windows[i].rva&~(page-1)),page,MEM_COMMIT,PAGE_READWRITE)!=NULL,"owned profile page");for(unsigned j=0;j<24;++j)engine[windows[i].rva+j]=BYTE(Hex(windows[i].hex[j*2])*16+Hex(windows[i].hex[j*2+1]));}
    Check(VirtualAlloc(engine+0x1734000,page*2,MEM_COMMIT,PAGE_READWRITE)!=NULL,"owned vtable page");PutPointer(engine,0x1734c08+0x268,engine+0x253a6);
    BYTE*bad=static_cast<BYTE*>(VirtualAlloc(NULL,page,MEM_COMMIT|MEM_RESERVE,PAGE_NOACCESS));Check(bad!=NULL,"owned inaccessible page");
    BYTE king[16]={0},list[128]={0},town[0x500]={0},proxy[24]={0};const void*array[4096];
    PutPointer(king,8,list);PutCount(list,1);PutPointer(list,0x60,array);array[0]=town;
    PutPointer(town,0,engine+0x1734c08);PutPointer(town,0x4f8,proxy);PutPointer(proxy,0,town);unsigned state=3;std::memcpy(proxy+8,&state,4);PutPointer(proxy,16,reinterpret_cast<void*>(42));
    GsmTownDiagnostic::Stats stats;const void*candidate=reinterpret_cast<void*>(9);
    Check(sizeof(stats)==52,"fixed scalar-only census metadata");
    Check(GsmTownDiagnostic::Read(false,reinterpret_cast<void*>(1),reinterpret_cast<void*>(1),false,stats,candidate)==S_FALSE&&!candidate&&!stats.total,"OFF no native queries");
    Check(GsmTownDiagnostic::Read(true,reinterpret_cast<void*>(1),reinterpret_cast<void*>(1),false,stats,candidate)==E_ACCESSDENIED&&!candidate&&!stats.total,"unverified source no native queries");
    Check(GsmTownDiagnostic::Read(true,king,engine,true,stats,candidate)==S_OK&&candidate==town&&stats.total==1&&stats.primary==1&&stats.state[3]==1&&stats.candidates==1,"actual owned state3 census and first candidate");
    BYTE towns[11][0x500]={0},proxies[11][24]={0};
    for(unsigned i=0;i<11;++i){array[i]=towns[i];PutPointer(towns[i],0,engine+0x1734c08);PutPointer(towns[i],0x4f8,proxies[i]);PutPointer(proxies[i],0,towns[i]);PutPointer(proxies[i],16,reinterpret_cast<void*>(99));}
    array[0]=NULL;PutPointer(towns[1],0,engine+0x1733fa8);PutPointer(towns[1],0x4f8,reinterpret_cast<void*>(1));PutPointer(towns[2],0x4f8,NULL);PutPointer(proxies[3],0,town);
    for(unsigned i=4;i<11;++i){unsigned s=i<7?i-4:i==8?17:3;std::memcpy(proxies[i]+8,&s,4);}PutPointer(proxies[7],16,NULL);PutCount(list,11);
    Check(GsmTownDiagnostic::Read(true,king,engine,true,stats,candidate)==S_OK,"mixed actual census succeeds");
    Check(stats.total==11&&stats.primary==9&&stats.nullTown==1&&stats.otherType==1,"only exact primary Town vtable admitted");
    Check(stats.missingProxy==1&&stats.ownerMismatch==1,"missing proxy and wrong owner counted separately");
    Check(stats.state[0]==1&&stats.state[1]==1&&stats.state[2]==1&&stats.state[3]==3&&stats.otherState==1,"native states counted without clamping");
    Check(stats.nullEntity==1&&stats.candidates==2&&candidate==towns[9],"state3 null entity unavailable and first valid candidate retained");
    for(unsigned i=0;i<4096;++i)array[i]=town;PutCount(list,4096);
    Check(GsmTownDiagnostic::Read(true,king,engine,true,stats,candidate)==S_OK&&stats.total==4096&&stats.candidates==4096&&candidate==town,"maximum4096 actual pointers copied and examined");
    PutCount(list,4097);PutPointer(list,0x60,bad);
    Check(GsmTownDiagnostic::Read(true,king,engine,true,stats,candidate)==E_INVALIDARG&&!candidate&&!stats.total,"overflow refused before inaccessible array");
    PutCount(list,0);PutPointer(list,0x60,NULL);
    Check(GsmTownDiagnostic::Read(true,king,engine,true,stats,candidate)==S_OK&&!candidate&&!stats.total,"empty native list accepted without array");
    PutCount(list,1);
    Check(GsmTownDiagnostic::Read(true,king,engine,true,stats,candidate)==E_INVALIDARG,"nonnull count requires array");
    PutPointer(list,0x60,reinterpret_cast<BYTE*>(array)+1);
    Check(GsmTownDiagnostic::Read(true,king,engine,true,stats,candidate)==E_INVALIDARG,"misaligned pointer span refused");
    PutCount(list,2);PutPointer(list,0x60,reinterpret_cast<void*>(ULONG_PTR(-1)-7));
    Check(GsmTownDiagnostic::Read(true,king,engine,true,stats,candidate)==E_INVALIDARG,"pointer span overflow refused");
    PutCount(list,1);PutPointer(list,0x60,array);array[0]=town;
    for(unsigned i=0;i<5;++i){engine[windows[i].rva]^=1;Check(GsmTownDiagnostic::Read(true,king,engine,true,stats,candidate)==E_ACCESSDENIED&&!candidate&&!stats.total,"each exact code24 pin required");engine[windows[i].rva]^=1;}
    PutPointer(engine,0x1734c08+0x268,engine+0x253a7);
    Check(GsmTownDiagnostic::Read(true,king,engine,true,stats,candidate)==E_ACCESSDENIED,"primary vtable thunk identity required");PutPointer(engine,0x1734c08+0x268,engine+0x253a6);
    Check(GsmTownDiagnostic::Read(true,king,reinterpret_cast<void*>(ULONG_PTR(-1)-15),true,stats,candidate)==E_ACCESSDENIED,"engine addition overflow refused");
    Check(GsmTownDiagnostic::Read(true,bad,engine,true,stats,candidate)==E_FAIL&&!candidate&&!stats.total,"inaccessible SharedKing contained");
    PutPointer(king,8,bad);Check(GsmTownDiagnostic::Read(true,king,engine,true,stats,candidate)==E_FAIL&&!candidate&&!stats.total,"inaccessible TownList contained");PutPointer(king,8,list);
    PutPointer(list,0x60,bad);Check(GsmTownDiagnostic::Read(true,king,engine,true,stats,candidate)==E_FAIL&&!candidate&&!stats.total,"inaccessible array contained");PutPointer(list,0x60,array);
    array[0]=bad;Check(GsmTownDiagnostic::Read(true,king,engine,true,stats,candidate)==E_FAIL&&!candidate&&!stats.total,"inaccessible Town contained");array[0]=town;
    PutPointer(town,0x4f8,bad);Check(GsmTownDiagnostic::Read(true,king,engine,true,stats,candidate)==E_FAIL&&!candidate&&!stats.total,"inaccessible proxy contained");PutPointer(town,0x4f8,proxy);
    PutCount(list,2);array[1]=bad;Check(GsmTownDiagnostic::Read(true,king,engine,true,stats,candidate)==E_FAIL&&!candidate&&!stats.total,"fault after candidate does not publish partial census");
    Check(GsmTownDiagnostic::Read(false,bad,bad,true,stats,candidate)==S_FALSE&&!candidate&&!stats.total,"OFF bypasses inaccessible pinned engine and source");
    Check(GsmTownDiagnostic::Read(true,bad,bad,false,stats,candidate)==E_ACCESSDENIED&&!candidate&&!stats.total,"unverified bypasses inaccessible pinned engine and source");
    guarded=static_cast<BYTE*>(VirtualAlloc(NULL,page,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));Check(guarded!=NULL,"owned changing-source trigger page");PutPointer(guarded,0,engine+0x1733fa8);
    BYTE alternative[128]={0};mutationList=list;mutationKing=king;mutationProxy=proxy;mutationTown=town;mutationEngine=engine;mutationAlternative=alternative;
    PVOID handler=AddVectoredExceptionHandler(1,ChangeDuringRead);Check(handler!=NULL,"owned-only deterministic source-change observer");
    for(unsigned kind=1;kind<=8;++kind){
        PutPointer(king,8,list);PutCount(list,2);PutPointer(list,0x60,array);array[0]=town;array[1]=guarded;
        PutPointer(town,0,engine+0x1734c08);PutPointer(town,0x4f8,proxy);PutPointer(proxy,0,town);PutPointer(proxy,16,reinterpret_cast<void*>(42));state=3;std::memcpy(proxy+8,&state,4);
        DWORD old=0;Check(VirtualProtect(guarded,page,PAGE_READWRITE|PAGE_GUARD,&old)!=FALSE,"owned trigger armed");mutation=kind;unsigned before=fired;
        HRESULT hr=GsmTownDiagnostic::Read(true,king,engine,true,stats,candidate);
        if(hr!=E_ABORT||fired!=before+1||candidate||stats.total)std::fprintf(stderr,"mutation=%u hr=%lx fired=%u candidate=%p total=%u\n",kind,hr,fired-before,candidate,stats.total);
        Check(hr==E_ABORT&&fired==before+1&&!candidate&&!stats.total,"changed header/listowner/candidate identity after copy refuses whole publication");
    }
    Check(RemoveVectoredExceptionHandler(handler)!=0,"owned source-change observer released");
    Check(VirtualFree(guarded,0,MEM_RELEASE)!=FALSE&&VirtualFree(bad,0,MEM_RELEASE)!=FALSE,"owned fault pages released");
    Check(VirtualFree(engine,0,MEM_RELEASE)!=FALSE,"owned sparse engine released");
    std::printf("town census: %u checks PASS\n",checks);return 0;
}catch(...){return 1;}}
