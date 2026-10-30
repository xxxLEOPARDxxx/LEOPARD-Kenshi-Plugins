// Тесты ядра Dynamic Factions: только core/, без игры.
// Сборка и запуск: tests/build_tests.ps1. Код возврата - число провалов.
#include <cstdio>
#include <limits>
#include <string>
#include <vector>

#include "core/DfNames.h"
#include "core/DfState.h"
#include "core/DfBrain.h"
#include "core/DfWork.h"

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

// Запомненные жилы - в файле состояния, по сейвам (этап 2а).
static void test_state_nodes_roundtrip()
{
    std::vector<DfFactionState> all(1, MakeState("df", "n"));
    DfKnownNode a;
    a.x = -53421.5f;
    a.y = 908.0f;
    a.z = 2493.25f;
    a.itemSid = "1866-gamedata.base";
    a.stored = 17;
    all[0].nodes.push_back(a);
    a.itemSid = "1540824-GenMod.mod";
    a.stored = 0;
    all[0].nodes.push_back(a);
    std::vector<DfFactionState> back;
    CHECK(DfDeserialize(DfSerialize(all), back));
    CHECK(back.size() == 1 && back[0].nodes.size() == 2);
    if (back.size() != 1 || back[0].nodes.size() != 2)
        return;
    CHECK(back[0].nodes[0].x == -53421.5f && back[0].nodes[0].z == 2493.25f);
    CHECK(back[0].nodes[0].itemSid == "1866-gamedata.base" && back[0].nodes[0].stored == 17);
    CHECK(back[0].nodes[1].itemSid == "1540824-GenMod.mod");
}

