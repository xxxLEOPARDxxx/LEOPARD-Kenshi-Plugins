#pragma once
// Своё состояние динамических фракций и его файл рядом с сохранением.
// Деньги и численность здесь не хранятся - их берём у игры.

#include <string>
#include <vector>

struct DfEvent
{
    double hour;
    std::string text;           // английский ключ + числа, перевод при показе
};

// Жила, которую фракция видела (жилы видны, только пока загружен их
// участок карты - запоминаем). stored - условный запас руды, накопанный,
// пока жила не загружена (настоящий лежит в самой жиле).
struct DfKnownNode
{
    float x, y, z;
    std::string itemSid;        // stringID предмета, который даёт жила
    int stored;
};

struct DfFactionState
{
    std::string recordId;       // StringID записи FACTION из .mod
    std::string name;
    std::string race;
    bool founded;
    bool extinct;
    double foundedHour;
    double lastTickHour;
    int techLevel;
    int techPoints;
    std::vector<DfEvent> events;    // не больше kDfMaxEvents, старые выбрасываются
    std::vector<DfKnownNode> nodes; // запомненные жилы

    DfFactionState();
};

const size_t kDfMaxEvents = 20;

void DfAddEvent(DfFactionState& s, double hour, const std::string& text);

// Строки key=value, секции [faction N]. Переводы строк и '\' в значениях экранируются.
std::string DfSerialize(const std::vector<DfFactionState>& all);
// false - файл испорчен, out пуст. Пустой текст - true и пустой out.
bool DfDeserialize(const std::string& text, std::vector<DfFactionState>& out);
