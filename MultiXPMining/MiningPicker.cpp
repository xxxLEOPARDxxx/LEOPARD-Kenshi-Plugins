#define KLOC_DOMAIN "multi_xp_mining"
#include <PluginSettings.h>
#include <Localization.h>
#include <ModConfigMenu.h>
#include <WidgetRef.h>
#include <PluginDiagnostics.h>
#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_Button.h>
#include <mygui/MyGUI_EditBox.h>
#include <mygui/MyGUI_ScrollBar.h>
#include <kenshi/gui/OptionsWindow.h>
#include <kenshi/gui/ToolTip.h>
#include <algorithm>
#include <map>
#include "MiningCore.h"
#include "MiningPicker.h"
#include "MiningPickerLayout.h"

namespace {
    WidgetRef g_root;
    MyGUI::EditBox* g_search=NULL;
    MyGUI::ScrollBar* g_bar=NULL;
    std::vector<MiningCore::SourceInfo> g_sources;
    std::vector<std::wstring> g_names;
    std::vector<size_t> g_filtered;
    std::vector<MyGUI::Button*> g_entries;
    std::map<std::string,unsigned> g_duplicates;
    std::string g_selected,g_selectedName,g_filter;
    size_t g_top=0;

    std::wstring Fold(const std::string& text) {
        int n=MultiByteToWideChar(CP_UTF8,0,text.c_str(),-1,NULL,0);
        if(n<=0)return std::wstring();std::vector<wchar_t>b(n);
        MultiByteToWideChar(CP_UTF8,0,text.c_str(),-1,&b[0],n);CharLowerBuffW(&b[0],n-1);return &b[0];
    }
    bool NameLess(const MiningCore::SourceInfo& a,const MiningCore::SourceInfo& b) { return a.name==b.name ? a.sid<b.sid : a.name<b.name; }
    std::string Escape(const std::string& s) { std::string out;out.reserve(s.size());for(size_t i=0;i<s.size();++i){out+=s[i];if(s[i]=='#')out+='#';}return out; }
    void Refresh() {
        HMODULE m=GetModuleHandleA("ModConfigMenu.dll");
        MCM_OpenPageFn fn=m?(MCM_OpenPageFn)GetProcAddress(m,"MCM_OpenPage"):NULL;
        if(!fn) { ErrorLog("MultiXPMining: MCM_OpenPage unavailable; selected source parameters could not refresh"); return; }
        if(PluginDiagnostics::Enabled()) PluginDiagnostics::Write("MultiXPMining: source refresh requested ["+g_selected+"] "+g_selectedName);
        fn("MultiXPMining");
    }

