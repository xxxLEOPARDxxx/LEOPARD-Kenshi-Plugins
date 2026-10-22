// Тесты ядра Dynamic Factions: только core/, без игры.
// Сборка и запуск: tests/build_tests.ps1. Код возврата - число провалов.
#include <cstdio>
#include <string>
#include <vector>

#include "core/DfNames.h"
#include "core/DfState.h"
#include "core/DfBrain.h"

static int g_failed = 0;
static int g_checks = 0;

#define CHECK(cond)                                                           \
    do                                                                        \
    {                                                                         \
        ++g_checks;                                                           \
        if (!(cond))                                                          \
        {                                                                     \
            ++g_failed;                                                       \
            std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond);        \
        }                                                                     \
    } while (0)

static void test_harness_runs()
{
    CHECK(1 + 1 == 2);
}

static bool IsAscii(const std::string& s)
{
    for (size_t i = 0; i < s.size(); ++i)
        if (static_cast<unsigned char>(s[i]) > 0x7E || static_cast<unsigned char>(s[i]) < 0x20)
            return false;
    return true;
}

static void test_names_deterministic()
{
    const std::vector<std::string> none;
    CHECK(DfMakeName("human", 7, none) == DfMakeName("human", 7, none));
}

static void test_names_skip_taken()
{
    const std::vector<std::string> none;
    std::vector<std::string> taken;
    taken.push_back(DfMakeName("human", 7, none));
    const std::string second = DfMakeName("human", 7, taken);
    CHECK(second != taken[0]);
    CHECK(!second.empty());
}

static void test_names_all_races_ascii()
{
    const std::vector<std::string> none;
    const char* races[] = { "human", "shek", "skeleton", "hive" };
    for (int i = 0; i < 4; ++i)
    {
        const std::string n = DfMakeName(races[i], 3, none);
        CHECK(!n.empty());
        CHECK(IsAscii(n));
    }
}

static void test_names_unknown_race_is_human()
{
    const std::vector<std::string> none;
    CHECK(DfMakeName("unknown", 1, none) == DfMakeName("human", 1, none));
}

static void test_names_exhausted_get_numeral()
{
    // все 256 сочетаний заняты - имя с суффиксом
    std::vector<std::string> taken;
    for (unsigned seed = 0; seed < 4096 && taken.size() < 256; ++seed)
    {
        const std::string n = DfMakeName("human", seed, taken);
        if (n.find(' ') != std::string::npos && n.find(" II") == std::string::npos)
            taken.push_back(n);
    }
    const std::string n = DfMakeName("human", 1, taken);
    CHECK(n.find(" II") != std::string::npos);
}

static const char* const kDictText =
    "# test\n"
    "[human]\n"
    "noun=Jackals|p\n"
    "noun=Hand|f\n"
    "adj=Iron|Irona|Irono|Ironi\n"
    "[shek]\n"
    "noun=Horn|m\n"
    "adj=Red|Reda|Redo|Redi\n";

static void test_dict_parse()
{
    DfNameDict d;
    CHECK(DfParseNameDict(kDictText, d));
    CHECK(d.size() == 2);
    CHECK(d["human"].nouns.size() == 2 && d["human"].adj.size() == 1);
    CHECK(d["human"].nouns[1].gender == 1);
    CHECK(d["shek"].adj[0].forms[2] == "Redo");
}

static void test_dict_bad_line_rejected()
{
    DfNameDict d;
    CHECK(!DfParseNameDict("[human]\nnoun=Jackals|x\n", d));
    CHECK(!DfParseNameDict("[human]\nadj=One|Two\n", d));
    CHECK(!DfParseNameDict("noun=Jackals|p\n", d));
}

