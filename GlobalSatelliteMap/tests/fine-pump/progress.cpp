#define BOOST_ALL_NO_LIB
#define BOOST_ERROR_CODE_HEADER_ONLY
#include "../../atlas-terrain/fine-coordinator/FineCoordinator.h"
#include <cstdio>
static unsigned checks=0;
static void C(bool b,const char*s){++checks;if(!b){std::printf("FAIL %s\n",s);throw 1;}}
#define FIELD(n) {FineCoordinator::Progress b=a;b.n=1;C(!FineCoordinator::SameProgress(a,b),#n " change is explicit progress");}
int main(){try{FineCoordinator::Progress a;
 C(FineCoordinator::SameProgress(a,a),"same progress ends ready-only batch");
 FIELD(nativeState);FIELD(view);FIELD(stage);FIELD(cursor);FIELD(input);FIELD(ownerFlags);FIELD(stored);FIELD(nativeWorkerState);FIELD(consumerWorkerState);FIELD(error);
 FineCoordinator::Progress b=a;b.nativeWorkerState=-1;C(!FineCoordinator::SameProgress(a,b),"signed asynchronous worker failure preserved");
 std::printf("PASS %u explicit progress snapshot controls sizeof=%u\n",checks,unsigned(sizeof(a)));return 0;
 }catch(...){return 1;}}
