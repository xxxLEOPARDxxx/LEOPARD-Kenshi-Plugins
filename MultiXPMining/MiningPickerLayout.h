#pragma once
#include <algorithm>
#include <cstddef>
namespace MiningPickerLayout {
    const int RowHeight=34;
    struct Layout {
        int left,width,rowWidth,listHeight;size_t rows;
        Layout(int w,int h) {
            left=std::min(20,std::max(0,w/8));width=std::max(0,w-2*left);
            rowWidth=std::max(0,width-34);
            rows=std::min(size_t(64),(size_t)std::max(0,h-94)/RowHeight);
            listHeight=(int)rows*RowHeight;
        }
    };
    inline size_t MaxTop(size_t count,size_t visible){return count>visible?count-visible:0;}
    inline size_t ClampTop(size_t top,size_t count,size_t visible){return std::min(top,MaxTop(count,visible));}
    inline size_t WheelTop(size_t top,int rel,size_t count,size_t visible) {
        top=ClampTop(top,count,visible);
        if(rel>0)return top>3?top-3:0;
        if(rel<0){const size_t remaining=MaxTop(count,visible)-top;return top+std::min(size_t(3),remaining);}
        return top;
    }
}
