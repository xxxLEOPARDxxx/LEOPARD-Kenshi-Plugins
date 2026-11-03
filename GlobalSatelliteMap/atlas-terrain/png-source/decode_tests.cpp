#define _CRT_SECURE_NO_WARNINGS
#include "PinnedPng.h"
#include <vector>
#include <fstream>
#include <cstdio>
static unsigned checks=0;
extern const WorldBank::RgbaPin&ExternalPin(unsigned);
static void C(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}}
struct Foreign{PinnedPng::Decoder*d;const WorldBank::RgbaPin*p;const BYTE*input;size_t bytes;BYTE*out;HRESULT hr,clear;};
static DWORD WINAPI ForeignDecode(void*arg){Foreign&v=*static_cast<Foreign*>(arg);v.hr=v.d->Decode(*v.p,v.input,v.bytes,v.out,v.p->bytes,NULL);v.clear=v.d->Clear();return 0;}
int main(){try{
 PinnedPng::Decoder d;C(SUCCEEDED(d.Init()),"owned worker COM decoder initializes");
 for(unsigned i=0;i<129;++i){const WorldBank::RgbaPin&p=WorldBank::RgbaPins[i];
  std::ifstream f((std::string("D:/SteamLibrary/steamapps/common/Kenshi/")+p.path).c_str(),std::ios::binary|std::ios::ate);C(!!f&&size_t(f.tellg())==p.encodedBytes,"real encoded PNG profile");std::vector<BYTE>encoded(p.encodedBytes),rgba(p.bytes,77);f.seekg(0);f.read(reinterpret_cast<char*>(&encoded[0]),encoded.size());C(!!f,"one real encoded input");
  C(SUCCEEDED(d.Decode(p,&encoded[0],encoded.size(),&rgba[0],rgba.size(),NULL)),"actual WIC pixels match independently pinned Pillow RGBA");
  if(i==27){const WorldBank::RgbaPin&external=ExternalPin(i);C(&external!=&p&&PinnedPng::Side(external)==1024,"canonical identity survives separate translation unit");C(SUCCEEDED(d.Decode(external,&encoded[0],encoded.size(),&rgba[0],rgba.size(),NULL)),"separate bank TU canonical source decodes");WorldBank::RgbaPin forged=external;forged.sha[0]^=1;C(!PinnedPng::Side(forged),"copied pin must retain decoded identity");forged=external;forged.encodedSha="wrong";C(!PinnedPng::Side(forged),"copied pin must retain encoded identity");forged=external;forged.path="wrong";C(!PinnedPng::Side(forged),"copied pin must retain logical source identity");}
  BYTE digest[32];C(AtlasView::Digest(&rgba[0],DWORD(rgba.size()),digest)&&!std::memcmp(digest,p.sha,32),"independent exact decoded pixel digest");
  const unsigned side=i>=64&&i<128?2048:1024;C(rgba.size()==side*side*4,"actual bounded output shape");
  if(i==27){std::vector<BYTE>before=rgba;encoded[encoded.size()/2]^=1;C(FAILED(d.Decode(p,&encoded[0],encoded.size(),&rgba[0],rgba.size(),NULL))&&rgba==before,"bad input refuses before decoded output mutation");encoded[encoded.size()/2]^=1;
   C(FAILED(d.Decode(p,&encoded[0],encoded.size(),&rgba[0],rgba.size()-1,NULL)),"short destination refuses");HANDLE cancel=CreateEventA(NULL,TRUE,TRUE,NULL);C(cancel!=NULL,"owned cancel event");C(d.Decode(p,&encoded[0],encoded.size(),&rgba[0],rgba.size(),cancel)==HRESULT_FROM_WIN32(ERROR_CANCELLED),"pre-cancel refuses");CloseHandle(cancel);
   encoded[29]^=1;C(FAILED(d.Decode(p,&encoded[0],encoded.size(),&rgba[0],rgba.size(),NULL))&&rgba==before,"changed IHDR checksum refuses before WIC");encoded[29]^=1;
   std::vector<BYTE>alias(rgba.size(),77);std::memcpy(&alias[0],&encoded[0],encoded.size());std::vector<BYTE>aliasBefore=alias;
   C(FAILED(d.Decode(p,&alias[0],encoded.size(),&alias[0],rgba.size(),NULL))&&alias==aliasBefore,"overlapping source/destination refuses before mutation");
   Foreign other={&d,&p,&encoded[0],encoded.size(),&rgba[0],S_OK,S_OK};HANDLE thread=CreateThread(NULL,0,ForeignDecode,&other,0,NULL);C(thread&&WaitForSingleObject(thread,5000)==WAIT_OBJECT_0,"foreign decoder test joined");CloseHandle(thread);C(other.hr==RPC_E_WRONG_THREAD&&other.clear==RPC_E_WRONG_THREAD,"foreign decode/release refuse without touching COM");
  }
 }
 d.Clear();C(SUCCEEDED(d.Init()),"decoder clean reuse after COM release");d.Clear();
 std::printf("PASS %u pinned PNG decoder checks; all129 real images\n",checks);return 0;
 }catch(...){return 1;}}
