#include "../GsmTownValues.h"
#include <cstdio>
static unsigned checks=0;
static void C(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}}
static void P(BYTE*p,SIZE_T at,const void*v){std::memcpy(p+at,&v,8);}static void U(BYTE*p,SIZE_T at,unsigned v){std::memcpy(p+at,&v,4);}
static BYTE*trigger=NULL,*changedTown=NULL,*changedProxy=NULL,*changedList=NULL,*changedKing=NULL,*alternative=NULL;static const void**changedArray=NULL;static volatile LONG*changedGeneration=NULL;static unsigned mutation=0,fired=0;
static LONG CALLBACK Change(EXCEPTION_POINTERS*e){if(!mutation||e->ExceptionRecord->ExceptionCode!=STATUS_GUARD_PAGE_VIOLATION)return EXCEPTION_CONTINUE_SEARCH;ULONG_PTR at=e->ExceptionRecord->ExceptionInformation[1];if(at<reinterpret_cast<ULONG_PTR>(trigger)||at>=reinterpret_cast<ULONG_PTR>(trigger)+4096)return EXCEPTION_CONTINUE_SEARCH;unsigned m=mutation;mutation=0;++fired;
 if(m==1)U(changedTown,0x64,99);if(m>=2&&m<=4)U(changedTown,0x48+(m-2)*4,0x3f800000);if(m==5)P(changedTown,0x4f8,NULL);if(m==6)P(changedProxy,0,NULL);if(m==7)U(changedProxy,8,3);if(m==8)P(changedTown,0,NULL);if(m==9)changedArray[1]=NULL;if(m==10)U(changedList,0x58,0);if(m==11)P(changedList,0x60,NULL);if(m==12)P(changedKing,8,alternative);if(m==13)++*changedGeneration;
 return EXCEPTION_CONTINUE_EXECUTION;}
