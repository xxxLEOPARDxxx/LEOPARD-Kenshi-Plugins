// Тесты ядра Dynamic Factions: только core/, без игры.
// Сборка и запуск: tests/build_tests.ps1. Код возврата - число провалов.
#include <cstdio>

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

int main()
{
    test_harness_runs();
    std::printf("%d checks, %d failed\n", g_checks, g_failed);
    return g_failed;
}
