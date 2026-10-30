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
    XpPanelTextStyle::MeasureCache g_nameMeasure,g_valueMeasure;
    void HidePanel(DatapanelGUI* panel,int category)
    {
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
    void DrawPanel(CharStats* stats,DatapanelGUI* panel,int category)
    {
        if((!g_enabled || !g_bars) && !g_panelDirty)return;
        if(!panel || !stats || !stats->me){PanelStatus("missing native panel/stat owner");return;}
        MyGUI::Widget* root=panel->getWidget();if(!root){PanelStatus("missing native root widget");return;}
        const hand owner(stats->me);
        if(root!=g_panelRoot.get() || !(owner==g_panelCharacter)){
            g_slots=XpInfoCore::Slots();g_panelCharacter=owner;
            for(int i=0;i<5;++i)g_rowSkill[i]=-1;
            if(g_divider.get())MyGUI::Gui::getInstance().destroyWidget(g_divider.get());
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
        // Read a bounded list of existing vanilla line widgets. Use their actual
        // parent and pitch rather than translated captions or a global UI search.
        int bottom=0,pitch=20,left=0,width=root->getWidth(),valueRight=width-4;
        int rowBottoms[512],rowCount=0;
        MyGUI::Widget* parent=root;
        MyGUI::Widget* nameStyle=NULL;MyGUI::Widget* valueStyle=NULL;
        const int lines=panel->getNumLines(category);
        for(int i=0;i<lines && i<512;++i){
            DataPanelLine* line=panel->getLineByNum(category,i);if(!line)continue;
            if(line->getType()==DataPanelLine::DPL_PROGRESS)continue;
            MyGUI::Widget* w=line->w2?line->w2:line->w1;
            if(!w || !w->getVisible())continue;
            rowBottoms[rowCount++]=w->getBottom();
            if(w->getBottom()>bottom){
                bottom=w->getBottom();parent=w->getParent();
                nameStyle=line->w1;valueStyle=line->w2;
                if(line->w1)left=line->w1->getLeft();
                if(line->w2)valueRight=line->w2->getLeft()+line->w2->getWidth();
            }
        }
        // getLineByNum walks a key-sorted native map, not visual row order.
        // Original XP Info sorts non-progress row bottoms and takes their
        // smallest positive gap. A late map key must not turn
        // one normal row pitch into a two/three-row jump and erase capacity.
        std::sort(rowBottoms,rowBottoms+rowCount);
        int smallestGap=0;
        for(int i=1;i<rowCount;++i){const int gap=rowBottoms[i]-rowBottoms[i-1];if(gap>0 && (!smallestGap || gap<smallestGap))smallestGap=gap;}
        if(smallestGap)pitch=smallestGap;
        if(!parent || pitch<=0){PanelStatus("missing usable native layout");return;}
        width=parent->getWidth()-left-4;
        // Native w1/w2 overlap: both start at 3% of the row, while w2's
        // text is right-aligned. Its left edge is not a value-column boundary.
        // DataPanelLine_Progress(showKey=true) uses 48% of its parent width.
        // xp_info aligns that native bar to vanilla w2's RIGHT edge +10px.
        const int barWidth=(int)(parent->getWidth()*.48f);
        int barRight=valueRight+10;
        if(barRight>parent->getWidth()-2)barRight=parent->getWidth()-2;
        const int barLeft=barRight-barWidth;
        int top=bottom;
        const int availableHeight=parent->getHeight()<panel->getHeight()?parent->getHeight():panel->getHeight();
        // The final row needs its actual widget height, not a trailing full
        // pitch. Native185px panels can fit24px after bottom161 exactly.
        const int rowHeight=pitch-2;
        const int remaining=availableHeight-top;
        int capacity=rowHeight>0 && remaining>=rowHeight?1+(remaining-rowHeight)/pitch:0;
        if(capacity>g_maxRows)capacity=g_maxRows;
        if(capacity<0 || width<24 || barWidth<8 || barLeft<left+8 || barLeft+barWidth>parent->getWidth())capacity=0;
        // Reserve a divider only from spare space after the full allowed
        // capacity, so adding another recent skill cannot shift the first row.
        const bool dividerAllowed=capacity>0 && remaining-(rowHeight+(capacity-1)*pitch)>=2;
        if(dividerAllowed)top+=2;
        MyGUI::ISubWidgetText* const nameText=XpPanelTextStyle::RenderedText(nameStyle);
        MyGUI::ISubWidgetText* const valueText=XpPanelTextStyle::RenderedText(valueStyle);
        const bool paddingDivider=!dividerAllowed && bottom>0 &&
            XpPanelTextStyle::PaddingBefore(nameStyle,parent->getAbsoluteTop()+bottom-1,g_nameMeasure) &&
            XpPanelTextStyle::PaddingBefore(valueStyle,parent->getAbsoluteTop()+bottom-1,g_valueMeasure);
        const int dividerTop=dividerAllowed?bottom:bottom-1;
        g_slots.Select(skills,(int)kCount,GetTickCount(),g_recent,capacity);
        PanelStatus(capacity<=0?"no room for XP rows":g_slots.count?"showing recent XP rows":"no recent XP for this character");
        for(int slot=0;slot<g_slots.count;++slot){
            const int index=g_slots.ids[slot];const float value=skills[index].value;
            const float fraction=value-(float)std::floor(value);
            const std::string& key=SlotKey(slot);
            DataPanelLine_Progress* line=NULL;
            bool created=!panel->lineExists(key,category);
            if(!created){DataPanelLine* base=panel->getLine(key,category);if(base && base->getType()==DataPanelLine::DPL_PROGRESS)line=static_cast<DataPanelLine_Progress*>(base);}
            if(created || (line && (g_rowSkill[slot]!=index || g_rowValue[slot]!=value))){
                char number[32];sprintf_s(number,"%d%%",(int)(fraction*100));
                if(created)line=panel->setLineProgress(key,category,fraction,number,true);
                else line->setProgress(fraction,number);
            }
            if(!line)continue;
            XpPanelTextStyle::Copy(line->textBox,nameText);
            XpPanelTextStyle::Copy(line->progressText,valueText);
            if(created || g_rowSkill[slot]!=index){if(line->textBox)line->textBox->setCaption(Name(index));g_rowSkill[slot]=index;}
            g_rowValue[slot]=value;
            if(line->progress && !line->progress->getVisible())line->setVisible(true);
            const MyGUI::IntCoord coord(barLeft,top+slot*pitch,barWidth,rowHeight);
            if(line->progress && line->progress->getCoord()!=coord)line->progress->setCoord(coord);
            if(line->progressText && line->progressText->getCoord()!=coord)line->progressText->setCoord(coord);
            const MyGUI::IntCoord nameCoord(left,top+slot*pitch,barLeft-left-4,rowHeight);
            if(line->textBox && line->textBox->getCoord()!=nameCoord)line->textBox->setCoord(nameCoord);
        }
        for(int slot=g_slots.count;slot<5;++slot){
            const std::string& key=SlotKey(slot);
            if(panel->lineExists(key,category)){DataPanelLine* row=panel->getLine(key,category);
                if(row && row->getType()==DataPanelLine::DPL_PROGRESS){DataPanelLine_Progress* progress=static_cast<DataPanelLine_Progress*>(row);
                    if(progress->progress && progress->progress->getVisible())row->setVisible(false);}}
        }
        if(g_slots.count && (dividerAllowed || paddingDivider)){
            if(g_divider.get() && g_divider.get()->getParent()!=parent)MyGUI::Gui::getInstance().destroyWidget(g_divider.get());
            if(!g_divider.get()){
                MyGUI::Widget* divider=parent->createWidget<MyGUI::Widget>("WhiteSkin",MyGUI::IntCoord(left,dividerTop,width,1),MyGUI::Align::Default);
                divider->setNeedMouseFocus(false);divider->setColour(MyGUI::Colour(.35f,.3f,.25f));g_divider.set(divider);
            }
            MyGUI::Widget* divider=g_divider.get();
            const MyGUI::IntCoord coord(left,dividerTop,width,1);
            if(divider->getCoord()!=coord)divider->setCoord(coord);
            if(!divider->getVisible())divider->setVisible(true);
        }else if(g_divider.get() && g_divider.get()->getVisible())g_divider.get()->setVisible(false);
        if(PluginDiagnostics::Enabled() && (g_diagRows!=g_slots.count || g_diagBottom!=bottom || g_diagWidth!=width ||
            g_diagParentHeight!=parent->getHeight() || g_diagValueRight!=valueRight || g_diagCapacity!=capacity)){
            std::ostringstream s;s<<"SkillLevelUpInfo: panel height="<<panel->getHeight()<<" vanillaBottom="<<bottom
                <<" parent="<<parent<<" parentSize="<<parent->getWidth()<<"x"<<parent->getHeight()
                <<" pitch="<<pitch<<" width="<<width<<" nameLeft="<<left<<" valueRight="<<valueRight
                <<" barLeft="<<barLeft<<" barWidth="<<barWidth<<" capacity="<<capacity<<" rows="<<g_slots.count;
            DebugLog(s.str());g_diagRows=g_slots.count;g_diagBottom=bottom;g_diagWidth=width;
            g_diagParentHeight=parent->getHeight();g_diagValueRight=valueRight;g_diagCapacity=capacity;
        }
        g_panelDirty=false;
    }
    void (*g_origPanel)(CharStats*,DatapanelGUI*,int,bool)=NULL;
    void Panel(CharStats* stats,DatapanelGUI* panel,int category,bool combat)
    {
        g_origPanel(stats,panel,category,combat);
        if(!g_uiThread || GetCurrentThreadId()!=g_uiThread){PanelStatus("waiting for GUI thread");return;}
        DrawPanel(stats,panel,category);
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
