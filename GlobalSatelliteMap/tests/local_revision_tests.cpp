#include <Windows.h>
#include <cstdio>
#include <cstring>
#include "../GsmLocalRevision.h"
struct Value {float position[3],range,direction[3],power,diffuse[4],specular[4],attenuation[3],custom[4],spot[3];unsigned type;float effective;};
typedef char ValueShape[sizeof(Value)==112?1:-1];
struct Info {float SourceSceneOrigin[3];};
struct Snapshot {unsigned count,queries,nullIndex;long generation;bool valid;Info info;Value value[4097];Snapshot():count(1),queries(0),nullIndex(~0u),generation(7),valid(true){std::memset(&info,0,sizeof(info));std::memset(value,0,sizeof(value));}bool Valid(long g)const{return valid&&generation==g;}unsigned Count()const{return count;}const Info&Info()const{return info;}const Value*Light(unsigned i)const{++const_cast<Snapshot*>(this)->queries;return i==nullIndex?NULL:value+i;}};
static GsmLocalRevision::State<Value,4096> state;static Snapshot s;static unsigned checks=0,bad=0;
#define C(x) do{++checks;if(!(x)){++bad;std::printf("FAIL line %u: %s\n",__LINE__,#x);}}while(0)
int main(){C(state.Observe(s,7));C(!state.Observe(s,7));
 // Every copied word can change without count/time advancing (paused game).
 for(unsigned w=0;w<sizeof(Value)/4;++w){unsigned*words=reinterpret_cast<unsigned*>(&s.value[0]);++words[w];C(state.Observe(s,7));C(!state.Observe(s,7));--words[w];C(state.Observe(s,7));}
 s.info.SourceSceneOrigin[0]=-51296.75f;C(state.Observe(s,7));C(!state.Observe(s,7));
 s.count=4096;s.queries=0;C(state.Observe(s,7)&&s.queries==4096);C(!state.Observe(s,7));
 s.count=4097;s.queries=0;C(state.Observe(s,7)&&s.queries==0);C(!state.Observe(s,7)&&s.queries==0);
 s.count=4096;C(state.Observe(s,7));C(!state.Observe(s,7)); // restore identical count/data after refusal
 s.nullIndex=4095;C(state.Observe(s,7));s.nullIndex=~0u;C(state.Observe(s,7));C(!state.Observe(s,7));
 s.valid=false;C(state.Observe(s,7));C(!state.Observe(s,7));s.valid=true;C(state.Observe(s,7));
 s.generation=8;C(state.Observe(s,8));C(!state.Observe(s,8));C(state.Observe(s,7));C(state.Observe(s,8));
 s.count=0;C(state.Observe(s,8));C(!state.Observe(s,8));s.count=1;C(state.Observe(s,8));state.Reset();C(state.Observe(s,8));
 std::printf("PASS %u actual local revision checks, %u failed\n",checks,bad);return bad?1:0;}