static void test_dict_agreement()
{
    DfNameDict d;
    DfParseNameDict(kDictText, d);
    const std::vector<std::string> none;
    std::vector<std::string> seen;
    for (unsigned seed = 0; seed < 50; ++seed)
        seen.push_back(DfMakeNameFrom(d, "human", seed, none));
    bool plural = false, feminine = false;
    for (size_t i = 0; i < seen.size(); ++i)
    {
        plural = plural || seen[i] == "Ironi Jackals";
        feminine = feminine || seen[i] == "Irona Hand";
        CHECK(seen[i] == "Ironi Jackals" || seen[i] == "Irona Hand");
    }
    CHECK(plural && feminine);
}

static void test_dict_fallbacks()
{
    DfNameDict d;
    DfParseNameDict(kDictText, d);
    const std::vector<std::string> none;
    CHECK(DfMakeNameFrom(d, "hive", 2, none) == DfMakeNameFrom(d, "human", 2, none));
    DfNameDict empty;
    CHECK(DfMakeNameFrom(empty, "human", 2, none) == DfMakeName("human", 2, none));
}

static void test_dict_taken_numeral()
{
    DfNameDict d;
    DfParseNameDict(kDictText, d);
    std::vector<std::string> taken;
    taken.push_back("Ironi Jackals");
    taken.push_back("Irona Hand");
    const std::string n = DfMakeNameFrom(d, "human", 0, taken);
    CHECK(n.find(" II") != std::string::npos);
}

static DfFactionState MakeState(const std::string& id, const std::string& name)
{
    DfFactionState s;
    s.recordId = id;
    s.name = name;
    s.race = "human";
    s.founded = true;
    s.extinct = false;
    s.foundedHour = 12.5;
    s.lastTickHour = 40.25;
    s.techLevel = 3;
    s.techPoints = 170;
    return s;
}

static void test_state_roundtrip()
{
    std::vector<DfFactionState> all;
    all.push_back(MakeState("df-faction-human-1", "Iron Jackals = best\nfriends"));
    DfAddEvent(all[0], 13.0, "found 4");
    DfAddEvent(all[0], 14.0, "recruit a=b");
    all.push_back(MakeState("df-faction-human-2", "Rust Hounds"));
    all[1].founded = false;
    all[1].extinct = true;
    all[1].techLevel = 0;
    all[1].techPoints = 0;

    std::vector<DfFactionState> back;
    CHECK(DfDeserialize(DfSerialize(all), back));
    CHECK(back.size() == 2);
    if (back.size() != 2)
        return;
    for (size_t i = 0; i < 2; ++i)
    {
        CHECK(back[i].recordId == all[i].recordId);
        CHECK(back[i].name == all[i].name);
        CHECK(back[i].race == all[i].race);
        CHECK(back[i].founded == all[i].founded);
        CHECK(back[i].extinct == all[i].extinct);
        CHECK(back[i].foundedHour == all[i].foundedHour);
        CHECK(back[i].lastTickHour == all[i].lastTickHour);
        CHECK(back[i].techLevel == all[i].techLevel);
        CHECK(back[i].techPoints == all[i].techPoints);
        CHECK(back[i].events.size() == all[i].events.size());
    }
    CHECK(back[0].events.size() == 2 && back[0].events[1].hour == 14.0 && back[0].events[1].text == "recruit a=b");
}

static void test_state_events_capped()
{
    DfFactionState s = MakeState("x", "y");
    char buf[16];
    for (int i = 0; i < 25; ++i)
    {
        sprintf_s(buf, sizeof(buf), "e%d", i);
        DfAddEvent(s, i, buf);
    }
    CHECK(s.events.size() == 20);
    CHECK(s.events.front().text == "e5");
    CHECK(s.events.back().text == "e24");
}

static void test_state_garbage_rejected()
{
    std::vector<DfFactionState> out;
    out.push_back(MakeState("x", "y"));
    CHECK(!DfDeserialize("[faction 0]\nname", out));
    CHECK(out.empty());
}

static void test_state_empty_ok()
{
    std::vector<DfFactionState> out;
    out.push_back(MakeState("x", "y"));
    CHECK(DfDeserialize("", out));
    CHECK(out.empty());
}

