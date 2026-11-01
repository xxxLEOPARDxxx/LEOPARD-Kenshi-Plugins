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
    struct Placement {Rect panel,image;int dialogX,dialogY;};
    inline Placement PlacePreview(int x,int y,int width,int height,int viewW,int viewH,unsigned imageW,unsigned imageH,bool hasImage)
    {
        Placement p={};if(viewW<=28||viewH<=28||width<=12||height<=0)return p;
        const int maxW=(std::min)(width,viewW-16),maxH=(std::min)(viewH/2,viewH-height-20);
        if(maxW<=12||maxH<=12)return p;
        if(hasImage)
        {
            if(!imageW||!imageH)return p;
            // Storage stays bounded at 640x360. Display may scale up to the
            // dialog width; a short viewport caps height without cropping.
            const double scale=(std::min)(double(maxW-12)/imageW,double(maxH-12)/imageH);
            const Size s((std::max)(1U,unsigned(imageW*scale)),
                         (std::max)(1U,unsigned(imageH*scale)));
            if(!s.width||!s.height)return p;
            p.panel.width=maxW;p.panel.height=int(s.height)+12;
            p.image.x=(p.panel.width-int(s.width))/2;p.image.y=6;
            p.image.width=s.width;p.image.height=s.height;
        }
        else {p.panel.width=maxW;p.panel.height=(std::min)(maxH,48);}
        p.dialogX=(std::max)(8,(std::min)(x,viewW-width-8));
        p.dialogY=(std::max)(8,(std::min)(y,viewH-height-4-p.panel.height-8));
        p.panel.x=(std::max)(8,(std::min)(p.dialogX+(width-p.panel.width)/2,viewW-p.panel.width-8));
        p.panel.y=p.dialogY+height+4;return p;
    }
}
