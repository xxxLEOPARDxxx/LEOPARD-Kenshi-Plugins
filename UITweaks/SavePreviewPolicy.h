#pragma once
#include <string>
#include <algorithm>

// Pure production boundaries, exercised without game objects or GPU access.
namespace SavePreviewPolicy
{
    static const unsigned MaxWidth=1600, MaxHeight=900;
    static const unsigned MaxEncodedBytes=6*1024*1024;
    static const unsigned MaxReadbackPixels=3840*2160;
    struct Size { unsigned width,height; Size(unsigned w=0,unsigned h=0):width(w),height(h){} };
    inline Size Fit(unsigned w,unsigned h,unsigned maxW,unsigned maxH)
    {
        if(!w||!h||!maxW||!maxH)return Size();
        const double scale=(std::min)(1.0,(std::min)(double(maxW)/w,double(maxH)/h));
        return Size((std::max)(1U,unsigned(w*scale)),(std::max)(1U,unsigned(h*scale)));
    }
    inline bool AllowedReadback(unsigned w,unsigned h)
    {return w&&h&&static_cast<unsigned __int64>(w)*h<=MaxReadbackPixels;}
    inline bool ValidDimensions(unsigned w,unsigned h)
    {return w&&h&&w<=MaxWidth&&h<=MaxHeight;}
    inline bool ValidEncodedSize(unsigned n){return n&&n<=MaxEncodedBytes;}
    struct Pending
    {
        bool active,captured,complete;unsigned serial;std::string path;
        Pending():active(false),captured(false),complete(false),serial(0){}
        void Reset(){active=captured=complete=false;path.clear();}
        void Begin(const std::string& p,unsigned n){Reset();path=p;serial=n;active=true;}
        void Captured(){if(active)captured=true;}
        void Complete(bool success){if(!success)Reset();else if(active)complete=true;}
        bool Ready()const{return active&&captured&&complete;}
    };
    struct Rect {int x,y,width,height;};
    struct Placement {Rect panel,image;int dialogX,dialogY;};
    inline Placement PlacePreview(int x,int y,int width,int height,int viewW,int viewH,unsigned imageW,unsigned imageH,bool hasImage)
    {
        Placement p={};p.dialogX=x;p.dialogY=y;
        if(!hasImage||!imageW||!imageH||viewW<=28||viewH<=28||width<=12||height<=0)return p;
        const int below=y+height+4;
        if(below<8||below>=viewH-20)return p;
        const int maxW=(std::min)(width,viewW-16),maxH=viewH-below-8;
        if(maxW<=12||maxH<=12)return p;
        // Fit without cropping at the dialog's current position. Opening the
        // menu reserves this space once; selection changes never move it.
        const double scale=(std::min)(double(maxW-12)/imageW,double(maxH-12)/imageH);
        const Size fit((std::max)(1U,unsigned(imageW*scale)),(std::max)(1U,unsigned(imageH*scale)));
        p.panel.width=maxW;p.image.x=(maxW-int(fit.width))/2;p.image.y=6;
        p.image.width=fit.width;p.image.height=fit.height;
        p.panel.height=p.image.height+12;
        p.panel.x=(std::max)(8,(std::min)(x+(width-maxW)/2,viewW-maxW-8));
        p.panel.y=below;return p;
    }
    struct Point {int x,y;};
    inline Point AnchorPreviewDialog(int x,int y,int width,int height,int viewW,int viewH,bool reservePreview=true)
    {
        Point point={x,y};
        if(viewW<=28||viewH<=28)return point;
        if(!reservePreview){point.y=(std::min)(y,(std::max)(8,(viewH-height)/2));return point;}
        const Placement reserve=PlacePreview(x,8,width,height,viewW,viewH,viewW,viewH,true);
        if(!reserve.panel.height)return point;
        const int stack=height+4+reserve.panel.height;
        point.y=(std::min)(y,(std::max)(8,(viewH-stack)/2));
        return point;
    }
    struct DialogHeightState
    {
        int base,last;DialogHeightState():base(0),last(0){}
        int Next(int current,bool sameRoot,int percent,int width,int viewW,int viewH,bool reservePreview)
        {
            if(!sameRoot||current!=last||base<=0)base=current;
            if(base<=0||viewW<=28||viewH<=28){last=current;return current;}
            percent=(std::max)(100,(std::min)(percent,160));
            int cap=viewH-16;
            if(reservePreview)
            {
                const int panelW=(std::min)(width,viewW-16);
                const int reserve=panelW>12?int(double(panelW-12)*viewH/viewW)+12:0;
                cap=(std::min)(cap,(std::max)(base,viewH-20-reserve));
            }
            last=(std::max)(1,(std::min)(int(static_cast<unsigned __int64>(base)*percent/100),cap));
            return last;
        }
    };
}