static int FindAction(const std::vector<DfAction>& acts, DfActionType type)
{
    for (size_t i = 0; i < acts.size(); ++i)
        if (acts[i].type == type)
            return static_cast<int>(i);
    return -1;
}

static DfFactionState Founded(double lastTick)
{
    DfFactionState s;
    s.recordId = "df-faction-human-1";
    s.name = "Iron Jackals";
    s.race = "human";
    s.founded = true;
    s.foundedHour = lastTick;
    s.lastTickHour = lastTick;
    return s;
}

static DfSnapshot Snap(int members, int money, double hour, bool homeLoaded)
{
    DfSnapshot snap;
    snap.members = members;
    snap.money = money;
    snap.hour = hour;
    snap.homeLoaded = homeLoaded;
    snap.recruitPrice = -1;
    return snap;
}

static void test_brain_rules_values()
{
    const DfRules r = DfPhase1Rules();
    CHECK(r.startMembers == 4 && r.startMoney == 5000 && r.recruitCost == 1500 && r.maxMembers == 30);
    CHECK(r.incomePerMemberHour == 15 && r.techPointCost == 200 && r.maxTech == 10);
}

static void test_brain_found_only_when_home_loaded()
{
    DfFactionState s;
    s.recordId = "df-faction-human-1";
    CHECK(DfDecide(s, Snap(0, 0, 10.0, false), DfPhase1Rules()).empty());
    const std::vector<DfAction> acts = DfDecide(s, Snap(0, 0, 10.0, true), DfPhase1Rules());
    CHECK(acts.size() == 1 && acts[0].type == DF_FOUND);
    DfApplyBookkeeping(s, acts, 10.0);
    CHECK(s.founded && s.foundedHour == 10.0 && s.lastTickHour == 10.0);
}

static void test_brain_extinct()
{
    DfFactionState s = Founded(10.0);
    const std::vector<DfAction> acts = DfDecide(s, Snap(0, 500, 11.0, true), DfPhase1Rules());
    CHECK(acts.size() == 1 && acts[0].type == DF_EXTINCT);
    DfApplyBookkeeping(s, acts, 11.0);
    CHECK(s.extinct);
    CHECK(DfDecide(s, Snap(4, 5000, 12.0, true), DfPhase1Rules()).empty());
}

static void test_brain_income_no_money_no_recruit()
{
    DfFactionState s = Founded(10.0);
    const std::vector<DfAction> acts = DfDecide(s, Snap(4, 0, 12.0, true), DfPhase1Rules());
    const int inc = FindAction(acts, DF_INCOME);
    CHECK(inc >= 0 && acts[inc].amount == 120);
    CHECK(FindAction(acts, DF_RECRUIT) < 0);
    CHECK(FindAction(acts, DF_RESEARCH) < 0);
}

static void test_brain_recruit_then_research()
{
    DfFactionState s = Founded(10.0);
    DfSnapshot snap = Snap(4, 7000, 11.0, true);
    snap.recruitPrice = 1500;
    const std::vector<DfAction> acts = DfDecide(s, snap, DfPhase1Rules());
    const int inc = FindAction(acts, DF_INCOME);
    const int rec = FindAction(acts, DF_RECRUIT);
    const int res = FindAction(acts, DF_RESEARCH);
    CHECK(inc >= 0 && acts[inc].amount == 60);
    CHECK(rec >= 0 && acts[rec].amount == 1500);
    // 7060 - 1500 - 1000 запаса - 4000 фонда найма = 560 -> 2 очка
    CHECK(res >= 0 && acts[res].amount == 2);
    CHECK(rec < res);
}

static void test_brain_no_one_in_bar()
{
    DfFactionState s = Founded(10.0);
    const std::vector<DfAction> acts = DfDecide(s, Snap(4, 10000, 11.0, true), DfPhase1Rules());
    const int res = FindAction(acts, DF_RESEARCH);
    CHECK(FindAction(acts, DF_RECRUIT) < 0);
    CHECK(res >= 0 && acts[res].amount == 5);
}

