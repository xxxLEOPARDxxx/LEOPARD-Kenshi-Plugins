#pragma once
#include "GradeExtensions.h"
#include <string>
#include <map>
#include <vector>
#include <cstdlib>
#include <climits>
namespace IMExt {
typedef std::map<std::string,std::string> Fields;
struct Section { std::string name; Fields fields; };
inline std::string Trim(const std::string& s) {
    const size_t a=s.find_first_not_of(" \t\r"),b=s.find_last_not_of(" \t\r");
    return a==std::string::npos?std::string():s.substr(a,b-a+1);
}
inline bool Integer(const std::string& s,int lo,int hi,int& out) {
    if(s.empty()||s.size()>11)return false;
    char* end=0;const long v=std::strtol(s.c_str(),&end,10);
    if(!end||*end||v<lo||v>hi)return false;out=static_cast<int>(v);return true;
}
inline bool Rgb(const std::string& s,uint32_t& out) {
    if(s.size()!=7||s[0]!='#')return false;uint32_t n=0;
    for(unsigned i=1;i<7;++i) { const char c=s[i];int d=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;if(d<0)return false;n=n*16+d; }
    out=n;return true;
}
inline bool Parse(const std::string& text,std::vector<Section>& sections,std::string& error) {
    if(text.size()>65536||text.find('\0')!=std::string::npos) { error="profile size/NUL";return false; }
    size_t p=text.compare(0,3,"\xef\xbb\xbf")==0?3:0;
    while(p<text.size()) {
        size_t e=text.find('\n',p);if(e==std::string::npos)e=text.size();
        if(e-p>511) { error="line exceeds 511 bytes";return false; }
        const std::string line=Trim(text.substr(p,e-p));p=e+1;
        if(line.empty()||line[0]==';'||line[0]=='#')continue;
        if(line[0]=='['&&line[line.size()-1]==']') {
            if(sections.size()>=MaxRules+MaxGrades+1) { error="too many sections";return false; }
            Section s;s.name=line.substr(1,line.size()-2);
            for(size_t i=0;i<sections.size();++i)if(sections[i].name==s.name) { error="duplicate section";return false; }
            sections.push_back(s);continue;
        }
        const size_t eq=line.find('=');
        if(sections.empty()||eq==std::string::npos) { error="expected section/key=value";return false; }
        const std::string key=Trim(line.substr(0,eq)),value=Trim(line.substr(eq+1));
        if(key.empty()||!sections.back().fields.insert(std::make_pair(key,value)).second) { error="duplicate/empty key";return false; }
    }
    return true;
}
inline std::string Get(const Fields& f,const char* k,const char* fallback="") {
    const Fields::const_iterator it=f.find(k);return it==f.end()?std::string(fallback):it->second;
}
inline bool Keys(const Fields& f,const char* const* keys,unsigned count) {
    for(Fields::const_iterator it=f.begin();it!=f.end();++it) {
        unsigned i=0;for(;i<count;++i)if(it->first==keys[i])break;if(i==count)return false;
    }return true;
}
/* Each file owns its grade names. 'Palette' resolves a theme role once on reload,
   falling back to the required Colour. No theme/INI reads at query time. */
template<class Palette> bool LoadProfile(const std::string& text,Rules& destination,Palette& palette,std::string& error) {
    std::vector<Section> sections;if(!Parse(text,sections,error))return false;
    std::map<std::string,IM_Appearance> grades;std::vector<Rule> rules;
    for(size_t i=0;i<sections.size();++i)if(sections[i].name=="Profile") {
        static const char* k[]={"Enabled"};int on;
        if(!Keys(sections[i].fields,k,1)||!Integer(Get(sections[i].fields,"Enabled","1"),0,1,on)) { error="invalid Profile";return false; }
        if(!on)return true;
    }
    for(size_t i=0;i<sections.size();++i) {
        const Section& s=sections[i];if(s.name.compare(0,6,"Grade.")!=0)continue;
        static const char* k[]={"Rank","Colour","Palette"};const std::string name=s.name.substr(6);
        IM_Appearance a={sizeof(a),0,0};
        if(!Name(name.c_str())||grades.size()==MaxGrades||!Keys(s.fields,k,3)||
           !Integer(Get(s.fields,"Rank"),0,5,a.rank)||!Rgb(Get(s.fields,"Colour"),a.rgb)) { error="invalid Grade";return false; }
        const std::string role=Get(s.fields,"Palette");
        if(!role.empty()) { if(!Name(role.c_str())) { error="invalid palette role";return false; }a.rgb=palette(role.c_str(),a.rgb); }
        if(!Valid(a)) { error="invalid palette colour";return false; }grades[name]=a;
    }
    for(size_t i=0;i<sections.size();++i) {
        const Section& s=sections[i];if(s.name=="Profile"||s.name.compare(0,6,"Grade.")==0)continue;
        static const char* k[]={"ItemID","MaterialID","Type","LevelMin","LevelMax","Priority","Grade"};
        if(s.name.compare(0,5,"Rule.")!=0||!Name(s.name.substr(5).c_str())||!Keys(s.fields,k,7)) { error="unknown section/key";return false; }
        Rule r;const std::string item=Get(s.fields,"ItemID"),material=Get(s.fields,"MaterialID");
        const std::map<std::string,IM_Appearance>::const_iterator grade=grades.find(Get(s.fields,"Grade"));
        if(item.size()>=sizeof(r.item)||material.size()>=sizeof(r.material)||grade==grades.end()||
           !Integer(Get(s.fields,"Type","-1"),-1,4095,r.type)||!Integer(Get(s.fields,"LevelMin","-1"),-1,100,r.low)||
           !Integer(Get(s.fields,"LevelMax","-1"),-1,100,r.high)||!Integer(Get(s.fields,"Priority","0"),-10000,10000,r.priority)) { error="invalid Rule";return false; }
        std::memcpy(r.item,item.c_str(),item.size()+1);std::memcpy(r.material,material.c_str(),material.size()+1);r.appearance=grade->second;
        if(!Rules::ValidRule(r)) { error="unsafe/unbounded Rule";return false; }
        rules.push_back(r);
    }
    if(destination.count+rules.size()>MaxRules) { error="rule capacity";return false; }
    for(size_t i=0;i<rules.size();++i)destination.Add(rules[i]);return true;
}
}
