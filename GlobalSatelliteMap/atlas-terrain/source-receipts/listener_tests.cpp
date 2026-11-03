#define _CRT_SECURE_NO_WARNINGS
#include "MainGate.h"
#include <cstdio>
using namespace SourceReceipts;
static unsigned checks=0;
static void C(bool b,const char*s){++checks;if(!b){fprintf(stderr,"FAIL %s\n",s);throw 1;}}
int main(){try{
 for(unsigned mode=0;mode<4;++mode)for(unsigned n=0;n<3;++n){
  U64 listener=n==0?0:n==1?1:0x100000001ULL;
  C(ListenerPolicyAllows(mode,listener)==(mode==1||(mode==0&&n==0)),"explicit raw vs strict listener policy");
  for(unsigned owned=0;owned<2;++owned)
   C(EvidencePolicyAllows(mode,owned!=0)==(mode==0||(mode==1&&owned)),"raw evidence only new owned named input");
 }
 MainSnapshot a={},b={};a.manager=b.manager=1;a.group=b.group=2;
 a.archive=b.archive=3;a.module=b.module=4;a.profile=b.profile=5;
 C(SameSnapshot(a,b),"same strict-null metadata");
 b.listener=0x100000001ULL;
 C(!SameSnapshot(a,b),"listener replacement invalidates token");
 a.listener=b.listener;a.semantics=b.semantics=OwnedRawIndexedFile;
 C(SameSnapshot(a,b),"same explicit raw-indexed metadata");
 b.listener^=1ULL<<32;
 C(!SameSnapshot(a,b),"listener identity retains high32");
 b.listener=a.listener;b.semantics=NativeCompatible;
 C(!SameSnapshot(a,b),"raw source cannot silently become native-stream evidence");
 printf("PASS %u listener identity/semantics checks\n",checks);return 0;
 }catch(...){return 1;}}