// Пока людей меньше предела, на технику идёт только то, что сверх запаса и
// фонда найма (23.10: все деньги уходили в технику, нанять было не на что).
static void test_brain_hire_fund_kept()
{
    DfFactionState s = Founded(10.0);
    CHECK(FindAction(DfDecide(s, Snap(4, 4900, 11.0, true), DfPhase1Rules()), DF_RESEARCH) < 0);
    const std::vector<DfAction> full = DfDecide(s, Snap(30, 4900, 11.0, true), DfPhase1Rules());
    const int res = FindAction(full, DF_RESEARCH);
    CHECK(res >= 0 && full[res].amount == 5);       // 30 человек - фонд не нужен
}

// Рекрут виден, но денег мало - копим на него, на технику ни очка (23.10:
// рекруты по 6000, у фракции 5000, и то, что сверх, уходило в технику).
static void test_brain_saving_for_visible_recruit()
{
    DfFactionState s = Founded(10.0);
    DfSnapshot snap = Snap(4, 6500, 11.0, true);
    snap.recruitPrice = 6000;
    const std::vector<DfAction> acts = DfDecide(s, snap, DfPhase1Rules());
    CHECK(FindAction(acts, DF_RECRUIT) < 0);
    CHECK(FindAction(acts, DF_RESEARCH) < 0);
    snap.money = 20000;
    const std::vector<DfAction> rich = DfDecide(s, snap, DfPhase1Rules());
    const int rec = FindAction(rich, DF_RECRUIT);
    const int res = FindAction(rich, DF_RESEARCH);
    CHECK(rec >= 0 && rich[rec].amount == 6000);
    // 20060 - 6000 - 1000 - 4000 фонда на следующего = 9060 -> потолок 5
    CHECK(res >= 0 && rich[res].amount == 5);
}

static void test_brain_recruit_too_expensive()
{
    DfFactionState s = Founded(10.0);
    DfSnapshot snap = Snap(4, 3000, 11.0, true);
    snap.recruitPrice = 6000;
    CHECK(FindAction(DfDecide(s, snap, DfPhase1Rules()), DF_RECRUIT) < 0);
}

static void test_brain_free_recruit()
{
    DfFactionState s = Founded(10.0);
    DfSnapshot snap = Snap(4, 0, 11.0, true);
    snap.recruitPrice = 0;
    CHECK(FindAction(DfDecide(s, snap, DfPhase1Rules()), DF_RECRUIT) < 0);     // запас 1000 не набран
    snap.money = 2000;
    const std::vector<DfAction> acts = DfDecide(s, snap, DfPhase1Rules());
    const int rec = FindAction(acts, DF_RECRUIT);
    CHECK(rec >= 0 && acts[rec].amount == 0);
}

static void test_brain_no_recruit_at_max()
{
    DfFactionState s = Founded(10.0);
    DfSnapshot snap = Snap(30, 100000, 11.0, true);
    snap.recruitPrice = 100;
    const std::vector<DfAction> acts = DfDecide(s, snap, DfPhase1Rules());
    CHECK(FindAction(acts, DF_RECRUIT) < 0);
}

static void test_brain_no_recruit_when_home_unloaded()
{
    DfFactionState s = Founded(10.0);
    DfSnapshot snap = Snap(4, 100000, 11.0, false);
    snap.recruitPrice = 100;
    const std::vector<DfAction> acts = DfDecide(s, snap, DfPhase1Rules());
    CHECK(FindAction(acts, DF_RECRUIT) < 0);
    CHECK(FindAction(acts, DF_INCOME) >= 0);
}

static void test_brain_hours_capped()
{
    DfFactionState s = Founded(10.0);
    const std::vector<DfAction> acts = DfDecide(s, Snap(1, 0, 60.0, true), DfPhase1Rules());
    const int inc = FindAction(acts, DF_INCOME);
    CHECK(inc >= 0 && acts[inc].amount == 24 * 15);
}

