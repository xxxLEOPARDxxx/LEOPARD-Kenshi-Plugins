// SPDX-License-Identifier: GPL-3.0-only
// XP Info feature design: semper1dem (Alexander), Nexus mod 2160.
// Independent MCM implementation; see README.txt and XP_INFO_PARITY.md.
#define WIN32_LEAN_AND_MEAN
#define KLOC_DOMAIN "skill_level_up_info"
#include <PluginSettings.h>
#include <PluginDiagnostics.h>
#include <Localization.h>
#include <ModConfigMenu.h>
#include <Debug.h>
#include <core/Functions.h>
#include <kenshi/Globals.h>
#include <kenshi/GameWorld.h>
#include <kenshi/PlayerInterface.h>
#include <kenshi/SaveFileSystem.h>
#include <kenshi/Character.h>
#include <kenshi/CharStats.h>
#include <kenshi/gui/CharacterStatsWindow.h>
#include <kenshi/gui/ForgottenGUI.h>
#include <kenshi/gui/ScreenLabel.h>
#include <kenshi/gui/DatapanelGUI.h>
#include <kenshi/gui/DataPanelLine.h>
#include <mygui/MyGUI.h>
#include <WidgetRef.h>
#include <mygui/MyGUI_EditText.h>
#include <mygui/MyGUI_ResourceManager.h>
#include <mygui/MyGUI_ResourceSkin.h>
#include <mygui/MyGUI_CommonStateInfo.h>
#include <mygui/MyGUI_SubSkin.h>
#include "HeadAnchor.h"
#include "XpInfoCore.h"
#include "PanelTextStyle.h"
#include <map>
#include <string>
#include <sstream>
#include <cmath>
#include <algorithm>

