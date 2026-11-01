#pragma once
#include <stdexcept>

// Records only the synchronous native alpha traversal, including skin children.
// WeakRef must provide get/set/release and be stable at its allocated address.
template<class Widget,class WeakRef> class SavePreviewAlpha
{
    struct Record
    {
        WeakRef ref;float alpha;
        Record():alpha(0){}~Record(){ref.release();}
    private:Record(const Record&);Record& operator=(const Record&);
    };
    Record* records;unsigned count;bool restored;
public:
    enum {Capacity=1024};
    SavePreviewAlpha():records(new Record[Capacity]),count(0),restored(false){}
    ~SavePreviewAlpha(){Restore();delete[]records;}
    void Remember(Widget* widget)
    {
        for(unsigned i=0;i<count;++i)if(records[i].ref.get()==widget)return;
        if(count==Capacity)throw std::runtime_error("save preview modal alpha budget exceeded");
        records[count].ref.set(widget);
        if(records[count].ref.get()!=widget)throw std::runtime_error("save preview widget lifetime unavailable");
        records[count].alpha=widget->getAlpha();++count;
    }
    bool Restore()
    {
        if(restored)return true;restored=true;bool ok=true;
        for(unsigned i=count;i>0;--i)
        {
            Widget* w=records[i-1].ref.get();if(!w)continue;
            try{w->setAlpha(records[i-1].alpha);}catch(...){ok=false;}
        }
        return ok;
    }
    unsigned Count()const{return count;}
private:SavePreviewAlpha(const SavePreviewAlpha&);SavePreviewAlpha& operator=(const SavePreviewAlpha&);
};