static void test_brain_research_capped_per_tick()
{
    DfFactionState s = Founded(10.0);
    const std::vector<DfAction> acts = DfDecide(s, Snap(30, 100000, 11.0, true), DfPhase1Rules());
    const int res = FindAction(acts, DF_RESEARCH);
    CHECK(res >= 0 && acts[res].amount == 5);
}

static void test_brain_less_than_hour_nothing()
{
    DfFactionState s = Founded(10.0);
    CHECK(DfDecide(s, Snap(4, 100000, 10.9, true), DfPhase1Rules()).empty());
    CHECK(DfDecide(s, Snap(4, 100000, 5.0, true), DfPhase1Rules()).empty());
}

static void test_brain_tick_advances_whole_hours()
{
    DfFactionState s = Founded(10.0);
    const std::vector<DfAction> acts = DfDecide(s, Snap(4, 0, 12.5, true), DfPhase1Rules());
    DfApplyBookkeeping(s, acts, 12.5);
    CHECK(s.lastTickHour == 12.0);
}

static void test_brain_level_up_carries()
{
    DfFactionState s = Founded(10.0);
    s.techPoints = 99;
    DfAction a;
    a.type = DF_RESEARCH;
    a.amount = 1;
    DfApplyBookkeeping(s, std::vector<DfAction>(1, a), 11.0);
    CHECK(s.techLevel == 1 && s.techPoints == 0);
    s.techPoints = 199;
    a.amount = 5;
    DfApplyBookkeeping(s, std::vector<DfAction>(1, a), 12.0);
    CHECK(s.techLevel == 2 && s.techPoints == 4);
}

// Решение о найме - только начало похода в бар; событие «recruit» пишет
// адаптер, когда человек и правда перешёл во фракцию.
static void test_brain_recruit_decision_no_event()
{
    DfFactionState s = Founded(10.0);
    DfAction a;
    a.type = DF_RECRUIT;
    a.amount = 1500;
    DfApplyBookkeeping(s, std::vector<DfAction>(1, a), 11.0);
    CHECK(s.events.empty());
}

static void test_brain_max_tech_stops()
{
    DfFactionState s = Founded(10.0);
    s.techLevel = 10;
    s.techPoints = 0;
    DfAction a;
    a.type = DF_RESEARCH;
    a.amount = 3;
    DfApplyBookkeeping(s, std::vector<DfAction>(1, a), 11.0);
    CHECK(s.techLevel == 10 && s.techPoints == 0);
    CHECK(FindAction(DfDecide(s, Snap(30, 100000, 12.0, true), DfPhase1Rules()), DF_RESEARCH) < 0);
}

int main()
{
    test_harness_runs();
    test_names_deterministic();
    test_names_skip_taken();
    test_names_all_races_ascii();
    test_names_unknown_race_is_human();
    test_names_exhausted_get_numeral();
    test_dict_parse();
    test_dict_bad_line_rejected();
    test_dict_agreement();
    test_dict_fallbacks();
    test_dict_taken_numeral();
    test_state_roundtrip();
    test_state_events_capped();
    test_state_garbage_rejected();
    test_state_empty_ok();
    test_brain_rules_values();
    test_brain_found_only_when_home_loaded();
    test_brain_extinct();
    test_brain_income_no_money_no_recruit();
    test_brain_recruit_then_research();
    test_brain_no_one_in_bar();
    test_brain_hire_fund_kept();
    test_brain_recruit_too_expensive();
    test_brain_saving_for_visible_recruit();
    test_brain_free_recruit();
    test_brain_no_recruit_at_max();
    test_brain_no_recruit_when_home_unloaded();
    test_brain_hours_capped();
    test_brain_research_capped_per_tick();
    test_brain_less_than_hour_nothing();
    test_brain_tick_advances_whole_hours();
    test_brain_level_up_carries();
    test_brain_max_tech_stops();
    test_brain_recruit_decision_no_event();
    std::printf("%d checks, %d failed\n", g_checks, g_failed);
    return g_failed;
}
