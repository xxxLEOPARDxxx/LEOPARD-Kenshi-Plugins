// Тесты ядра Dynamic Factions: только core/, без игры.
// Сборка и запуск: tests/build_tests.ps1. Код возврата - число провалов.
#include <cstdio>
#include <string>
#include <vector>

#include "core/DfNames.h"
#include "core/DfState.h"

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

int main()
{
    test_harness_runs();
    test_names_deterministic();
    test_names_skip_taken();
    test_names_all_races_ascii();
    test_names_unknown_race_is_human();
    test_names_exhausted_get_numeral();
    test_state_roundtrip();
    test_state_events_capped();
    test_state_garbage_rejected();
    test_state_empty_ok();
    std::printf("%d checks, %d failed\n", g_checks, g_failed);
    return g_failed;
}
