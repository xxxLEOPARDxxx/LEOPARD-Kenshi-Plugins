#pragma once
// Найм «как у игрока»: те же люди, что сидят в барах и кого игрок нанял бы
// разговором, по той же цене. Бары игра пополняет сама, со временем.

#include <string>
#include <vector>

class Character;
class Faction;
class TownBase;
bool DfIsOwnFaction(Faction* faction);

// Самый дешёвый обычный рекрут города town (при равной цене - ближайший к
// центру города): его шаблон - из списков «choosefrom list» отрядов бара этого
// города (так игра сама заводит рекрутов в бары), он в городе - в баре или
// бродит по улицам (пользователь 23.10: «рекруты иногда шляются по городу,
// можно ловить и там»), живой, в сознании, не игрока, не наш, не
// уникальный, в диалоге открыта реплика «вступить в отряд» - с условиями
// разговора, как если бы с ним говорил член фракции (так же RecruitMarkers
// проверяет за игрока). price - наибольшее DA_TAKE_MONEY среди открытых
// реплик найма (0 - бесплатно). false - нанять некого. Главный поток; под SEH.
bool DfFindBarRecruit(TownBase* town, Faction* ours, Character*& who, int& price);

// Сколько отсеялось на каждом шаге последнего поиска - для журнала.
struct DfRecruitSearchStats
{
    int templates;              // шаблонов рекрутов у баров города
    int generic;                // живых персонажей с такими шаблонами в городе
    int inBar;                  // из них сейчас в баре (для журнала)
    int recruitable;            // из них с открытой репликой найма
    std::string list;           // «имя цена; ...» нанимаемых (до 8)
};
const DfRecruitSearchStats& DfLastRecruitSearch();

// Не предлагать этого рекрута ms миллисекунд (не дошли до него).
void DfRecruitSkip(Character* c, unsigned long ms);

// Разговор о найме с членом динамической фракции закрыт: хук на
// DialogLineData::checkConditions - условие ложно у любой реплики, под
// которой (или в которой) есть действие «вступить в отряд», если персонаж,
// чей это разговор, из наших фракций. Ветка найма не начинается вовсе;
// прочие разговоры и чужие персонажи - как в игре. (23.10: запирание
// реплик через Dialogue::setLineLocked не держалось; блок одной последней
// реплики оставлял «Идёт (3000 Катов)», после которого ничего не было.)
bool DfInstallHireBlock();

// Наши фракции игры - для блока найма. Главный поток.
void DfSetOwnFactions(const std::vector<Faction*>& factions);

// Before native world teardown/load: metadata only, never resolves old handles.
// Explicit even when the own-faction list was already empty.
void DfRecruitResetSession();
unsigned long DfRecruitSession();
