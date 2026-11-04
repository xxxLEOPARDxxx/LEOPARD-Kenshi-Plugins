#include <cstdio>
#include <cstring>
static const unsigned Capacity=1024;
#ifdef BASELINE_FIXED_START
// The observed previous Main loop starts at visible index zero every frame.
struct CursorState{unsigned count;CursorState():count(0){}unsigned Cursor(unsigned c){count=c;return 0;}unsigned Index(unsigned scan)const{return scan<count?scan:Capacity;}bool Published(unsigned i){return i<count;}};
#else
#include "../../GsmFineShadeCursor.h"
typedef GsmFineShadeCursor::State CursorState;
#endif
static unsigned checks=0;
static void C(bool b,const char*s){++checks;if(!b){std::printf("FAIL %s\n",s);throw 1;}}
static void Frame(CursorState&cursor,unsigned count,unsigned epoch,unsigned*shaded,unsigned*seen){
 cursor.Cursor(count);unsigned done=0;bool indicesValid=true,publicationValid=true;
 for(unsigned pass=0;pass<2&&done<2;++pass)for(unsigned scan=0;scan<count&&done<2;++scan){unsigned i=cursor.Index(scan);if(i>=count){indicesValid=false;break;}if((pass==0)!=(shaded[i]==0)||shaded[i]==epoch)continue;shaded[i]=epoch;++seen[i];publicationValid=cursor.Published(i)&&publicationValid;++done;}
 C(indicesValid&&publicationValid,"scanned and published indices belong to current visible set");C(done<=2,"two passes share two-render frame cap");
}
static void Eventually(unsigned count){CursorState cursor;unsigned shaded[Capacity],seen[Capacity]={};for(unsigned i=0;i<count;++i)shaded[i]=1;
 for(unsigned frame=0;frame<(count+1)/2;++frame)Frame(cursor,count,2+frame/60,shaded,seen);
 for(unsigned i=0;i<count;++i)C(seen[i]!=0,"epoch changes at60FPS cannot starve any already-shaded visible tile");
}
int main(){try{Eventually(338);Eventually(512);Eventually(518);Eventually(1024);
 CursorState c;C(c.Cursor(0)==0&&c.Index(0)==Capacity&&!c.Published(0),"empty visible set cannot publish or divide by zero");
 C(c.Cursor(512)==0,"initial cursor starts at first visible index");C(c.Published(511),"last visible publication valid");C(c.Cursor(512)==0,"last index wraps");C(c.Published(500),"arbitrary successful index advances");C(c.Cursor(338)==501%338&&c.Index(337)==(501%338+337)%338,"visible count shrink normalizes saved cursor");
 C(!c.Published(338)&&c.Index(338)==Capacity&&c.Index(~0u)==Capacity,"invalid publication/scan rejected");
 C(c.Cursor(Capacity+1)==0&&c.Index(0)==Capacity&&!c.Published(0),"oversized set refuses rather than indexing beyond the mixed visible capacity");
 unsigned start=c.Cursor(5);C(c.Published(4)&&c.Index(0)==start,"publication leaves the current scan snapshot stable");C(c.Cursor(5)==0,"next pass starts after successful publication");
 unsigned shaded[Capacity],seen[Capacity]={};for(unsigned i=0;i<512;++i)shaded[i]=1;shaded[211]=shaded[432]=0;
 Frame(c,512,2,shaded,seen);C(shaded[211]==2&&shaded[432]==2,"new unshaded tiles win both slots before older dirty tiles");
 C(c.Cursor(0)==0&&c.Index(0)==Capacity,"hidden/empty transition is safe after publications");
 c.Cursor(1);C(c.Index(0)==0&&c.Published(0)&&c.Index(1)==Capacity,"single visible tile never escapes zero");
 CursorState stable;stable.Cursor(512);C(stable.Published(100)&&stable.Cursor(512)==101,"publication selects successor");C(stable.Cursor(512)==101,"failed or skipped work without publication cannot advance");
 const unsigned counts[]={1,2,338,512,518,1024};for(unsigned t=0;t<6;++t){bool visited[Capacity]={};unsigned n=counts[t];stable.Cursor(n);for(unsigned scan=0;scan<n;++scan){unsigned at=stable.Index(scan);C(at<n&&!visited[at],"round robin visits each current index exactly once");visited[at]=true;}C(stable.Index(n)==Capacity,"scan past current count refuses");}
 std::printf("PASS %u fair shade cursor controls sizeof=%u\n",checks,unsigned(sizeof(c)));return 0;}catch(...){return 1;}}
