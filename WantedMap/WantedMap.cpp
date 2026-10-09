// WantedMap - метки разыскиваемых на большой карте.
//
// Выдели персонажа с листовками «Разыскивается» (в инвентаре или в
// рюкзаке) - на большой карте появятся метки, где цели.
//
// ОТКУДА ЦЕЛЬ. Листовка в данных игры - предмет-«книга» без ссылки на
// того, кого разыскивают. Цель подбирается при первом открытии карты по
// данным самой игры (уже склеенным из всех модов), четырьмя проходами:
//   1. ручные пары из WantedMap_overrides.txt (необязательный файл);
//   2. имя после двоеточия («WANTED: X», «Розыск: X») = имя персонажа или
//      шаблона отряда;
//   3. имена персонажей с наградой, целиком встречающиеся в описании
//      листовки (групповые: «Фрис, Тай, Лод...»);
//   4. награда из описания = «bounty amount» персонажа и общее слово в
//      имени (или он из того же мода, если вариантов не больше трёх).
// Сравнение без учёта регистра через Юникод (CharLowerBuffW), так что
// язык не важен: какие названия в данных - такими и сравниваем.
//
// ГДЕ ЦЕЛЬ. Точнее - выше:
//   * цель загружена - сам персонаж (точно);
//   * отряд цели далеко - отряд по шаблону (Platoon есть и незагруженным);
//   * отряд ещё не появлялся - поселение, где он живёт по данным.
//
// УБИТЫЕ. Убитую цель показываем серой меткой «убит» на месте смерти
// DeadMarkerHours игровых часов (сутки), потом метка пропадает. О смерти
// узнаём так:
//   * хук Character::declareDead - игра объявила персонажа мёртвым
//     (где угодно, даже пока карта закрыта): время и место;
//   * загруженная цель с isDead();
//   * «состояния мира» игры (WORLD_EVENT_STATE, «NPC is»: 0 мёртв,
//     1 жив, 2 в плену) - WorldEventStateQuery::isTrue; когда умерла, тут
//     неизвестно - метки нет;
//   * цель уже была в мире (её отряд находился), а теперь отряда нет
//     нигде, ни загруженного, ни выгруженного, - погибла; метки нет.
// Память - файл на прохождение (по имени фракции игрока) в папке memory
// рядом с DLL. С отметкой о смерти хранится время игры: если сейчас в
// игре раньше (загрузили старый сейв), отметка снимается.
//
// НАГРУЗКА. Только пока открыта карта (хук MapScreen::update). Листовки -
// раз в ScanSeconds и только у выделенных. Подбор целей - один раз за
// игру. Обход фракций - при смене листовок и раз в WorldScanSeconds.
// Функций, заполняющих lektor, в частых путях не зовём: у lektor нет
// деструктора, каждый вызов - утечка (кроме getCharactersInArea раз в
// 30 с для загруженных отрядов-целей и getDataOfType раз за игру).

#define KLOC_DOMAIN "wanted_map"
#include <Localization.h>
#include <ModConfigMenu.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <Debug.h>
#include <core/Functions.h>

#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_Widget.h>
#include <mygui/MyGUI_TextBox.h>
#include <mygui/MyGUI_ImageBox.h>
#include <OgreResourceGroupManager.h>

#include <kenshi/Globals.h>
#include <kenshi/GameWorld.h>
#include <kenshi/GameData.h>
#include <kenshi/GameDataManager.h>
#include <kenshi/PlayerInterface.h>
#include <kenshi/Character.h>
#include <kenshi/Inventory.h>
#include <kenshi/Item.h>
#include <kenshi/Enums.h>
#include <kenshi/RootObjectBase.h>
#include <kenshi/WorldEventStateQuery.h>
#include <kenshi/util/hand.h>
#include <kenshi/util/lektor.h>
#include <kenshi/gui/InventoryGUI.h>

