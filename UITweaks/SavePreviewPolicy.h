#pragma once
#include <string>
#include <algorithm>

// Pure production boundaries, exercised without game objects or GPU access.
namespace SavePreviewPolicy
{
    static const unsigned MaxWidth=640, MaxHeight=360;
    static const unsigned MaxEncodedBytes=256*1024;
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
    inline Rect PlaceBelow(int x,int y,int width,int height,int viewW,int viewH,double aspect)
    {
        Rect r={0,0,0,0};if(viewW<=16||viewH<=16||width<=0||height<=0||aspect<=0)return r;
        r.width=(std::min)(width,viewW-16);r.height=(std::min)(int(r.width/aspect),viewH/2);
        r.x=(std::max)(8,(std::min)(x,viewW-r.width-8));
        // Caller moves only the load dialog upwards if needed; its own children remain intact.
        r.y=(std::max)(8,(std::min)(y+height+4,viewH-r.height-8));
        if(r.y<height+8){r.y=(std::min)(height+8,viewH-8);r.height=(std::max)(0,viewH-r.y-8);}
        return r;
    }
}
