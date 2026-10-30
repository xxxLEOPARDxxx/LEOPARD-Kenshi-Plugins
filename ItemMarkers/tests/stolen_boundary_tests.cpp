#include "../StolenPresentation.h"
#include <cstdio>
#include <stdexcept>

static int checks=0,failures=0;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; std::printf("FAIL line%d: %s\n",__LINE__,#x); } } while(0)
struct Item { bool actualStolen; bool detached; int owner; int holder; int quantity; };
struct Port {
    typedef Item ItemType;
    bool enabled,owned; Item* selected; int queries;
    Port() : enabled(true),owned(true),selected(NULL),queries(0) {}
    bool Enabled() const {return enabled;}
    bool PlayerOwned(void*) const {return owned;}
    Item* Selected(void*,const char*) const {return selected;}
    bool NativeStolen(Item* item) {++queries;return item->actualStolen || item->detached;}
};
struct Pickup {
    ItemMarkersStolen::DragState& state; Port& port; Item* source; Item* clone;
    bool split,fail,throwing; int calls; ItemMarkersStolen::IconState drag,ordinary;
    Pickup(ItemMarkersStolen::DragState& s,Port& p,Item* i) : state(s),port(p),source(i),clone(NULL),split(false),fail(false),throwing(false),calls(0) { ItemMarkersStolen::Reset(drag);ItemMarkersStolen::Reset(ordinary); }
    bool Original() {
        ++calls;
        if(throwing)throw std::runtime_error("native simulation");
        if(fail)return false;
        Item* held=source;
        if(split){--source->quantity;clone->quantity=1;held=clone;ItemMarkersStolen::SplitResult(state,&port,source,clone);}
        held->detached=true;
        ItemMarkersStolen::OnIcon(drag,state,held);
        Item other={true,false,7,1,1};ItemMarkersStolen::OnIcon(ordinary,state,&other);
        CHECK(ItemMarkersStolen::Show(ordinary,port,&other));
        return true;
    }
};
int main() {
    using namespace ItemMarkersStolen;
    DragState state={};Port port;Item clean={false,false,1,1,6};port.selected=&clean;
    DragState next=Capture(port,&port,"bag");Pickup pickup(state,port,&clean);
    CHECK(Run(state,next,pickup));CHECK(pickup.calls==1);CHECK(!state.active);
    CHECK(!Show(pickup.drag,port,&clean)); // Current production isStolen(true) turns true after detach.
    CHECK(clean.owner==1 && clean.holder==1 && clean.quantity==6);
    IconState cancelled={};clean.detached=false;OnIcon(cancelled,state,&clean);CHECK(!Show(cancelled,port,&clean));

    Item stolen={true,false,7,1,2};port.selected=&stolen;next=Capture(port,&port,"main");Pickup theft(state,port,&stolen);
    CHECK(Run(state,next,theft));CHECK(Show(theft.drag,port,&stolen));CHECK(stolen.owner==7);

    clean.detached=false;port.selected=&clean;Item clone={false,false,1,1,0};next=Capture(port,&port,"bag");Pickup split(state,port,&clean);split.split=true;split.clone=&clone;
    CHECK(Run(state,next,split));CHECK(clean.quantity==5 && clone.quantity==1);CHECK(!Show(split.drag,port,&clone));
    IconState remaining={};OnIcon(remaining,state,&clean);CHECK(!Show(remaining,port,&clean));

    port.owned=false;clean.detached=false;next=Capture(port,&port,"shop");CHECK(!next.active);Pickup foreign(state,port,&clean);
    CHECK(Run(state,next,foreign));CHECK(Show(foreign.drag,port,&clean));

    port.owned=true;clean.detached=false;next=Capture(port,&port,"main");Pickup failed(state,port,&clean);failed.fail=true;
    CHECK(!Run(state,next,failed));CHECK(!state.active);CHECK(!clean.detached);
    Pickup fault(state,port,&clean);fault.throwing=true;
    try{Run(state,next,fault);CHECK(false);}catch(const std::runtime_error&){CHECK(!state.active);}

    port.enabled=false;CHECK(!Capture(port,&port,"main").active);port.enabled=true;
    const int before=port.queries;for(int i=0;i<10000;++i)CHECK(!Show(pickup.drag,port,&clean));
    CHECK(port.queries==before); // Cached presentation must add no repeated native predicate calls.
    // A new/moved icon is initialized again: a drag snapshot never becomes an item-level exemption.
    clean.actualStolen=true;OnIcon(cancelled,state,&clean);CHECK(!cancelled.frozen);CHECK(Show(cancelled,port,&clean));clean.actualStolen=false;
    port.selected=NULL;CHECK(!Capture(port,&port,"main").active);port.selected=&clean;
    next=Capture(port,&port,"main");DragState foreignScope=next;foreignScope.gui=&pickup;
    SplitResult(foreignScope,&port,&clean,&clone);CHECK(foreignScope.held==&clean);
    IconState unrelated={};OnIcon(unrelated,next,&clone);CHECK(!unrelated.frozen);
    DragState previous=next;state=previous;Pickup nested(state,port,&clean);nested.throwing=true;
    try{Run(state,DragState(),nested);CHECK(false);}catch(const std::runtime_error&){CHECK(state.active==previous.active && state.held==previous.held);}
    state=DragState();IconState reused=pickup.drag;OnIcon(reused,state,&clone);CHECK(!reused.frozen);
    std::printf("%d checks, %d failures\n",checks,failures);return failures?1:0;
}