#define private public
#define protected public
#include <kenshi/gui/MapScreen.h>
#include <kenshi/Faction.h>
#include <kenshi/FactionWarMgr.h>
#include <kenshi/Platoon.h>
#include <kenshi/Town.h>
#include <kenshi/SharedKing.h>
#undef private
#undef protected

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace
{
    // ---------------------------------------------------------------
    // Настройки
    // ---------------------------------------------------------------

    bool g_enabled = true;
    DWORD g_scanMs = 2000;
    DWORD g_worldScanMs = 30000;
    bool g_showLabels = true;
    bool g_townFallback = true;
    bool g_rememberSeen = true;
    bool g_posterBadge = true;      // значок на листовке: цели убиты / в плену
    double g_deadHours = 24.0;
    bool g_debug = false;
    std::set<std::wstring> g_stopWords;

    std::string ModuleDir()
    {
        char path[MAX_PATH] = {};
        const DWORD n = GetModuleFileNameA(reinterpret_cast<HMODULE>(&__ImageBase), path, MAX_PATH);
        std::string s(path, n);
        const std::string::size_type slash = s.find_last_of("\\/");
        return slash == std::string::npos ? std::string() : s.substr(0, slash + 1);
    }

    std::wstring Wide(const std::string& s)
    {
        if (s.empty())
            return std::wstring();
        const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), NULL, 0);
        std::wstring w(n, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), &w[0], n);
        return w;
    }

    // Без пробелов по краям, «ё» как «е» (регистр сохраняется).
    std::wstring Plain(const std::wstring& s)
    {
        size_t a = 0, b = s.size();
        while (a < b && iswspace(s[a])) ++a;
        while (b > a && iswspace(s[b - 1])) --b;
        std::wstring w = s.substr(a, b - a);
        for (size_t i = 0; i < w.size(); ++i)
        {
            if (w[i] == 0x0451)
                w[i] = 0x0435;
            else if (w[i] == 0x0401)
                w[i] = 0x0415;
        }
        return w;
    }

    // То же в нижнем регистре - на любом языке.
    std::wstring Lower(const std::wstring& s)
    {
        std::wstring w = Plain(s);
        if (!w.empty())
            CharLowerBuffW(&w[0], static_cast<DWORD>(w.size()));
        return w;
    }

    std::string IniPath() { return ModuleDir() + "WantedMap.ini"; }

    // Зовётся при запуске и из MCM после изменения настройки.
    void __cdecl LoadSettings()
    {
        const std::string ini = IniPath();
        const char* sec = "WantedMap";
        g_enabled = GetPrivateProfileIntA(sec, "Enabled", 1, ini.c_str()) != 0;
        int s = GetPrivateProfileIntA(sec, "ScanSeconds", 2, ini.c_str());
        g_scanMs = static_cast<DWORD>((s < 1 ? 1 : s) * 1000);
        int w = GetPrivateProfileIntA(sec, "WorldScanSeconds", 30, ini.c_str());
        g_worldScanMs = static_cast<DWORD>((w < 5 ? 5 : w) * 1000);
        g_showLabels = GetPrivateProfileIntA(sec, "ShowLabels", 1, ini.c_str()) != 0;
        g_townFallback = GetPrivateProfileIntA(sec, "TownFallback", 1, ini.c_str()) != 0;
        g_rememberSeen = GetPrivateProfileIntA(sec, "RememberSeen", 1, ini.c_str()) != 0;
        g_posterBadge = GetPrivateProfileIntA(sec, "PosterBadge", 1, ini.c_str()) != 0;
        g_deadHours = GetPrivateProfileIntA(sec, "DeadMarkerHours", 24, ini.c_str());
        g_debug = GetPrivateProfileIntA(sec, "Debug", 0, ini.c_str()) != 0;

        // Служебные слова, которые не считаются общим словом в имени
        // («король», «лидер»...). Язык любой - строка в ini в UTF-8.
        g_stopWords.clear();
        char buf[2048] = {};
        // Список по умолчанию - из перевода: на каждом языке свои служебные
        // слова (в DLL только английский).
        GetPrivateProfileStringA("Match", "StopWords", Tr("the,of,and,wanted,king,boss,leader"),
                                 buf, sizeof(buf), ini.c_str());
        std::wstring all = Lower(Wide(buf));
        std::wstring cur;
        for (size_t i = 0; i <= all.size(); ++i)
        {
            if (i == all.size() || all[i] == L',')
            {
                std::wstring t = Lower(cur);
                if (!t.empty())
                    g_stopWords.insert(t);
                cur.clear();
            }
            else
                cur += all[i];
        }
    }


    // ---------------------------------------------------------------
    // Подбор целей по данным игры (один раз за игру)
    // ---------------------------------------------------------------

    std::map<std::string, std::vector<std::string> > g_posterTargets;  // листовка -> цели
    std::map<std::string, std::vector<std::string> > g_squadMembers;   // шаблон -> персонажи-цели
    std::map<std::string, std::vector<std::string> > g_charSquads;     // персонаж -> шаблоны
    std::map<std::string, std::vector<std::string> > g_squadTowns;     // шаблон -> поселения
    std::set<std::string> g_targetSquadSet;                             // цели-отряды
    std::set<std::string> g_targetChars;                                // цели-персонажи всех листовок

    struct StateRef { GameData* state; int value; };   // «NPC is»
    std::map<std::string, std::vector<StateRef> > g_charStates;
    bool g_indexBuilt = false;

    int IntField(GameData* d, const char* key, int def)
    {
        ogre_unordered_map<std::string, int>::type::iterator it = d->idata.find(key);
        return it == d->idata.end() ? def : it->second;
    }

    std::string StrField(GameData* d, const char* key)
    {
        ogre_unordered_map<std::string, std::string>::type::iterator it = d->sdata.find(key);
        return it == d->sdata.end() ? std::string() : it->second;
    }

    const Ogre::vector<GameDataReference>::type* Refs(GameData* d, const char* list)
    {
        return d->getReferenceListIfExists(list);
    }

    bool IsWordChar(wchar_t c)
    {
        return IsCharAlphaNumericW(c) || c == L'\'';
    }

    // Опорные слова имени: по первым пяти буквам («Тора»/«Торы»).
    std::set<std::wstring> Words(const std::wstring& lower)
    {
        std::set<std::wstring> out;
        std::wstring cur;
        for (size_t i = 0; i <= lower.size(); ++i)
        {
            if (i < lower.size() && IsCharAlphaW(lower[i]))
                cur += lower[i];
            else
            {
                if (cur.size() >= 2 && !g_stopWords.count(cur))
                    out.insert(cur.substr(0, 5));
                cur.clear();
            }
        }
        return out;
    }

    // Суммы из описания: «Награда: 60,000», «c.10,000», «к. 20 000».
    std::set<int> Numbers(const std::wstring& text)
    {
        std::set<int> out;
        for (size_t i = 0; i < text.size(); )
        {
            if (!iswdigit(text[i]))
            {
                ++i;
                continue;
            }
            long long v = 0;
            size_t j = i;
            while (j < text.size())
            {
                if (iswdigit(text[j]))
                    v = v * 10 + (text[j] - L'0');
                else if ((text[j] == L',' || text[j] == L'.' || text[j] == L' ' || text[j] == 0x00A0) &&
                         j + 1 < text.size() && iswdigit(text[j + 1]))
                    ;
                else
                    break;
                if (v > 100000000)
                    break;
                ++j;
            }
            if (v >= 500)
                out.insert(static_cast<int>(v));
            i = j + 1;
        }
        return out;
    }

    bool WholeWord(const std::wstring& hay, const std::wstring& needle)
    {
        if (needle.empty())
            return false;
        size_t pos = 0;
        while ((pos = hay.find(needle, pos)) != std::wstring::npos)
        {
            const bool left = pos == 0 || !IsWordChar(hay[pos - 1]);
            const size_t end = pos + needle.size();
            const bool right = end >= hay.size() || !IsWordChar(hay[end]);
            if (left && right)
                return true;
            ++pos;
        }
        return false;
    }

    std::string ModOf(const std::string& sid)
    {
        const std::string::size_type d = sid.find('-');
        return d == std::string::npos ? sid : sid.substr(d + 1);
    }

    void LoadOverrides(std::map<std::string, std::vector<std::string> >& out)
    {
        std::ifstream f((ModuleDir() + "WantedMap_overrides.txt").c_str(), std::ios::binary);
        std::string line;
        while (std::getline(f, line))
        {
            const std::string::size_type hash = line.find('#');
            if (hash != std::string::npos)
                line = line.substr(0, hash);
            const std::string::size_type bar = line.find('|');
            if (bar == std::string::npos)
                continue;
            std::string poster = line.substr(0, bar);
            std::string rest = line.substr(bar + 1);
            while (!poster.empty() && isspace(static_cast<unsigned char>(poster[poster.size() - 1]))) poster.erase(poster.size() - 1);
            while (!poster.empty() && isspace(static_cast<unsigned char>(poster[0]))) poster.erase(0, 1);
            std::vector<std::string> list;
            std::string cur;
            for (size_t i = 0; i <= rest.size(); ++i)
            {
                if (i == rest.size() || rest[i] == ',')
                {
                    while (!cur.empty() && isspace(static_cast<unsigned char>(cur[0]))) cur.erase(0, 1);
                    while (!cur.empty() && isspace(static_cast<unsigned char>(cur[cur.size() - 1]))) cur.erase(cur.size() - 1);
                    if (!cur.empty())
                        list.push_back(cur);
                    cur.clear();
                }
                else if (rest[i] != '\r')
                    cur += rest[i];
            }
            if (!poster.empty() && !list.empty())
                out[poster] = list;
        }
    }

    void BuildIndex()
    {
        g_indexBuilt = true;
        if (ou == NULL)
            return;
        GameDataContainer& gd = ou->gamedata;
        const DWORD t0 = GetTickCount();

        lektor<GameData*> chars, squads, towns, items, states;
        gd.getDataOfType(chars, CHARACTER);
        gd.getDataOfType(squads, SQUAD_TEMPLATE);
        gd.getDataOfType(towns, TOWN);
        gd.getDataOfType(items, ITEM);
        gd.getDataOfType(states, WORLD_EVENT_STATE);

        // Имена персонажей и отрядов -> записи.
        std::map<std::wstring, std::vector<std::string> > byName;
        // name - с регистром: в описании ищем имена собственные, а не
        // «бандит» или «боец» в середине фразы.
        struct Bounty { std::string sid; int amount; std::wstring name; std::set<std::wstring> words; };
        std::vector<Bounty> bounty;
        std::set<std::string> squadSids;
        for (uint32_t i = 0; i < chars.size(); ++i)
        {
            GameData* const d = chars[i];
            if (d == NULL)
                continue;
            const std::wstring n = Lower(Wide(d->name));
            byName[n].push_back(d->stringID);
            const int amount = IntField(d, "bounty amount", 0);
            if (amount > 0)
            {
                Bounty b;
                b.sid = d->stringID;
                b.amount = amount;
                b.name = Plain(Wide(d->name));
                b.words = Words(n);
                bounty.push_back(b);
            }
        }
        for (uint32_t i = 0; i < squads.size(); ++i)
            if (squads[i] != NULL)
            {
                byName[Lower(Wide(squads[i]->name))].push_back(squads[i]->stringID);
                squadSids.insert(squads[i]->stringID);
            }

        std::map<std::string, std::vector<std::string> > overrides;
        LoadOverrides(overrides);

        // Книги «Префикс: имя». Листовки от книг-лора («Записки
        // Техохотников: Болота») отличаем без словаря: префикс листовочный,
        // если с ним хотя бы две книги, у которых после двоеточия точное
        // имя персонажа или отряда (ручная пара считается за две).
        std::vector<GameData*> books;
        std::map<std::wstring, int> prefixVotes;
        for (uint32_t i = 0; i < items.size(); ++i)
        {
            GameData* const d = items[i];
            if (d == NULL || IntField(d, "item function", 0) != ITEM_BOOK)
                continue;
            const std::string::size_type colon = d->name.find(':');
            if (colon == std::string::npos)
                continue;
            books.push_back(d);
            const std::wstring prefix = Lower(Wide(d->name.substr(0, colon)));
            if (overrides.count(d->stringID))
                prefixVotes[prefix] += 2;
            else if (byName.count(Lower(Wide(d->name.substr(colon + 1)))))
                prefixVotes[prefix] += 1;
        }

        unsigned byOverride = 0, byNameN = 0, byDesc = 0, byReward = 0, missed = 0;
        for (size_t i = 0; i < books.size(); ++i)
        {
            GameData* const d = books[i];
            const std::string::size_type colon = d->name.find(':');
            const bool posterPrefix = prefixVotes[Lower(Wide(d->name.substr(0, colon)))] >= 2;
            std::vector<std::string> hits;

            std::map<std::string, std::vector<std::string> >::const_iterator ov = overrides.find(d->stringID);
            if (ov != overrides.end())
            {
                hits = ov->second;
                ++byOverride;
            }
            const std::wstring target = Lower(Wide(d->name.substr(colon + 1)));
            if (hits.empty())
            {
                std::map<std::wstring, std::vector<std::string> >::const_iterator n = byName.find(target);
                if (n != byName.end())
                {
                    hits = n->second;
                    ++byNameN;
                }
            }
            const std::wstring desc = posterPrefix ? Plain(Wide(StrField(d, "description"))) : std::wstring();
            if (hits.empty() && !desc.empty())
            {
                for (size_t b = 0; b < bounty.size(); ++b)
                    if (bounty[b].name.size() >= 2 && WholeWord(desc, bounty[b].name))
                        hits.push_back(bounty[b].sid);
                if (hits.size() > 10)
                    hits.clear();
                if (!hits.empty())
                    ++byDesc;
            }
            if (hits.empty() && !desc.empty())
            {
                const std::set<int> rewards = Numbers(desc);
                const std::set<std::wstring> pw = Words(target);
                std::vector<std::string> byWord, byMod;
                for (size_t b = 0; b < bounty.size() && !rewards.empty(); ++b)
                {
                    if (!rewards.count(bounty[b].amount))
                        continue;
                    bool common = false;
                    for (std::set<std::wstring>::const_iterator w = pw.begin(); w != pw.end() && !common; ++w)
                        common = bounty[b].words.count(*w) != 0;
                    if (common)
                        byWord.push_back(bounty[b].sid);
                    else if (ModOf(bounty[b].sid) == ModOf(d->stringID))
                        byMod.push_back(bounty[b].sid);
                }
                if (!byWord.empty())
                    hits = byWord;
                else if (!byMod.empty() && byMod.size() <= 3)
                    hits = byMod;
                if (!hits.empty())
                    ++byReward;
            }
            if (hits.empty())
            {
                if (!posterPrefix)
                    continue;      // книга, не листовка
                ++missed;
                if (g_debug)
                    DebugLog("WantedMap: no target for '" + d->name + "' (" + d->stringID + ")");
                continue;
            }
            g_posterTargets[d->stringID] = hits;
            for (size_t h = 0; h < hits.size(); ++h)
            {
                if (squadSids.count(hits[h]))
                    g_targetSquadSet.insert(hits[h]);
                else
                    g_targetChars.insert(hits[h]);
            }
        }

        // Персонаж -> шаблоны отрядов, где он состоит.
        static const char* const MEMBER_LISTS[] = { "leader", "squad", "squad2", "slaves" };
        for (uint32_t i = 0; i < squads.size(); ++i)
        {
            GameData* const s = squads[i];
            if (s == NULL)
                continue;
            for (int l = 0; l < 4; ++l)
            {
                const Ogre::vector<GameDataReference>::type* refs = Refs(s, MEMBER_LISTS[l]);
                if (refs == NULL)
                    continue;
                for (size_t r = 0; r < refs->size(); ++r)
                {
                    g_charSquads[(*refs)[r].sid].push_back(s->stringID);
                    g_squadMembers[s->stringID].push_back((*refs)[r].sid);
                }
            }
        }
        for (std::set<std::string>::const_iterator t = g_targetSquadSet.begin(); t != g_targetSquadSet.end(); ++t)
            g_squadMembers[*t];   // цель-отряд есть в таблице и без членов-целей

        // Отряд -> поселения, где он живёт (списки с «squad»/«resident»/«spawn»).
        for (uint32_t i = 0; i < towns.size(); ++i)
        {
            GameData* const t = towns[i];
            if (t == NULL)
                continue;
            for (ogre_unordered_map<std::string, Ogre::vector<GameDataReference>::type>::type::iterator
                     it = t->objectReferences.begin(); it != t->objectReferences.end(); ++it)
            {
                const std::string& cat = it->first;
                if (cat.find("squad") == std::string::npos && cat.find("resident") == std::string::npos &&
                    cat.find("spawn") == std::string::npos)
                    continue;
                for (size_t r = 0; r < it->second.size(); ++r)
                    g_squadTowns[it->second[r].sid].push_back(t->stringID);
            }
        }

        // «Состояния мира»: NPC is -> мёртв/жив/в плену.
        for (uint32_t i = 0; i < states.size(); ++i)
        {
            GameData* const s = states[i];
            if (s == NULL)
                continue;
            const Ogre::vector<GameDataReference>::type* refs = Refs(s, "NPC is");
            if (refs == NULL || refs->size() != 1)   // только «про одного»
                continue;
            StateRef sr;
            sr.state = s;
            sr.value = (*refs)[0].values.value[0];
            g_charStates[(*refs)[0].sid].push_back(sr);
        }

        char note[256];
        sprintf_s(note, "WantedMap: targets matched in %u ms: posters %u (override %u, name %u, "
                  "description %u, reward %u), no target %u; squads %u, states %u",
                  static_cast<unsigned>(GetTickCount() - t0),
                  static_cast<unsigned>(g_posterTargets.size()), byOverride, byNameN, byDesc,
                  byReward, missed, static_cast<unsigned>(g_charSquads.size()),
                  static_cast<unsigned>(g_charStates.size()));
        DebugLog(note);

        if (g_debug)
        {
            // Таблица в файл - сверить с офлайновым разбором.
            std::ofstream f((ModuleDir() + "WantedMap_runtime_table.txt").c_str(), std::ios::binary);
            for (std::map<std::string, std::vector<std::string> >::const_iterator p = g_posterTargets.begin();
                 p != g_posterTargets.end(); ++p)
            {
                f << "poster|" << p->first << "|";
                for (size_t k = 0; k < p->second.size(); ++k)
                    f << (k ? "," : "") << p->second[k];
                f << "\n";
            }
        }
    }


    // ---------------------------------------------------------------
    // Память: где цель видели, где видели мёртвой (на прохождение)
    // ---------------------------------------------------------------

    // seen - цель (или её отряд) уже была в мире; dead - убита: когда (в
    // часах игры) и где.
    struct Seen
    {
        bool seen;
        bool dead;
        double deathHours;
        bool hasPos;
        Ogre::Vector3 pos;
        Seen() : seen(false), dead(false), deathHours(0.0), hasPos(false), pos(0, 0, 0) {}
    };

    std::map<std::string, Seen> g_seen;
    std::string g_memoryFile;
    bool g_memoryDirty = false;

    std::string MemoryFileFor(const std::string& faction)
    {
        unsigned long long h = 1469598103934665603ULL;
        for (size_t i = 0; i < faction.size(); ++i)
        {
            h ^= static_cast<unsigned char>(faction[i]);
            h *= 1099511628211ULL;
        }
        char name[64];
        sprintf_s(name, "memory\\%016llx.txt", h);
        return ModuleDir() + name;
    }

    void LoadMemory()
    {
        g_seen.clear();
        std::ifstream f(g_memoryFile.c_str(), std::ios::binary);
        std::string line;
        while (std::getline(f, line))
        {
            if (line.empty() || line[0] == '#')
                continue;
            // sid|seen|dead|deathHours|hasPos|x|y|z
            std::vector<std::string> p;
            std::string cur;
            for (size_t i = 0; i <= line.size(); ++i)
            {
                if (i == line.size() || line[i] == '|')
                {
                    p.push_back(cur);
                    cur.clear();
                }
                else if (line[i] != '\r')
                    cur += line[i];
            }
            Seen s;
            if (p.size() == 10)
            {
                // Первая версия: sid|lastX|lastY|lastZ|hasLast|dead|hasDeadPos|x|y|z
                s.seen = p[4] == "1";
                s.dead = p[5] == "1";
                s.hasPos = p[6] == "1";
                s.pos = Ogre::Vector3(static_cast<float>(atof(p[7].c_str())), static_cast<float>(atof(p[8].c_str())),
                                      static_cast<float>(atof(p[9].c_str())));
                g_seen[p[0]] = s;
                g_memoryDirty = true;
                continue;
            }
            if (p.size() != 8)
                continue;
            s.seen = p[1] == "1";
            s.dead = p[2] == "1";
            s.deathHours = atof(p[3].c_str());
            s.hasPos = p[4] == "1";
            s.pos = Ogre::Vector3(static_cast<float>(atof(p[5].c_str())), static_cast<float>(atof(p[6].c_str())),
                                  static_cast<float>(atof(p[7].c_str())));
            g_seen[p[0]] = s;
        }
    }

    void SaveMemory(const std::string& faction)
    {
        if (!g_memoryDirty)
            return;
        g_memoryDirty = false;
        CreateDirectoryA((ModuleDir() + "memory").c_str(), NULL);
        std::ofstream f(g_memoryFile.c_str(), std::ios::binary | std::ios::trunc);
        f << "# WantedMap: " << faction << "\n";
        for (std::map<std::string, Seen>::const_iterator it = g_seen.begin(); it != g_seen.end(); ++it)
        {
            const Seen& s = it->second;
            char buf[160];
            sprintf_s(buf, "|%d|%d|%.3f|%d|%.1f|%.1f|%.1f\n", s.seen ? 1 : 0, s.dead ? 1 : 0,
                      s.deathHours, s.hasPos ? 1 : 0, s.pos.x, s.pos.y, s.pos.z);
            f << it->first << buf;
        }
    }

    double GameHours()
    {
        return ou == NULL ? 0.0 : ou->getTimeStamp_inGameHours().getTotalHours();
    }

    void MarkSeen(const std::string& sid)
    {
        if (!g_rememberSeen)
            return;
        Seen& s = g_seen[sid];
        if (!s.seen || s.dead)
        {
            s.seen = true;
            s.dead = false;       // жива - старая отметка о смерти не в счёт
            g_memoryDirty = true;
        }
    }

    void MarkDead(const std::string& sid, double hours, bool hasPos, const Ogre::Vector3& pos)
    {
        if (!g_rememberSeen)
            return;
        Seen& s = g_seen[sid];
        if (s.dead)
            return;
        s.seen = true;
        s.dead = true;
        s.deathHours = hours;
        s.hasPos = hasPos;
        s.pos = pos;
        g_memoryDirty = true;
        if (g_debug)
            DebugLog("WantedMap: target died: " + sid);
    }

    // Убита по памяти. Если сейчас в игре раньше, чем она умерла, -
    // загрузили старый сейв: отметку снимаем.
    const Seen* RememberedDead(const std::string& sid, double now)
    {
        std::map<std::string, Seen>::iterator it = g_seen.find(sid);
        if (it == g_seen.end() || !it->second.dead)
            return NULL;
        if (now > 0.0 && it->second.deathHours > 0.0 && now + 0.05 < it->second.deathHours)
        {
            it->second.dead = false;
            g_memoryDirty = true;
            return NULL;
        }
        return &it->second;
    }

    bool RememberedSeen(const std::string& sid)
    {
        std::map<std::string, Seen>::const_iterator it = g_seen.find(sid);
        return it != g_seen.end() && it->second.seen;
    }


    // ---------------------------------------------------------------
    // Хук смерти: Character::declareDead
    // ---------------------------------------------------------------

    // Зовётся из потоков игры - здесь только запоминаем, кто, когда и где,
    // под замком; разбираем в хуке карты. Ключ - запись персонажа в данных
    // (GameData живут всю игру), так что набор не растёт сверх числа
    // разных персонажей.
    struct Death { double hours; Ogre::Vector3 pos; };
    CRITICAL_SECTION g_deathLock;
    std::map<GameData*, Death> g_deaths;
    void (*g_origDeclareDead)(Character* self) = NULL;

    void DeclareDead_hook(Character* self)
    {
        g_origDeclareDead(self);
        if (self == NULL || self->data == NULL)
            return;
        Death d;
        d.hours = GameHours();
        d.pos = self->getPosition();
        EnterCriticalSection(&g_deathLock);
        if (g_deaths.size() < 100000)
            g_deaths[self->data] = d;
        LeaveCriticalSection(&g_deathLock);
    }

    void ProcessDeaths()
    {
        std::map<GameData*, Death> deaths;
        EnterCriticalSection(&g_deathLock);
        deaths.swap(g_deaths);
        LeaveCriticalSection(&g_deathLock);
        for (std::map<GameData*, Death>::const_iterator it = deaths.begin(); it != deaths.end(); ++it)
            if (g_targetChars.count(it->first->stringID))
                MarkDead(it->first->stringID, it->second.hours, true, it->second.pos);
    }

    // Прохождение сменилось (загрузили другой сейв) - другая память.
    void SyncMemoryFile()
    {
        if (ou == NULL || ou->player == NULL)
            return;
        const std::string file = MemoryFileFor(ou->player->factionName);
        if (file != g_memoryFile)
        {
            g_memoryFile = file;
            LoadMemory();
        }
    }


    // ---------------------------------------------------------------
    // Листовки у выделенных персонажей
    // ---------------------------------------------------------------

    std::map<std::string, std::string> g_heldPosters;   // листовка -> подпись

    std::string StripPrefix(const std::string& name)
    {
        const std::string::size_type colon = name.find(':');
        std::string s = colon == std::string::npos ? name : name.substr(colon + 1);
        while (!s.empty() && s[0] == ' ')
            s.erase(0, 1);
        return s;
    }

    // Инвентарь - через getInventory() и разделы, как в GearCompare и
    // BetterLooting (поле Character::inventory и Inventory::getAllItems из
    // заголовков KenshiLib давали пустоту).
    void CollectBooks(Inventory* inv, std::map<std::string, std::string>& out, int depth)
    {
        if (inv == NULL || depth > 2)
            return;
        lektor<InventorySection*>& sections = inv->getAllSections();
        for (uint32_t s = 0; s < sections.size(); ++s)
        {
            InventorySection* const section = sections[s];
            if (section == NULL)
                continue;
            const Ogre::vector<InventorySection::SectionItem>::type& items = section->getItems();
            for (size_t i = 0; i < items.size(); ++i)
            {
                Item* const it = items[i].item;
                if (it == NULL || it->data == NULL)
                    continue;
                if (g_posterTargets.find(it->data->stringID) != g_posterTargets.end())
                    out[it->data->stringID] = StripPrefix(it->getName());
                Inventory* const inner = it->getInventory();
                if (inner != NULL && inner != inv)
                    CollectBooks(inner, out, depth + 1);
            }
        }
    }

    // Только у выделенных («навигатор»). Никто не выделен - false и метки
    // остаются прежними.
    bool ScanPosters()
    {
        std::map<std::string, std::string> found;
        bool anySelected = false;
        if (ou != NULL && ou->player != NULL)
        {
            const lektor<Character*>& squad = ou->player->playerCharacters;
            for (uint32_t i = 0; i < squad.size(); ++i)
            {
                Character* const c = squad[i];
                if (c == NULL || !ou->player->isObjectSelected(c))
                    continue;
                anySelected = true;
                CollectBooks(c->getInventory(), found, 0);
            }
        }
        if (!anySelected)
            return false;
        const bool changed = found != g_heldPosters;
        g_heldPosters.swap(found);
        if (changed && g_debug)
            for (std::map<std::string, std::string>::const_iterator p = g_heldPosters.begin();
                 p != g_heldPosters.end(); ++p)
                DebugLog("WantedMap: holding '" + p->second + "' (" + p->first + ")");
        return changed;
    }


    // ---------------------------------------------------------------
    // Поиск целей в мире
    // ---------------------------------------------------------------

    enum Accuracy { ACC_EXACT, ACC_SQUAD, ACC_TOWN, ACC_DEAD };

    struct Marker
    {
        hand object;            // персонаж, отряд или поселение
        bool fixed;             // место смерти из памяти, без объекта
        Ogre::Vector3 pos;
        Accuracy accuracy;
        std::string label;
        bool imprisoned;
    };

    std::vector<Marker> g_markers;
    std::set<std::string> g_wantedChars;
    // Кто в плену по последнему обходу карты (отряд пленный). «Состояние
    // мира» плен показывает не у всех: у Иголки на карте «в плену», а
    // значок на листовке замка не ставил (08.10.2026).
    std::set<std::string> g_imprisonedSeen;
    std::set<std::string> g_wantedSquads;
    std::map<GameData*, bool> g_squadDecision;

    void BuildWantedSets()
    {
        g_wantedChars.clear();
        g_wantedSquads.clear();
        g_squadDecision.clear();
        for (std::map<std::string, std::string>::const_iterator p = g_heldPosters.begin();
             p != g_heldPosters.end(); ++p)
        {
            const std::vector<std::string>& targets = g_posterTargets[p->first];
            for (size_t i = 0; i < targets.size(); ++i)
            {
                const std::string& t = targets[i];
                if (g_targetSquadSet.count(t))
                {
                    g_wantedSquads.insert(t);
                    continue;
                }
                g_wantedChars.insert(t);
                std::map<std::string, std::vector<std::string> >::const_iterator sq = g_charSquads.find(t);
                if (sq != g_charSquads.end())
                    g_wantedSquads.insert(sq->second.begin(), sq->second.end());
            }
        }
    }

    bool SquadWanted(GameData* tmpl)
    {
        if (tmpl == NULL)
            return false;
        std::map<GameData*, bool>::const_iterator c = g_squadDecision.find(tmpl);
        if (c != g_squadDecision.end())
            return c->second;
        const bool want = g_wantedSquads.count(tmpl->stringID) != 0;
        g_squadDecision[tmpl] = want;
        return want;
    }

    std::string LabelFor(const std::string& sid)
    {
        for (std::map<std::string, std::string>::const_iterator p = g_heldPosters.begin();
             p != g_heldPosters.end(); ++p)
        {
            const std::vector<std::string>& t = g_posterTargets[p->first];
            for (size_t i = 0; i < t.size(); ++i)
                if (t[i] == sid)
                    return p->second;
        }
        return "?";
    }

    // Состояние по «состояниям мира» игры: 0 мёртв, 1 жив, 2 в плену,
    // -1 неизвестно. «Жив» = ложь смертью не считаем: так бывает и у
    // того, кто ещё ни разу не появлялся.
    int WorldState(const std::string& sid)
    {
        std::map<std::string, std::vector<StateRef> >::iterator it = g_charStates.find(sid);
        if (it == g_charStates.end())
            return -1;
        int result = -1;
        for (size_t i = 0; i < it->second.size(); ++i)
        {
            // Запрос берём у игры каждый раз (это поиск в её же таблице
            // statesData), а не храним у себя: после загрузки другого сейва
            // сохранённый указатель мог бы смотреть на удалённый объект.
            WorldEventStateQuery* const q = WorldEventStateQuery::getFromData(it->second[i].state);
            if (q == NULL)
                continue;
            const bool t = q->isTrue();
            const int v = it->second[i].value;
            if (t && (v == 0 || v == 2))
                return v;
            if (v == 1 && t)
                result = 1;
        }
        return result;
    }

    // Обход списков отрядов фракции под __try: их игра правит и из своих
    // потоков. Здесь только копирование указателей.
    int CollectPlatoonsRaw(Faction* f, Platoon** out, int cap)
    {
        int n = 0;
        __try
        {
            const lektor<Platoon*>* lists[2] = { f->getActivePlatoons(), f->getUnloadedPlatoons() };
            for (int l = 0; l < 2; ++l)
            {
                const lektor<Platoon*>* list = lists[l];
                if (list == NULL)
                    continue;
                for (uint32_t i = 0; i < list->count && n < cap; ++i)
                {
                    Platoon* const p = list->stuff[i];
                    if (p != NULL && !p->isDead && p->squadTemplate != NULL)
                        out[n++] = p;
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return -1;
        }
        return n;
    }

    Platoon* g_platoonBuf[16384];

    // Цель не на карте: метка на поселении, где живёт её отряд по данным.
    void AddTownLabels(const std::string& sid, const std::string& label,
                       std::map<std::string, std::string>& out)
    {
        if (!g_townFallback)
            return;
        std::map<std::string, std::vector<std::string> >::const_iterator cs = g_charSquads.find(sid);
        if (cs == g_charSquads.end())
            return;
        for (size_t s = 0; s < cs->second.size(); ++s)
        {
            const std::vector<std::string>& towns = g_squadTowns[cs->second[s]];
            for (size_t t = 0; t < towns.size(); ++t)
                if (out[towns[t]].empty())
                    out[towns[t]] = label;
        }
    }

    void ScanWorld()
    {
        g_markers.clear();
        if (ou == NULL || ou->factionMgr == NULL || g_heldPosters.empty())
            return;
        const lektor<Faction*>* factions = ou->factionMgr->getAllFactions();
        if (factions == NULL)
            return;

        std::map<std::string, Character*> exact;        // цель -> загруженный персонаж
        std::map<std::string, Platoon*> squadOf;        // цель -> её отряд (далеко)
        std::map<std::string, Platoon*> targetSquads;   // цель-отряд -> отряд

        for (uint32_t fi = 0; fi < factions->size(); ++fi)
        {
            Faction* const f = factions->stuff[fi];
            if (f == NULL)
                continue;
            const int n = CollectPlatoonsRaw(f, g_platoonBuf, 16384);
            for (int i = 0; i < n; ++i)
            {
                Platoon* const p = g_platoonBuf[i];
                if (!SquadWanted(p->squadTemplate))
                    continue;
                const std::string& tsid = p->squadTemplate->stringID;
                if (g_targetSquadSet.count(tsid) && !targetSquads.count(tsid))
                    targetSquads[tsid] = p;
                if (p->activePlatoon != NULL)
                {
                    lektor<RootObject*> chars;
                    p->activePlatoon->getCharactersInArea(chars, p->getPosition(), 1.0e7f, false);
                    for (uint32_t c = 0; c < chars.size(); ++c)
                    {
                        if (chars[c] == NULL || chars[c]->getDataType() != CHARACTER)
                            continue;
                        Character* const ch = static_cast<Character*>(chars[c]);
                        if (ch->data != NULL && g_wantedChars.count(ch->data->stringID))
                            exact[ch->data->stringID] = ch;
                    }
                }
                const std::vector<std::string>& members = g_squadMembers[tsid];
                for (size_t k = 0; k < members.size(); ++k)
                    if (g_wantedChars.count(members[k]) && !squadOf.count(members[k]))
                        squadOf[members[k]] = p;
            }
        }

        std::map<std::string, std::string> townLabel;   // поселение -> подпись
        std::set<std::string> squadMarked;              // отряд уже с меткой
        const double now = GameHours();

        for (std::set<std::string>::const_iterator c = g_wantedChars.begin(); c != g_wantedChars.end(); ++c)
        {
            const std::string& sid = *c;
            Marker m;
            m.fixed = false;
            m.imprisoned = false;
            m.label = LabelFor(sid);

            std::map<std::string, Character*>::const_iterator ex = exact.find(sid);
            if (ex != exact.end())
            {
                Character* const ch = ex->second;
                if (ch->isDead())
                {
                    MarkDead(sid, now, true, ch->getPosition());
                    g_imprisonedSeen.erase(sid);
                }
                else
                {
                    MarkSeen(sid);
                    m.object = hand(ch);
                    m.accuracy = ACC_EXACT;
                    m.label = ch->getName();
                    m.imprisoned = ch->platoon != NULL && ch->platoon->me != NULL && ch->platoon->me->imprisoned;
                    if (m.imprisoned)
                        g_imprisonedSeen.insert(sid);
                    else
                        g_imprisonedSeen.erase(sid);
                    g_markers.push_back(m);
                    continue;
                }
            }

            // Игра прямо говорит «жива» или «в плену» (пленник жив), а мы
            // помним её убитой - память устарела: воскресили, обменяли,
            // выпустили (08.10.2026). Отметку о смерти снимаем.
            const int state = WorldState(sid);
            if (state == 1 || state == 2)
            {
                std::map<std::string, Seen>::iterator was = g_seen.find(sid);
                if (was != g_seen.end() && was->second.dead)
                {
                    was->second.dead = false;
                    g_memoryDirty = true;
                    if (g_debug)
                        DebugLog("WantedMap: '" + m.label + "' is alive again by the world state");
                }
            }

            // Убита: серая метка на месте смерти - первые DeadMarkerHours
            // часов игры, потом ничего.
            if (const Seen* const dead = RememberedDead(sid, now))
            {
                if (dead->hasPos && dead->deathHours > 0.0 && now - dead->deathHours < g_deadHours)
                {
                    m.fixed = true;
                    m.pos = dead->pos;
                    m.accuracy = ACC_DEAD;
                    g_markers.push_back(m);
                }
                continue;
            }
            if (state == 0)
                continue;
            m.imprisoned = state == 2;

            std::map<std::string, Platoon*>::const_iterator sq = squadOf.find(sid);
            if (sq != squadOf.end())
            {
                MarkSeen(sid);
                if (squadMarked.insert(sq->second->squadTemplate->stringID).second)
                {
                    m.object = hand(sq->second);
                    m.accuracy = ACC_SQUAD;
                    m.imprisoned = m.imprisoned || sq->second->imprisoned;
                    if (m.imprisoned)
                        g_imprisonedSeen.insert(sid);
                    else
                        g_imprisonedSeen.erase(sid);
                    g_markers.push_back(m);
                }
                continue;
            }
            // Была в мире, а отряда нет нигде - погибла (отряд убрали).
            if (RememberedSeen(sid))
            {
                if (g_debug)
                    DebugLog("WantedMap: '" + m.label + "' is gone, treated as dead");
                continue;
            }
            AddTownLabels(sid, m.label, townLabel);
        }

        // Цели-отряды.
        for (std::set<std::string>::const_iterator s = g_wantedSquads.begin(); s != g_wantedSquads.end(); ++s)
        {
            if (!g_targetSquadSet.count(*s))
                continue;
            std::map<std::string, Platoon*>::const_iterator ts = targetSquads.find(*s);
            if (ts != targetSquads.end())
            {
                MarkSeen(*s);
                Marker m;
                m.fixed = false;
                m.object = hand(ts->second);
                m.accuracy = ts->second->activePlatoon ? ACC_EXACT : ACC_SQUAD;
                m.label = LabelFor(*s);
                m.imprisoned = ts->second->imprisoned;
                g_markers.push_back(m);
            }
            else if (g_townFallback && !RememberedSeen(*s))   // был и пропал - разбит
            {
                const std::vector<std::string>& towns = g_squadTowns[*s];
                for (size_t t = 0; t < towns.size(); ++t)
                    if (townLabel[towns[t]].empty())
                        townLabel[towns[t]] = LabelFor(*s);
            }
        }

        // Поселения - из менеджера поселений (shou->townList): его списки
        // и есть живые поселения. Списки фракций (warMgr->myTowns) после
        // загрузки сейва держали уже удалённые: 05.10.2026 игра упала на
        // town->data (мусор) в townLabel.find через 20 с после загрузки.
        if (!townLabel.empty() && shou != NULL && shou->townList != NULL)
        {
            TownList* const tl0 = shou->townList;
            const lektor<RootObject*>* const lists[2] = { &tl0->getAllTowns(), &tl0->nests };
            for (int li = 0; li < 2; ++li)
            {
                const lektor<RootObject*>& towns = *lists[li];
                for (uint32_t t = 0; t < towns.size(); ++t)
                {
                    TownBase* const town = static_cast<TownBase*>(towns.stuff[t]);
                    if (town == NULL || town->data == NULL)
                        continue;
                    std::map<std::string, std::string>::iterator tl = townLabel.find(town->data->stringID);
                    if (tl == townLabel.end() || tl->second.empty())
                        continue;
                    Marker m;
                    m.fixed = false;
                    m.object = hand(town);
                    m.accuracy = ACC_TOWN;
                    m.label = tl->second;
                    m.imprisoned = false;
                    g_markers.push_back(m);
                    tl->second.clear();
                }
            }
        }

        if (ou->player != NULL)
            SaveMemory(ou->player->factionName);
    }


    // Защита от мусора в данных игры: если обход мира всё же упадёт на
    // испорченном указателе (как 05.10 на удалённом поселении), метки этого
    // прохода пропадут и в журнал уйдёт строка - но игра не упадёт.
    // Отдельная функция: в самой ScanWorld есть объекты с деструкторами, а
    // __try с ними несовместим.
    bool SafeScanWorld()
    {
        __try
        {
            ScanWorld();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }


    // ---------------------------------------------------------------
    // Метки на карте
    // ---------------------------------------------------------------

    const std::string TAG_LAYER = "WantedMapLayer";
    const int DOT = 10;

    MyGUI::Widget* FindLayer(MyGUI::Widget* parent)
    {
        const size_t n = parent->getChildCount();
        for (size_t i = 0; i < n; ++i)
        {
            MyGUI::Widget* const c = parent->getChildAt(i);
            if (!c->getUserString(TAG_LAYER).empty())
                return c;
        }
        return NULL;
    }

    MyGUI::Colour ColourOf(const Marker& m)
    {
        if (m.accuracy == ACC_DEAD)
            return MyGUI::Colour(0.55f, 0.55f, 0.55f);
        if (m.imprisoned)
            return MyGUI::Colour(0.40f, 0.65f, 0.95f);
        switch (m.accuracy)
        {
        case ACC_EXACT:    return MyGUI::Colour(0.90f, 0.25f, 0.20f);
        case ACC_SQUAD:    return MyGUI::Colour(0.95f, 0.55f, 0.15f);
        default:           return MyGUI::Colour(0.90f, 0.80f, 0.30f);
        }
    }

    std::string LabelText(const Marker& m)
    {
        std::string s = m.label;
        if (m.accuracy == ACC_DEAD)
            s += std::string(" (") + Tr("dead") + ")";
        else if (m.imprisoned)
            s += std::string(" (") + Tr("imprisoned") + ")";
        else if (m.accuracy == ACC_SQUAD)
            s += std::string(" (") + Tr("approx.") + ")";
        else if (m.accuracy == ACC_TOWN)
            s += std::string(" (") + Tr("lives here") + ")";
        return s;
    }

    size_t g_layerSignature = 0;

    void DrawMarkers(MapScreen* map)
    {
        MyGUI::Widget* const image = map->mapImage;
        if (image == NULL)
            return;
        MyGUI::Widget* layer = FindLayer(image);
        if (layer == NULL)
        {
            layer = image->createWidget<MyGUI::Widget>("PanelEmpty",
                MyGUI::IntCoord(0, 0, image->getWidth(), image->getHeight()),
                MyGUI::Align::Stretch, "");
            layer->setUserString(TAG_LAYER, "1");
            layer->setNeedMouseFocus(false);
            g_layerSignature = 0;
        }
        if (layer->getSize() != image->getSize())
            layer->setSize(image->getSize());

        size_t sig = g_markers.size() * 131;
        for (size_t i = 0; i < g_markers.size(); ++i)
            sig = sig * 31 + g_markers[i].label.size() * 7 + g_markers[i].accuracy * 3 +
                  (g_markers[i].imprisoned ? 5 : 0) + (g_markers[i].fixed ? 11 : 0);
        if (sig != g_layerSignature || layer->getChildCount() != g_markers.size() * 3)
        {
            g_layerSignature = sig;
            while (layer->getChildCount() > 0)
                MyGUI::Gui::getInstance().destroyWidget(layer->getChildAt(0));
            for (size_t i = 0; i < g_markers.size(); ++i)
            {
                MyGUI::Widget* const frame = layer->createWidget<MyGUI::Widget>(
                    "WhiteSkin", MyGUI::IntCoord(0, 0, DOT + 2, DOT + 2), MyGUI::Align::Default, "");
                frame->setColour(MyGUI::Colour(0.05f, 0.05f, 0.05f));
                frame->setNeedMouseFocus(false);
                MyGUI::Widget* const dot = layer->createWidget<MyGUI::Widget>(
                    "WhiteSkin", MyGUI::IntCoord(0, 0, DOT, DOT), MyGUI::Align::Default, "");
                dot->setColour(ColourOf(g_markers[i]));
                dot->setNeedMouseFocus(false);
                MyGUI::TextBox* const text = layer->createWidget<MyGUI::TextBox>(
                    "Kenshi_TextboxStandardText", MyGUI::IntCoord(0, 0, 400, 20), MyGUI::Align::Default, "");
                text->setNeedMouseFocus(false);
                text->setTextShadow(true);
                text->setTextColour(ColourOf(g_markers[i]));
                text->setCaption(LabelText(g_markers[i]));
                text->setVisible(g_showLabels);
            }
        }

        for (size_t i = 0; i < g_markers.size(); ++i)
        {
            MyGUI::Widget* const frame = layer->getChildAt(i * 3);
            MyGUI::Widget* const dot = layer->getChildAt(i * 3 + 1);
            MyGUI::Widget* const text = layer->getChildAt(i * 3 + 2);
            Ogre::Vector3 pos;
            if (g_markers[i].fixed)
                pos = g_markers[i].pos;
            else
            {
                RootObjectBase* const obj = g_markers[i].object.getRootObjectBase();
                if (obj == NULL)
                {
                    frame->setVisible(false);
                    dot->setVisible(false);
                    text->setVisible(false);
                    continue;
                }
                pos = obj->getPosition();
            }
            const MyGUI::IntPoint p = map->worldToMapCoords(pos);
            frame->setPosition(p.left - DOT / 2 - 1, p.top - DOT / 2 - 1);
            dot->setPosition(p.left - DOT / 2, p.top - DOT / 2);
            text->setPosition(p.left + DOT / 2 + 4, p.top - 10);
            frame->setVisible(true);
            dot->setVisible(true);
            text->setVisible(g_showLabels);
        }
    }


    // ---------------------------------------------------------------
    // Хук карты
    // ---------------------------------------------------------------

    DWORD g_lastScan = 0;
    DWORD g_lastWorldScan = 0;

    void (*g_origMapUpdate)(MapScreen* self) = NULL;

    // ---------------------------------------------------------------
    // Значок на самой листовке (08.10.2026, просьба пользователя)
    //
    // Листовка в инвентаре: все её цели убиты - зелёная галка в левом нижнем
    // углу; остальные в плену - замок; закрыта часть - счёт «2/3». Значки
    // вещей игра создаёт конструктором InventoryIcon - после него и рисуем
    // (как глаз на чертежах в DarkUiTweaks). Цели - только персонажи: у
    // целей-отрядов смерть не отследить.
    // ---------------------------------------------------------------

    const std::string& BadgeTexture(const char* file)
    {
        static std::map<std::string, std::string> resolved;
        std::map<std::string, std::string>::iterator it = resolved.find(file);
        if (it != resolved.end())
            return it->second;
        const std::string names[] = { std::string("gui/gfx/") + file, std::string("gui\\gfx\\") + file, file,
                                      std::string("mods/WantedMap/gui/gfx/") + file };
        std::string texture;
        for (size_t i = 0; i < sizeof(names) / sizeof(names[0]) && texture.empty(); ++i)
        {
            try
            {
                if (Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(names[i]))
                    texture = names[i];
            }
            catch (...)
            {
            }
        }
        if (texture.empty())
            ErrorLog(std::string("WantedMap: gui/gfx/") + file + " not found - no badge on posters");
        return resolved[file] = texture;
    }

    // Сколько целей-персонажей листовки, сколько убито и сколько в плену.
    void PosterState(const std::string& posterSid, int& total, int& dead, int& captive)
    {
        total = dead = captive = 0;
        std::map<std::string, std::vector<std::string> >::const_iterator it = g_posterTargets.find(posterSid);
        if (it == g_posterTargets.end())
            return;
        const double now = GameHours();
        for (size_t i = 0; i < it->second.size(); ++i)
        {
            const std::string& sid = it->second[i];
            if (!g_targetChars.count(sid))
                continue;
            ++total;
            const int state = WorldState(sid);
            if (state == 0 || RememberedDead(sid, now) != NULL)
                ++dead;
            else if (state == 2 || g_imprisonedSeen.count(sid) != 0)
                ++captive;
        }
    }

    void AddPosterBadge(InventoryIcon* self, Item* item)
    {
        if (item == NULL || item->data == NULL || ou == NULL)
            return;
        if (!g_indexBuilt)
            BuildIndex();
        if (g_posterTargets.find(item->data->stringID) == g_posterTargets.end())
            return;
        SyncMemoryFile();
        ProcessDeaths();                // убили при закрытой карте - тоже в счёт
        int total = 0, dead = 0, captive = 0;
        PosterState(item->data->stringID, total, dead, captive);
        if (total == 0 || dead + captive == 0)
            return;
        MyGUI::Widget* const icon = self->getWidget();
        if (icon == NULL)
            return;
        const MyGUI::IntSize size = icon->getSize();
        int side = (size.width < size.height ? size.width : size.height) * 2 / 5;
        if (side < 14) side = 14;
        if (side > 28) side = 28;
        if (dead + captive == total)
        {
            const std::string& texture = BadgeTexture(captive > 0 ? "wantedmap_captive.png" : "wantedmap_done.png");
            if (texture.empty())
                return;
            MyGUI::ImageBox* const badge = icon->createWidget<MyGUI::ImageBox>(
                "ImageBox", MyGUI::IntCoord(2, size.height - side - 2, side, side),
                MyGUI::Align::Left | MyGUI::Align::Bottom);
            if (badge != NULL)
            {
                badge->setImageTexture(texture);
                badge->setNeedMouseFocus(false);    // клики и перетаскивание - значку
            }
            return;
        }
        // Закрыта часть целей - счёт.
        char text[16];
        sprintf_s(text, "%d/%d", dead + captive, total);
        MyGUI::TextBox* const count = icon->createWidget<MyGUI::TextBox>(
            "Kenshi_TextboxStandardText", MyGUI::IntCoord(3, size.height - 20, size.width - 6, 18),
            MyGUI::Align::Left | MyGUI::Align::Bottom);
        if (count != NULL)
        {
            count->setCaption(text);
            count->setTextColour(MyGUI::Colour(1.0f, 0.42f, 0.36f));
            count->setTextShadow(true);
            count->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Bottom);
            count->setNeedMouseFocus(false);
        }
    }

    void SafeAddPosterBadge(InventoryIcon* self, Item* item)
    {
        __try
        {
            AddPosterBadge(self, item);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            static bool told = false;
            if (!told)
            {
                told = true;
                ErrorLog("WantedMap: the poster badge failed once - skipped");
            }
        }
    }

    InventoryIcon* (*g_origIconCtor)(InventoryIcon*, Item*, const MyGUI::IntPoint&, MyGUI::Widget*) = NULL;

    InventoryIcon* IconCtor_hook(InventoryIcon* self, Item* item, const MyGUI::IntPoint& position,
                                 MyGUI::Widget* parent)
    {
        InventoryIcon* const result = g_origIconCtor(self, item, position, parent);
        if (g_enabled && g_posterBadge)
            SafeAddPosterBadge(self, item);
        return result;
    }

    void MapUpdate_hook(MapScreen* self)
    {
        g_origMapUpdate(self);
        if (self == NULL || !self->getVisible())
            return;
        if (!g_enabled)
        {
            // Выключили в MCM на ходу - прячем слой меток.
            if (self->mapImage != NULL)
                if (MyGUI::Widget* const layer = FindLayer(self->mapImage))
                    layer->setVisible(false);
            return;
        }

        if (!g_indexBuilt)
            BuildIndex();
        SyncMemoryFile();
        ProcessDeaths();

        const DWORD now = GetTickCount();
        bool changed = false;
        if (g_lastScan == 0 || now - g_lastScan >= g_scanMs)
        {
            g_lastScan = now;
            changed = ScanPosters();
            if (changed)
                BuildWantedSets();
        }
        if (changed || g_lastWorldScan == 0 || now - g_lastWorldScan >= g_worldScanMs)
        {
            g_lastWorldScan = now;
            if (!SafeScanWorld())
            {
                g_markers.clear();
                ErrorLog("WantedMap: the world scan hit broken game data and was skipped");
            }
            if (g_debug)
            {
                char note[200];
                sprintf_s(note, "WantedMap: game hours %.2f; %u posters held, %u targets, %u squads wanted, %u markers",
                          GameHours(),
                          static_cast<unsigned>(g_heldPosters.size()),
                          static_cast<unsigned>(g_wantedChars.size()),
                          static_cast<unsigned>(g_wantedSquads.size()),
                          static_cast<unsigned>(g_markers.size()));
                DebugLog(note);
                for (size_t i = 0; i < g_markers.size(); ++i)
                {
                    char line[256];
                    sprintf_s(line, "WantedMap:   acc=%d fixed=%d '", g_markers[i].accuracy,
                              g_markers[i].fixed ? 1 : 0);
                    DebugLog(line + g_markers[i].label + "'");
                }
            }
        }

        DrawMarkers(self);
    }
}


// Страница в ModConfigMenu (вкладка MCM в настройках игры), если он есть.
// Описание API - shared/ModConfigMenu.h. У каждой строки - подсказка.
extern "C" __declspec(dllexport) void MCM_Describe(MCM_Api* api)
{
    static const std::string ini = IniPath();
    api->beginMod(api, "WantedMap", "Wanted Map", ini.c_str(), &LoadSettings);
    if (api->version >= 2)
        api->info(api, Tr("Markers on the big map for the targets of the wanted posters the selected character carries."));
    api->section(api, Tr("Markers"));
    api->toggle(api, "WantedMap", "Enabled", Tr("Enabled"), Tr("Markers on the big map for the targets of wanted posters held by the selected characters."), 1, 0);
    api->toggle(api, "WantedMap", "PosterBadge", Tr("Badge on the poster"), Tr("On the poster in the inventory: a green tick - all its targets are dead, a lock - the rest are imprisoned, 2/3 - how many are done."), 1, 0);
    api->toggle(api, "WantedMap", "ShowLabels", Tr("Names next to markers"), Tr("The target's name, and dead or imprisoned, next to its marker on the map."), 1, 0);
    api->toggle(api, "WantedMap", "TownFallback", Tr("Mark the home of targets not seen yet"), Tr("If the target's squad has not appeared in the world yet: a yellow marker on the town or camp where it lives."), 1, 0);
    api->section(api, Tr("Killed targets"));
    api->toggle(api, "WantedMap", "RememberSeen", Tr("Remember seen and killed targets"), Tr("Remembers which targets were already in the world and who was killed, where and when. Kept per playthrough in the memory folder next to the plugin."), 1, 0);
    api->integer(api, "WantedMap", "DeadMarkerHours", Tr("Show killed targets for, game hours"), Tr("How many game hours a grey marker stays where a target was killed. 0 - killed targets are not shown."), 24, 0, 168, 0);
    api->section(api, Tr("Search"));
    api->integer(api, "WantedMap", "ScanSeconds", Tr("Poster check, seconds"), Tr("How often the selected characters' inventories and backpacks are checked for posters while the map is open."), 2, 1, 60, 0);
    api->integer(api, "WantedMap", "WorldScanSeconds", Tr("Target search, seconds"), Tr("How often all factions are searched for the targets' squads while the map is open. In between only the positions of found markers move."), 30, 5, 300, 0);
    api->text(api, "Match", "StopWords", Tr("Words ignored in names"), Tr("Comma-separated words that do not count when a poster's name is compared with a character's name, such as king or leader. Any language."), Tr("the,of,and,wanted,king,boss,leader"), MCM_RESTART);
    api->section(api, Tr("Diagnostics"));
    api->toggle(api, "WantedMap", "Debug", Tr("Detailed log"), Tr("Details in RE_Kenshi_log.txt: posters, targets and markers, and the matched table WantedMap_runtime_table.txt next to the plugin."), 0, 0);
}


__declspec(dllexport) void startPlugin()
{
    LoadSettings();
    // Хуки ставим и при Enabled=0: выключатель проверяется в каждом кадре,
    // так что его можно переключать из MCM без перезапуска.
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&MapScreen::update), MapUpdate_hook, &g_origMapUpdate))
    {
        ErrorLog("WantedMap: could not hook MapScreen::update, no markers");
        return;
    }
    InitializeCriticalSection(&g_deathLock);
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&Character::declareDead), DeclareDead_hook, &g_origDeclareDead))
        ErrorLog("WantedMap: could not hook Character::declareDead, deaths seen only on the map");
    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            KenshiLib::GetRealAddress(&InventoryIcon::_CONSTRUCTOR), IconCtor_hook, &g_origIconCtor))
        ErrorLog("WantedMap: could not hook InventoryIcon, no badge on posters");
    DebugLog("WantedMap: installed");
}
