#include <Windows.h>
#include <ModConfigMenu.h>
#include "MiningCore.h"
#include "MiningPicker.h"
#include <map>
#include <vector>
#include <string>
#include <set>
#include <cstdio>
#include <cstdlib>

namespace MiningCore {
    std::map<std::string,Rule> rules;
    std::vector<SourceInfo> Sources() {
        std::vector<SourceInfo> out;
        for (std::map<std::string,Rule>::iterator i=rules.begin();i!=rules.end();++i) {
            SourceInfo s;s.sid=i->first;s.name=i->second.name;out.push_back(s);
        }
        return out;
    }
    void ApplySettings() {} void ReloadSources() {}
    bool ReadSource(const std::string& id,Rule& r,Rule* defaults) {
        std::map<std::string,Rule>::const_iterator found=rules.find(id);
        if(found==rules.end())return false;
        r=found->second;if(defaults)*defaults=Rule();return true;
    }
    bool WriteSource(const std::string& id,const std::string&,float v,int skill,bool cap) {
        std::map<std::string,Rule>::iterator found=rules.find(id);
        if(found==rules.end())return false;
        if(skill<0)found->second.enabled=v!=0;
        else if(cap)found->second.cap[skill]=v;
        else found->second.mult[skill]=v;
        return true;
    }
}
// The real picker owns a ScrollView. This runner tests MiningSettings.cpp's
// rows and callbacks, so supply only its selected ID and attach boundaries.
namespace MiningPicker {
    std::string selected;
    std::string Selected(){return selected;}
    void Attach(void*,void*){}
    void Detach(void*){}
}
extern "C" void MCM_Describe(MCM_Api*);
struct Row {
    std::string label,def;
    MCM_SetIntFn setInt;MCM_SetFloatFn setFloat;MCM_GetFloatFn getFloat;void* ud;
    Row():setInt(NULL),setFloat(NULL),getFloat(NULL),ud(NULL){}
};
std::vector<Row> rows;
bool customAreaRegistered=false;
void Check(bool ok,const char* message){if(!ok){printf("FAIL: %s\n",message);exit(1);}}
Row& Add(const char* label){Row r;r.label=label?label:"";rows.push_back(r);return rows.back();}
Row Find(const char* label){for(size_t i=0;i<rows.size();++i)if(rows[i].label==label)return rows[i];Check(false,label);return Row();}
bool Has(const char* label){for(size_t i=0;i<rows.size();++i)if(rows[i].label==label)return true;return false;}
void Begin(MCM_Api*,const char*,const char*,const char*,MCM_ApplyFn){}
void Section(MCM_Api*,const char* label){Add(label);}
void Toggle(MCM_Api*,const char*,const char*,const char* l,const char*,int,unsigned){Add(l);}
void Number(MCM_Api*,const char*,const char*,const char* l,const char*,float,float,float,int,unsigned){Add(l);}
void Info(MCM_Api*,const char*){}
void Action(MCM_Api*,const char* l,const char*,MCM_ActionFn,void*,unsigned){Add(l);}
void ToggleFn(MCM_Api*,const char* l,const char*,MCM_GetIntFn,MCM_SetIntFn set,void* ud,unsigned){Row&r=Add(l);r.setInt=set;r.ud=ud;}
void NumberFn(MCM_Api*,const char* l,const char*,MCM_GetFloatFn get,MCM_SetFloatFn set,void* ud,float,float,int,unsigned){Row&r=Add(l);r.getFloat=get;r.setFloat=set;r.ud=ud;}
void Default(MCM_Api*,const char* value){rows.back().def=value;}
void DynamicRows(MCM_Api* api,MCM_RowsFn fn,void* ud){fn(api,ud);}
void CustomArea(MCM_Api*,MCM_AttachFn attach,MCM_DetachFn detach,void*,int height){
    Check(attach==MiningPicker::Attach&&detach==MiningPicker::Detach,"real picker attach and detach callbacks registered");
    Check(height>=10&&height<=90,"picker area leaves space for source fields");
    customAreaRegistered=true;
}
void Build(){
    rows.clear();customAreaRegistered=false;MCM_Api api={};api.version=6;
    api.beginMod=Begin;api.section=Section;api.toggle=Toggle;api.number=Number;
    api.info=Info;api.action=Action;api.toggleFn=ToggleFn;api.numberFn=NumberFn;
    api.defaultValue=Default;api.rows=DynamicRows;api.customArea=CustomArea;
    MCM_Describe(&api);
}
int main(){
    using namespace MiningCore;
    for(int i=0;i<128;++i){char id[24];sprintf_s(id,"source%03d",i);Rule r;r.name="Ore";r.mult[0]=(float)i+2;rules[id]=r;}
    MiningPicker::selected="source000";Build();
    Check(customAreaRegistered,"picker custom area is registered without null function calls");
    Check(rows.size()==15,"128 sources still expose one compact source editor");
    Check(rows.back().label=="Detailed log","Debug remains the last setting");
    Row old=Find("Strength XP");Check(old.def=="0.25","default XP comes from source defaults");
    Check(Find("Strength limit").def=="15","default cap comes from source defaults");
    std::set<void*> userdata;
    for(size_t i=0;i<rows.size();++i)if(rows[i].ud)userdata.insert(rows[i].ud);
    Check(userdata.size()==7,"enabled and six numeric source fields have distinct callback data");
    Build();Check(Find("Strength XP").ud==old.ud,"rebuilding the same source reuses stable callback data");
    MiningPicker::selected="source001";Build();
    Row current=Find("Strength XP");float value=0;current.getFloat(current.ud,&value);
    Check(value==3,"picker selection changes the displayed source");
    Check(old.setFloat(old.ud,7,NULL,0)==0,"pending old-row edit is accepted");
    Check(rules["source000"].mult[0]==7&&rules["source001"].mult[0]==3,"pending edit cannot modify newly selected source");
    Check(current.setFloat(current.ud,9,NULL,0)==0,"current row edit succeeds");
    Check(rules["source001"].mult[0]==9&&rules["source000"].mult[0]==7,"current edit remains in selected source");
    Row cap=Find("Strength limit");Check(cap.setFloat(cap.ud,42,NULL,0)==0,"source cap edit succeeds");
    Check(rules["source001"].cap[0]==42&&rules["source001"].mult[0]==9,"cap edit does not alter XP multiplier");
    Row enabled=Find("Source enabled");Check(enabled.setInt(enabled.ud,0,NULL,0)==0,"source can be disabled");
    Check(!rules["source001"].enabled&&rules["source000"].enabled,"enabled callback is isolated to selected source");
    MiningPicker::selected.clear();Build();
    Check(Has("No matching sources")&&!Has("Strength XP"),"empty picker selection does not expose stale fields");
    MiningPicker::selected="missing-source";Build();
    Check(!Has("Strength XP"),"missing selected rule does not expose stale fields");
    rules.erase("source000");
    Check(old.setFloat(old.ud,5,NULL,0)!=0,"pending edit of a removed source fails without recreating it");
    Check(rules.find("source000")==rules.end(),"removed source remains absent");
    printf("Mining settings: picker registration, compact rows, defaults, stable callback data, source isolation and missing-source handling passed\n");
    return 0;
}
