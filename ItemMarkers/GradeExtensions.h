#pragma once
#include <ItemMarkersApi.h>
#include <cstring>
namespace IMExt {
enum { MaxProviders=32, MaxGrades=128, MaxRules=512 };
inline bool Valid(const IM_Appearance& a) {
    return a.size==sizeof(a) && a.rank>=0 && a.rank<=5 && a.rgb<=0xffffffu;
}
inline bool Text(const char* s,unsigned cap) { return s && std::memchr(s,0,cap)!=0; }
inline bool Name(const char* s) {
    if(!s||!*s)return false;
    for(unsigned i=0;i<64;++i) {
        const char c=s[i];if(!c)return true;
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='_'||c=='-'))return false;
    }return false;
}
struct Rule {
    char item[128], material[128];
    int type, low, high, priority;
    IM_Appearance appearance;
    Rule() : type(-1),low(-1),high(-1),priority(0) { item[0]=material[0]=0; std::memset(&appearance,0,sizeof(appearance)); }
};
class Rules {
public:
    Rule entries[MaxRules]; unsigned count;
    Rules():count(0){}
    static bool ValidRule(const Rule& r) {
        if(!Text(r.item,sizeof(r.item))||!Text(r.material,sizeof(r.material))||
           (!*r.item&&!*r.material&&r.type<0)||r.type< -1||r.type>4095||r.low< -1||r.low>100||
           r.high< -1||r.high>100||(r.low>=0&&r.high>=0&&r.low>r.high)||
           r.priority< -10000||r.priority>10000||!Valid(r.appearance)) return false;
        return true;
    }
    bool Add(const Rule& r) {
        if(count==MaxRules||!ValidRule(r))return false;
        entries[count++]=r;return true;
    }
    bool Query(const IM_ItemView& v, IM_Appearance& out) const {
        if(v.size!=sizeof(v)||!v.item_id||!v.material_id) return false;
        const Rule* best=0;
        for(unsigned i=0;i<count;++i) {
            const Rule& r=entries[i];
            if((r.type>=0&&r.type!=v.type)||(*r.item&&std::strcmp(r.item,v.item_id))||
               (*r.material&&std::strcmp(r.material,v.material_id))) continue;
            if((r.low>=0||r.high>=0)&&(v.native_level<0||
               (r.low>=0&&v.native_level<r.low)||(r.high>=0&&v.native_level>r.high))) continue;
            if(!best||r.priority>best->priority) best=&r;
        }
        if(!best)return false;out=best->appearance;return true;
    }
};
class Registry {
public:
    struct Slot { IM_Provider provider; IM_Handle handle; char name[64]; bool disabled; };
    Slot slots[MaxProviders]; unsigned count; bool querying; IM_Handle next;
    Registry():count(0),querying(false),next(1){}
    void Rebind() { for(unsigned i=0;i<count;++i) slots[i].provider.name=slots[i].name; }
    int Add(const IM_Provider& p, IM_Handle& h) {
        h=0;if(querying)return IM_BUSY;
        if(p.size!=sizeof(p)||!Name(p.name)||!p.query||p.priority< -10000||p.priority>10000)return IM_INVALID;
        for(unsigned i=0;i<count;++i)if(std::strcmp(slots[i].name,p.name)==0)return IM_INVALID;
        if(count==MaxProviders||next==0)return IM_FULL;
        unsigned at=count;while(at&&slots[at-1].provider.priority<p.priority) { slots[at]=slots[at-1];--at; }
        Slot& s=slots[at];s.provider=p;s.handle=next++;s.disabled=false;std::memcpy(s.name,p.name,std::strlen(p.name)+1);
        ++count;h=s.handle;Rebind();return IM_OK;
    }
    int Remove(IM_Handle h) {
        if(querying)return IM_BUSY;
        for(unsigned i=0;i<count;++i)if(slots[i].handle==h) {
            for(unsigned k=i+1;k<count;++k)slots[k-1]=slots[k];--count;Rebind();return IM_OK;
        }
        return IM_NOT_FOUND;
    }
    bool Contains(IM_Handle h) const { for(unsigned i=0;i<count;++i)if(slots[i].handle==h)return true;return false; }
    template<class Invoke> bool Query(const IM_ItemView& v,IM_Appearance& out,Invoke& call) {
        if(querying||v.size!=sizeof(v))return false;
        querying=true;bool found=false;
        for(unsigned i=0;i<count;++i) {
            Slot& s=slots[i];if(s.disabled)continue;
            IM_Appearance a={sizeof(a),0,0};
            const int result=call(s.provider.query,v,a,s.provider.context);
            if(result==0)continue;
            if(result!=1||!Valid(a)) { s.disabled=true;continue; }
            out=a;found=true;break;
        }
        querying=false;return found;
    }
};
}
