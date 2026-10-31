#pragma once
// Работа фракции (этап 2а): кто копает, кто носит руду, кто ждёт. Без игры.
//
// Руда копится в хранилище самой жилы (предел 50 - на пределе копание
// стоит; пользователь, 23.10). Копатели работают, пока в жиле есть место;
// носильщик забирает накопленное и несёт торговцу.

#include <vector>

enum DfRole
{
    DF_ROLE_IDLE,
    DF_ROLE_MINE,
    DF_ROLE_HAUL,
    DF_ROLE_HIRE,               // посланник найма - работу не получает
    DF_ROLE_SUPPLY              // настоящее снабжение - приоритет над добычей
};

struct DfMember
{
    int id;
    bool able;                  // жив, в сознании, не в бою
    bool hiring;                // занят наймом
    bool supplying;
    int carried;                // руды при себе (носильщик)
    int node;                   // индекс жилы или -1
    DfRole role;
    bool canHaul;
    DfMember() : id(0), able(false), hiring(false), supplying(false), carried(0), node(-1), role(DF_ROLE_IDLE), canHaul(true) {}
};

struct DfNodeState
{
    int value;                  // цена того, что даёт жила
    float distance;             // от дома
    int stored;                 // руды в хранилище жилы
    int cap;                    // предел хранилища (50)
    int maxMiners;
};

const int kDfHaulAt = 1;        // First real output starts delivery; take all available units on arrival.

// Выгода жилы: value / (1 + distance / 100). Индексы по убыванию выгоды;
// value <= 0, отрицательные/NaN/бесконечные расстояния - не попадают.
struct DfNodeOffer
{
    int value;
    float distance;
};
void DfRankNodes(const std::vector<DfNodeOffer>& nodes, std::vector<int>& order);

// Пишет role и node каждому. Дом не загружен - все IDLE (кроме найма);
// при грузе сохраняется node для продолжения после перерыва. Работоспособный
// человек с грузом продолжает перевозку независимо от прежней роли и других
// носильщиков; у жилы с запасом >= kDfHaulAt
// и без носильщика (если торговцу есть чем платить) - один носильщик, из её
// копателей, иначе из свободных; копатели - к выгодным жилам с местом.
void DfAssignRoles(std::vector<DfMember>& members, const std::vector<DfNodeState>& nodes, bool homeLoaded,
                   bool traderHasMoney);
