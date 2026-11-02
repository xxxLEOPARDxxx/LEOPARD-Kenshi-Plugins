#include "CensusRoute.h"
#include <cstdio>
#include <cstdlib>
using namespace NativeLoadedCensus;
unsigned checks=0;
void Check(bool v,const char*s){++checks;if(!v){std::printf("FAIL %s\n",s);std::exit(1);}}
struct Fake {unsigned starts,keys,count,inspects,finish,firstCount;bool changed;Fake():starts(0),keys(1),count(1),inspects(0),finish(0),firstCount(~0u),changed(false){} HRESULT Begin(){++starts;return S_OK;}unsigned Keys(){return keys;}unsigned Entries(unsigned k){return k==0&&firstCount!=~0u?firstCount:count;}HRESULT Inspect(unsigned,unsigned,Result&r,void*&candidate){++inspects;if(!SelectionRoom(r))return S_FALSE;candidate=this;++r.selected;return S_OK;}bool End(void*){++finish;return !changed;}};
int main(){Result r;void*c=reinterpret_cast<void*>(1);Fake f;
 Check(ReadRoute(false,true,1,f,r,c)==S_FALSE&&f.starts==0&&!c&&!r.selected,"OFF zero native queries");
 Check(ReadRoute(true,false,1,f,r,c)==E_ACCESSDENIED&&f.starts==0,"unverified zero queries");
 Check(ReadRoute(true,true,0,f,r,c)==E_ACCESSDENIED&&f.starts==0,"zero generation no queries");
 Check(ReadRoute(true,true,1,f,r,c)==S_OK&&f.starts==1&&r.selected==1&&c==&f&&f.finish==1,"actual route publish");
 Fake old;old.changed=true;Check(ReadRoute(true,true,1,old,r,c)==E_ABORT&&!c&&!r.selected,"generation/header changed rejects all publication");
 Fake over;over.keys=EntryLimit+1;Check(ReadRoute(true,true,1,over,r,c)==E_INVALIDARG&&over.inspects==0&&!c,"key limit refuses before inspection");
 Fake many;many.count=EntryLimit+1;Check(ReadRoute(true,true,1,many,r,c)==E_INVALIDARG&&many.inspects==0,"bad floor entry count refuses");
 Fake cap;cap.keys=2;cap.count=EntryLimit;Check(ReadRoute(true,true,1,cap,r,c)==S_OK&&cap.inspects==EntryLimit&&r.visited==EntryLimit&&r.incomplete,"total entries bounded across keys");
 Fake cross;cross.keys=2;cross.firstCount=1;cross.count=EntryLimit;Check(ReadRoute(true,true,1,cross,r,c)==S_OK&&cross.inspects==EntryLimit&&r.visited==EntryLimit&&r.incomplete,"cross-key entry cap exact");
 Check(ValidArray(0,0,0,8),"empty array no pointer");
 Check(!ValidArray(0,1,1,8)&&!ValidArray(8,2,1,8)&&!ValidArray(8,4097,4097,8),"array count/null bounds");
 Check(!ValidArray(9,1,1,8)&&!ValidArray(~0ULL-7,2,2,8),"alignment and address overflow");
 char name[NameBytes];Check(CopyName("actual",6,name)&&!std::strcmp(name,"actual"),"actual bounded string copy");
 char large[129];std::memset(large,'x',sizeof(large));Check(!CopyName(large,128,name)&&!name[0],"overlong name refuses");
 for(unsigned ch=0;ch<32;++ch){char v[3]={'a',char(ch),'b'};Check(!CopyName(v,3,name)&&!name[0],"name controls fail closed");}
 Result room;Check(SelectionRoom(room)&&MaterialRoom(room,SubLimit),"actual metadata capacity accepts bound");room.selected=EntityLimit;Check(!SelectionRoom(room)&&room.incomplete,"selected entity bound");room=Result();room.materialCount=MaterialLimit;Check(!MaterialRoom(room,1)&&room.incomplete,"material count bound");room=Result();Check(!MaterialRoom(room,SubLimit+1)&&room.unsupported==1,"oversized subentities explicit");room=Result();Check(!MaterialRoom(room,0)&&room.unsupported==1,"empty subentities explicit");room=Result();room.materialCount=MaterialLimit+1;Check(!MaterialRoom(room,1),"material subtraction overflow refuses");
 std::printf("PASS %u production shared route checks\n",checks);return 0;}