int main(){try{
 SYSTEM_INFO si;GetSystemInfo(&si);SIZE_T page=si.dwPageSize;BYTE*engine=static_cast<BYTE*>(VirtualAlloc(NULL,0x1736000,MEM_RESERVE,PAGE_NOACCESS));C(engine!=NULL,"owned sparse source module");
 for(unsigned group=0;group<2;++group){const GsmTownDiagnostic::CodePin*pins=group?GsmTownValues::FieldPins:GsmTownDiagnostic::Pins;unsigned count=group?sizeof(GsmTownValues::FieldPins)/sizeof(GsmTownValues::FieldPins[0]):sizeof(GsmTownDiagnostic::Pins)/sizeof(GsmTownDiagnostic::Pins[0]);for(unsigned i=0;i<count;++i){C(VirtualAlloc(engine+(pins[i].rva&~(page-1)),page,MEM_COMMIT,PAGE_READWRITE)!=NULL,"owned code evidence page");std::memcpy(engine+pins[i].rva,pins[i].bytes,24);}}
 C(VirtualAlloc(engine+0x1734000,page*2,MEM_COMMIT,PAGE_READWRITE)!=NULL,"owned primary vtable page");P(engine,0x1734c08+0x268,engine+0x253a6);
 BYTE king[16]={0},list[128]={0},town[0x500]={0},proxy[24]={0};const void*array[4096];P(king,8,list);U(list,0x58,1);P(list,0x60,array);array[0]=town;P(town,0,engine+0x1734c08);U(town,0x64,0xffffffff);float xyz[3]={123.25f,0,-456.5f};std::memcpy(town+0x48,xyz,12);
 GsmTownValues::Result out;volatile LONG generation=7;
 C(GsmTownValues::Read(false,false,reinterpret_cast<void*>(1),reinterpret_cast<void*>(1),7,reinterpret_cast<volatile LONG*>(1),out)==S_FALSE&&!out.count,"OFF no native queries");
 C(GsmTownValues::Read(true,false,reinterpret_cast<void*>(1),reinterpret_cast<void*>(1),7,&generation,out)==E_ACCESSDENIED&&!out.count,"unverified no native queries");
 C(GsmTownValues::Read(true,true,king,engine,7,&generation,out)==S_OK&&out.generation==7&&out.total==1&&out.count==1,"complete native value catalog published");
 C(out.towns[0].id==0xffffffff&&out.towns[0].storedXYZ[0]==xyz[0]&&out.towns[0].storedXYZ[1]==0&&out.towns[0].storedXYZ[2]==xyz[2]&&!out.towns[0].hasProxy&&out.towns[0].proxyState==-1,"unsigned id and exact stored XYZ copied without lazy height update");
 xyz[1]=123.75f;std::memcpy(town+0x48,xyz,12);C(GsmTownValues::Read(true,true,king,engine,7,&generation,out)==S_OK&&out.towns[0].storedXYZ[1]==123.75f,"nonzero stored height preserved without terrain sampling");xyz[1]=0;std::memcpy(town+0x48,xyz,12);
 BYTE before[sizeof(town)];std::memcpy(before,town,sizeof(town));C(GsmTownValues::Read(true,true,king,engine,7,&generation,out)==S_OK&&!std::memcmp(before,town,sizeof(town)),"catalog read performs no native height/cache write");
 P(town,0x4f8,proxy);P(proxy,0,town);
 for(unsigned state=0;state<5;++state){U(proxy,8,state);C(GsmTownValues::Read(true,true,king,engine,7,&generation,out)==S_OK&&out.towns[0].hasProxy&&out.towns[0].proxyState==int(state),"all native proxy states preserved without loaded-only filtering");}
 U(proxy,8,0xffffffff);C(GsmTownValues::Read(true,true,king,engine,7,&generation,out)==S_OK&&out.towns[0].hasProxy&&out.towns[0].proxyState==-1,"unknown signed proxy state preserved with presence flag");
 U(town,0x48,0x80000000);C(GsmTownValues::Read(true,true,king,engine,7,&generation,out)==S_OK&&GsmTownValues::Bits(out.towns[0].storedXYZ[0])==0x80000000,"coordinate bit identity includes negative zero");std::memcpy(town+0x48,xyz,12);
 BYTE second[0x500]={0};P(second,0,engine+0x1734c08);U(second,0x64,0);std::memcpy(second+0x48,xyz,12);array[0]=town;array[1]=NULL;array[2]=second;U(list,0x58,3);
 C(GsmTownValues::Read(true,true,king,engine,7,&generation,out)==S_OK&&out.total==3&&out.count==2&&out.nullEntries==1&&!out.otherTypes&&out.towns[1].id==0,"null entries counted and unsigned id zero retained");
 BYTE*p=static_cast<BYTE*>(VirtualAlloc(NULL,page*2,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));C(p!=NULL,"owned edge/fault pages");DWORD old=0;C(VirtualProtect(p+page,page,PAGE_NOACCESS,&old)!=FALSE,"owned inaccessible page");
 BYTE*other=p+page-8;P(other,0,engine+0x1733fa8);array[1]=other;
 C(GsmTownValues::Read(true,true,king,engine,7,&generation,out)==S_OK&&out.count==2&&out.otherTypes==1&&!out.nullEntries,"other type counted without reading inaccessible Town-only fields");
 BYTE*edgeProxy=p+page-16;P(edgeProxy,0,town);U(edgeProxy,8,2);P(town,0x4f8,edgeProxy);U(list,0x58,1);array[0]=town;
 C(GsmTownValues::Read(true,true,king,engine,7,&generation,out)==S_OK&&out.towns[0].hasProxy&&out.towns[0].proxyState==2,"hasProxy never dereferences Entity beyond proxy status");P(town,0x4f8,proxy);U(proxy,8,2);
 P(proxy,0,second);C(GsmTownValues::Read(true,true,king,engine,7,&generation,out)==E_INVALIDARG&&!out.count&&!out.generation,"wrong proxy owner refuses whole catalog");P(proxy,0,town);
 U(town,0x4c,0x7fc00000);C(GsmTownValues::Read(true,true,king,engine,7,&generation,out)==E_INVALIDARG&&!out.count,"nonfinite stored coordinates refuse publication");std::memcpy(town+0x48,xyz,12);
 for(unsigned i=0;i<4096;++i)array[i]=town;U(list,0x58,4096);C(GsmTownValues::Read(true,true,king,engine,7,&generation,out)==S_OK&&out.count==4096&&out.towns[4095].id==0xffffffff,"all4096 records preserved without truncation or id deduplication");
 U(list,0x58,4097);P(list,0x60,p+page);C(GsmTownValues::Read(true,true,king,engine,7,&generation,out)==E_INVALIDARG&&!out.count&&!out.total,"4097 refused before inaccessible array");
 U(list,0x58,0);P(list,0x60,NULL);C(GsmTownValues::Read(true,true,king,engine,7,&generation,out)==S_OK&&!out.count&&out.generation==7,"empty list no array accepted");U(list,0x58,1);P(list,0x60,array);array[0]=town;
 C(GsmTownValues::Read(true,true,king,engine,0,&generation,out)==E_INVALIDARG,"zero generation refused");C(GsmTownValues::Read(true,true,king,engine,7,NULL,out)==E_INVALIDARG,"missing live-generation owner refused");C(GsmTownValues::Read(true,true,king,engine,6,&generation,out)==E_ABORT,"stale generation refused before source traversal");
 C(GsmTownValues::Read(true,true,p+page,engine,7,&generation,out)==E_FAIL&&!out.count,"inaccessible SharedKing contained");P(list,0x60,p+page);C(GsmTownValues::Read(true,true,king,engine,7,&generation,out)==E_FAIL&&!out.count,"inaccessible array contained");P(list,0x60,array);array[0]=p+page;C(GsmTownValues::Read(true,true,king,engine,7,&generation,out)==E_FAIL&&!out.count,"inaccessible entry contained");array[0]=town;P(town,0x4f8,p+page);C(GsmTownValues::Read(true,true,king,engine,7,&generation,out)==E_FAIL&&!out.count,"inaccessible proxy contained");P(town,0x4f8,proxy);
 for(unsigned i=0;i<2;++i){engine[GsmTownValues::FieldPins[i].rva]^=1;C(GsmTownValues::Read(true,true,king,engine,7,&generation,out)==E_ACCESSDENIED&&!out.count,"both native id/position field pins required");engine[GsmTownValues::FieldPins[i].rva]^=1;}
 trigger=static_cast<BYTE*>(VirtualAlloc(NULL,page,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));C(trigger!=NULL,"owned mid-read observer page");P(trigger,0,engine+0x1733fa8);BYTE altList[128]={0};changedList=list;changedKing=king;changedProxy=proxy;changedArray=array;changedGeneration=&generation;alternative=altList;
 P(second,0x4f8,proxy);PVOID observer=AddVectoredExceptionHandler(1,Change);C(observer!=NULL,"owned source-change observer installed");
 for(unsigned m=1;m<=13;++m){changedTown=second;P(king,8,list);P(list,0x60,array);U(list,0x58,3);array[0]=town;array[1]=second;array[2]=trigger;P(town,0,engine+0x1734c08);P(town,0x4f8,NULL);P(second,0,engine+0x1734c08);P(second,0x4f8,proxy);P(proxy,0,second);U(proxy,8,2);U(second,0x64,0xffffffff);std::memcpy(second+0x48,xyz,12);generation=7;mutation=m;unsigned previous=fired;
  C(VirtualProtect(trigger,page,PAGE_READWRITE|PAGE_GUARD,&old)!=FALSE,"owned observer armed");HRESULT hr=GsmTownValues::Read(true,true,king,engine,7,&generation,out);
  C(hr==E_ABORT&&fired==previous+1&&!out.count&&!out.total&&!out.generation,"same-count later Town identity/header/array/generation mutation refuses entire catalog");
 }
 C(RemoveVectoredExceptionHandler(observer)!=0,"owned observer removed");C(VirtualFree(trigger,0,MEM_RELEASE)!=FALSE&&VirtualFree(p,0,MEM_RELEASE)!=FALSE,"owned source pages released");
 C(sizeof(GsmTownValues::TownValue)==24&&sizeof(GsmTownValues::Result)<128*1024,"fixed scalar catalog below128KiB");
 C(VirtualFree(engine,0,MEM_RELEASE)!=FALSE,"owned sparse module released");std::printf("town values: %u checks PASS\n",checks);return 0;
}catch(...){return 1;}}
