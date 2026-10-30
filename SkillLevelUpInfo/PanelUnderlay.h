// SPDX-License-Identifier: GPL-3.0-only
// GUI-thread-only mapping of the verified stock StatsPanel atlas geometry.
// This is an asset profile, not colour detection: replacement artwork with the
// same resource/UV metadata cannot be distinguished. Other profiles keep the
// existing native progress layout. No texture/GPU/private object reads.
#ifndef XP_PANEL_UNDERLAY_H
#define XP_PANEL_UNDERLAY_H
namespace XpPanelUnderlay
{
    inline bool Near(float a,float b){return std::fabs(a-b)<.00001f;}
    inline bool StockUV(const MyGUI::FloatRect& uv)
    {
        return Near(uv.left,0) && Near(uv.top,907.f/1080) &&
               Near(uv.right,240.f/1920) && Near(uv.bottom,1072.f/1080);
    }
    // Legal protected member-pointer access, applied to a real SubSkin after
    // RTTI validation. No downcast to an invented derived runtime object.
    struct UvAccess:MyGUI::SubSkin
    {
        static MyGUI::FloatRect MyGUI::SubSkin::* Member(){return &UvAccess::mRectTexture;}
        static const MyGUI::FloatRect& Read(MyGUI::SubSkin* quad){return quad->*Member();}
    };
    inline bool NativeUvProfile()
    {
#ifdef XP_PANEL_FAKE_GUI
        return true;
#else
        // VC10 member layout is independently checked against native _setUVSet.
        // Unknown/replaced native bodies conservatively retain ca0 geometry.
        static const unsigned char prefix[]={0x48,0x83,0xec,0x78,0xf3,0x0f,0x10,0x41,0x50};
        static const bool compatible=XpPanelTextStyle::GetterMatches(GetModuleHandleW(L"MyGUIEngine_x64.dll"),
            "?_setUVSet@SubSkin@MyGUI@@UEAAXAEBU?$TRect@M@types@2@@Z",prefix,sizeof(prefix));
        return compatible;
#endif
    }
    inline bool StockResource()
    {
        MyGUI::ResourceManager* manager=MyGUI::ResourceManager::getInstancePtr();
        MyGUI::IResource* resource=manager?manager->getByName("Kenshi_StatsPanelSkin",false):NULL;
        MyGUI::ResourceSkin* skin=resource?resource->castType<MyGUI::ResourceSkin>(false):NULL;
        if(!skin || skin->getTextureName()!="Kenshi_UI.png")return false;
        const MyGUI::VectorSubWidgetInfo& basis=skin->getBasisInfo();
        if(basis.size()!=1 || basis[0].type!="SubSkin" || basis[0].coord.left || basis[0].coord.top ||
           basis[0].coord.width<=0 || basis[0].coord.height<=0)return false;
        const MyGUI::MapWidgetStateInfo& states=skin->getStateInfo();
        MyGUI::MapWidgetStateInfo::const_iterator normal=states.find("normal");
        if(normal==states.end() || normal->second.size()!=1 || !normal->second[0])return false;
        MyGUI::SubSkinStateInfo* state=normal->second[0]->castType<MyGUI::SubSkinStateInfo>(false);
        if(!state)return false;
        return StockUV(state->getRect());
    }
    struct Cache
    {
        WidgetRef source;
        MyGUI::ISubWidgetRect* main;
        bool known,applied;
        unsigned int resourceReads;
        Cache():main(NULL),known(false),applied(false),resourceReads(0){}
        bool Resolve(MyGUI::Widget* root,MyGUI::Widget* parent,int valueRight,int& left,int& right)
        {
            applied=false;MyGUI::Widget* background=NULL;MyGUI::ISubWidgetRect* quad=NULL;
            // The native content root can be nested in scroll/window clients.
            // Its nearest whole-widget background supplies the actual transform.
            MyGUI::Widget* w=root;
            for(int depth=0;w && depth<6;++depth,w=w->getParent()){
                MyGUI::ISubWidgetRect* q=w->getSubWidgetMain();
                if(!q || w->_getTextureName()!="Kenshi_UI.png")continue;
                const MyGUI::IntCoord& c=q->getCoord();
                if(c.left || c.top || c.width!=w->getWidth() || c.height!=w->getHeight() || c.width<=0 || c.height<=0)continue;
                if(w->getAbsoluteLeft()>root->getAbsoluteLeft() || w->getAbsoluteTop()>root->getAbsoluteTop() ||
                   w->getAbsoluteLeft()+c.width<root->getAbsoluteLeft()+root->getWidth() ||
                   w->getAbsoluteTop()+c.height<root->getAbsoluteTop()+root->getHeight())continue;
                background=w;quad=q;break;
            }
            if(!background){source.reset();main=NULL;known=false;return false;}
            if(source.get()!=background || main!=quad){
                source.set(background);main=quad;++resourceReads;known=StockResource();
            }
            if(!known || !NativeUvProfile())return false;
            MyGUI::SubSkin* actual=quad->castType<MyGUI::SubSkin>(false);
            // A globally registered stock resource cannot prove the selected
            // widget uses it. Validate its actual UV on each draw, including
            // state changes that retain the same widget/main pointer.
            if(!actual || !StockUV(UvAccess::Read(actual)))return false;
            // Stock atlas x150 is the boundary of the value backing inside its
            // x0..240 stretch quad. Round inward, then retain native value-right.
            const int mapped=background->getAbsoluteLeft()+(int)std::ceil(quad->getCoord().width*(150.0/240.0))-parent->getAbsoluteLeft();
            const int end=valueRight<parent->getWidth()-2?valueRight:parent->getWidth()-2;
            if(mapped<0 || end-mapped<8)return false;
            left=mapped;right=end;applied=true;return true;
        }
    };
}
#endif
