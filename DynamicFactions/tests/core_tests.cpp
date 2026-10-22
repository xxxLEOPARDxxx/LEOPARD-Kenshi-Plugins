// Тесты ядра Dynamic Factions: только core/, без игры.
// Сборка и запуск: tests/build_tests.ps1. Код возврата - число провалов.
#include <cstdio>
#include <string>
#include <vector>

#include "core/DfNames.h"

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

int main()
{
    test_harness_runs();
    test_names_deterministic();
    test_names_skip_taken();
    test_names_all_races_ascii();
    test_names_unknown_race_is_human();
    test_names_exhausted_get_numeral();
    std::printf("%d checks, %d failed\n", g_checks, g_failed);
    return g_failed;
}
