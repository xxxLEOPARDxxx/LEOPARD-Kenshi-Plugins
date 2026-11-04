#include <cstdlib>
#include <new>
#include <cstdio>
static unsigned allocations=0;
static unsigned long long allocatedBytes=0;static bool failAllocation=false;
void*operator new(size_t n)throw(std::bad_alloc){if(failAllocation){failAllocation=false;throw std::bad_alloc();}++allocations;allocatedBytes+=n;void*p=std::malloc(n?n:1);if(!p)throw std::bad_alloc();return p;}
void operator delete(void*p)throw(){std::free(p);}
void*operator new[](size_t n)throw(std::bad_alloc){return ::operator new(n);}
void operator delete[](void*p)throw(){::operator delete(p);}
#ifndef FINE_INDEX_HEADER
#define FINE_INDEX_HEADER "../../GsmFineIndex.h"
#endif
#include FINE_INDEX_HEADER
static unsigned checks=0;
static void C(bool value,const char*name){++checks;if(!value){std::printf("FAIL %s\n",name);std::exit(1);}}
static FineWindows::Key Key(unsigned spacing,unsigned x,unsigned z,FineWindows::U64 source=7,FineWindows::U64 material=9){FineWindows::Key k={spacing,x,z,{source,material}};return k;}
static FineWindows::Request View(double x,double z,double w,double h){FineWindows::Request r={{x,z,w,h},1024,1024,{7,9},false};return r;}
int main(){
 GsmFineIndex::Index index;C(index.Count()==0&&index.Revision()!=0,"empty initial revision");FineWindows::U64 first=index.Revision();
 C(index.Insert(Key(1,576,576),0),"insert actual1m key");C(index.Count()==1&&index.Find(Key(1,576,576))==0&&index.Revision()!=first,"lookup and revision after insertion");
 first=index.Revision();C(!index.Insert(Key(1,576,576),1)&&!index.Insert(Key(3,192,192),0),"duplicate key and duplicate physical slot refused");C(index.Count()==1&&index.Revision()==first,"refusals are transactional");
 C(index.Insert(Key(3,192,192),1),"mixed spacing same world rectangle");C(index.Insert(Key(1,576,576,7+(1ULL<<32)),2),"source generation high32 is independent");C(index.Insert(Key(1,576,576,7,9+(1ULL<<32)),3),"material generation high32 is independent");
 C(index.Find(Key(1,576,576,7+(1ULL<<32)))==2&&index.Find(Key(1,576,576,7,9+(1ULL<<32)))==3,"full64bit generation lookup");
 C(!index.Erase(Key(1,576,576),1)&&index.Find(Key(1,576,576))==0,"wrong-slot erase preserves entry");C(index.Erase(Key(1,576,576),0)&&index.Find(Key(1,576,576))==-1,"matching erase removes exact key");C(index.Insert(Key(1,576,576),0),"erased physical slot reusable");
 C(!index.Insert(Key(9,64,64),4)&&!index.Insert(Key(1,1152,0),4)&&!index.Insert(Key(3,384,0),4)&&!index.Insert(Key(1,0,0,0),4)&&!index.Insert(Key(1,0,0),GsmCacheProfile::MaxFineCacheSlots),"invalid fine key and slot bounds");
 first=index.Revision();unsigned priorCount=index.Count();failAllocation=true;C(!index.Insert(Key(1,17,23),42)&&index.Count()==priorCount&&index.Revision()==first&&index.Find(Key(1,17,23))==-1,"actual map allocation failure is transactional");C(index.Insert(Key(1,17,23),42),"same physical slot reusable after map OOM");C(index.Erase(Key(1,17,23),42),"erase recovered allocation");
 GsmFineIndex::Less less;C(less(Key(3,3,2),Key(3,0,3))&&!less(Key(3,0,3),Key(3,3,2))&&less(Key(1,576,576),Key(3,192,192)),"explicit spacing/z/x total order");
 unsigned result[192];for(unsigned n=0;n<192;++n)result[n]=99999;FineWindows::Request view=View(0,0,256,256);unsigned count=index.Visible(view,result,192);C(count==2&&result[0]==0&&result[1]==1,"visible mixed1m3m keys respect current stamp");
 C(index.Visible(view,result,191)==0&&result[0]==0,"short output capacity refuses without writes");C(index.Visible(view,NULL,192)==0,"null output refuses");
 view=View(256,0,256,256);count=index.Visible(view,result,192);C(count==1&&result[0]==1,"right boundary excludes previous1m tile");view=View(-256,0,256,256);C(index.Visible(view,result,192)==0,"touching left boundary is not overlap");
 view=View(0,-256,256,256);C(index.Visible(view,result,192)==0,"touching lower boundary is not overlap");
 index.Clear();C(index.Count()==0&&index.Find(Key(3,192,192))==-1,"Clear removes all entries");first=index.Revision();index.Clear();C(index.Revision()==first,"empty Clear has no spurious revision");
 C(index.Insert(Key(1,0,0),0)&&index.Insert(Key(3,0,0),1),"world corner keys");view=View(-147456.5,-147456.5,1,1);C(index.Visible(view,result,192)==2,"negative world boundary clipped before planner index");view=View(-147457,-147457,.5,.5);C(index.Visible(view,result,192)==0,"fully outside world returns empty");
 C(index.Insert(Key(1,1151,1151),2)&&index.Insert(Key(3,383,383),3),"last world corner keys");view=View(147455.5,147455.5,1,1);C(index.Visible(view,result,192)==2,"positive world boundary clipped");view=View(147456,147456,1,1);C(index.Visible(view,result,192)==0,"exact outer boundary excluded");
 FineWindows::Request bad=View(0,0,1,1);bad.world.x=HUGE_VAL;C(index.Visible(bad,result,192)==0,"infinite coordinate refuses");bad=View(0,0,1,1);bad.world.w=0;C(index.Visible(bad,result,192)==0,"zero extent refuses");bad=View(0,0,1,1);bad.stamp.material=0;C(index.Visible(bad,result,192)==0,"zero generation refuses");bad=View(0,0,1,1);bad.screenWidth=0;C(index.Visible(bad,result,192)==0,"zero screen refuses");bad=View(0,0,1,1);bad.screenHeight=16385;C(index.Visible(bad,result,192)==0,"oversized screen refuses");bad=View(0,0,1,1);bad.world.x=DBL_MAX;bad.world.w=DBL_MAX;C(index.Visible(bad,result,192)==0,"overflowed endpoint refuses");bad=View(0,0,1,1);bad.world.x=1e30;C(index.Visible(bad,result,192)==0,"lost-positive-span endpoint refuses");
 index.Clear();for(unsigned z=0;z<12;++z)for(unsigned x=0;x<12;++x){unsigned slot=z*12+x;C(index.Insert(Key(1,576+x,576+z),slot),"dense1m grid insert");C(index.Insert(Key(3,192+x,192+z),144+slot),"dense3m grid insert");}
 view=View(0,0,2304,2304);count=index.Visible(view,result,192);C(count==9,"worst1001m skipped while9 actual3m remain");
 // At this exact footprint Worst1=81, actual1=64; Worst3=16,actual3=9.
 view=View(0,0,2048,2048);count=index.Visible(view,result,192);C(count==73,"both feasible grids included exactly");
 view=View(0,0,100000,100000);C(index.Visible(view,result,192)==0,"both overly broad spacings skip");
 index.Clear();const unsigned maximum=GsmCacheProfile::MaxFineCacheSlots;const unsigned long long startBytes=allocatedBytes;for(unsigned n=0;n<maximum;++n)C(index.Insert(Key(1,n%1152,n/1152),n),"6144 bounded metadata entries");C(index.Count()==maximum,"physical metadata maximum admitted");C(index.MetadataBytes()>=allocatedBytes-startBytes,"node ledger covers measured VC10 map allocations");
 bool matches=true;for(unsigned n=0;n<maximum;++n)if(index.Find(Key(1,n%1152,n/1152))!=int(n))matches=false;C(matches,"all physical slots retrievable");C(!index.Insert(Key(3,10,10),maximum),"6145th physical slot rejected");C(index.MetadataBytes()<4ULL*1024*1024&&index.MetadataBytes()>maximum*32ULL,"metadata is conservative fewMiB not eager texture cache");
 unsigned before=allocations;view=View(-147456,-147456,2048,1024);for(unsigned n=0;n<10000;++n){index.Find(Key(1,n%1152,n/1152));index.Visible(view,result,192);}C(allocations==before,"Find and Visible allocate nothing");
 std::printf("PASS %u fine-index controls; maximum6144 metadata=%I64u bytes\n",checks,index.MetadataBytes());return 0;
}