    // Run only on attach/filter/scroll. UTF-32 truncation never splits UTF-8,
    // and escaping after truncation cannot leave half a MyGUI colour escape.
    void FitCaption(MyGUI::TextBox* box,const std::string& raw) {
        const MyGUI::UString text(raw);
        box->setCaption(Escape(raw));
        const int width=std::max(0,box->getTextRegion().width-6);
        if(box->getTextSize().width<=width)return;
        size_t low=0,high=text.size();
        while(low<high) {
            const size_t mid=low+(high-low+1)/2;
            box->setCaption(Escape(text.substr(0,mid).asUTF8())+"...");
            if(box->getTextSize().width<=width)low=mid;else high=mid-1;
        }
        box->setCaption(Escape(text.substr(0,low).asUTF8())+"...");
    }
    void PaintRows() {
        if(!g_root.get() || !g_bar)return;
        g_top=MiningPickerLayout::ClampTop(g_top,g_filtered.size(),g_entries.size());
        for(size_t row=0;row<g_entries.size();++row) {
            MyGUI::Button* b=g_entries[row];
            if(g_top+row>=g_filtered.size()) {b->setVisible(false);continue;}
            const size_t i=g_filtered[g_top+row];
            const MiningCore::SourceInfo& source=g_sources[i];
            const std::string label=source.name+(g_duplicates[source.name]>1 ? " ["+source.sid+"]" : "");
            b->setVisible(true);FitCaption(b,label);b->setStateSelected(source.sid==g_selected);
            OptionsWindow* options=OptionsWindow::getSingleton();
            if(options && options->tooltip)options->tooltip->setup(b,Escape(source.name+"\n"+source.sid));
            char n[32];sprintf_s(n,"%u",(unsigned)i);b->setUserString("source_index",n);
        }
        const size_t maximum=MiningPickerLayout::MaxTop(g_filtered.size(),g_entries.size());
        g_bar->setVisible(maximum>0);
        g_bar->setScrollRange(maximum+1);g_bar->setScrollPosition(g_top);
        g_bar->setScrollPage(1);g_bar->setScrollWheelPage(3);
        g_bar->setScrollViewPage(std::max(size_t(1),g_entries.size()));
        const int line=std::max(1,g_bar->getLineSize());
        g_bar->setMinTrackSize(std::min(line,36));
        const int track=g_filtered.empty()?line:(int)((size_t)line*g_entries.size()/g_filtered.size());
        g_bar->setTrackSize(std::max(1,track));
    }
    void Wheel(MyGUI::Widget*,int rel) {
        if(!g_root.get() || rel==0)return;
        g_top=MiningPickerLayout::WheelTop(g_top,rel,g_filtered.size(),g_entries.size());PaintRows();
    }
    void Scroll(MyGUI::ScrollBar* sender,size_t position) {
        if(!g_root.get() || sender!=g_bar)return;
        g_top=position;PaintRows();
    }
    void Select(MyGUI::Widget* widget) {
        if(!g_root.get() || std::find(g_entries.begin(),g_entries.end(),widget)==g_entries.end())return;
        const size_t i=(size_t)atoi(widget->getUserString("source_index").c_str());
        if(i>=g_sources.size())return;
        // MCM_OpenPage can defer or reject a request during options teardown.
        // A repeated click must retry, even when this ID is already selected.
        g_selected=g_sources[i].sid;g_selectedName=g_sources[i].name;
        if(PluginDiagnostics::Enabled()) PluginDiagnostics::Write("MultiXPMining: source clicked ["+g_selected+"] "+g_selectedName);
        Refresh();
    }
    void Filter() {
        const std::wstring filter=Fold(g_filter);g_filtered.clear();
        for(size_t i=0;i<g_sources.size();++i)
            if(filter.empty() || g_names[i].find(filter)!=std::wstring::npos)g_filtered.push_back(i);
        PaintRows();
    }
    void Search(MyGUI::EditBox* edit) {g_filter=edit->getOnlyText().asUTF8();g_top=0;Filter();}
}
namespace MiningPicker {
    std::string Selected() {
        MiningCore::Rule r;
        if(g_selected.empty() || !MiningCore::ReadSource(g_selected,r)) {
            const std::vector<MiningCore::SourceInfo> all=MiningCore::Sources();
            g_selected=all.empty()?std::string():all[0].sid;
            g_selectedName=all.empty()?std::string():all[0].name;
        }
        return g_selected;
    }
    std::string SelectedName() { Selected();return g_selectedName; }
    void Detach(void*) {
        MyGUI::Widget* w=g_root.get();
        g_root.reset();g_search=NULL;g_bar=NULL;g_entries.clear();
        if(w && MyGUI::Gui::getInstancePtr())MyGUI::Gui::getInstance().destroyWidget(w);
        g_sources.clear();g_names.clear();g_filtered.clear();g_duplicates.clear();
    }
    void Attach(void*,void* parent) {
        Detach(NULL);
        MyGUI::Widget* p=(MyGUI::Widget*)parent;if(!p)return;
        const MyGUI::IntSize size=p->getSize();
        MyGUI::Widget* root=p->createWidget<MyGUI::Widget>("PanelEmpty",MyGUI::IntCoord(0,0,size.width,size.height),MyGUI::Align::Default);
        g_root.set(root);g_sources=MiningCore::Sources();std::sort(g_sources.begin(),g_sources.end(),NameLess);g_names.reserve(g_sources.size());
        for(size_t i=0;i<g_sources.size();++i){g_names.push_back(Fold(g_sources[i].name+" "+g_sources[i].sid));++g_duplicates[g_sources[i].name];if(g_sources[i].sid==g_selected)g_selectedName=g_sources[i].name;}
        const MiningPickerLayout::Layout layout(size.width,size.height);
        MyGUI::TextBox* title=root->createWidget<MyGUI::TextBox>("Kenshi_TextboxStandardText",MyGUI::IntCoord(layout.left,12,layout.width,26),MyGUI::Align::Default);
        FitCaption(title,Tr("Select a mining source from the list. Search filters the visible names."));
        title->setNeedMouseFocus(false);
        g_search=root->createWidget<MyGUI::EditBox>("Kenshi_EditBox",MyGUI::IntCoord(layout.left,42,layout.width,32),MyGUI::Align::Default);
        g_search->setTextAlign(MyGUI::Align::Left|MyGUI::Align::VCenter);
        g_search->setMaxTextLength(128);g_search->setOnlyText(g_filter);g_search->eventEditTextChange+=MyGUI::newDelegate(&Search);
        MyGUI::Widget* viewport=root->createWidget<MyGUI::Widget>("PanelEmpty",MyGUI::IntCoord(layout.left,82,layout.rowWidth,layout.listHeight),MyGUI::Align::Default);
        viewport->eventMouseWheel+=MyGUI::newDelegate(&Wheel);
        g_bar=root->createWidget<MyGUI::ScrollBar>("Kenshi_ScrollBarV",MyGUI::IntCoord(layout.left+layout.rowWidth+6,82,28,layout.listHeight),MyGUI::Align::Default);
        g_bar->eventScrollChangePosition+=MyGUI::newDelegate(&Scroll);
        for(size_t row=0;row<layout.rows;++row) {
            MyGUI::Button* b=viewport->createWidget<MyGUI::Button>("Kenshi_Button1Skin",MyGUI::IntCoord(0,(int)row*MiningPickerLayout::RowHeight,layout.rowWidth,MiningPickerLayout::RowHeight-3),MyGUI::Align::Default);
            b->setFontName("Kenshi_PaintedTextFont_Medium");b->setFontHeight(20);b->setTextAlign(MyGUI::Align::Left|MyGUI::Align::VCenter);
            b->eventMouseButtonClick+=MyGUI::newDelegate(&Select);b->eventMouseWheel+=MyGUI::newDelegate(&Wheel);g_entries.push_back(b);
        }
        Filter();
    }
}
