#pragma once
#include <cstddef>
// Presentation-only drag scope. No engine fields, allocations or ownership writes.
namespace ItemMarkersStolen {
struct DragState { bool active; bool stolen; const void* gui; const void* source; const void* held; };
struct IconState { bool frozen; bool stolen; };
inline void Reset(IconState& icon) {icon.frozen=false;icon.stolen=false;}
template<class Port,class Section>
DragState Capture(Port& port,void* gui,const Section& section) {
    DragState state={};
    if(!port.Enabled() || !port.PlayerOwned(gui))return state;
    typename Port::ItemType* source=port.Selected(gui,section);
    if(source==NULL)return state;
    state.active=true;state.stolen=port.NativeStolen(source);state.gui=gui;state.source=source;state.held=source;
    return state;
}
inline void SplitResult(DragState& state,const void* gui,const void* source,const void* result) {
    if(state.active && state.gui==gui && state.source==source){state.held=result;if(result==NULL)state.active=false;}
}
inline void OnIcon(IconState& icon,const DragState& state,const void* item) {
    Reset(icon);
    if(state.active && item!=NULL && item==state.held){icon.frozen=true;icon.stolen=state.stolen;}
}
struct Scope {
    DragState& current;DragState previous;
    Scope(DragState& slot,const DragState& next) : current(slot),previous(slot){current=next;}
    ~Scope(){current=previous;}
private:Scope(const Scope&);Scope& operator=(const Scope&);
};
template<class Original>
bool Run(DragState& current,const DragState& next,Original& original) {Scope scope(current,next);return original.Original();}
template<class Port>
bool Show(const IconState& icon,Port& port,typename Port::ItemType* item) {
    return icon.frozen ? icon.stolen : port.NativeStolen(item);
}
}
