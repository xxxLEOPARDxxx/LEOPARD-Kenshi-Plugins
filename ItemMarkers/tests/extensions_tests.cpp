#include "../GradeExtensions.h"
#include "../GradeProfiles.h"
#include <cstdio>
#include <cstdlib>
#include <new>
static unsigned allocations=0;
void* operator new(size_t n) { ++allocations;void* p=std::malloc(n?n:1);if(!p)throw std::bad_alloc();return p; }
void operator delete(void* p) { std::free(p); }
void* operator new[](size_t n) { ++allocations;void* p=std::malloc(n?n:1);if(!p)throw std::bad_alloc();return p; }
void operator delete[](void* p) { std::free(p); }
#define CHECK(x) do { ++checks; if(!(x)) { std::printf("FAIL line %d: %s\n",__LINE__,#x); std::exit(1); } } while(0)
static unsigned checks=0,calls=0;
static int __cdecl Backpack(const IM_ItemView* v,IM_Appearance* a,void*) {
    ++calls; if(std::strcmp(v->material_id,"asur.high")!=0) return 0;
    a->size=sizeof(*a);a->rank=4;a->rgb=0x31ef87;return 1;
}
struct Invoke { int operator()(IM_Query q,const IM_ItemView& v,IM_Appearance& a,void* c) { return q(&v,&a,c); } };
struct Palette { uint32_t operator()(const char*,uint32_t fallback) { return fallback; } };
static int __cdecl Broken(const IM_ItemView*,IM_Appearance* a,void*) { a->rank=99; return 1; }
static int __cdecl Reentrant(const IM_ItemView*,IM_Appearance*,void* c) {
    IMExt::Registry* r=static_cast<IMExt::Registry*>(c); IM_Handle h=0;
    IM_Provider p={sizeof(p),"nested",0,Backpack,0};CHECK(r->Add(p,h)==IM_BUSY);return 0;
}
int main() {
    IMExt::Rules rules; IMExt::Rule rule;
    std::strcpy(rule.item,"backpack.mod");std::strcpy(rule.material,"asur.high");
    rule.appearance.size=sizeof(IM_Appearance);rule.appearance.rank=4;rule.appearance.rgb=0x31ef87;
    CHECK(rules.Add(rule));
    IM_ItemView v={sizeof(v),1,0,"backpack.mod","asur.high",-1,0}; IM_Appearance a={};
    CHECK(rules.Query(v,a));CHECK(a.rgb==0x31ef87&&a.rank==4);
    v.material_id="asur.low";CHECK(!rules.Query(v,a));v.material_id="asur.high";
    rule.low=10;CHECK(rules.Add(rule));CHECK(rules.Query(v,a)); // -1 is not Gear level zero
    IMExt::Rule specific=rule;specific.low=-1;specific.priority=4;specific.appearance.rgb=0x123456;
    CHECK(rules.Add(specific));CHECK(rules.Query(v,a)&&a.rgb==0x123456);
    rule.item[0]=rule.material[0]=0;rule.type=-1;CHECK(!rules.Add(rule));
    IMExt::Registry r; Invoke invoke; IM_Handle h=0;
    IM_Provider p={sizeof(p),"asur.backpacks",10,Backpack,0};
    CHECK(r.Add(p,h)==IM_OK&&h!=0);CHECK(r.Contains(h));
    IM_Handle dup=0;CHECK(r.Add(p,dup)==IM_INVALID);
    CHECK(r.Query(v,a,invoke));CHECK(calls==1&&a.rgb==0x31ef87);
    const unsigned allocated=allocations;
    for(unsigned i=0;i<10000;++i) { CHECK(r.Query(v,a,invoke));CHECK(rules.Query(v,a)); }
    CHECK(allocations==allocated);
    p.name="bad";p.priority=20;p.query=Broken;CHECK(r.Add(p,dup)==IM_OK);
    CHECK(r.Query(v,a,invoke)&&a.rank==4);CHECK(r.slots[0].disabled);
    CHECK(r.Remove(dup)==IM_OK);CHECK(r.Remove(dup)==IM_NOT_FOUND);
    p.name="recursive";p.query=Reentrant;p.context=&r;CHECK(r.Add(p,dup)==IM_OK);
    CHECK(r.Query(v,a,invoke));CHECK(r.Remove(dup)==IM_OK);
    CHECK(r.Remove(h)==IM_OK);CHECK(!r.Query(v,a,invoke));CHECK(!r.Contains(h));
    p.query=Backpack;p.context=0;
    char names[IMExt::MaxProviders][64];IM_Handle hs[IMExt::MaxProviders];
    for(unsigned i=0;i<IMExt::MaxProviders;++i) { std::sprintf(names[i],"provider.%u",i);p.name=names[i];CHECK(r.Add(p,hs[i])==IM_OK); }
    p.name="overflow";CHECK(r.Add(p,h)==IM_FULL);
    for(unsigned i=0;i<IMExt::MaxProviders;++i) CHECK(r.Remove(hs[i])==IM_OK);
    CHECK(r.count==0);
    Palette palette;std::string error;IMExt::Rules parsed;
    const std::string profile="[Grade.high]\nRank=4\nColour=#31ef87\n[Rule.pack]\nItemID=backpack.mod\nMaterialID=asur.high\nGrade=high\n";
    CHECK(IMExt::LoadProfile(profile,parsed,palette,error));CHECK(parsed.Query(v,a)&&a.rgb==0x31ef87);
    const unsigned before=parsed.count;
    CHECK(!IMExt::LoadProfile(profile+"[Rule.broken]\nGrade=missing\n",parsed,palette,error));CHECK(parsed.count==before);
    CHECK(!IMExt::LoadProfile(profile+"[Rule.pack]\nGrade=high\n",parsed,palette,error));CHECK(parsed.count==before);
    CHECK(!IMExt::LoadProfile("[Grade.bad]\nRank=4\nColour=#1234567\n",parsed,palette,error));
    CHECK(!IMExt::LoadProfile("[Rule.no-selector]\nGrade=high\n",parsed,palette,error));
    CHECK(!IMExt::LoadProfile(std::string(65537,'x'),parsed,palette,error));
    CHECK(IMExt::LoadProfile("[Profile]\nEnabled=0\n",parsed,palette,error)&&parsed.count==before);
    CHECK(!IMExt::LoadProfile("[Grade.bad]\nRank=4\nRank=3\nColour=#123456\n",parsed,palette,error));
    CHECK(!IMExt::LoadProfile("[Grade.bad]\nRank=4\nColour=#123456\nTypo=1\n",parsed,palette,error));
    IMExt::Rule bounded;std::strcpy(bounded.item,"capacity.item");bounded.appearance.size=sizeof(IM_Appearance);bounded.appearance.rank=0;
    while(parsed.count<IMExt::MaxRules)CHECK(parsed.Add(bounded));CHECK(!parsed.Add(bounded));
    bounded.item[0]=0;bounded.type=-1;CHECK(!IMExt::Rules::ValidRule(bounded));
    std::memset(bounded.item,'a',sizeof(bounded.item));CHECK(!IMExt::Rules::ValidRule(bounded));
    CHECK(!IMExt::Name(""));CHECK(!IMExt::Name("invalid name"));CHECK(!IMExt::Name(std::string(64,'a').c_str()));
    std::printf("%u extension checks passed\n",checks);return 0;
}
