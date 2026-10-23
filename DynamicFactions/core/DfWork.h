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
    DF_ROLE_HIRE                // посланник найма - работу не получает
};

struct DfMember
{
    int id;
    bool able;                  // жив, в сознании, не в бою
    bool hiring;                // занят наймом
    int carried;                // руды при себе (носильщик)
    int node;                   // индекс жилы или -1
    DfRole role;
};

struct DfNodeState
{
    int value;                  // цена того, что даёт жила
    float distance;             // от дома
    int stored;                 // руды в хранилище жилы
    int cap;                    // предел хранилища (50)
    int maxMiners;
};

const int kDfHaulAt = 30;       // в жиле столько - зовём носильщика

// Выгода жилы: value / (1 + distance / 100). Индексы по убыванию выгоды;
// value <= 0 - не попадает.
struct DfNodeOffer
{
    int value;
    float distance;
};
void DfRankNodes(const std::vector<DfNodeOffer>& nodes, std::vector<int>& order);

// Пишет role и node каждому. Дом не загружен - все IDLE (кроме найма);
// носильщик с грузом остаётся носильщиком; у жилы с запасом >= kDfHaulAt
// и без носильщика (если торговцу есть чем платить) - один носильщик, из её
// копателей, иначе из свободных; копатели - к выгодным жилам с местом.
void DfAssignRoles(std::vector<DfMember>& members, const std::vector<DfNodeState>& nodes, bool homeLoaded,
                   bool traderHasMoney);
