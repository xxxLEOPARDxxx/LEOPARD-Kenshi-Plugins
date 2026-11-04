#include "../../GsmFineWidgetSlots.h"
#include <cstdio>
static unsigned checks=0;
static void C(bool b,const char*s){++checks;if(!b){std::printf("FAIL %s\n",s);throw 1;}}
int main(){try{
 GsmFineWidgets::Slots s;
 const unsigned limit=GsmFineWidgets::MaxWidgets;
 C(limit==2*GsmCacheProfile::MaxVisible&&limit==1024&&s.Count()==0,"bounded empty visible pool");
 C(s.Find(0)==-1&&s.Owner(0)==-1,"empty maps absent both ways");
 C(s.Bind(GsmCacheProfile::MaxFineCacheSlots)==-1&&s.Find(~0U)==-1&&!s.Unbind(~0U)&&s.Owner(~0U)==-1,"invalid cache and pool ids refused");
 for(unsigned i=0;i<GsmFineWidgets::MaxWidgets;++i){int p=s.Bind(i);C(p==int(i)&&s.Find(i)==p&&s.Owner(p)==int(i),"firstfree bidirectional binding");}
 C(s.Count()==limit&&s.Bind(limit+1)==-1&&s.Find(limit+1)==-1,"full pool never grows or publishes failed bind");
 C(s.Bind(limit-1)==int(limit-1)&&s.Count()==limit,"repeated visible bind idempotent even full");
 for(unsigned slot=limit;slot<GsmCacheProfile::MaxFineCacheSlots;++slot){unsigned old=slot-limit;int p=s.Find(old);C(p>=0&&s.Unbind(old)&&s.Find(old)==-1&&s.Owner(p)==-1,"old visible owner reclaimed");int next=s.Bind(slot);C(next==p&&s.Owner(p)==int(slot)&&s.Count()==limit,"6144 unique visits reuse bounded visible slots");}
 C(!s.Unbind(0)&&s.Count()==limit,"absent unbind preserves other owners");
 s.Clear();C(s.Count()==0,"world or layer reset empties count");
 for(unsigned i=0;i<GsmCacheProfile::MaxFineCacheSlots;++i)C(s.Find(i)==-1,"reset removes every old mapping");
 for(unsigned i=0;i<GsmFineWidgets::MaxWidgets;++i)C(s.Owner(i)==-1,"reset removes all reverse owners");
 s.Clear();C(s.Bind(6143)==0&&s.Find(6143)==0&&s.Owner(0)==6143&&s.Count()==1,"last physical cache slot binds after repeated reset");
 C(s.Unbind(6143)&&s.Owner(0)==-1&&s.Count()==0,"last-slot release restores empty pool");
 C(sizeof(s)<=15*1024,"fixed CPU metadata stays below15KiB");
 std::printf("PASS %u visible-widget slots controls sizeof=%u\n",checks,unsigned(sizeof(s)));return 0;
 }catch(...){return 1;}}
