#include <cstdio>
#include <cstdlib>
#ifndef GEOGRAPHY_COMPLETION_HEADER
#define GEOGRAPHY_COMPLETION_HEADER "GeographyCompletion.h"
#endif
#include GEOGRAPHY_COMPLETION_HEADER
static unsigned checks=0;
static void Check(bool ok,const char* text){++checks;if(!ok){std::fprintf(stderr,"FAIL %s\n",text);std::exit(1);}}
// Native boundary model: notify credits, setLoaded changes state, unloaded
// partially credited resources cannot be evicted safely. Production uses this
// same template around actual allocation, streaming writes and native exports.
struct Boundary
{
    bool& uncertain;bool available,loaded,creator,partialFail,loadedFail;
    unsigned throwAt,creates,uploads,credits,finishes,order;
    Boundary(bool& u):uncertain(u),available(true),loaded(false),creator(true),partialFail(false),loadedFail(false),
        throwAt(0),creates(0),uploads(0),credits(0),finishes(0),order(0){}
    bool Available(){return available;}
    void Create(){Check(uncertain,"uncertainty latched before allocation");++creates;order=1;if(throwAt==1)throw 1;}
    bool Preflight(){Check(uncertain,"uncertainty latched during identity/Loaded check");order=order*10+2;return creator&&!loaded;}
    bool Upload(){Check(uncertain,"uncertainty latched through every upload");uploads=partialFail?3:24;order=order*10+3;if(throwAt==3)throw 3;return !partialFail;}
    void Notify(){Check(uncertain,"uncertainty latched before native credit");++credits;order=order*10+4;if(throwAt==4)throw 4;}
    void SetLoaded(){Check(uncertain,"uncertainty latched before setLoaded");++finishes;order=order*10+5;if(throwAt==5)throw 5;loaded=!loadedFail;}
    bool Loaded(){Check(uncertain,"uncertainty latched through final verification");order=order*10+6;if(throwAt==6)throw 6;return loaded;}
};
int main()
{
    using SatelliteGeography::CompleteBank;
    {bool u=false;Boundary b(u);Check(CompleteBank(b,u),"complete production bank");Check(!u&&b.order==123456&&b.credits==1&&b.finishes==1,"one credit and Loaded after all layers");
     b.available=false;Check(!CompleteBank(b,u)&&b.creates==1&&b.credits==1,"existing owned bank second attempt refused");}
    {bool u=false;Boundary b(u);b.available=false;Check(!CompleteBank(b,u)&&!u&&!b.creates,"name/ownership collision before allocation");}
    for(unsigned reason=0;reason<3;++reason)
    {bool u=false;Boundary b(u);if(reason==0)b.loaded=true;if(reason==1)b.creator=false;if(reason==2)b.partialFail=true;
     Check(!CompleteBank(b,u)&&u&&!b.credits&&!b.finishes,"Loaded/creator/partial-upload refusal never credits");
     Check(b.uploads==(reason==2?3u:0u),"ownership failures precede GPU writes");
     unsigned creates=b.creates,uploads=b.uploads;Check(!CompleteBank(b,u)&&b.creates==creates&&b.uploads==uploads,"uncertain bank cannot be retried or evicted");}
    for(unsigned stage=1;stage<=6;++stage)if(stage!=2)
    {bool u=false;Boundary b(u);b.throwAt=stage;bool threw=false;try{CompleteBank(b,u);}catch(unsigned value){threw=value==stage;}catch(int value){threw=unsigned(value)==stage;}
     Check(threw&&u,"allocation/upload/credit/finish/verify exception quarantines");
     Check(b.credits==(stage>=4?1u:0u)&&b.finishes==(stage>=5?1u:0u),"native stage stops after throwing with partial credit retained");
     b.throwAt=0;unsigned creates=b.creates,credits=b.credits;Check(!CompleteBank(b,u)&&b.creates==creates&&b.credits==credits,"partial-credit second attempt refused");}
    {bool u=false;Boundary b(u);b.loadedFail=true;Check(!CompleteBank(b,u)&&u&&b.credits==1&&b.finishes==1,"setLoaded verification failure retains credited resource");Check(!CompleteBank(b,u)&&b.credits==1,"failed Loaded cannot credit again");}
    std::printf("PASS %u production geographic allocation/upload/ownership/accounting/quarantine checks\n",checks);
}
