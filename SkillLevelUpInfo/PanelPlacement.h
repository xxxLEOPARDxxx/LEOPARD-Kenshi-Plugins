// SPDX-License-Identifier: GPL-3.0-only
#pragma once
// GUI thread only. Preserve native sizes; retain an authoritative baseline for
// each live widget and restore it without overwriting external native changes.
namespace XpPanelPlacement {
const int Limit=64;
struct Entry {
    WidgetRef widget;MyGUI::IntCoord original,applied;
};
struct Cache {
    WidgetRef parent;Entry entries[Limit];int count;unsigned int writes;
    Cache():count(0),writes(0){}
    void RestoreEntry(int i)
    {
        MyGUI::Widget* w=entries[i].widget.get();
        MyGUI::Widget* owner=parent.get();
        if(owner && w && w->getParent()==owner && w->getCoord()==entries[i].applied && w->getCoord()!=entries[i].original){w->setCoord(entries[i].original);++writes;}
    }
    void Restore()
    {
        for(int i=0;i<count;++i){RestoreEntry(i);entries[i].widget.reset();}
        count=0;parent.reset();
    }
    bool Capture(MyGUI::Widget* p,MyGUI::Widget** widgets,int size)
    {
        if(size<0 || size>Limit)return false;
        if(parent.get()!=p)Restore();
        bool same=size==count;
        for(int i=0;same && i<size;++i)same=entries[i].widget.get()==widgets[i];
        if(!same){
            // A new native visible-row set is an authoritative new layout.
            // Restore retired widgets only; shared widgets may have just been
            // relaid out by the game to positions equal to our prior compacting.
            for(int i=0;i<count;++i){bool retained=false;
                for(int j=0;j<size;++j)if(entries[i].widget.get()==widgets[j])retained=true;
                if(!retained)RestoreEntry(i);
                entries[i].widget.reset();
            }
            count=size;parent.set(p);
            for(int i=0;i<size;++i){entries[i].widget.set(widgets[i]);entries[i].original=entries[i].applied=widgets[i]->getCoord();}
        }else for(int i=0;i<size;++i){
            const MyGUI::IntCoord current=widgets[i]->getCoord();
            if(current!=entries[i].applied)entries[i].original=entries[i].applied=current;
        }
        return true;
    }
    MyGUI::IntCoord Original(MyGUI::Widget* w)
    {
        for(int i=0;i<count;++i)if(entries[i].widget.get()==w)return entries[i].original;
        return w->getCoord();
    }
    void Apply(MyGUI::Widget* w,const MyGUI::IntCoord& coord)
    {
        for(int i=0;i<count;++i)if(entries[i].widget.get()==w){
            if(w->getCoord()!=coord){w->setCoord(coord);++writes;}entries[i].applied=coord;return;
        }
    }
};
}
