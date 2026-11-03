#pragma once
#include "RegionMaterial.h"
#include "../source-receipts/NativeMetadata.h"
#include <kenshi/Globals.h>
#include <kenshi/GameWorld.h>
#include <kenshi/GameData.h>
#include <kenshi/Enums.h>
#include "NativePins.h"
namespace NativeRegionMaterial { namespace Internal {
inline bool EngineProfile(){
 const BYTE*base=reinterpret_cast<const BYTE*>(GetModuleHandleA(NULL));IMAGE_DOS_HEADER dos={};IMAGE_NT_HEADERS64 nt={};
 if(!SourceReceipts::ReadMemory(base,&dos,sizeof(dos))||dos.e_magic!=IMAGE_DOS_SIGNATURE||dos.e_lfanew<64||dos.e_lfanew>4096||!SourceReceipts::ReadMemory(base+dos.e_lfanew,&nt,sizeof(nt))||nt.Signature!=IMAGE_NT_SIGNATURE||nt.FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64||nt.FileHeader.TimeDateStamp!=0x65d604d7||nt.OptionalHeader.SizeOfImage!=0x232c000||nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC)return false;
 for(unsigned n=0;n<sizeof(CellCodePins)/sizeof(CellCodePins[0]);++n){BYTE code[24];const CodePin&p=CellCodePins[n];if(p.rva>nt.OptionalHeader.SizeOfImage-sizeof(code)||!SourceReceipts::ReadMemory(base+p.rva,code,sizeof(code))||std::memcmp(code,p.code,sizeof(code)))return false;}
 return base==reinterpret_cast<const BYTE*>(GetModuleHandleA(NULL));
}
inline bool NativeRoot(const char*gameRoot){
 char path[MAX_PATH]={};DWORD n=GetModuleFileNameA(NULL,path,MAX_PATH);if(!n||n>=MAX_PATH)return false;
 char*last=NULL;for(char*p=path;*p;++p)if(*p=='/'||*p=='\\')last=p;if(!last)return false;*last=0;
 char*tail=path;for(char*p=path;*p;++p)if(*p=='/'||*p=='\\')tail=p+1;
 if(!_stricmp(tail,"RE_Kenshi")){if(tail==path)return false;tail[-1]=0;}
 try{return !_stricmp(Helpers::Canonical(gameRoot).c_str(),Helpers::Canonical(path).c_str());}catch(...){return false;}
}
struct NativeAccess{
 typedef GameData Record;SourceReceipts::Scope scope;
 explicit NativeAccess(const SourceReceipts::Scope&s):scope(s){}
 bool Current(const SourceReceipts::Scope&now)const{
  if(!SourceReceipts::ScopeValid(now)||now.thread!=GetCurrentThreadId()||!SourceReceipts::SameScope(scope,now)||!ou||reinterpret_cast<U64>(ou)!=now.world||!EngineProfile())return false;
  const BYTE*base=reinterpret_cast<const BYTE*>(GetModuleHandleA(NULL));const void*world=NULL;
  return SourceReceipts::ReadMemory(base+0x2133960,&world,sizeof(world))&&world==ou;
 }
 Record*GetData(const char*sid){return Current(scope)?ou->gamedata.getData(sid):NULL;}
 bool IsBiome(const Record*record)const{return record&&record->type==BIOMES;}
 bool Cell(const SatelliteRegion::Region&region,unsigned index,void*(&ids)[5],BYTE(&tree)[2048],unsigned&bytes){
  bytes=0;if(index>=16||region.x>=8||region.z>=8||!Current(scope))return false;
  const BYTE*base=reinterpret_cast<const BYTE*>(GetModuleHandleA(NULL));const BYTE*world=NULL,*map=NULL,*table=NULL,*source=NULL;unsigned width=0,height=0;
  if(!SourceReceipts::ReadMemory(base+0x2133960,&world,sizeof(world))||world!=reinterpret_cast<const BYTE*>(ou)||!SourceReceipts::ReadMemory(world+0x168220,&map,sizeof(map))||!SourceReceipts::ReadMemory(map+0x50,&width,4)||!SourceReceipts::ReadMemory(map+0x54,&height,4)||width!=32||height!=32||!SourceReceipts::ReadMemory(map+0x58,&table,sizeof(table)))return false;
  const BYTE*cell=table+(((region.z*4+index/4)*32)+(region.x*4+index%4))*48;BYTE first=0;
  if(!SourceReceipts::ReadMemory(cell,ids,40)||!SourceReceipts::ReadMemory(cell+40,&source,sizeof(source))||!SourceReceipts::ReadMemory(source,&first,1))return false;
  const unsigned count=((first&0xe0)>0x20)?2048:1;if(!SourceReceipts::ReadMemory(source,tree,count))return false;
  const BYTE*afterWorld=NULL,*afterMap=NULL,*afterTable=NULL,*afterSource=NULL;void*afterIds[5];
  if(!SourceReceipts::ReadMemory(base+0x2133960,&afterWorld,sizeof(afterWorld))||afterWorld!=world||!SourceReceipts::ReadMemory(world+0x168220,&afterMap,sizeof(afterMap))||afterMap!=map||!SourceReceipts::ReadMemory(map+0x58,&afterTable,sizeof(afterTable))||afterTable!=table||!SourceReceipts::ReadMemory(cell+40,&afterSource,sizeof(afterSource))||afterSource!=source||!SourceReceipts::ReadMemory(cell,afterIds,sizeof(afterIds))||std::memcmp(ids,afterIds,sizeof(afterIds))||!Current(scope))return false;
  bytes=count;return true;
 }
};
inline bool CaptureImpl(const SourceReceipts::Scope&scope,unsigned region,const char*root,U64 generation,bool nativeFade,Snapshot&out){
 std::memset(&out,0,sizeof(out));if(!RootValid(root)||!NativeRoot(root))return false;NativeAccess access(scope);return CaptureWith(access,scope,region,root,generation,nativeFade,out);
}
}
// Call only from the caller's already-checked Main/native context. Same-ou load
// epoch and queued output freshness are checked by the owning coordinator.
inline bool Capture(const SourceReceipts::Scope&scope,unsigned region,const char*gameRoot,U64 regionMaterialGeneration,bool nativeFade,Snapshot&out){
 __try{return Internal::CaptureImpl(scope,region,gameRoot,regionMaterialGeneration,nativeFade,out);}
 __except(EXCEPTION_EXECUTE_HANDLER){std::memset(&out,0,sizeof(out));return false;}
}
}
