// SPDX-License-Identifier: GPL-3.0-only
#pragma once

// Call only on the GUI thread, after MyGUI/EditText are available.
namespace XpPanelTextStyle {
extern bool g_nativeProfile;
inline bool GetterMatches(HMODULE module,const char* name,const unsigned char* expected,size_t length)
{
    const unsigned char* base=reinterpret_cast<const unsigned char*>(module);
    const IMAGE_DOS_HEADER* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if(!base || dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<=0 || dos->e_lfanew>4096)return false;
    const IMAGE_NT_HEADERS* pe=reinterpret_cast<const IMAGE_NT_HEADERS*>(base+dos->e_lfanew);
    if(pe->Signature!=IMAGE_NT_SIGNATURE)return false;
    const ULONG_PTR begin=reinterpret_cast<ULONG_PTR>(base),end=begin+pe->OptionalHeader.SizeOfImage;
    const unsigned char* code=reinterpret_cast<const unsigned char*>(GetProcAddress(module,name));
    for(int hop=0;hop<4;++hop){
        const ULONG_PTR at=reinterpret_cast<ULONG_PTR>(code);
        if(!code || at<begin || at>end || end-at<length || end-at<5)return false;
        if(code[0]!=0xe9){for(size_t i=0;i<length;++i)if(code[i]!=expected[i])return false;return true;}
        const LONG relative=*reinterpret_cast<const LONG*>(code+1);code=code+5+relative;
    }
    return false;
}
inline bool InitializeNativeProfile()
{
    HMODULE module=GetModuleHandleW(L"MyGUIEngine_x64.dll");
    static const unsigned char nameGetter[]={0x48,0x8b,0x81,0xe0,0,0,0,0x48,0x83,0xc0,8,0xc3};
    static const unsigned char heightGetter[]={0x8b,0x81,0xf0,0,0,0,0xc3};
    g_nativeProfile=GetterMatches(module,"?getFontName@EditText@MyGUI@@UEBAAEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@XZ",nameGetter,sizeof(nameGetter)) &&
        GetterMatches(module,"?getFontHeight@EditText@MyGUI@@UEBAHXZ",heightGetter,sizeof(heightGetter));
    return g_nativeProfile;
}
inline bool HasFont(MyGUI::ISubWidgetText* text)
{
    if(!g_nativeProfile || !text || !text->isType<MyGUI::EditText>())return false;
    // Installed MyGUI's exported EditText::getFontName reads mFont at0xe0.
    // SDK protected fields differ by8 bytes here; do not use its mFont offset.
    // Read-only null guard; all font operations below use public virtual APIs.
    const char* native=reinterpret_cast<const char*>(static_cast<MyGUI::EditText*>(text));
    return *reinterpret_cast<void* const*>(native+0xe0)!=NULL;
}
struct MeasureCache {
    WidgetRef source,renderedSource;
    MyGUI::ISubWidgetText* text;const void* fontResource;MyGUI::UString caption;
    std::string face;int fontHeight,width,height,align,textHeight;
    bool shadow,wordWrap,renderedWordWrap;
    MeasureCache():text(NULL),fontResource(NULL),fontHeight(-1),width(-1),height(-1),align(-1),textHeight(-1),shadow(false),wordWrap(false),renderedWordWrap(false){}
};
inline MyGUI::ISubWidgetText* RenderedText(MyGUI::Widget* widget,MyGUI::Widget** owner=NULL)
{
    if(owner)*owner=NULL;
    MyGUI::Widget* chain[4];int count=0;
    while(widget && count<4){
        bool repeated=false;for(int i=0;i<count;++i)if(chain[i]==widget)repeated=true;
        if(repeated)break;
        chain[count++]=widget;widget=widget->getClientWidget();
    }
    // Composite EditBox text lives in its client; prefer that rendered text
    // to the outer TextBox's empty/nonvirtual font metadata. No child search.
    while(count){
        MyGUI::ISubWidgetText* text=chain[--count]->getSubWidgetText();
        if(HasFont(text)){if(owner)*owner=chain[count];return text;}
    }
    return NULL;
}
inline void Copy(MyGUI::Widget* destination,MyGUI::ISubWidgetText* source)
{
    if(!source)return;
    const int height=source->getFontHeight();
    const std::string& face=source->getFontName();
    if(height<=0 || face.empty())return;
    MyGUI::ISubWidgetText* target=RenderedText(destination);
    if(!target)return;
    if(target->getFontName()!=face)target->setFontName(face);
    if(target->getFontHeight()!=height)target->setFontHeight(height);
}
inline MyGUI::ISubWidgetText* Measure(MyGUI::Widget* source,MeasureCache& cache)
{
    MyGUI::Widget* rendered=NULL;
    MyGUI::ISubWidgetText* text=RenderedText(source,&rendered);
    if(!text)return NULL;
    const MyGUI::IntCoord& region=text->getCoord();
    const int fontHeight=text->getFontHeight();const std::string& face=text->getFontName();
    if(region.width<=0 || region.height<=0 || fontHeight<=0 || face.empty())return NULL;
    const MyGUI::Align alignment=text->getTextAlign();
    const int align=alignment.isBottom()?2:alignment.isVCenter()?1:0;
    const MyGUI::UString& caption=text->getCaption();const bool shadow=text->getShadow();
    MyGUI::EditBox* edit=source->castType<MyGUI::EditBox>(false);
    const bool wrap=edit && edit->getEditWordWrap();
    MyGUI::EditBox* renderedEdit=rendered->castType<MyGUI::EditBox>(false);
    const bool renderedWrap=rendered==source?wrap:renderedEdit && renderedEdit->getEditWordWrap();
    const void* font=*reinterpret_cast<void* const*>(reinterpret_cast<const char*>(static_cast<MyGUI::EditText*>(text))+0xe0);
    // Compare references/state without copies or allocation in stable frames.
    // Live refs invalidate on deletion even if a new source reuses its address.
    if(cache.source.get()!=source || cache.renderedSource.get()!=rendered || cache.text!=text || cache.fontResource!=font ||
       cache.caption!=caption || cache.shadow!=shadow || cache.wordWrap!=wrap || cache.renderedWordWrap!=renderedWrap || cache.face!=face || cache.fontHeight!=fontHeight ||
       cache.width!=region.width || cache.height!=region.height || cache.align!=align){
        cache.source.set(source);cache.renderedSource.set(rendered);cache.text=text;cache.fontResource=font;
        cache.caption=caption;cache.shadow=shadow;cache.wordWrap=wrap;cache.renderedWordWrap=renderedWrap;
        cache.face=face;cache.fontHeight=fontHeight;cache.width=region.width;cache.height=region.height;cache.align=align;
        cache.textHeight=text->getTextSize().height;
    }
    return text;
}
inline bool PaddingBefore(MyGUI::Widget* source,int absoluteBoundary,MeasureCache& cache)
{
    MyGUI::ISubWidgetText* text=Measure(source,cache);
    if(!text || cache.textHeight<=0 || text->getViewOffset().top!=0)return false;
    const MyGUI::IntCoord& region=text->getCoord();const int align=cache.align;
    int free=region.height-cache.textHeight;if(free<0)free=0;
    const int offset=align==2?free:align==1?free/2:0;
    int extent=offset+cache.textHeight;if(extent>region.height)extent=region.height;
    return text->getAbsolutePosition().top+extent<=absoluteBoundary;
}
}
