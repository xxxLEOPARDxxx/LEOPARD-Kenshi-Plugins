#pragma once
// Адаптер ядра к игре: фракция игры, Хаб, основание, снимок, файл состояния.
// Только главный поток. Каждое обращение к объектам игры - под SEH: сбой -
// ErrorLog один раз на место, функция возвращает «не вышло».

#include <string>
#include <vector>

#include "core/DfBrain.h"

class Faction;
class TownBase;

// Записи DynamicFactions.mod (tools/df_make_mod.py) и город Хаб.
extern const char* const kDfFactionRecord;      // FACTION, копия Nameless
extern const char* const kDfSquadRecord;        // SQUAD_TEMPLATE, копия Drifter squad
extern const char* const kDfHubRecord;          // Хаб

// Фракция игры по записи FACTION; NULL - записи нет (мод не включён).
Faction* DfBindFaction(const std::string& recordId);

// Хаб, когда его область загружена (TownBase::isActive); иначе NULL.
TownBase* DfFindHub();

// Игровое время в часах от начала игры; < 0 - мира нет.
double DfGameHour();

// Численность, деньги, время, загружен ли дом.
bool DfTakeSnapshot(Faction* f, DfSnapshot& out);

// Стартовый отряд в Хабе и стартовые деньги. false - отряд не создан.
bool DfFound(Faction* f, TownBase* hub, int members, int money);

// Имя фракции в игре = name (ставится только при расхождении).
void DfApplyName(Faction* f, const std::string& name);

// Названия всех фракций игры - чтобы новое имя не совпало.
void DfGameFactionNames(std::vector<std::string>& out);

// Файл состояния в папке сохранения: <location>\<name>\dynamicfactions.txt.
std::string DfStatePath(const std::string& location, const std::string& name);
