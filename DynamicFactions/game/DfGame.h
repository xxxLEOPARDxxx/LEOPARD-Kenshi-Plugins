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
extern const char* const kDfFounderRecord;      // CHARACTER: «Drifter» без вещей и денег

// Фракция игры по записи FACTION; NULL - записи нет (мод не включён).
Faction* DfBindFaction(const std::string& recordId);

// Хаб, когда его область загружена (TownBase::isActive); иначе NULL.
TownBase* DfFindHub();

// Игровое время в часах от начала игры; < 0 - мира нет.
double DfGameHour();

// Численность (живые по взводам фракции), деньги, время, загружен ли дом.
bool DfTakeSnapshot(Faction* f, DfSnapshot& out);

// Стартовый отряд в Хабе (members человек, все навыки = statLevel) и
// стартовые деньги. false - отряд не создан.
bool DfFound(Faction* f, TownBase* hub, int members, int money, float statLevel);

// Выполнить решение ядра. DF_INCOME - деньги фракции; DF_RECRUIT - нанять
// из бара Хаба, как нанял бы игрок (взвод фракции загружен, нанимаемый
// есть - иначе не выполнено); DF_RESEARCH -
// плата за очки; DF_EXTINCT - ничего. Деньги проверяются заново прямо перед
// вычитанием: не хватает - действие не выполняется (бюджет не уходит в минус).
// false - не выполнено, в учёт не идёт.
bool DfExecute(Faction* f, TownBase* hub, const DfAction& a, const DfRules& r);

// Имя фракции в игре = name (ставится только при расхождении).
void DfApplyName(Faction* f, const std::string& name);

// Названия всех фракций игры - чтобы новое имя не совпало.
void DfGameFactionNames(std::vector<std::string>& out);

// Файл состояния в папке сохранения: <location>\<name>\dynamicfactions.txt.
std::string DfStatePath(const std::string& location, const std::string& name);
