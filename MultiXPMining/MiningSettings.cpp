#define KLOC_DOMAIN "multi_xp_mining"
#include <PluginSettings.h>
#include <Localization.h>
#include <ModConfigMenu.h>
#include "MiningCore.h"
#include "MiningPicker.h"
#include <map>
#include <algorithm>

namespace
{
    std::string g_selected;
    struct Field { std::string sid, key; int skill; bool cap; };
    // MCM retains userdata for the session. Nodes are stable across rebuilds.
    std::map<std::string, Field> g_fields;
    int Reload(void*,char*,unsigned) { MiningCore::ReloadSources(); return 0; }
    Field* FieldFor(const std::string& sid,const std::string& key,int skill,bool cap)
    {
        Field& f=g_fields[sid+"|"+key];
        f.sid=sid; f.key=key; f.skill=skill; f.cap=cap;
        return &f;
    }
    int Get(void* ud,float* out)
    {
        const Field& f=*(Field*)ud; MiningCore::Rule rule;
        if (!MiningCore::ReadSource(f.sid,rule)) return 1;
        *out=f.skill<0 ? (rule.enabled ? 1.0f : 0.0f) : f.cap ? rule.cap[f.skill] : rule.mult[f.skill];
        return 0;
    }
    int Set(void* ud,float value,char*,unsigned)
    {
        const Field& f=*(Field*)ud;
        return MiningCore::WriteSource(f.sid,f.key,value,f.skill,f.cap) ? 0 : 1;
    }
    int GetEnabled(void* ud,int* out) { float v=0; int result=Get(ud,&v); *out=v!=0; return result; }
    int SetEnabled(void* ud,int value,char* err,unsigned size) { return Set(ud,value ? 1.0f : 0.0f,err,size); }
    void Default(MCM_Api* api,float value)
    {
        if (api->version>=5) { char text[48]; sprintf_s(text,"%.6g",value); api->defaultValue(api,text); }
    }
    void SourceRows(MCM_Api* api,void*)
    {
        g_selected=MiningPicker::Selected();
        if(g_selected.empty()) { api->section(api,Tr("No matching sources"));return; }
        api->section(api,Tr("Selected source"));
        MiningCore::Rule rule,defaults;
        if (!MiningCore::ReadSource(g_selected,rule,&defaults)) return;
        api->toggleFn(api,Tr("Source enabled"),Tr("Allow extra skill training at this source."),GetEnabled,SetEnabled,FieldFor(g_selected,"Enabled",-1,false),0);
        Default(api,defaults.enabled ? 1.0f : 0.0f);
        const char* keys[]={"Strength","Toughness","Dexterity"};
        const char* xp[]={Tr("Strength XP"),Tr("Toughness XP"),Tr("Dexterity XP")};
        const char* caps[]={Tr("Strength limit"),Tr("Toughness limit"),Tr("Dexterity limit")};
        for (int i=0;i<3;++i)
        {
            api->numberFn(api,xp[i],Tr("Extra raw skill ticks per Labouring tick. 0 disables this skill; 1 gives one extra tick. Racial and XP Overhaul modifiers still apply."),
                Get,Set,FieldFor(g_selected,std::string(keys[i])+"Multiplier",i,false),0,10,2,0);
            Default(api,defaults.mult[i]);
            api->numberFn(api,caps[i],Tr("Maximum base skill level from this source. Applies only when skill caps are enabled; normal game XP is unchanged."),
                Get,Set,FieldFor(g_selected,std::string(keys[i])+"Cap",i,true),0,1000,1,0);
            Default(api,defaults.cap[i]);
        }
    }
}
extern "C" __declspec(dllexport) void MCM_Describe(MCM_Api* api)
{
    static const std::string p=PluginSettings::Path();
    api->beginMod(api,"MultiXPMining","Multi XP Mining",p.c_str(),MiningCore::ApplySettings);
    if (api->version>=2) api->info(api,Tr("Train Strength, Toughness and Dexterity through actual mining work. All loaded mining sources are detected automatically; source overrides keep priority."));
    api->toggle(api,"General","Enabled",Tr("Enabled"),Tr("Extra skill experience for work sources listed in SourceData."),1,0);
    api->number(api,"General","GlobalXpMultiplier",Tr("Extra XP multiplier"),Tr("Multiplies the extra skill ticks of every configured source."),1,0,10,2,0);
    api->toggle(api,"General","EnableCaps",Tr("Enable skill caps"),Tr("Stop the extra experience at each source's configured skill limit."),1,0);
    api->number(api,"General","GlobalCapMultiplier",Tr("Cap multiplier"),Tr("Multiplies every source's skill limits."),1,0,10,2,0);
    api->toggle(api,"General","GenerateUnknownSourcesIni",Tr("Record other work sources"),Tr("Mining sources are detected automatically. This writes other unconfigured work sources to UnknownSources.ini without granting them extra XP."),1,0);
    if (api->version>=4) api->customArea(api,MiningPicker::Attach,MiningPicker::Detach,NULL,35);
    if (api->version>=6) api->rows(api,SourceRows,NULL);
    if (api->version>=3) api->action(api,Tr("Reload source tables"),Tr("Reload SourceData INI files in filename order, preserving per-key overrides."),Reload,NULL,0);
    api->toggle(api,"General","Debug",Tr("Detailed log"),Tr("Details in RE_Kenshi_log.txt: loaded rules, active work sources, extra XP ticks and limits. Worker summaries are limited to one per five seconds."),0,0);
}