static void test_state_bad_node_rejected()
{
    std::vector<DfFactionState> out;
    CHECK(!DfDeserialize("[faction 0]\nid=df\nnode=1|2|x|sid|3\n", out));
    CHECK(out.empty());
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
    CHECK(r.startMembers == 4 && r.startMoney == 5000 && r.maxMembers == 30);
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

// Вымирание объявляется только при загруженном доме: вдали численность
// держится на счётчиках незагруженных взводов - не доверяем ей вечную смерть
// фракции (ревью 23.10); вернётся игрок - пересчёт по живым персонажам.
static void test_brain_no_extinction_when_home_unloaded()
{
    DfFactionState s = Founded(10.0);
    CHECK(DfDecide(s, Snap(0, 500, 11.0, false), DfPhase1Rules()).empty());
    const std::vector<DfAction> acts = DfDecide(s, Snap(0, 500, 11.0, true), DfPhase1Rules());
    CHECK(acts.size() == 1 && acts[0].type == DF_EXTINCT);
}

// Идёт поход за рекрутом - его цена отложена, в снимке денег может стать
// меньше нуля: ни второго найма, ни техники (ревью этапа 1, #13).
static void test_brain_negative_money_no_spend()
{
    DfFactionState s = Founded(10.0);
    DfSnapshot snap = Snap(4, -500, 11.0, true);
    snap.recruitPrice = 0;
    const std::vector<DfAction> acts = DfDecide(s, snap, DfPhase1Rules());
    CHECK(FindAction(acts, DF_RECRUIT) < 0);
    CHECK(FindAction(acts, DF_RESEARCH) < 0);
    CHECK(FindAction(acts, DF_INCOME) >= 0);
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

// ---- работа: выбор жилы и роли (этап 2а) ----

static DfNodeOffer Offer(int value, float distance)
{
    DfNodeOffer o;
    o.value = value;
    o.distance = distance;
    return o;
}

static void test_rank_value_beats_distance()
{
    std::vector<DfNodeOffer> nodes;
    nodes.push_back(Offer(150, 100.0f));    // железо: 150 / 2 = 75
    nodes.push_back(Offer(800, 400.0f));    // золото: 800 / 5 = 160
    std::vector<int> order;
    DfRankNodes(nodes, order);
    CHECK(order.size() == 2 && order[0] == 1 && order[1] == 0);
}

static void test_rank_near_cheap_beats_far_rich()
{
    std::vector<DfNodeOffer> nodes;
    nodes.push_back(Offer(800, 1500.0f));   // 800 / 16 = 50
    nodes.push_back(Offer(150, 10.0f));     // 150 / 1.1 = 136
    std::vector<int> order;
    DfRankNodes(nodes, order);
    CHECK(order.size() == 2 && order[0] == 1);
}

static void test_rank_skips_no_value()
{
    std::vector<DfNodeOffer> nodes;
    nodes.push_back(Offer(0, 10.0f));
    nodes.push_back(Offer(70, 50.0f));
    std::vector<int> order;
    DfRankNodes(nodes, order);
    CHECK(order.size() == 1 && order[0] == 1);
}

static DfMember Member(int id, DfRole role, int node, int carried)
{
    DfMember m;
    m.id = id;
    m.able = true;
    m.hiring = false;
    m.carried = carried;
    m.node = node;
    m.role = role;
    return m;
}

static DfNodeState Node(int value, float distance, int stored, int maxMiners)
{
    DfNodeState n;
    n.value = value;
    n.distance = distance;
    n.stored = stored;
    n.cap = 50;
    n.maxMiners = maxMiners;
    return n;
}

static int CountRole(const std::vector<DfMember>& ms, DfRole role, int node)
{
    int n = 0;
    for (size_t i = 0; i < ms.size(); ++i)
        if (ms[i].role == role && (node < 0 || ms[i].node == node))
            ++n;
    return n;
}

static void test_work_home_unloaded_all_idle()
{
    std::vector<DfMember> ms;
    ms.push_back(Member(1, DF_ROLE_MINE, 0, 0));
    ms.push_back(Member(2, DF_ROLE_IDLE, -1, 0));
    ms[1].hiring = true;
    std::vector<DfNodeState> nodes(1, Node(150, 100.0f, 0, 3));
    DfAssignRoles(ms, nodes, false, true);
    CHECK(ms[0].role == DF_ROLE_IDLE && ms[0].node == -1);
    CHECK(ms[1].role == DF_ROLE_HIRE);
}

static void test_work_hiring_kept()
{
    std::vector<DfMember> ms(1, Member(1, DF_ROLE_MINE, 0, 0));
    ms[0].hiring = true;
    std::vector<DfNodeState> nodes(1, Node(150, 100.0f, 0, 3));
    DfAssignRoles(ms, nodes, true, true);
    CHECK(ms[0].role == DF_ROLE_HIRE);
}

static void test_work_miners_limit()
{
    std::vector<DfMember> ms;
    for (int i = 0; i < 3; ++i)
        ms.push_back(Member(i, DF_ROLE_IDLE, -1, 0));
    std::vector<DfNodeState> nodes(1, Node(150, 100.0f, 0, 2));
    DfAssignRoles(ms, nodes, true, true);
    CHECK(CountRole(ms, DF_ROLE_MINE, 0) == 2);
    CHECK(CountRole(ms, DF_ROLE_IDLE, -1) == 1);
}

static void test_work_haul_when_stored()
{
    std::vector<DfMember> ms;
    for (int i = 0; i < 3; ++i)
        ms.push_back(Member(i, DF_ROLE_MINE, 0, 0));
    std::vector<DfNodeState> nodes(1, Node(150, 100.0f, kDfHaulAt, 3));
    DfAssignRoles(ms, nodes, true, true);
    CHECK(CountRole(ms, DF_ROLE_HAUL, 0) == 1);
    CHECK(CountRole(ms, DF_ROLE_MINE, 0) == 2);
}

static void test_work_no_trader_money_no_haul()
{
    std::vector<DfMember> ms;
    for (int i = 0; i < 3; ++i)
        ms.push_back(Member(i, DF_ROLE_MINE, 0, 0));
    std::vector<DfNodeState> nodes(1, Node(150, 100.0f, 40, 3));
    DfAssignRoles(ms, nodes, true, false);
    CHECK(CountRole(ms, DF_ROLE_HAUL, -1) == 0);
}

static void test_work_hauler_keeps_load()
{
    std::vector<DfMember> ms(1, Member(1, DF_ROLE_HAUL, 0, 5));
    std::vector<DfNodeState> nodes(1, Node(150, 100.0f, 0, 3));
    DfAssignRoles(ms, nodes, true, false);
    CHECK(ms[0].role == DF_ROLE_HAUL && ms[0].node == 0);
}

static void test_work_hauler_done_back_to_mine()
{
    std::vector<DfMember> ms(1, Member(1, DF_ROLE_HAUL, 0, 0));
    std::vector<DfNodeState> nodes(1, Node(150, 100.0f, 0, 3));
    DfAssignRoles(ms, nodes, true, true);
    CHECK(ms[0].role == DF_ROLE_MINE && ms[0].node == 0);
}

static void test_work_full_node_moves_miner()
{
    std::vector<DfMember> ms;
    ms.push_back(Member(1, DF_ROLE_MINE, 0, 0));
    ms.push_back(Member(2, DF_ROLE_MINE, 0, 0));
    std::vector<DfNodeState> nodes;
    nodes.push_back(Node(150, 100.0f, 50, 3));     // полна
    nodes.push_back(Node(150, 300.0f, 0, 3));
    DfAssignRoles(ms, nodes, true, true);
    CHECK(CountRole(ms, DF_ROLE_HAUL, 0) == 1);
    CHECK(CountRole(ms, DF_ROLE_MINE, 1) == 1);
    CHECK(CountRole(ms, DF_ROLE_MINE, 0) == 0);
}

static void test_work_unable_idle()
{
    std::vector<DfMember> ms(1, Member(1, DF_ROLE_MINE, 0, 0));
    ms[0].able = false;
    std::vector<DfNodeState> nodes(1, Node(150, 100.0f, 0, 3));
    DfAssignRoles(ms, nodes, true, true);
    CHECK(ms[0].role == DF_ROLE_IDLE && ms[0].node == -1);
}

// Cargo belongs to its carrier, not to the one-empty-hauler reservation.
static void test_work_loaded_haulers_keep_all_cargo()
{
    std::vector<DfMember> ms;
    ms.push_back(Member(1, DF_ROLE_HAUL, 0, 5));
    ms.push_back(Member(2, DF_ROLE_HAUL, 0, 7));
    ms.push_back(Member(3, DF_ROLE_IDLE, -1, 0));
    std::vector<DfNodeState> nodes(1, Node(150, 100.0f, 50, 3));
    DfAssignRoles(ms, nodes, true, true);
    CHECK(ms[0].role == DF_ROLE_HAUL && ms[0].node == 0);
    CHECK(ms[1].role == DF_ROLE_HAUL && ms[1].node == 0);
    CHECK(ms[0].carried == 5 && ms[1].carried == 7);
    CHECK(ms[2].role == DF_ROLE_IDLE);
}

static void test_work_interrupted_carrier_recovers()
{
    std::vector<DfMember> ms(1, Member(1, DF_ROLE_HAUL, 0, 5));
    std::vector<DfNodeState> nodes(1, Node(150, 100.0f, 0, 3));
    ms[0].able = false;
    DfAssignRoles(ms, nodes, true, true);
    CHECK(ms[0].role == DF_ROLE_IDLE && ms[0].node == 0 && ms[0].carried == 5);
    ms[0].able = true;
    DfAssignRoles(ms, nodes, true, false);
    CHECK(ms[0].role == DF_ROLE_HAUL && ms[0].node == 0);
}

static void test_work_unloaded_carrier_keeps_origin()
{
    std::vector<DfMember> ms(1, Member(1, DF_ROLE_HAUL, 0, 5));
    std::vector<DfNodeState> nodes(1, Node(150, 100.0f, 0, 3));
    DfAssignRoles(ms, nodes, false, true);
    CHECK(ms[0].role == DF_ROLE_IDLE && ms[0].node == 0 && ms[0].carried == 5);
    DfAssignRoles(ms, nodes, true, false);
    CHECK(ms[0].role == DF_ROLE_HAUL && ms[0].node == 0);
}

static void test_work_hiring_carrier_keeps_origin()
{
    std::vector<DfMember> ms(1, Member(1, DF_ROLE_HAUL, 0, 5));
    std::vector<DfNodeState> nodes(1, Node(150, 100.0f, 0, 3));
    ms[0].hiring = true;
    DfAssignRoles(ms, nodes, true, true);
    CHECK(ms[0].role == DF_ROLE_HIRE && ms[0].node == 0);
    ms[0].hiring = false;
    DfAssignRoles(ms, nodes, true, false);
    CHECK(ms[0].role == DF_ROLE_HAUL && ms[0].node == 0 && ms[0].carried == 5);
}

static void test_work_cargo_recovers_from_idle_role()
{
    std::vector<DfMember> ms(1, Member(1, DF_ROLE_IDLE, 0, 5));
    std::vector<DfNodeState> nodes(1, Node(150, 100.0f, 0, 3));
    DfAssignRoles(ms, nodes, true, false);
    CHECK(ms[0].role == DF_ROLE_HAUL && ms[0].node == 0);
}

static void test_rank_rejects_invalid_distances()
{
    std::vector<DfNodeOffer> nodes;
    nodes.push_back(Offer(800, -100.0f));
    nodes.push_back(Offer(800, std::numeric_limits<float>::infinity()));
    nodes.push_back(Offer(800, std::numeric_limits<float>::quiet_NaN()));
    nodes.push_back(Offer(150, 0.0f));
    std::vector<int> order;
    DfRankNodes(nodes, order);
    CHECK(order.size() == 1);
    CHECK(!order.empty() && order[0] == 3);
}

static void test_work_invalid_node_releases_miner()
{
    std::vector<DfMember> ms(1, Member(1, DF_ROLE_MINE, 0, 0));
    std::vector<DfNodeState> nodes;
    nodes.push_back(Node(0, 100.0f, 0, 3));
    nodes.push_back(Node(150, 300.0f, 0, 3));
    DfAssignRoles(ms, nodes, true, true);
    CHECK(ms[0].role == DF_ROLE_MINE && ms[0].node == 1);
}

static void test_work_invalid_distance_releases_miner()
{
    std::vector<DfMember> ms(1, Member(1, DF_ROLE_MINE, 0, 0));
    std::vector<DfNodeState> nodes(1, Node(150, std::numeric_limits<float>::infinity(), 0, 3));
    DfAssignRoles(ms, nodes, true, true);
    CHECK(ms[0].role == DF_ROLE_IDLE && ms[0].node == -1);
}

#include "state_validation_tests.inc"
#include "haul_tests.inc"
#include "transfer_tests.inc"
#include "economy_reload_tests.inc"

static void test_economy_rules_no_created_income_or_tech()
{
    DfRules r = DfPhase2Rules();
    CHECK(r.incomePerMemberHour == 0 && r.techPointCost == 0);
    DfFactionState s = Founded(10.0);
    DfSnapshot snap = Snap(4, 7000, 11.0, true);
    snap.recruitPrice = 3000;
    std::vector<DfAction> a = DfDecide(s, snap, r);
    CHECK(FindAction(a, DF_INCOME) < 0);
    CHECK(FindAction(a, DF_RESEARCH) < 0);
    CHECK(FindAction(a, DF_RECRUIT) >= 0);
    snap.money = 0;
    a = DfDecide(s, snap, r);
    CHECK(FindAction(a, DF_INCOME) < 0 && FindAction(a, DF_RECRUIT) < 0);
}

static void test_work_small_node_still_hauls()
{
    std::vector<DfMember> ms(1, Member(1, DF_ROLE_MINE, 0, 0));
    std::vector<DfNodeState> nodes(1, Node(150, 10.0f, 5, 1));
    nodes[0].cap = 5;
    DfAssignRoles(ms, nodes, true, true);
    CHECK(ms[0].role == DF_ROLE_HAUL && ms[0].node == 0);
}

int main()
{
    test_harness_runs();
    RunStateValidationTests();
    RunHaulTests();
    RunTransferTests();
    RunEconomyReloadTests();
    test_economy_rules_no_created_income_or_tech();
    test_work_small_node_still_hauls();
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
    test_state_nodes_roundtrip();
    test_state_bad_node_rejected();
    test_state_events_capped();
    test_state_garbage_rejected();
    test_state_empty_ok();
    test_brain_rules_values();
    test_brain_found_only_when_home_loaded();
    test_brain_extinct();
    test_brain_no_extinction_when_home_unloaded();
    test_brain_negative_money_no_spend();
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
    test_rank_value_beats_distance();
    test_rank_near_cheap_beats_far_rich();
    test_rank_skips_no_value();
    test_work_home_unloaded_all_idle();
    test_work_hiring_kept();
    test_work_miners_limit();
    test_work_haul_when_stored();
    test_work_no_trader_money_no_haul();
    test_work_hauler_keeps_load();
    test_work_hauler_done_back_to_mine();
    test_work_full_node_moves_miner();
    test_work_unable_idle();
    test_work_loaded_haulers_keep_all_cargo();
    test_work_interrupted_carrier_recovers();
    test_work_unloaded_carrier_keeps_origin();
    test_work_hiring_carrier_keeps_origin();
    test_work_cargo_recovers_from_idle_role();
    test_rank_rejects_invalid_distances();
    test_work_invalid_node_releases_miner();
    test_work_invalid_distance_releases_miner();
    std::printf("%d checks, %d failed\n", g_checks, g_failed);
    return g_failed;
}