bool XpPanelTextStyle::g_nativeProfile=false;
typedef char CharStatsAbiSizeMustMatchOriginal[(sizeof(CharStats)==0x230)?1:-1];
namespace
{
    bool g_enabled = true, g_selectedOnly = false, g_log = false;
    bool g_floating = true, g_levels = true, g_bars = true;
    int g_maxRows = 2;
    float g_minPercent = 1;
    DWORD g_emit = 1000, g_recent = 120000, g_uiThread = 0;
    CRITICAL_SECTION g_lock;
    volatile LONG g_trackEnabled = 0;
    struct Lock { Lock(){EnterCriticalSection(&g_lock);}~Lock(){LeaveCriticalSection(&g_lock);} };
    const StatsEnumerated kStats[] = {
        STAT_STRENGTH, STAT_TOUGHNESS, STAT_DEXTERITY, STAT_PERCEPTION,
        STAT_KATANAS, STAT_SABRES, STAT_HACKERS, STAT_HEAVYWEAPONS, STAT_BLUNT, STAT_POLEARMS,
        STAT_MELEE_ATTACK, STAT_MELEE_DEFENCE, STAT_DODGE, STAT_MARTIALARTS,
        STAT_TURRETS, STAT_CROSSBOWS, STAT_FRIENDLY_FIRE,
        STAT_STEALTH, STAT_LOCKPICKING, STAT_THIEVING, STAT_ASSASSINATION,
        STAT_ATHLETICS, STAT_SWIMMING, STAT_MEDIC, STAT_ENGINEERING, STAT_ROBOTICS,
        STAT_SCIENCE, STAT_SMITHING_WEAPON, STAT_SMITHING_ARMOUR, STAT_SMITHING_BOW,
        STAT_LABOURING, STAT_FARMING, STAT_COOKING,
        STAT_HIVEMEDIC, STAT_VET, STAT_MASSCOMBAT, STAT_SURVIVAL
    };
    const size_t kCount = sizeof(kStats) / sizeof(kStats[0]);
    const char* const kStatNames[kCount] = {
        "Strength", "Toughness", "Dexterity", "Perception",
        "Katanas", "Sabres", "Hackers", "Heavy weapons", "Blunt", "Polearms",
        "Melee attack", "Melee defence", "Dodge", "Martial arts",
        "Turrets", "Crossbows", "Precision shooting",
        "Stealth", "Lockpicking", "Thievery", "Assassination",
        "Athletics", "Swimming", "Medic", "Engineering", "Robotics",
        "Science", "Weapon smith", "Armour smith", "Crossbow smith",
        "Labouring", "Farming", "Cooking",
        "Hive medic", "Vet", "Mass combat", "Survival"
    };
    struct Snapshot {
        XpInfoCore::Skill skills[kCount]; float* refs[kCount]; bool live,pending;
        Snapshot():live(false),pending(false){for(size_t i=0;i<kCount;++i)refs[i]=NULL;}
    };
    std::map<hand, Snapshot> g_seen;
    struct Binding {
        XpInfoCore::Skill* skill;Snapshot* owner;size_t statIndex;unsigned __int64 token;
        Binding():skill(NULL),owner(NULL),statIndex(0),token(0){}
        Binding(Snapshot* snapshot,size_t index,unsigned __int64 serial):
            skill(&snapshot->skills[index]),owner(snapshot),statIndex(index),token(serial){}
        bool Same(const Binding& b)const{return token==b.token && owner==b.owner && statIndex==b.statIndex && skill==b.skill;}
    };
    std::map<float*, Binding> g_index;
    unsigned __int64 g_nextBindingToken=0;
    void RemoveRef(float* ref,XpInfoCore::Skill* skill)
    {
        if(!ref)return;
        std::map<float*,Binding>::iterator i=g_index.find(ref);
        if(i!=g_index.end() && i->second.skill==skill)g_index.erase(i);
    }
    DWORD g_last = 0;
    volatile LONG g_loadGeneration = 0;
    LONG g_seenGeneration = 0;
    WidgetRef g_panelRoot, g_divider;
    DatapanelGUI* g_panel = NULL;
    int g_panelCategory = 0;
    hand g_panelCharacter;
    XpInfoCore::Slots g_slots;
    bool g_panelDirty = true;
    int g_rowSkill[5]={-1,-1,-1,-1,-1};
    float g_rowValue[5]={0};
    int g_diagRows=-1,g_diagBottom=-1,g_diagWidth=-1;
    int g_diagParentHeight=-1,g_diagValueRight=-1,g_diagCapacity=-1;
    const char* g_diagPanelStatus=NULL;
    void PanelStatus(const char* status)
    {
        if(!PluginDiagnostics::Enabled() || g_diagPanelStatus==status)return;
        DebugLog(std::string("SkillLevelUpInfo: panel status=")+status);
        g_diagPanelStatus=status;
    }
    void Reset()
    {
        Lock lock;
        g_index.clear(); g_seen.clear(); g_last=0;
        g_slots=XpInfoCore::Slots(); g_panelDirty=true;
    }
    void LoadSettings()
    {
        InterlockedIncrement(&g_loadGeneration);
        const std::string p = PluginSettings::Path();
        g_enabled = PluginSettings::Bool("SkillLevelUpInfo", "Enabled", true, p);
        g_selectedOnly = PluginSettings::Bool("SkillLevelUpInfo", "SelectedOnly", false, p);
        g_log = PluginSettings::Bool("SkillLevelUpInfo", "LogToGame", false, p);
        g_floating = PluginSettings::Bool("SkillLevelUpInfo", "FloatingLabels", true, p);
        g_levels = PluginSettings::Bool("SkillLevelUpInfo", "LevelUpLabels", true, p);
        g_bars = PluginSettings::Bool("SkillLevelUpInfo", "PanelBars", true, p);
        g_maxRows = (int)PluginSettings::Number("SkillLevelUpInfo", "MaxRows", 2, 1, 2, p);
        g_minPercent = PluginSettings::Number("SkillLevelUpInfo", "MinPercent", 1, 0, 100, p);
        g_emit = (DWORD)(PluginSettings::Number("SkillLevelUpInfo", "AnnounceEverySeconds", 1, .1f, 60, p)*1000);
        g_recent = (DWORD)(PluginSettings::Number("SkillLevelUpInfo", "RecentSeconds", 120, 1, 3600, p)*1000);
        PluginDiagnostics::Configure("SkillLevelUpInfo", p);
        if (PluginDiagnostics::Enabled())
        {
            std::ostringstream s;s<<"SkillLevelUpInfo: settings enabled="<<g_enabled<<" selected="<<g_selectedOnly
                <<" gains="<<g_floating<<" levels="<<g_levels<<" bars="<<g_bars<<" rows="<<g_maxRows
                <<" min="<<g_minPercent<<" emitMs="<<g_emit<<" recentMs="<<g_recent<<" gameLog="<<g_log;
            DebugLog(s.str());
        }
        // Switching the notification scope never announces old gains.
        InterlockedExchange(&g_trackEnabled, 0);
        Reset();
        InterlockedExchange(&g_trackEnabled, g_enabled ? 1 : 0);
    }
    std::string Name(size_t index)
    {
        CharacterStatsWindow::Stat* const s = CharacterStatsWindow::getStat(kStats[index]);
        return s != NULL && !s->name.empty() ? s->name : Tr(kStatNames[index]);
    }
    void Emit(Character* c,const hand& h,const std::string& text,bool level)
    {
        if(text.empty())return;
        ScreenLabel* const label=gui->createScreenLabel(text,
            level?MyGUI::Colour(1,.84f,.35f):MyGUI::Colour(.62f,.86f,1),
            level?ScreenLabel::LS_MEDIUM:ScreenLabel::LS_SMALL,ScreenLabel::RS_NORMAL);
        if(label)label->setTracking(h,SkillHeadAnchor::TrackingOffset(c));
        if(PluginDiagnostics::Enabled())DebugLog("SkillLevelUpInfo: "+c->displayName+" -> "+text);
        if(g_log && level)ou->showPlayerAMessage_withLog(c->displayName+": "+text,true);
    }
    void Append(std::string& text,int& count,const std::string& line)
    {
        if(count<4){if(count)text+='\n';text+=line;}++count;
    }
    void Finish(std::string& text,int count)
    {
        if(count>4){
            char n[16];sprintf_s(n,"%d",count-4);
            std::string suffix=Tr("+{1} more");const size_t place=suffix.find("{1}");
            if(place!=std::string::npos)suffix.replace(place,3,n);
            text+='\n';text+=suffix;
        }
    }
    // Worker hooks only touch synchronized scalar state; all native/UI access
    // stays in the GUI thread. The reference index rejects unrelated stats first.
    void (*g_origIncrease)(float&,float,float)=NULL;
    void Increase(float& value,float amount,float limit)
    {
        const LONG generation=InterlockedCompareExchange(&g_loadGeneration,0,0);
        Binding captured;
        if(InterlockedCompareExchange(&g_trackEnabled,0,0)){
            Lock lock;
            if(generation==g_seenGeneration && generation==InterlockedCompareExchange(&g_loadGeneration,0,0)){
                std::map<float*,Binding>::iterator it=g_index.find(&value);
                if(it!=g_index.end())captured=it->second;
            }
        }
        const float before=value;
        g_origIncrease(value,amount,limit);
        const float after=value;
        if(!(after>before) || !captured.token || !InterlockedCompareExchange(&g_trackEnabled,0,0))return;
        Lock lock;
        if(generation!=g_seenGeneration || generation!=InterlockedCompareExchange(&g_loadGeneration,0,0))return;
        std::map<float*,Binding>::iterator it=g_index.find(&value);
        if(it!=g_index.end() && captured.Same(it->second)){
            captured.skill->Observe(after,GetTickCount());
            if(captured.skill->pending>0)captured.owner->pending=true;
        }
    }
    void ProcessNotifications()
    {
        if(!g_enabled || !ou || !ou->player || !gui)return;
        const DWORD now=GetTickCount();
        for(std::map<hand,Snapshot>::iterator it=g_seen.begin();it!=g_seen.end();++it){
            int events[kCount]={0};float values[kCount],gains[kCount];bool ready=false;
            {
                Lock lock;Snapshot& state=it->second;
                if(!state.pending)continue;
                state.pending=false;
                for(size_t j=0;j<kCount;++j){
                    XpInfoCore::Skill& s=state.skills[j];
                    values[j]=s.value;gains[j]=s.pending*100;
                    events[j]=s.Take(now,g_minPercent,g_emit);
                    if(events[j])ready=true;
                    if(s.pending>0)state.pending=true;
                }
            }
            if(!ready)continue;
            Character* c=it->first.getCharacter();
            if(!c || c->isDead() || !c->isPlayerCharacter() || (g_selectedOnly && !ou->player->isObjectSelected(c)))continue;
            std::string levels,gain;int levelCount=0,gainCount=0;
            for(size_t j=0;j<kCount;++j){
                if(events[j]==2 && g_levels){std::ostringstream s;s<<Name(j)<<" "<<(int)std::floor(values[j]);Append(levels,levelCount,s.str());}
                else if(events[j]==1 && g_floating){char n[32];sprintf_s(n,"+%.0f%% ",gains[j]);Append(gain,gainCount,std::string(n)+Name(j));}
            }
            Finish(levels,levelCount);Finish(gain,gainCount);
            Emit(c,it->first,levels,true);Emit(c,it->first,gain,false);
        }
    }
    void Scan()
    {
        if (!g_enabled) return;
        if (ou == NULL || ou->player == NULL || gui == NULL) { Reset(); return; }
        const DWORD now=GetTickCount();
        for (std::map<hand, Snapshot>::iterator i = g_seen.begin(); i != g_seen.end(); ++i)
            i->second.live = false;
        const lektor<Character*>& characters = ou->player->playerCharacters;
        for (unsigned i = 0; i < characters.size(); ++i)
        {
            Character* const c = characters[i];
            if (c == NULL || c->isDead()) continue;
            CharStats* const stats = c->getStats();
            if (stats == NULL) continue;
            const hand h(c);
            {
                Lock lock;
                Snapshot& state=g_seen[h];state.live=true;
                for(size_t j=0;j<kCount;++j)
                {
                    float* ref=&stats->getStatRef(kStats[j]);
                    const ULONG_PTR address=reinterpret_cast<ULONG_PTR>(ref);
                    const ULONG_PTR base=reinterpret_cast<ULONG_PTR>(stats);
                    bool valid=address>=base && address+sizeof(float)<=base+sizeof(CharStats) && !(address%sizeof(float));
                    for(size_t previous=0;previous<j && valid;++previous)
                        if(state.refs[previous]==ref)valid=false;
                    if(!valid){
                        RemoveRef(state.refs[j],&state.skills[j]);state.refs[j]=NULL;
                        state.skills[j]=XpInfoCore::Skill();continue;
                    }
                    std::map<float*,Binding>::iterator registration=g_index.find(ref);
                    if(state.refs[j]!=ref || registration==g_index.end() || registration->second.owner!=&state || registration->second.statIndex!=j){
                        RemoveRef(state.refs[j],&state.skills[j]);
                        state.refs[j]=ref;g_index[ref]=Binding(&state,j,++g_nextBindingToken);
                    }
                    XpInfoCore::Skill& s=state.skills[j];
                    s.Observe(stats->getStat(kStats[j],true),now);
                    if(s.pending>0)state.pending=true;
                }
            }
        }
        {
            Lock lock;
            for (std::map<hand, Snapshot>::iterator i = g_seen.begin(); i != g_seen.end();)
                if (!i->second.live){for(size_t j=0;j<kCount;++j)RemoveRef(i->second.refs[j],&i->second.skills[j]);g_seen.erase(i++);}else ++i;
        }
        ProcessNotifications();
    }
    const std::string& SlotKey(int i)
    {
        static const std::string keys[5]={"skill_xp_info_slot_0","skill_xp_info_slot_1","skill_xp_info_slot_2","skill_xp_info_slot_3","skill_xp_info_slot_4"};
        return keys[i];
    }
    #include "PanelPlacement.h"
    #include "PanelUnderlay.h"
    XpPanelPlacement::Cache g_placement;
    XpPanelUnderlay::Cache g_underlay;
    XpPanelTextStyle::MeasureCache g_nameMeasure,g_valueMeasure;
    XpPanelTextStyle::MeasureCache g_percentMeasure[5];
    WidgetRef g_labels[5],g_labelSources[5];
    // Main-thread scalar counters; the detailed ring is populated only with
    // Debug enabled. A bounded read-only probe can distinguish native rebuilds
    // from our context retirement without frame-by-frame log output.
    unsigned int g_panelCalls=0,g_contextResets=0,g_progressCreates=0;
    unsigned int g_dividerCreates=0,g_dividerContextHides=0,g_dividerParentDestroys=0;
    struct PanelTrace
    {
        volatile LONG sequence;DWORD tick;unsigned int contextMask;LONG generation;
        UINT_PTR panel,root,owner;int category,combat;
        UINT_PTR before[2],after[2],dividerBefore,dividerNative,dividerAfter;
    };
    typedef char PanelTraceAbi[(sizeof(PanelTrace)==104)?1:-1];
    PanelTrace g_panelTrace[32];unsigned int g_panelTraceHead=0;
    std::string g_labelSkins[5],g_labelTypes[5];
    unsigned int g_labelCreates=0,g_labelDestroys=0,g_labelTypeReads=0;
    void HideLabels()
    {
        for(int i=0;i<5;++i)if(g_labels[i].get() && g_labels[i].get()->getVisible())g_labels[i].get()->setVisible(false);
    }
    void HidePanel(DatapanelGUI* panel,int category)
    {
        g_placement.Restore();HideLabels();
        for(int i=0;i<5;++i){
            const std::string& key=SlotKey(i);
            if(panel->lineExists(key,category)){
                DataPanelLine* line=panel->getLine(key,category);
                if(line && line->getType()==DataPanelLine::DPL_PROGRESS){
                    DataPanelLine_Progress* row=static_cast<DataPanelLine_Progress*>(line);
                    if(row->progress && row->progress->getVisible())row->setVisible(false);
                }
            }
        }
        if(g_divider.get())g_divider.get()->setVisible(false);
    }
    MyGUI::TextBox* NativeLabel(int slot,MyGUI::Widget* parent,MyGUI::TextBox* source,const std::string& skin,
                               const MyGUI::IntCoord& coord,bool& created)
    {
        created=false;if(!source || skin.empty())return NULL;
        MyGUI::Widget* label=g_labels[slot].get();
        if(!label || label->getParent()!=parent || g_labelSources[slot].get()!=source || g_labelSkins[slot]!=skin){
            // Slot zero already validated this live source/skin in this draw.
            // Native RU refresh can replace the source every frame; two labels
            // must not repeat the same RTTI query for that one binding.
            const bool shared=slot>0 && g_labelSources[0].get()==source && g_labelSkins[0]==skin && !g_labelTypes[0].empty();
            if(!shared)++g_labelTypeReads;
            const std::string& type=shared?g_labelTypes[0]:source->getTypeName();
            if(!label || label->getParent()!=parent || g_labelSkins[slot]!=skin || g_labelTypes[slot]!=type){
                if(label){MyGUI::Gui::getInstance().destroyWidget(label);++g_labelDestroys;}
                label=parent->createWidgetT(type,skin,coord,MyGUI::Align::Default);
                if(label)++g_labelCreates;
                g_labels[slot].set(label);created=true;
                // Native source may be an EditBox with a rendered TextBox client.
                // This column is one line; wrapping long translated names would
                // silently create a second line outside its measured native row.
                MyGUI::EditBox* edit=label?label->castType<MyGUI::EditBox>(false):NULL;
                if(edit){edit->setEditStatic(true);edit->setEditWordWrap(false);}
                MyGUI::Widget* chain[4];int count=0;MyGUI::Widget* current=label;
                while(current && count<4){bool repeated=false;
                    for(int n=0;n<count;++n)if(chain[n]==current)repeated=true;
                    if(repeated)break;chain[count++]=current;
                    current->setNeedMouseFocus(false);current->setNeedKeyFocus(false);current->setInheritsPick(false);
                    current=current->getClientWidget();
                }
            }
            g_labelSources[slot].set(source);g_labelSkins[slot]=skin;g_labelTypes[slot]=type;
        }
        if(label && label->getCoord()!=coord)label->setCoord(coord);
        // The factory type is taken from an existing native TextBox (possibly
        // EditBox-derived), so it has the same TextBox base and virtual caption.
        return static_cast<MyGUI::TextBox*>(label);
    }
    struct NativeRow {
        DataPanelLine* line;MyGUI::IntCoord name,value;int top,bottom;
    };
    bool RowBefore(const NativeRow& a,const NativeRow& b){return a.top<b.top;}
    unsigned int ContextMask(DatapanelGUI* panel,MyGUI::Widget* root,const hand& owner,int category)
    {
        return (panel!=g_panel?1:0)|(root!=g_panelRoot.get()?2:0)|(!(owner==g_panelCharacter)?4:0)|(category!=g_panelCategory?8:0);
    }
    void DrawPanel(CharStats* stats,DatapanelGUI* panel,int category)
    {
        if((!g_enabled || !g_bars) && !g_panelDirty)return;
        if(!panel || !stats || !stats->me){PanelStatus("missing native panel/stat owner");return;}
        MyGUI::Widget* root=panel->getWidget();if(!root){PanelStatus("missing native root widget");return;}
        const hand owner(stats->me);
        if(ContextMask(panel,root,owner,category)){
            ++g_contextResets;
            g_placement.Restore();HideLabels();g_slots=XpInfoCore::Slots();g_panelCharacter=owner;
            for(int i=0;i<5;++i)g_rowSkill[i]=-1;
            // Ineligible/NPC/quiet panels can interleave with the main panel.
            // Preserve ownership and reuse its divider on return; a genuinely
            // different parent is retired below when another divider is needed.
            if(g_divider.get() && g_divider.get()->getVisible()){g_divider.get()->setVisible(false);++g_dividerContextHides;}
        }
        g_panelRoot.set(root);g_panel=panel;g_panelCategory=category;
        if(!g_enabled || !g_bars){PanelStatus("disabled");HidePanel(panel,category);g_panelDirty=false;return;}
        if(!stats->me->isPlayerCharacter()){PanelStatus("non-player panel");HidePanel(panel,category);return;}
        if(InterlockedCompareExchange(&g_loadGeneration,0,0)!=g_seenGeneration){PanelStatus("waiting for session baseline");HidePanel(panel,category);return;}
        XpInfoCore::Skill skills[kCount];
        {
            Lock lock;std::map<hand,Snapshot>::iterator found=g_seen.find(owner);
            if(found==g_seen.end()){PanelStatus("waiting for character baseline");HidePanel(panel,category);return;}
            for(size_t i=0;i<kCount;++i)skills[i]=found->second.skills[i];
        }
        const DWORD now=GetTickCount();bool recent=false;
        for(size_t i=0;i<kCount;++i)if(skills[i].Recent(now,g_recent))recent=true;
        if(!recent){g_slots.Select(skills,(int)kCount,now,g_recent,0);PanelStatus("no recent XP for this character");HidePanel(panel,category);return;}
        // Only the native rows of this panel; no UI tree/name lookup.
        NativeRow rows[XpPanelPlacement::Limit/2];MyGUI::Widget* widgets[XpPanelPlacement::Limit];
        int rowCount=0,widgetCount=0;MyGUI::Widget* parent=NULL;
        const int lines=panel->getNumLines(category);
        for(int i=0;i<lines && i<512;++i){
            DataPanelLine* line=panel->getLineByNum(category,i);
            if(!line || line->getType()==DataPanelLine::DPL_PROGRESS)continue;
            if(!line->w1 || !line->w2 || !line->w1->getVisible() || !line->w2->getVisible())continue;
            MyGUI::Widget* p=line->w1->getParent();
            if(!p || line->w2->getParent()!=p || (parent && p!=parent) || rowCount>=XpPanelPlacement::Limit/2){
                PanelStatus("unsupported mixed native layout");HidePanel(panel,category);return;
            }
            parent=p;rows[rowCount++].line=line;widgets[widgetCount++]=line->w1;widgets[widgetCount++]=line->w2;
        }
        if(!rowCount || !g_placement.Capture(parent,widgets,widgetCount)){PanelStatus("missing usable native layout");HidePanel(panel,category);return;}
        int total=0,rowHeight=0;
        for(int i=0;i<rowCount;++i){
            NativeRow& row=rows[i];row.name=g_placement.Original(row.line->w1);row.value=g_placement.Original(row.line->w2);
            row.top=row.name.top<row.value.top?row.name.top:row.value.top;
            row.bottom=row.name.bottom()>row.value.bottom()?row.name.bottom():row.value.bottom();
            if(row.top<0 || row.bottom<=row.top){PanelStatus("invalid native row bounds");HidePanel(panel,category);return;}
            const int height=row.bottom-row.top;total+=height;if(height>rowHeight)rowHeight=height;
        }
        std::sort(rows,rows+rowCount,RowBefore);
        MyGUI::TextBox* nameStyle=rows[rowCount-1].line->w1;MyGUI::TextBox* valueStyle=rows[rowCount-1].line->w2;
        const std::string& nameSkin=rows[rowCount-1].line->skinW1;
        const int left=rows[rowCount-1].name.left,valueRight=rows[rowCount-1].value.right();
        const int width=parent->getWidth()-left-4;
        const int availableHeight=parent->getHeight()<panel->getHeight()?parent->getHeight():panel->getHeight();
        // Preserve complete native row sizes. Redistribute only proven empty
        // top/inter-row spacing, bounded by the configured capacity, not the
        // number of currently active skills (the first XP row must not jump).
        int capacity=rowHeight>2?(availableHeight-total-1)/rowHeight:0;
        if(capacity>g_maxRows)capacity=g_maxRows;
        int barWidth=(int)(parent->getWidth()*.48f);
        int barRight=valueRight+10;if(barRight>parent->getWidth()-2)barRight=parent->getWidth()-2;
        int barLeft=barRight-barWidth;
        if(g_underlay.Resolve(root,parent,valueRight,barLeft,barRight))barWidth=barRight-barLeft;
        if(capacity<0 || width<24 || barWidth<8 || barLeft<left+8 || nameSkin.empty())capacity=0;
        if(!capacity){g_slots.Select(skills,(int)kCount,now,g_recent,0);PanelStatus("no room for complete native XP rows");HidePanel(panel,category);return;}
        int spare=availableHeight-total-capacity*rowHeight-1;
        int cursor=rows[0].top<spare?rows[0].top:spare;spare-=cursor;
        for(int i=0;i<rowCount;++i){
            NativeRow& row=rows[i];if(i){int gap=row.top-rows[i-1].bottom;if(gap<0)gap=0;if(gap>spare)gap=spare;cursor+=gap;spare-=gap;}
            const int delta=cursor-row.top;MyGUI::IntCoord name=row.name,value=row.value;name.top+=delta;value.top+=delta;
            g_placement.Apply(row.line->w1,name);g_placement.Apply(row.line->w2,value);cursor+=row.bottom-row.top;
        }
        const int bottom=cursor,dividerTop=bottom,top=bottom+1;
        g_slots.Select(skills,(int)kCount,now,g_recent,capacity);
        MyGUI::ISubWidgetText* const nameText=XpPanelTextStyle::RenderedText(nameStyle);
        MyGUI::ISubWidgetText* const valueText=XpPanelTextStyle::RenderedText(valueStyle);
        for(int slot=0;slot<g_slots.count;++slot){
            const int index=g_slots.ids[slot];const float value=skills[index].value;
            const float fraction=value-(float)std::floor(value);const std::string& key=SlotKey(slot);
            DataPanelLine_Progress* line=NULL;const bool created=!panel->lineExists(key,category);
            if(!created){DataPanelLine* base=panel->getLine(key,category);if(base && base->getType()==DataPanelLine::DPL_PROGRESS)line=static_cast<DataPanelLine_Progress*>(base);}
            if(created || (line && (g_rowSkill[slot]!=index || g_rowValue[slot]!=value))){
                char number[32];sprintf_s(number,"%d%%",(int)(fraction*100));
                if(created)line=panel->setLineProgress(key,category,fraction,number,true);else line->setProgress(fraction,number);
            }
            if(!line)continue;
            if(created)++g_progressCreates;
            const int y=top+slot*rowHeight;
            bool createdName=false;MyGUI::TextBox* label=NativeLabel(slot,parent,nameStyle,nameSkin,
                MyGUI::IntCoord(left,y,barLeft-left-4,rowHeight),createdName);
            XpPanelTextStyle::Copy(label,nameText);XpPanelTextStyle::Copy(line->progressText,valueText);
            if(label){if(createdName || created || g_rowSkill[slot]!=index)label->setCaption(Name(index));if(!label->getVisible())label->setVisible(true);}
            g_rowSkill[slot]=index;g_rowValue[slot]=value;
            if(line->progress && !line->progress->getVisible())line->setVisible(true);
            // Its generic flat TextBox can carry a heavy scaled frame/insets.
            // Keep native ownership/bookkeeping, but display our native-style label.
            if(line->textBox && line->textBox->getVisible())line->textBox->setVisible(false);
            // Native content-height/getBottom bookkeeping can still inspect
            // hidden widgets. Keep its placeholder inside the same row bounds.
            const MyGUI::IntCoord placeholder(left,y,barLeft-left-4,rowHeight);
            if(line->textBox && line->textBox->getCoord()!=placeholder)line->textBox->setCoord(placeholder);
            const MyGUI::IntCoord barCoord(barLeft,y+2,barWidth,rowHeight-2);
            if(line->progress && line->progress->getCoord()!=barCoord)line->progress->setCoord(barCoord);
            MyGUI::ISubWidgetText* percent=XpPanelTextStyle::Measure(line->progressText,g_percentMeasure[slot]);
            MyGUI::Widget* client=g_percentMeasure[slot].renderedSource.get();
            const bool lowerPercent=percent && client && percent->getTextAlign().isTop() && percent->getViewOffset().top==0 &&
                client->getAbsoluteTop()==line->progressText->getAbsoluteTop() && client->getHeight()==line->progressText->getHeight() &&
                percent->getCoord().top==0 && percent->getCoord().height==line->progressText->getHeight() &&
                g_percentMeasure[slot].textHeight>0 && g_percentMeasure[slot].textHeight<=rowHeight-2 &&
                (!g_percentMeasure[slot].shadow || g_percentMeasure[slot].textHeight<rowHeight-2);
            const MyGUI::IntCoord percentCoord=lowerPercent?barCoord:MyGUI::IntCoord(barLeft,y,barWidth,rowHeight);
            if(line->progressText && line->progressText->getCoord()!=percentCoord){
                line->progressText->setCoord(percentCoord);XpPanelTextStyle::Measure(line->progressText,g_percentMeasure[slot]);
            }
        }
        for(int slot=g_slots.count;slot<5;++slot){
            if(g_labels[slot].get() && g_labels[slot].get()->getVisible())g_labels[slot].get()->setVisible(false);
            const std::string& key=SlotKey(slot);
            if(panel->lineExists(key,category)){DataPanelLine* row=panel->getLine(key,category);
                if(row && row->getType()==DataPanelLine::DPL_PROGRESS){DataPanelLine_Progress* progress=static_cast<DataPanelLine_Progress*>(row);
                    if(progress->progress && progress->progress->getVisible())row->setVisible(false);}}
        }
        if(g_slots.count){
            if(g_divider.get() && g_divider.get()->getParent()!=parent){MyGUI::Gui::getInstance().destroyWidget(g_divider.get());++g_dividerParentDestroys;}
            if(!g_divider.get()){
                MyGUI::Widget* divider=parent->createWidget<MyGUI::Widget>("WhiteSkin",MyGUI::IntCoord(left,dividerTop,width,1),MyGUI::Align::Default);
                ++g_dividerCreates;
                divider->setNeedMouseFocus(false);divider->setColour(MyGUI::Colour(.35f,.3f,.25f));g_divider.set(divider);
            }
            MyGUI::Widget* divider=g_divider.get();const MyGUI::IntCoord coord(left,dividerTop,width,1);
            if(divider->getCoord()!=coord)divider->setCoord(coord);if(!divider->getVisible())divider->setVisible(true);
        }else HidePanel(panel,category);
        PanelStatus("showing recent XP rows");
        if(PluginDiagnostics::Enabled() && (g_diagRows!=g_slots.count || g_diagBottom!=bottom || g_diagWidth!=width ||
            g_diagParentHeight!=parent->getHeight() || g_diagValueRight!=valueRight || g_diagCapacity!=capacity)){
            std::ostringstream s;s<<"SkillLevelUpInfo: panel height="<<panel->getHeight()<<" vanillaBottom="<<bottom
                <<" parent="<<parent<<" parentSize="<<parent->getWidth()<<"x"<<parent->getHeight()
                <<" pitch="<<rowHeight<<" width="<<width<<" nameLeft="<<left<<" valueRight="<<valueRight
                <<" barLeft="<<barLeft<<" barWidth="<<barWidth<<" capacity="<<capacity<<" rows="<<g_slots.count;
            DebugLog(s.str());g_diagRows=g_slots.count;g_diagBottom=bottom;g_diagWidth=width;
            g_diagParentHeight=parent->getHeight();g_diagValueRight=valueRight;g_diagCapacity=capacity;
        }
        g_panelDirty=false;
    }
    void (*g_origPanel)(CharStats*,DatapanelGUI*,int,bool)=NULL;
    UINT_PTR TraceProgress(DatapanelGUI* panel,int category,int slot)
    {
        if(!panel || !panel->lineExists(SlotKey(slot),category))return 0;
        DataPanelLine* line=panel->getLine(SlotKey(slot),category);
        return line && line->getType()==DataPanelLine::DPL_PROGRESS?
            reinterpret_cast<UINT_PTR>(static_cast<DataPanelLine_Progress*>(line)->progress):0;
    }
    void Panel(CharStats* stats,DatapanelGUI* panel,int category,bool combat)
    {
        PanelTrace* trace=NULL;LONG sequence=0;
        if(g_uiThread && GetCurrentThreadId()==g_uiThread && PluginDiagnostics::Enabled()){
            trace=&g_panelTrace[g_panelTraceHead++%32];sequence=(LONG)g_panelTraceHead;
            InterlockedExchange(&trace->sequence,0);trace->tick=GetTickCount();trace->generation=InterlockedCompareExchange(&g_loadGeneration,0,0);
            trace->panel=reinterpret_cast<UINT_PTR>(panel);trace->owner=reinterpret_cast<UINT_PTR>(stats?stats->me:NULL);
            trace->category=category;trace->combat=combat?1:0;trace->dividerBefore=reinterpret_cast<UINT_PTR>(g_divider.get());
            for(int i=0;i<2;++i)trace->before[i]=TraceProgress(panel,category,i);
        }
        // Retire the old arrangement before native code lays out another
        // owner/category. This avoids restoring old coordinates over a fresh
        // native context that happens to reuse the same widgets/positions.
        if(g_uiThread && GetCurrentThreadId()==g_uiThread && g_panel &&
           (panel!=g_panel || category!=g_panelCategory || (stats && stats->me && !(hand(stats->me)==g_panelCharacter)))){
            g_placement.Restore();HideLabels();
        }
        g_origPanel(stats,panel,category,combat);
        if(!g_uiThread || GetCurrentThreadId()!=g_uiThread){PanelStatus("waiting for GUI thread");return;}
        ++g_panelCalls;
        if(trace){
            MyGUI::Widget* root=panel?panel->getWidget():NULL;trace->root=reinterpret_cast<UINT_PTR>(root);
            trace->contextMask=stats && stats->me?ContextMask(panel,root,hand(stats->me),category):16;
            trace->dividerNative=reinterpret_cast<UINT_PTR>(g_divider.get());
            for(int i=0;i<2;++i)trace->after[i]=TraceProgress(panel,category,i);
        }
        DrawPanel(stats,panel,category);
        if(trace){trace->dividerAfter=reinterpret_cast<UINT_PTR>(g_divider.get());InterlockedExchange(&trace->sequence,sequence);}
    }
    void (*g_origUpdate)(ForgottenGUI*) = NULL;
    void Update(ForgottenGUI* self)
    {
        if(!g_uiThread)g_uiThread=GetCurrentThreadId();
        g_origUpdate(self);
        if(GetCurrentThreadId()!=g_uiThread)return;
        if(g_panelDirty && g_panelRoot.get() && g_panel){HidePanel(g_panel,g_panelCategory);g_panelDirty=false;}
        if (!g_enabled) return;
        const DWORD now = GetTickCount();
        const LONG generation = InterlockedCompareExchange(&g_loadGeneration, 0, 0);
        if (generation != g_seenGeneration)
        {
            Reset();
            PluginDiagnostics::Write("SkillLevelUpInfo: save load, skill baselines reset");
            {Lock lock;g_seenGeneration = generation;}
        }
        if(!g_last || now-g_last>=250){g_last=now;Scan();}
        else ProcessNotifications();
    }
    // Re-baseline on save load, including a quick load with the same handles.
    void (*g_origLoad)(Character*, GameSaveState*) = NULL;
    void Load(Character* c, GameSaveState* state)
    {
        InterlockedIncrement(&g_loadGeneration);
        g_origLoad(c, state);
    }
    void (*g_origNewGame)(SaveFileSystem*) = NULL;
    void NewGame(SaveFileSystem* self)
    {
        InterlockedIncrement(&g_loadGeneration);
        g_origNewGame(self);
    }
}
extern "C" __declspec(dllexport) void MCM_Describe(MCM_Api* api)
{
    static const std::string p = PluginSettings::Path();
    api->beginMod(api, "SkillLevelUpInfo", "Skill Level Up Info", p.c_str(), LoadSettings);
    if (api->version >= 2) api->info(api, Tr("XP gain text, skill level-ups and recent training bars from XP Info."));
    api->toggle(api, "SkillLevelUpInfo", "Enabled", Tr("Enabled"), Tr("Enable XP gain text, level-up messages and character-panel bars."), 1, 0);
    api->toggle(api, "SkillLevelUpInfo", "FloatingLabels", Tr("XP gain text"), Tr("Show accumulated skill gains as rising blue text, such as +3% Athletics."), 1, 0);
    api->toggle(api, "SkillLevelUpInfo", "LevelUpLabels", Tr("Skill level-up text"), Tr("Show reached whole skill levels as rising gold text."), 1, 0);
    api->toggle(api, "SkillLevelUpInfo", "PanelBars", Tr("XP bars in character panel"), Tr("Show recently trained skills below the vanilla character stats, separated by a divider."), 1, 0);
    api->integer(api, "SkillLevelUpInfo", "MaxRows", Tr("Maximum XP rows"), Tr("Show up to two recent skills in the character panel. Only one row appears when space is limited."), 2, 1, 2, 0);
    api->number(api, "SkillLevelUpInfo", "MinPercent", Tr("Minimum XP gain, %"), Tr("Accumulate small skill gains until this percentage can be announced. Level-ups always appear immediately."), 1, 0, 100, 2, 0);
    api->number(api, "SkillLevelUpInfo", "AnnounceEverySeconds", Tr("XP announcement interval, s"), Tr("Minimum collection time for fractional XP messages. Level-up messages do not wait."), 1, .1f, 60, 1, 0);
    api->integer(api, "SkillLevelUpInfo", "RecentSeconds", Tr("Recent training duration, s"), Tr("Keep a skill in the panel for this long after its last XP gain."), 120, 1, 3600, 0);
    api->toggle(api, "SkillLevelUpInfo", "SelectedOnly", Tr("Selected characters only"), Tr("Track the whole player faction, but only announce gains for selected characters."), 0, 0);
    api->toggle(api, "SkillLevelUpInfo", "LogToGame", Tr("Copy to game log"), Tr("Also add the character name and new skill level to the game's message log."), 0, 0);
    api->toggle(api, "SkillLevelUpInfo", "Debug", Tr("Detailed log"), Tr("Details in RE_Kenshi_log.txt: settings, XP and level messages, and save-load baseline resets."), 0, 0);
}
__declspec(dllexport) void startPlugin()
{
    InitializeCriticalSection(&g_lock);
    if(!XpPanelTextStyle::InitializeNativeProfile())
        ErrorLog("SkillLevelUpInfo: unsupported MyGUI font ABI; retaining default XP text style");
    LoadSettings();
    if (KenshiLib::AddHook(KenshiLib::GetRealAddress(&increaseStat), Increase, &g_origIncrease) != KenshiLib::SUCCESS)
        ErrorLog("SkillLevelUpInfo: cannot hook XP increase; baseline polling remains active");
    if (KenshiLib::AddHook(KenshiLib::GetRealAddress(&CharStats::getGUIDataForMainInfo), Panel, &g_origPanel) != KenshiLib::SUCCESS)
        ErrorLog("SkillLevelUpInfo: cannot hook character-panel XP bars");
    if (KenshiLib::AddHook(KenshiLib::GetRealAddress(&ForgottenGUI::update), Update, &g_origUpdate) != KenshiLib::SUCCESS)
        ErrorLog("SkillLevelUpInfo: cannot hook GUI update");
    if (KenshiLib::AddHook(KenshiLib::GetRealAddress(&Character::_NV_loadFromSerialise), Load, &g_origLoad) != KenshiLib::SUCCESS)
        ErrorLog("SkillLevelUpInfo: cannot hook character load");
    if (KenshiLib::AddHook(KenshiLib::GetRealAddress(&SaveFileSystem::newGame), NewGame, &g_origNewGame) != KenshiLib::SUCCESS)
        ErrorLog("SkillLevelUpInfo: cannot hook new game");
}
