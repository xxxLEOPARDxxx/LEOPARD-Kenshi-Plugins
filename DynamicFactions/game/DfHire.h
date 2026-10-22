#pragma once
// Найм «вживую» (23.10.2026, пользователь: «честная симуляция, если мы всё
// видим»): член фракции идёт в бар к рекруту, они обмениваются короткими
// репликами над головами (Character::say, как NPC говорят между собой), и
// только тогда рекрут переходит во фракцию и получает деньги.
//
// Шаги, раз в секунду из тика плагина:
//   GO   - посланник идёт к текущей точке рекрута (Character::
//          setDestination каждую секунду: ИИ отряда NPC перебивает приказы -
//          «следовать» сбрасывал через 15 с, «прогулка» уводила в магазин,
//          goal SHOPPING, 23.10). Рекрут бродит - догоняем. Сам посланник
//          15 с стоит на месте (сдвиг < 1,5 м) - недостижим, рекрута 10
//          минут не предлагать;
//   TALK - рядом (< 5 м) или стоит рядом (< 10 м и 8 с не подходит
//          ближе - рекрут за столом): посланник встаёт, четыре реплики по
//          очереди, по 3 с;
//   JOIN - деньги ещё есть: рекрут во взвод фракции, реплики найма
//          заперты, деньги списаны.
// Срыв (кто-то погиб, без сознания, ушёл в другую фракцию, не дошли за
// 4 минуты, денег не стало) - приказ посланнику снимается, деньги целы.
// Только главный поток; всё под SEH.

#include <string>

#include <ogre/OgreVector3.h>

#include <kenshi/util/hand.h>

class Faction;
class TownBase;

enum DfHirePhase
{
    DF_HIRE_IDLE,
    DF_HIRE_GO,
    DF_HIRE_TALK
};

enum DfHireResult
{
    DF_HIRE_RUNNING,
    DF_HIRE_DONE,
    DF_HIRE_FAILED
};

struct DfHireTask
{
    DfHirePhase phase;
    hand envoy;                 // наш, кто идёт договариваться
    hand recruit;
    int price;
    unsigned long startedMs;
    unsigned long phaseMs;      // когда сказана последняя реплика / отдан приказ
    int line;                   // сколько реплик сказано
    Ogre::Vector3 lastPos;      // где посланник был, когда последний раз сдвинулся
    unsigned long movedMs;      // когда последний раз сдвинулся
    float closest;              // ближе всего подходил
    unsigned long closerMs;     // когда последний раз подошёл ближе
    std::string why;            // почему сорвалось - для журнала

    DfHireTask() : phase(DF_HIRE_IDLE), price(0), startedMs(0), phaseMs(0), line(0), lastPos(0.0f, 0.0f, 0.0f), movedMs(0), closest(0.0f), closerMs(0) {}
    bool busy() const { return phase != DF_HIRE_IDLE; }
};

// Начать: самый дешёвый рекрут бара города (DfFindBarRecruit) не дороже
// maxPrice и ближайший к нему член фракции в загруженном взводе. false -
// некого нанять или некого послать.
bool DfHireStart(DfHireTask& t, Faction* f, TownBase* town, int maxPrice);

// Шаг раз в секунду. factionName - для реплики посланника.
DfHireResult DfHireStep(DfHireTask& t, Faction* f, const std::string& factionName);

// Бросить (загрузка, вымирание): снять приказ посланнику.
void DfHireCancel(DfHireTask& t);

// Подробный журнал похода (расстояние, цель посланника раз в 5 с).
void DfHireSetDebug(bool on);

// Для окна: что сейчас с наймом (пусто - не идёт), уже переведено.
std::string DfHireStatus(const DfHireTask& t);
