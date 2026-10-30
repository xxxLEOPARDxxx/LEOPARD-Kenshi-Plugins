#pragma once
// Своё состояние динамических фракций и его файл рядом с сохранением.
// Members and items use the native save. NPC faction treasury needs a snapshot:
// Kenshi serializes platoon wallets, but not Faction::factionOwnerships.

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
    bool workFault;             // durable quarantine; never retry an unknown transfer after loading
    int treasury;               // -1 = legacy/absent; otherwise exact balance captured at the save boundary
    double foundedHour;
    double lastTickHour;
    int techLevel;
    int techPoints;
    std::vector<DfEvent> events;    // не больше kDfMaxEvents, старые выбрасываются
    std::vector<DfKnownNode> nodes; // запомненные жилы

    DfFactionState();
};

const size_t kDfMaxEvents = 20;
// Defensive file limits, above the currently supported faction/node counts.
const size_t kDfMaxStateBytes = 16 * 1024 * 1024;
const size_t kDfMaxStateLineBytes = 64 * 1024;
const size_t kDfMaxStateFactions = 1024;
const size_t kDfMaxStateNodesPerFaction = 16384;
// Legacy bookkeeping's complete technology range (not the game's research).
const int kDfMaxStateTechLevel = 10;

void DfAddEvent(DfFactionState& s, double hour, const std::string& text);

// Строки key=value, секции [faction N]. Переводы строк и '\' в значениях экранируются.
std::string DfSerialize(const std::vector<DfFactionState>& all);
// false - файл испорчен/превышает лимиты, out пуст. Пустой текст - true.
// Unknown fields are ignored. No native inventory or money is restored here.
bool DfDeserialize(const std::string& text, std::vector<DfFactionState>& out);
