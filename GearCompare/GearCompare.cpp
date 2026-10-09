// GearCompare - сравнение вещи под курсором с тем, что надето.
//
// ЧТО ДЕЛАЕТ
//
// Рядом со всплывающим описанием вещи (инвентарь, рюкзак, магазин)
// ставится вторая панель: что сейчас надето в том же слоте, и по каждому
// свойству - «надето -> станет -> разница» зелёным или красным. Внизу три
// оси и вердикт:
//
//   броня:   Защита (сопротивление с учётом покрытия), Свойства, Вес
//   оружие:  Урон, Свойства, Вес
//
// Оружие сравнивается с надетым оружием того же типа (катана с катаной,
// арбалет с арбалетом) - так решил владелец сборки. Итоговый процент по
// желанию, через ini (ShowTotal).
//
// С КЕМ СРАВНИВАЕМ
//
// Вещь лежит у персонажа игрока (в инвентаре или в его рюкзаке) - с ним.
// Иначе (магазин, сундук, земля) - с выделенным персонажем.
//
// ЧЕГО НЕ ТРОГАЕТ
//
// Игровое окошко описания. Его уже перехватывает LtEast's Rarity
// Backgrounds (ToolTipInventory::setup/show/update), а KEP дописывает строки
// в описания оружия (getTooltipData1 клинков и арбалетов). Мы узнаём,
// какую вещь оно показывает, через ToolTipInventory::setContent и
// getTooltipData1 снаряжения - с LtEast функции не общие, с KEP общие, но
// исходную мы зовём первой - и ставим свою панель рядом.
//
// ПОЧЕМУ ТАК
//
// * Надетое берём из секций инвентаря, которыми владеет игра
//   (getAllSections, isAnEquippedItemSection). getEquippedArmour и
//   getEquippedWeapons не годятся: они заполняют lektor, у которого нет
//   деструктора, - каждый вызов оставлял бы в памяти массив.
// * Панель рисуем из ForgottenGUI::update: оттуда MyGUI трогать можно, из
//   игрового цикла - нет (см. shared\GameTheme.h).
// * Панель и все её надписи не ловят мышь: иначе курсор, заехав на неё,
//   снимал бы наведение с вещи, и описание мигало бы.

#define KLOC_DOMAIN "gear_compare"
#include <Localization.h>
#include <ModConfigMenu.h>
#include <GameTheme.h>
#include <HoldKey.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cctype>
#include <cmath>
#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <Debug.h>
#include <core/Functions.h>

#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_Widget.h>
#include <mygui/MyGUI_TextBox.h>
#include <mygui/MyGUI_RenderManager.h>

#include <kenshi/Globals.h>
#include <kenshi/GameWorld.h>
#include <kenshi/GameData.h>
#include <kenshi/Character.h>
#include <kenshi/Inventory.h>
#include <kenshi/Item.h>
#include <kenshi/Gear.h>
#include <kenshi/GunClass.h>
#include <kenshi/Enums.h>
#include <kenshi/util/hand.h>
#include <kenshi/util/StringPair.h>
#include <kenshi/gui/ForgottenGUI.h>
#include <kenshi/gui/ToolTip.h>

extern "C" IMAGE_DOS_HEADER __ImageBase;


namespace
{
    // ---------------------------------------------------------------
    // Настройки
    // ---------------------------------------------------------------

    bool g_enabled = true;
    bool g_debug = false;
    bool g_showTotal = false;

    // Сравнивать и с пустым слотом («Надето: ничего»). Тестеры попросили
    // выключить: при пустом слоте панель лишь повторяет окошко вещи.
    bool g_compareEmpty = false;

    // Клавиша, пока зажата которая видна панель; 0 - видна всегда.
    HoldKey::Key g_holdKey;    // ALT; vk 0 - показывать всегда (см. HoldKey.h)
    float g_totalPerProperty = 3.0f;   // % итога за каждое «лучше/хуже»
    float g_totalPerKg = 2.0f;         // % итога за каждый килограмм
    // auto - брать у игрового окошка (цвет чисел с «+» и с «-»); #RRGGBB -
    // свой цвет.
    std::string g_colourBetter = "auto";
    std::string g_colourWorse = "auto";

    // Порог «заметной» разницы для вердикта, в процентах главной оси.
    const float kSameThreshold = 2.0f;


    std::string IniPath()
    {
        char path[MAX_PATH] = {};
        const DWORD length = GetModuleFileNameA(
            reinterpret_cast<HMODULE>(&__ImageBase), path, MAX_PATH);
        if (length == 0 || length >= MAX_PATH)
            return std::string();

        std::string file(path, length);
        const std::string::size_type dot = file.rfind('.');
        return dot == std::string::npos ? std::string()
                                        : file.substr(0, dot) + ".ini";
    }


    std::string ReadString(const std::string& ini, const char* key,
                           const std::string& fallback)
    {
        char buffer[64] = {};
        GetPrivateProfileStringA("GearCompare", key, fallback.c_str(),
                                 buffer, sizeof(buffer), ini.c_str());
        return buffer;
    }


    float ReadFloat(const std::string& ini, const char* key, float fallback)
    {
        char def[32];
        sprintf_s(def, "%g", fallback);
        const std::string text = ReadString(ini, key, def);
        char* end = NULL;
        const double value = strtod(text.c_str(), &end);
        return end == text.c_str() ? fallback : static_cast<float>(value);
    }


    void LoadSettings()
    {
        const std::string ini = IniPath();
        if (ini.empty())
            return;

        g_enabled = GetPrivateProfileIntA("GearCompare", "Enabled", 1, ini.c_str()) != 0;
        g_debug = GetPrivateProfileIntA("GearCompare", "Debug", 0, ini.c_str()) != 0;
        g_showTotal = GetPrivateProfileIntA("GearCompare", "ShowTotal", 0, ini.c_str()) != 0;
        g_compareEmpty = GetPrivateProfileIntA("GearCompare", "CompareEmptySlot", 0, ini.c_str()) != 0;

        g_holdKey = HoldKey::Parse(ReadString(ini, "HoldKey", "ALT"));   // любая клавиша, «NONE» - всегда
        g_totalPerProperty = ReadFloat(ini, "TotalPerProperty", g_totalPerProperty);
        g_totalPerKg = ReadFloat(ini, "TotalPerKg", g_totalPerKg);
        g_colourBetter = ReadString(ini, "ColourBetter", g_colourBetter);
        g_colourWorse = ReadString(ini, "ColourWorse", g_colourWorse);
    }


    // ---------------------------------------------------------------
    // Свойства вещи
    //
    // Каждое свойство - строка сравнения. Тип определяет, как его писать
    // и считать разницу:
    //   PERCENT - доля, пишем в процентах (0.09 -> 9%);
    //   MULT    - множитель, пишем ×1.10, разницу - в процентах;
    //   POINTS  - целые пункты навыка (+2);
    //   KG      - килограммы;
    //   PLAIN   - число как есть.
    // better: +1 - больше лучше, -1 - меньше лучше, 0 - просто для сведения.
    // ---------------------------------------------------------------

    enum Kind { PERCENT, MULT, POINTS, KG, MONEY, PLAIN };

    struct Property
    {
        std::string label;
        Kind kind;
        int better;
        float value;
        bool present;   // есть ли свойство у вещи вообще
    };

    typedef std::vector<Property> Properties;

    // Нейтральное значение: у кого его нет, у того и свойства нет.
    float Neutral(Kind kind)
    {
        return kind == MULT ? 1.0f : 0.0f;
    }


    void Add(Properties& out, const char* label, Kind kind, int better,
             float value, bool present)
    {
        Property p;
        p.label = label;
        p.kind = kind;
        p.better = better;
        p.value = value;
        p.present = present;
        out.push_back(p);
    }


    void AddIfSet(Properties& out, const char* label, Kind kind, int better,
                  float value)
    {
        Add(out, label, kind, better, value,
            std::fabs(value - Neutral(kind)) > 0.0001f);
    }


    // Итог по вещи для главной оси и сама вещь в строках.
    struct Summary
    {
        Properties rows;
        float main;         // защита или урон - главная ось
        float weight;       // кг
        bool isArmour;
        std::string name;
        // Подпись главной оси в итогах - с тем, из чего она: игроки не
        // понимали, что «Урон -17%» - это режущий и дробящий вместе.
        std::string mainLabel;
    };


    // Защита с учётом покрытия: сопротивление работает только на
    // закрытых частях тела, поэтому по каждой части «покрытие ×
    // среднее из сопротивлений разрезу и дроблению». Части равноценны.
    float ArmourProtection(Armour* a)
    {
        float total = 0.0f;
        const float resistance = (a->cutResistance + a->bluntResistance) * 0.5f;

        for (ogre_unordered_map<GameData*, float>::type::const_iterator it =
                 a->bodypartCoverage.begin();
             it != a->bodypartCoverage.end(); ++it)
        {
            total += it->second * resistance;
        }

        return total;
    }


    void DescribeArmour(Armour* a, Summary& s)
    {
        s.isArmour = true;
        s.main = ArmourProtection(a);
        s.mainLabel = Tr("Protection (with coverage)");

        AddIfSet(s.rows, Tr("Cut resistance"), PERCENT, +1, a->cutResistance);
        AddIfSet(s.rows, Tr("Blunt resistance"), PERCENT, +1, a->bluntResistance);
        AddIfSet(s.rows, Tr("Harpoon resistance"), PLAIN, +1, a->pierceResistance);

        for (ogre_unordered_map<GameData*, float>::type::const_iterator it =
                 a->bodypartCoverage.begin();
             it != a->bodypartCoverage.end(); ++it)
        {
            if (it->first == NULL)
                continue;
            const std::string label =
                std::string(Tr("Coverage: ")) + it->first->name;
            AddIfSet(s.rows, label.c_str(), PERCENT, +1, it->second);
        }

        const struct { WeatherAffecting type; const char* label; } weather[] =
        {
            { WA_ACID, "Acid protection" },
            { WA_GAS, "Gas protection" },
            { WA_BURNING, "Burning protection" },
            { WA_DUSTSTORM, "Dust storm protection" },
            { WA_RAIN, "Rain protection" }
        };
        for (int i = 0; i < sizeof(weather) / sizeof(weather[0]); ++i)
            AddIfSet(s.rows, Tr(weather[i].label), PERCENT, +1,
                     a->getWeatherProtection_simple(weather[i].type));

        AddIfSet(s.rows, Tr("Attack"), POINTS, +1, static_cast<float>(a->combatSkillBonusAttk));
        AddIfSet(s.rows, Tr("Defence"), POINTS, +1, static_cast<float>(a->combatSkillBonusDef));
        AddIfSet(s.rows, Tr("Perception"), POINTS, +1, static_cast<float>(a->perceptionBonus));
        AddIfSet(s.rows, Tr("Martial arts"), POINTS, +1, static_cast<float>(a->unarmedBonus));
        AddIfSet(s.rows, Tr("Stealth"), MULT, +1, a->stealthMult);
        AddIfSet(s.rows, Tr("Assassination"), MULT, +1, a->assassinMult);
        AddIfSet(s.rows, Tr("Athletics"), MULT, +1, a->athleticsMult);
        AddIfSet(s.rows, Tr("Dexterity"), MULT, +1, a->dexterityMult);
        AddIfSet(s.rows, Tr("Dodge"), MULT, +1, a->dodgeMult);
        AddIfSet(s.rows, Tr("Combat speed"), MULT, +1, a->combatSpeedMult);
        AddIfSet(s.rows, Tr("Damage"), MULT, +1, a->damageMult);
        AddIfSet(s.rows, Tr("Ranged skills"), MULT, +1, a->rangedSkillMult);
        AddIfSet(s.rows, Tr("Fist injuries"), MULT, -1, a->fistInjuryMult);
    }


    // Урон против рас (просьба игрока, Steam 05.10). В описании игры это
    // строки «Урон пр. роботов / людей / зверей» и «Урон пр. <раса>».
    // Числа лежат в данных оружия: множители robot/human/animal damage mult
    // (1 - без изменений) и список ссылок «race damage» - раса и процент
    // (200 - вдвое). Нет поля - множитель 1: строка видна, только если у
    // одной из вещей он другой.
    float DataFloat(GameData* d, const char* key, float def)
    {
        ogre_unordered_map<std::string, float>::type::iterator it = d->fdata.find(key);
        return it == d->fdata.end() ? def : it->second;
    }

    void DescribeRaceDamage(Weapon* w, Summary& s)
    {
        GameData* const d = w->getGameData();
        if (d == NULL)
            return;
        AddIfSet(s.rows, Tr("Damage vs robots"), MULT, +1, DataFloat(d, "robot damage mult", 1.0f));
        AddIfSet(s.rows, Tr("Damage vs humans"), MULT, +1, DataFloat(d, "human damage mult", 1.0f));
        AddIfSet(s.rows, Tr("Damage vs animals"), MULT, +1, DataFloat(d, "animal damage mult", 1.0f));

        const Ogre::vector<GameDataReference>::type* const races = d->getReferenceListIfExists("race damage");
        if (races == NULL || ou == NULL)
            return;
        for (size_t i = 0; i < races->size(); ++i)
        {
            GameData* const race = (*races)[i].getPtr(&ou->gamedata);
            if (race == NULL || race->name.empty())
                continue;
            const std::string label = std::string(Tr("Damage vs")) + " " + race->name;
            AddIfSet(s.rows, label.c_str(), MULT, +1, (*races)[i].values.value[0] / 100.0f);
        }
    }


    void DescribeWeapon(Weapon* w, Summary& s)
    {
        s.isArmour = false;
        s.main = 0.0f;

        Sword* const sword = w->isSword();
        Crossbow* const crossbow = w->isCrossbow();

        s.mainLabel = Tr("Damage");
        if (sword != NULL)
        {
            s.main = sword->cutDamage + sword->bluntDamage;
            s.mainLabel = Tr("Damage (cut + blunt)");
            AddIfSet(s.rows, Tr("Cut damage"), PLAIN, +1, sword->cutDamage);
            AddIfSet(s.rows, Tr("Blunt damage"), PLAIN, +1, sword->bluntDamage);
        }

        AddIfSet(s.rows, Tr("Bleeding"), PLAIN, +1, w->bleedDamage);
        AddIfSet(s.rows, Tr("Attack"), POINTS, +1, static_cast<float>(w->modAttack));

        if (sword != NULL)
        {
            AddIfSet(s.rows, Tr("Defence"), POINTS, +1, static_cast<float>(sword->modDefence));
            AddIfSet(s.rows, Tr("Indoors"), POINTS, +1, static_cast<float>(sword->modIndoors));
        }

        if (crossbow != NULL && crossbow->gunClass != NULL)
        {
            GunClass* const gun = crossbow->gunClass;

            // Арбалет в руках - всегда GunClassPersonal: GunClassTurret
            // бывает только у турелей-построек. Проверки типа у класса
            // нет, поэтому приводим явно; урон лежит в наследнике.
            GunClassPersonal* const personal = static_cast<GunClassPersonal*>(gun);
            const float damage = 0.5f * (personal->minDamage + personal->maxDamage);
            s.main = damage;
            s.mainLabel = Tr("Damage (per shot)");
            AddIfSet(s.rows, Tr("Damage"), PLAIN, +1, damage);
            AddIfSet(s.rows, Tr("Range"), PLAIN, +1, gun->maxRange);
            AddIfSet(s.rows, Tr("Shot speed"), PLAIN, +1, gun->shotSpeed);
            AddIfSet(s.rows, Tr("Aim speed"), PLAIN, +1, gun->aimSpeed);
            AddIfSet(s.rows, Tr("Spread"), PLAIN, -1, gun->accuracyDeviationBase);
            AddIfSet(s.rows, Tr("Reload time"), PLAIN, -1,
                     0.5f * (gun->reloadTimeMin + gun->reloadTimeMax));
            AddIfSet(s.rows, Tr("Shots"), PLAIN, +1, static_cast<float>(gun->numShotsMax));
        }

        DescribeRaceDamage(w, s);
        AddIfSet(s.rows, Tr("Combat weight"), PLAIN, 0, w->getCombatWeight());
    }


    // ---------------------------------------------------------------
    // Строки описания вещи, как их строит сама игра
    //
    // Пробития доспеха, длины оружия и эффективности сопротивления разрезу
    // в полях вещи (KenshiLib) нет: игра считает их в описании, с учётом
    // производителя и качества. Поэтому берём их оттуда же, откуда окошко
    // игры: Item::getTooltipData1/2 - пары «подпись - текст» и число val1.
    // Подпись сверяем и по-английски, и с переводом игры (тот же текст
    // лежит в нашем .po), без двоеточия, тегов цвета и регистра.
    // ---------------------------------------------------------------

    struct TipLine
    {
        std::string label;
        std::string text;
        float value;
    };

    // Без тегов цвета «#RRGGBB»: в журнале игра отдаёт «#767676-Длина» и
    // «#76767630,00» - тег цвета, дефис строки и десятичная запятая.
    std::string StripColourTags(const std::string& in)
    {
        std::string out;
        for (size_t i = 0; i < in.size(); ++i)
        {
            if (in[i] == '#' && i + 6 < in.size() && isxdigit(static_cast<unsigned char>(in[i + 1])))
            {
                i += 6;
                continue;
            }
            out += in[i];
        }
        return out;
    }

    std::string NormalizeLabel(const std::string& in)
    {
        std::string out;
        const std::string plain = StripColourTags(in);
        for (size_t i = 0; i < plain.size(); ++i)
        {
            const unsigned char c = static_cast<unsigned char>(plain[i]);
            out += (c < 128) ? static_cast<char>(tolower(c)) : plain[i];
        }
        while (!out.empty() && (out[out.size() - 1] == ':' || out[out.size() - 1] == ' '))
            out.erase(out.size() - 1);
        while (!out.empty() && (out[0] == ' ' || out[0] == '-'))
            out.erase(0, 1);
        return out;
    }

    // Первое число текста: знак, цифры, запятая или точка («+30%», «0,63»).
    bool ParseTipNumber(const std::string& in, float& value)
    {
        const std::string text = StripColourTags(in);
        size_t i = 0;
        while (i < text.size() && text[i] != '+' && text[i] != '-'
               && !isdigit(static_cast<unsigned char>(text[i])))
            ++i;
        std::string number;
        for (; i < text.size(); ++i)
        {
            const char c = text[i];
            if (isdigit(static_cast<unsigned char>(c)) || ((c == '+' || c == '-') && number.empty()))
                number += c;
            else if (c == ',' || c == '.')
                number += '.';
            else
                break;
        }
        if (number.empty() || number == "+" || number == "-")
            return false;
        value = static_cast<float>(atof(number.c_str()));
        return true;
    }

    void GameTooltipLines(Item* item, std::vector<TipLine>& out)
    {
        out.clear();
        Ogre::vector<StringPair>::type lines;
        item->getTooltipData1(lines);
        item->getTooltipData2(lines);
        for (size_t i = 0; i < lines.size(); ++i)
        {
            TipLine line;
            line.label = lines[i].s1;
            line.text = lines[i].s2;
            line.value = lines[i].val1;
            out.push_back(line);
        }

        // Debug=1: что игра пишет в описание - по разу на вещь. По этим
        // строкам видно, нашлись ли подписи и в каком масштабе числа.
        static std::set<std::string> dumped;
        if (g_debug && dumped.insert(item->displayName).second)
        {
            for (size_t i = 0; i < out.size(); ++i)
            {
                DebugLog("GearCompare: tooltip '" + item->displayName + "' | " +
                         out[i].label + " | " + out[i].text);
            }
        }
    }

    // Число из строки описания - по тексту значения: val1 у этих строк
    // игра не заполняет (в журнале там мусор). Проценты - долей.
    bool FindTip(const std::vector<TipLine>& lines, const char* english, Kind kind, float& value)
    {
        const std::string a = NormalizeLabel(english);
        const std::string b = NormalizeLabel(Tr(english));
        for (size_t i = 0; i < lines.size(); ++i)
        {
            const std::string label = NormalizeLabel(lines[i].label);
            if (label != a && label != b)
                continue;
            float v = 0.0f;
            if (!ParseTipNumber(lines[i].text, v))
                return false;
            if (kind == PERCENT && lines[i].text.find('%') != std::string::npos)
                v /= 100.0f;
            value = v;
            return true;
        }
        return false;
    }

    // Длина оружия, если в описании её нет: поле предмета «length».
    bool DataLength(Item* item, float& value)
    {
        GameData* const data = item->getGameData();
        if (data == NULL)
            return false;
        const float f = data->fdata.count("length") ? data->fdata.find("length")->second : 0.0f;
        if (f > 0.0f)
        {
            value = f;
            return true;
        }
        const int n = data->idata.count("length") ? data->idata.find("length")->second : 0;
        if (n > 0)
        {
            value = static_cast<float>(n);
            return true;
        }
        return false;
    }

    // Просьбы игроков (Steam, 04.10): эффективность сопротивления разрезу
    // у брони, пробитие доспеха и длина у оружия.
    void DescribeFromTooltip(Item* item, Summary& s)
    {
        std::vector<TipLine> lines;
        GameTooltipLines(item, lines);

        float v = 0.0f;
        if (s.isArmour)
        {
            // Строку игра пишет не у всякой брони - у обычной её нет; там,
            // где её нет, эффективность полная (100%). Так в сравнении
            // видна разница, как только у одной из вещей она другая.
            if (!FindTip(lines, "Cut resistance efficiency", PERCENT, v))
                v = 1.0f;
            AddIfSet(s.rows, Tr("Cut resistance efficiency"), PERCENT, +1, v);
        }
        else
        {
            if (FindTip(lines, "Armour penetration", PERCENT, v))
                AddIfSet(s.rows, Tr("Armour penetration"), PERCENT, +1, v);
            // Длина - не лучше и не хуже: длинное достаёт дальше, но в
            // тесноте и против быстрых бывает хуже. Показываем без оценки.
            if (FindTip(lines, "Weapon length", PLAIN, v) || FindTip(lines, "Length", PLAIN, v)
                || DataLength(item, v))
                AddIfSet(s.rows, Tr("Weapon length"), PLAIN, 0, v);
        }
    }


    bool Describe(Item* item, Summary& s)
    {
        s.rows.clear();
        s.main = 0.0f;
        s.weight = 0.0f;
        s.name = item != NULL ? item->displayName : std::string();

        if (item == NULL)
            return false;

        if (Armour* const armour = item->isArmour())
            DescribeArmour(armour, s);
        else if (Weapon* const weapon = item->isWeapon())
            DescribeWeapon(weapon, s);
        else
            return false;

        DescribeFromTooltip(item, s);

        s.weight = item->getItemWeight();
        AddIfSet(s.rows, Tr("Weight"), KG, -1, s.weight);
        // Стоимость в магазине, а не при продаже: false - «не игрок»,
        // как у игрового окошка в строке «Стоимость».
        AddIfSet(s.rows, Tr("Price"), MONEY, 0,
                 static_cast<float>(item->getValueSingle(false)));
        return true;
    }


    // ---------------------------------------------------------------
    // С кем и с чем сравнивать
    // ---------------------------------------------------------------

    // Персонаж игрока, у которого лежит вещь (в инвентаре или в рюкзаке).
    Character* OwnerOf(Item* item)
    {
        const hand& where = item->getInventoryWeAreIn();

        Character* c = where.getCharacter();
        if (c == NULL)
        {
            Item* const container = where.getItem();
            if (container != NULL)
                c = container->getInventoryWeAreIn().getCharacter();
        }

        return c != NULL && c->isPlayerCharacter() ? c : NULL;
    }


    bool SameKind(Item* worn, Item* hovered)
    {
        if (hovered->isArmour() != NULL)
            return worn->isArmour() != NULL && worn->slotType == hovered->slotType;

        Weapon* const w = hovered->isWeapon();
        Weapon* const v = worn->isWeapon();
        return w != NULL && v != NULL && v->getCategory() == w->getCategory();
    }


    // С чем сравнивать и как назвать слот в заголовке.
    struct Counterpart
    {
        Item* worn;          // NULL - слот пуст (или подходящего нет)
        std::string slot;    // «Доспех», «Оружие II» - как в инвентаре
        bool wornItself;     // под курсором сама надетая вещь
        bool noSlot;         // оружию некуда встать вовсе
    };


    const char* ArmourSlotName(AttachSlot slot)
    {
        switch (slot)
        {
        case ATTACH_HAT:      return Tr("Head");
        case ATTACH_SHIRT:    return Tr("Shirt");
        case ATTACH_BODY:     return Tr("Armour");
        case ATTACH_LEGS:     return Tr("Trousers");
        case ATTACH_BOOTS:    return Tr("Boots");
        case ATTACH_BELT:     return Tr("Belt");
        case ATTACH_BACKPACK: return Tr("Backpack");
        default:              return Tr("Slot");
        }
    }


    // Номер слота оружия, как его пишет инвентарь. Секции у игры
    // называются «back» и «hip», и первая версия нумеровала их по порядку
    // - вышло наоборот: у пользователя клинок висит в «Оружие II», а лежит
    // в секции hip. В инвентаре «Оружие I» - спина, «Оружие II» - бедро.
    // Неизвестные имена - по порядку после них.
    int WeaponSlotNumber(const std::string& sectionName, int order)
    {
        if (sectionName == "back")
            return 1;
        if (sectionName == "hip")
            return 2;
        return 2 + order;
    }


    std::string WeaponSlotName(int number)
    {
        const char* const roman[] = { "I", "II", "III", "IV" };
        const int i = number >= 1 && number <= 4 ? number - 1 : 3;
        return std::string(Tr("Weapon")) + " " + roman[i];
    }


    struct WeaponSlot
    {
        int number;
        Item* occupant;     // оружие в слоте или NULL
        bool fits;          // вещь под курсором сюда встаёт
    };


    // Броня - с надетой в том же слоте. Оружие - так, как его надела бы
    // сама игра:
    //   1. надетое оружие того же типа (катана с катаной) - так решил
    //      владелец сборки;
    //   2. иначе оружие в слоте, куда вещь встаёт (Оружие I первым);
    //   3. иначе оружие в любом соседнем слоте (с 05.10 - игроки ждут
    //      сравнения со своим оружием, даже если второй слот свободен);
    //   4. оружия не надето вовсе - свободный слот, сравнение с «ничем».
    // Первая версия шла сразу в занятый слот, и оба слота принимают любое
    // оружие - поэтому булава, дзюттэ и тесак все сравнивались с катаной
    // во втором слоте, хотя встали бы в пустой первый.
    Counterpart FindCounterpart(Character* c, Item* hovered)
    {
        Counterpart out;
        out.worn = NULL;
        out.wornItself = false;
        out.noSlot = false;

        const bool weapon = hovered->isWeapon() != NULL;
        out.slot = weapon ? std::string(Tr("Weapon")) : ArmourSlotName(hovered->slotType);

        Inventory* const inventory = c->getInventory();
        if (inventory == NULL)
            return out;

        lektor<InventorySection*>& sections = inventory->getAllSections();

        Item* sameType = NULL;
        std::string sameTypeSlot;
        std::vector<WeaponSlot> weaponSlots;
        int unknownOrder = 0;

        for (uint32_t i = 0; i < sections.size(); ++i)
        {
            InventorySection* const section = sections[i];
            if (section == NULL || !section->isAnEquippedItemSection)
                continue;

            const bool weaponSection = section->limitedSlot == ATTACH_WEAPON;
            std::string label;
            WeaponSlot slot;
            if (weaponSection)
            {
                const bool known = section->name == "back" || section->name == "hip";
                slot.number = WeaponSlotNumber(section->name, known ? 0 : ++unknownOrder);
                slot.occupant = NULL;
                slot.fits = weapon && section->isLimitedSlotCompatible(hovered);
                label = WeaponSlotName(slot.number);

                // Какая секция каким слотом считается - в журнал, по разу.
                static std::set<std::string> logged;
                const std::string note = section->name + " = " + label;
                if (g_debug && logged.insert(note).second)
                    DebugLog("GearCompare: weapon section " + note);
            }

            for (size_t j = 0; j < section->items.size(); ++j)
            {
                Item* const worn = section->items[j].item;
                if (worn == NULL)
                    continue;
                if (worn == hovered)
                {
                    out.wornItself = true;
                    return out;
                }
                if (weaponSection && slot.occupant == NULL && worn->isWeapon() != NULL)
                    slot.occupant = worn;
                if (sameType == NULL && SameKind(worn, hovered))
                {
                    sameType = worn;
                    sameTypeSlot = weaponSection ? label : out.slot;
                }
            }

            if (weaponSection)
                weaponSlots.push_back(slot);
        }

        if (sameType != NULL)
        {
            out.worn = sameType;
            out.slot = sameTypeSlot;
            return out;
        }

        if (!weapon)
            return out;

        // Слоты по порядку инвентаря: Оружие I, потом II.
        const WeaponSlot* freeSlot = NULL;
        const WeaponSlot* takenSlot = NULL;
        for (int number = 1; number <= 8; ++number)
        {
            for (size_t k = 0; k < weaponSlots.size(); ++k)
            {
                const WeaponSlot& s = weaponSlots[k];
                if (s.number != number || !s.fits)
                    continue;
                if (s.occupant == NULL && freeSlot == NULL)
                    freeSlot = &s;
                if (s.occupant != NULL && takenSlot == NULL)
                    takenSlot = &s;
            }
        }

        // Свободный слот есть, но в соседнем висит оружие - сравниваем с ним
        // (просьба игрока, Steam 05.10): игра надела бы вещь в свободный
        // слот, но игроку нужно «лучше ли это моего оружия». Раньше в этом
        // случае шло сравнение с «ничем», а его панель по умолчанию не
        // показывается - выглядело как «не работает, пока второй слот пуст».
        // Соседний - первый занятый по порядку, даже если вещь туда не встаёт.
        const WeaponSlot* neighbour = takenSlot;
        for (int number = 1; number <= 8 && neighbour == NULL; ++number)
            for (size_t k = 0; k < weaponSlots.size() && neighbour == NULL; ++k)
                if (weaponSlots[k].number == number && weaponSlots[k].occupant != NULL)
                    neighbour = &weaponSlots[k];

        if (neighbour != NULL)
        {
            out.worn = neighbour->occupant;
            out.slot = WeaponSlotName(neighbour->number);
        }
        else if (freeSlot != NULL)
            out.slot = WeaponSlotName(freeSlot->number);
        else
            out.noSlot = true;

        return out;
    }


    // ---------------------------------------------------------------
    // Сравнение
    // ---------------------------------------------------------------

    struct Line
    {
        std::string label;
        std::string worn;
        std::string next;
        std::string delta;
        int verdict;     // +1 лучше, -1 хуже, 0 нейтрально
    };


    // Проценты игра не округляет, а отбрасывает дробную часть: 0.205
    // у неё 20%, не 21%. Делаем так же, иначе числа на панели на единицу
    // расходятся с окошком. Разницу считаем между уже показанными
    // значениями - чтобы «13% -> 21%» давало ровно «+8».
    int Shown(Kind kind, float value)
    {
        switch (kind)
        {
        case PERCENT: return static_cast<int>(floor(value * 100.0f + 0.0001f));
        case MULT:    return static_cast<int>(floor(value * 100.0f + 0.5f));
        default:      return static_cast<int>(floor(value + 0.5f));
        }
    }


    std::string Format(Kind kind, float value, bool present)
    {
        if (!present)
            return "-";

        char text[64];   // запас под любое float: %.2f от 1e38 - 42 знака
        switch (kind)
        {
        case PERCENT: sprintf_s(text, "%d%%", Shown(kind, value)); break;
        case MULT:    sprintf_s(text, "x%.2f", value); break;
        case POINTS:  sprintf_s(text, "%+d", Shown(kind, value)); break;
        case KG:      sprintf_s(text, "%.1f", value); break;
        case MONEY:   sprintf_s(text, "%d", Shown(kind, value)); break;
        default:      sprintf_s(text, "%.2f", value); break;
        }
        return text;
    }


    std::string Signed(float value, const char* unit, int decimals);


    std::string FormatDelta(Kind kind, float before, float after)
    {
        char text[64];   // запас под любое float: %.2f от 1e38 - 42 знака
        switch (kind)
        {
        case PERCENT:
        case MULT:
        case POINTS:
        case MONEY:
        {
            const int d = Shown(kind, after) - Shown(kind, before);
            sprintf_s(text, kind == MULT ? "%+d%%" : "%+d", d);
            break;
        }
        case KG:      return Signed(after - before, "", 1);
        default:      sprintf_s(text, "%+.2f", after - before); break;
        }
        return text;
    }


    struct Comparison
    {
        std::vector<Line> lines;
        int better;          // свойств лучше
        int worse;           // свойств хуже
        float mainPercent;   // главная ось, % к надетому
        bool mainNew;        // у надетого главная ось нулевая
        float weightDelta;   // кг
    };


    void Compare(const Summary& worn, const Summary& next, Comparison& out)
    {
        out.lines.clear();
        out.better = out.worse = 0;

        // Порядок строк - как у вещи под курсором, затем те, что есть
        // только у надетой.
        std::vector<const Property*> order;
        std::set<std::string> seen;
        for (size_t i = 0; i < next.rows.size(); ++i)
        {
            order.push_back(&next.rows[i]);
            seen.insert(next.rows[i].label);
        }
        for (size_t i = 0; i < worn.rows.size(); ++i)
            if (seen.find(worn.rows[i].label) == seen.end())
                order.push_back(&worn.rows[i]);

        for (size_t i = 0; i < order.size(); ++i)
        {
            const Property& p = *order[i];

            const Property* a = NULL;
            const Property* b = NULL;
            for (size_t k = 0; k < worn.rows.size(); ++k)
                if (worn.rows[k].label == p.label) a = &worn.rows[k];
            for (size_t k = 0; k < next.rows.size(); ++k)
                if (next.rows[k].label == p.label) b = &next.rows[k];

            const bool aHas = a != NULL && a->present;
            const bool bHas = b != NULL && b->present;
            if (!aHas && !bHas)
                continue;

            const float av = aHas ? a->value : Neutral(p.kind);
            const float bv = bHas ? b->value : Neutral(p.kind);
            const float d = bv - av;
            if (std::fabs(d) < 0.0001f)
                continue;   // одинаковые не показываем

            Line line;
            line.label = p.label;
            line.worn = Format(p.kind, av, aHas);
            line.next = Format(p.kind, bv, bHas);
            line.delta = FormatDelta(p.kind, av, bv);
            line.verdict = p.better == 0 ? 0 : (d * p.better > 0 ? +1 : -1);

            // Вес считается отдельной осью, цена - для сведения.
            if (p.kind != KG && line.verdict > 0) ++out.better;
            if (p.kind != KG && line.verdict < 0) ++out.worse;

            out.lines.push_back(line);
        }

        out.mainNew = worn.main <= 0.0001f;
        out.mainPercent = out.mainNew
            ? 0.0f : (next.main - worn.main) / worn.main * 100.0f;
        out.weightDelta = next.weight - worn.weight;
    }


    // Вердикт: главная ось решает, свойства - если главная не сдвинулась
    // или тянут в ту же сторону; вес - только при равенстве остального.
    enum VerdictKind { V_WORSE = -1, V_EQUAL = 0, V_BETTER = 1, V_MIXED = 2 };

    VerdictKind Verdict(const Comparison& c, float mainPercent)
    {
        const int props = c.better - c.worse;
        const bool up = mainPercent > kSameThreshold;
        const bool down = mainPercent < -kSameThreshold;

        if (up && props >= 0) return V_BETTER;
        if (down && props <= 0) return V_WORSE;
        if (up || down) return V_MIXED;
        if (props > 0) return V_BETTER;
        if (props < 0) return V_WORSE;
        if (c.weightDelta > 0.5f) return V_WORSE;
        if (c.weightDelta < -0.5f) return V_BETTER;
        return V_EQUAL;
    }


    const char* VerdictText(VerdictKind v)
    {
        switch (v)
        {
        case V_BETTER: return Tr("better");
        case V_WORSE:  return Tr("worse");
        case V_MIXED:  return Tr("mixed");
        default:       return Tr("equal");
        }
    }


    // ---------------------------------------------------------------
    // Панель
    // ---------------------------------------------------------------

    // Вёрстка. Первая версия держала ширины колонок числами, и «станет»
    // наезжало на «разница», а «+8» обрезалось у края; рамка была чужим
    // скином, шрифт - мельче игрового. Теперь:
    //   * рамка - Kenshi_FloatingPanelSkin, как у всплывающей панели игры;
    //   * шрифт и высота строки - как у надписи внутри самого игрового
    //     окошка (ProbeTooltipStyle), так они совпадут при любом моде на
    //     интерфейс;
    //   * ширина каждой колонки - по самому широкому тексту в ней.

    const int kPadX = 14;
    const int kPadY = 10;
    const int kGap = 14;

    struct RowSpec
    {
        bool wide;                  // одна надпись на всю ширину
        std::string text[4];
        MyGUI::Colour colour[4];
    };

    struct RowWidgets
    {
        MyGUI::TextBox* cells[4];
    };

    MyGUI::Widget* g_panel = NULL;
    std::vector<RowWidgets> g_rows;
    std::vector<RowSpec> g_specs;
    int g_panelWidth = 0;
    int g_panelHeight = 0;

    std::string g_font;             // пусто - шрифт скина надписи
    int g_rowHeight = 20;
    bool g_fontProbed = false;

    std::string g_slotLabel;        // слот для заголовка панели

    // Цвета по ролям - как в игровом окошке вещи. Асур справедливо заметил,
    // что прежние были не игровые: одна «Main» из палитры на всё, а строки
    // осей целиком зелёные или красные. Теперь каждую роль берём у самого
    // окошка (ProbeTooltipStyle), палитра - только запасной вариант, пока
    // окошко не показано ни разу.
    MyGUI::Colour g_title;      // название вещи - первая строка окошка
    MyGUI::Colour g_header;     // [Покрытие], [Навыки]
    MyGUI::Colour g_label;      // -Сопр. дроблению
    MyGUI::Colour g_value;      // числа справа
    MyGUI::Colour g_better;     // число «лучше»
    MyGUI::Colour g_worse;      // число «хуже»
    bool g_coloursReady = false;
    bool g_styleProbed = false;       // основные роли найдены
    bool g_betterProbed = false;      // цвет «лучше» найден в окошке
    bool g_worseProbed = false;


    MyGUI::Colour FromHex(const std::string& hex, const MyGUI::Colour& fallback)
    {
        return GameTheme::Parse(hex.c_str(), fallback);
    }


    void EnsureColours()
    {
        if (g_coloursReady)
            return;
        g_coloursReady = true;

        const MyGUI::Colour plain = GameTheme::ReadableColour("Main", "#AFA68B");
        g_title = g_header = g_label = g_value = plain;
        g_better = MyGUI::Colour(0.49f, 0.79f, 0.42f);
        g_worse = MyGUI::Colour(0.88f, 0.42f, 0.35f);

        // Свой цвет из ini - сильнее игрового.
        if (g_colourBetter != "auto")
        {
            g_better = FromHex(g_colourBetter, g_better);
            g_betterProbed = true;
        }
        if (g_colourWorse != "auto")
        {
            g_worse = FromHex(g_colourWorse, g_worse);
            g_worseProbed = true;
        }
    }


    // Первая непустая надпись внутри игрового окошка.
    MyGUI::TextBox* FirstText(MyGUI::Widget* widget, int depth)
    {
        if (widget == NULL || depth > 6)
            return NULL;

        MyGUI::TextBox* const box = widget->castType<MyGUI::TextBox>(false);
        if (box != NULL && !box->getCaption().empty())
            return box;

        const size_t count = widget->getChildCount();
        for (size_t i = 0; i < count; ++i)
        {
            MyGUI::TextBox* const found = FirstText(widget->getChildAt(i), depth + 1);
            if (found != NULL)
                return found;
        }
        return NULL;
    }


    void ApplyFont(MyGUI::TextBox* box)
    {
        if (!g_font.empty())
            box->setFontName(g_font);
    }


    // Надписи окошка: одиночные (название, [Покрытие]) и пары «подпись -
    // значение» - игра кладёт их двумя надписями в один виджет строки
    // (ToolTipLine: слева подпись, справа значение).
    struct TextSample
    {
        std::string caption;
        MyGUI::Colour colour;
        bool isValue;          // правая надпись пары
        std::string pairLabel; // для значения - подпись слева от него
    };


    unsigned int Pack(const MyGUI::Colour& c)
    {
        return (static_cast<unsigned int>(c.red * 255.0f + 0.5f) << 16) |
               (static_cast<unsigned int>(c.green * 255.0f + 0.5f) << 8) |
                static_cast<unsigned int>(c.blue * 255.0f + 0.5f);
    }


    // Цвет как «#RRGGBB» - для журнала.
    std::string HexColour(const MyGUI::Colour& c)
    {
        char text[16];
        sprintf_s(text, "#%06X", Pack(c));
        return text;
    }


    int HexDigit(char c)
    {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    }


    // Тег цвета MyGUI в позиции i: «#RRGGBB». «##» - это экранированная
    // решётка, не тег.
    bool TagAt(const std::string& text, size_t i, MyGUI::Colour* colour)
    {
        if (i + 7 > text.size() || text[i] != '#' || text[i + 1] == '#')
            return false;

        int v[6];
        for (int k = 0; k < 6; ++k)
        {
            v[k] = HexDigit(text[i + 1 + k]);
            if (v[k] < 0)
                return false;
        }

        if (colour != NULL)
            *colour = MyGUI::Colour((v[0] * 16 + v[1]) / 255.0f,
                                    (v[2] * 16 + v[3]) / 255.0f,
                                    (v[4] * 16 + v[5]) / 255.0f);
        return true;
    }


    // Текст без тегов цвета - чтобы узнавать роль по первому знаку.
    std::string StripTags(const std::string& text)
    {
        std::string out;
        for (size_t i = 0; i < text.size(); )
        {
            if (text[i] == '#' && i + 1 < text.size() && text[i + 1] == '#')
            {
                out += '#';
                i += 2;
            }
            else if (TagAt(text, i, NULL))
                i += 7;
            else
                out += text[i++];
        }
        return out;
    }


    // Цвет, которым надпись видна на экране. Игра красит строки окошка не
    // свойством цвета, а тегом прямо в тексте («#E0C080Штаны с латами»);
    // getTextColour при этом отдаёт цвет скина по умолчанию - тёмный, как
    // фон. Первая версия брала его, и название со значениями вышли почти
    // невидимыми. Поэтому: есть тег в начале - цвет по нему.
    //
    // Тег не всегда в начале: у цены он после «к.» («к.#7676761 425»).
    // Поэтому - первый тег в тексте.
    MyGUI::Colour ShownColour(MyGUI::TextBox* box, const std::string& raw)
    {
        MyGUI::Colour colour;
        for (size_t i = 0; i < raw.size(); ++i)
        {
            if (raw[i] == '#' && i + 1 < raw.size() && raw[i + 1] == '#')
            {
                ++i;   // «##» - экранированная решётка
                continue;
            }
            if (TagAt(raw, i, &colour))
                return colour;
        }
        return box->getTextColour();
    }


    void CollectTexts(MyGUI::Widget* widget, int depth, std::vector<TextSample>& out)
    {
        if (widget == NULL || depth > 8)
            return;

        // Надписи-дети этого виджета по порядку.
        std::vector<MyGUI::TextBox*> boxes;
        const size_t count = widget->getChildCount();
        for (size_t i = 0; i < count; ++i)
        {
            MyGUI::TextBox* const box = widget->getChildAt(i)->castType<MyGUI::TextBox>(false);
            if (box != NULL && !box->getCaption().empty() && box->getVisible())
                boxes.push_back(box);
        }

        for (size_t i = 0; i < boxes.size(); ++i)
        {
            const std::string raw = boxes[i]->getCaption();
            TextSample s;
            s.caption = StripTags(raw);
            s.colour = ShownColour(boxes[i], raw);
            s.isValue = boxes.size() >= 2 && i == boxes.size() - 1;
            if (s.isValue)
                s.pairLabel = StripTags(boxes[0]->getCaption());
            out.push_back(s);

            // Сырые надписи окошка - в журнал, один раз: по ним видно,
            // откуда какой цвет взят.
            //
            // Только строкой, без буфера: описание вещи - абзац русского
            // текста, в UTF-8 длиннее любого разумного буфера, а sprintf_s
            // при переполнении не обрезает, а завершает процесс. Так игра
            // и упала на «Защите сердца» при Debug=1 (27.09.2026).
            static int dumped = 0;
            if (g_debug && dumped < 40)
            {
                ++dumped;
                DebugLog("GearCompare: tooltip text " + HexColour(s.colour) +
                         " set " + HexColour(boxes[i]->getTextColour()) +
                         (s.isValue ? " value '" : " text '") + raw + "'");
            }
        }

        for (size_t i = 0; i < count; ++i)
            CollectTexts(widget->getChildAt(i), depth + 1, out);
    }


    // Самый частый цвет среди надписей, подошедших под условие.
    bool MostCommon(const std::vector<TextSample>& texts, bool (*pick)(const TextSample&),
                    MyGUI::Colour& out)
    {
        std::map<unsigned int, int> counts;
        std::map<unsigned int, MyGUI::Colour> colours;
        for (size_t i = 0; i < texts.size(); ++i)
        {
            if (!pick(texts[i]))
                continue;
            const unsigned int key = Pack(texts[i].colour);
            ++counts[key];
            colours[key] = texts[i].colour;
        }

        int best = 0;
        for (std::map<unsigned int, int>::const_iterator it = counts.begin();
             it != counts.end(); ++it)
        {
            if (it->second > best)
            {
                best = it->second;
                out = colours[it->first];
            }
        }
        return best > 0;
    }


    bool IsHeader(const TextSample& s) { return !s.isValue && !s.caption.empty() && s.caption[0] == '['; }
    bool IsLabel(const TextSample& s)  { return !s.isValue && !s.caption.empty() && s.caption[0] == '-'; }
    bool IsValue(const TextSample& s)  { return s.isValue; }


    // Шрифт, высота строки и цвета - у самого игрового окошка вещи.
    // Шрифт и основные цвета - один раз; «лучше» и «хуже» - как только
    // попадётся вещь с такими числами (у простых штанов их нет).
    // Возвращает true, если что-то узнали и панель стоит перестроить.
    bool ProbeTooltipStyle(MyGUI::Widget* tipPanel)
    {
        if (g_fontProbed && g_styleProbed && g_betterProbed && g_worseProbed)
            return false;

        std::vector<TextSample> texts;
        CollectTexts(tipPanel, 0, texts);
        if (texts.empty())
            return false;   // содержимое ещё не собрано - попробуем в следующий раз

        EnsureColours();
        bool changed = false;

        if (!g_fontProbed)
        {
            MyGUI::TextBox* const sample = FirstText(tipPanel, 0);
            if (sample != NULL)
            {
                g_fontProbed = true;
                g_font = sample->getFontName();

                int height = sample->getHeight();
                if (height < sample->getFontHeight() + 2)
                    height = sample->getFontHeight() + 2;
                if (height < 14) height = 14;
                if (height > 40) height = 40;
                g_rowHeight = height;

                for (size_t i = 0; i < g_rows.size(); ++i)
                    for (int c = 0; c < 4; ++c)
                        ApplyFont(g_rows[i].cells[c]);
                changed = true;
            }
        }

        if (!g_styleProbed)
        {
            g_styleProbed = true;

            // Почти чёрный цвет на тёмной рамке не читается - значит, взяли
            // не тот (цвет скина, а надпись красится как-то иначе). Тогда
            // для этой роли остаётся читаемый запасной.
            const MyGUI::Colour fallback = g_label;
            MyGUI::Colour found = texts[0].colour;   // первая строка - название
            g_title = GameTheme::Luma(found) >= 0.3f ? found : fallback;
            if (MostCommon(texts, &IsHeader, found) && GameTheme::Luma(found) >= 0.3f)
                g_header = found;
            if (MostCommon(texts, &IsLabel, found) && GameTheme::Luma(found) >= 0.3f)
                g_label = found;
            if (MostCommon(texts, &IsValue, found) && GameTheme::Luma(found) >= 0.3f)
                g_value = found;
            changed = true;
        }

        // «Лучше» и «хуже» - числа со знаком, окрашенные не как обычное
        // значение: «+2» зелёным, «-1» красным.
        for (size_t i = 0; i < texts.size(); ++i)
        {
            const TextSample& s = texts[i];
            if (!s.isValue || s.caption.empty() || Pack(s.colour) == Pack(g_value))
                continue;
            if (s.caption[0] == '+' && !g_betterProbed)
            {
                g_better = s.colour;
                g_betterProbed = true;
                changed = true;
            }
            if (s.caption[0] == '-' && !g_worseProbed)
            {
                g_worse = s.colour;
                g_worseProbed = true;
                changed = true;
            }
        }

        if (changed && g_debug)
        {
            char note[512];
            _snprintf_s(note, _TRUNCATE,
                "GearCompare: tooltip style - font '%s', row %d, title #%06X, header #%06X, "
                "label #%06X, value #%06X, better #%06X%s, worse #%06X%s",
                g_font.c_str(), g_rowHeight, Pack(g_title), Pack(g_header),
                Pack(g_label), Pack(g_value),
                Pack(g_better), g_betterProbed ? "" : " (fallback)",
                Pack(g_worse), g_worseProbed ? "" : " (fallback)");
            DebugLog(note);
        }

        return changed;
    }


    void EnsurePanel()
    {
        if (g_panel != NULL)
            return;

        g_panel = MyGUI::Gui::getInstance().createWidget<MyGUI::Widget>(
            "Kenshi_FloatingPanelSkin",
            MyGUI::IntCoord(0, 0, 100, 100),
            MyGUI::Align::Default, "ToolTip", "GearCompare_Panel");
        g_panel->setNeedMouseFocus(false);
        g_panel->setNeedKeyFocus(false);
        g_panel->setVisible(false);
    }


    MyGUI::TextBox* MakeCell()
    {
        MyGUI::TextBox* const box = g_panel->createWidget<MyGUI::TextBox>(
            "Kenshi_TextboxStandardText", MyGUI::IntCoord(0, 0, 10, g_rowHeight),
            MyGUI::Align::Default, "");
        box->setNeedMouseFocus(false);
        box->setNeedKeyFocus(false);
        ApplyFont(box);
        return box;
    }


    RowWidgets& Row(size_t index)
    {
        while (g_rows.size() <= index)
        {
            RowWidgets row;
            for (int c = 0; c < 4; ++c)
                row.cells[c] = MakeCell();
            g_rows.push_back(row);
        }
        return g_rows[index];
    }


    // Строка как у игры: подпись цветом подписи, значения цветом значения,
    // выделено цветом только то, что лучше или хуже.
    void AddRow(const std::string& a, const std::string& b, const std::string& c,
                const std::string& d, const MyGUI::Colour& deltaColour)
    {
        RowSpec spec;
        spec.wide = false;
        spec.text[0] = a; spec.text[1] = b; spec.text[2] = c; spec.text[3] = d;
        spec.colour[0] = g_label;
        spec.colour[1] = spec.colour[2] = g_value;
        spec.colour[3] = deltaColour;
        g_specs.push_back(spec);
    }


    // Заголовки колонок - цветом подзаголовков окошка ([Покрытие]).
    void AddColumnTitles()
    {
        RowSpec spec;
        spec.wide = false;
        spec.text[0] = "";
        spec.text[1] = Tr("worn");
        spec.text[2] = Tr("new");
        spec.text[3] = Tr("change");
        spec.colour[0] = spec.colour[1] = spec.colour[2] = spec.colour[3] = g_header;
        g_specs.push_back(spec);
    }


    // Итоговая строка: подпись слева, значение в последней колонке.
    void AddSummary(const std::string& label, const std::string& value,
                    const MyGUI::Colour& valueColour)
    {
        AddRow(label, "", "", value, valueColour);
    }


    void AddWide(const std::string& text, const MyGUI::Colour& colour)
    {
        RowSpec spec;
        spec.wide = true;
        spec.text[0] = text;
        spec.colour[0] = colour;
        g_specs.push_back(spec);
    }


    int TextWidth(MyGUI::TextBox* box, const std::string& text)
    {
        box->setCaption(text);
        return box->getTextSize().width;
    }


    // Расставляет надписи по g_specs и считает размер панели.
    void Layout()
    {
        int column[4] = { 0, 0, 0, 0 };
        int wideWidth = 0;

        for (size_t i = 0; i < g_specs.size(); ++i)
        {
            RowWidgets& row = Row(i);
            const RowSpec& spec = g_specs[i];

            if (spec.wide)
            {
                const int w = TextWidth(row.cells[0], spec.text[0]);
                if (w > wideWidth) wideWidth = w;
                continue;
            }

            for (int c = 0; c < 4; ++c)
            {
                const int w = TextWidth(row.cells[c], spec.text[c]);
                if (w > column[c]) column[c] = w;
            }
        }

        int inner = column[0] + column[1] + column[2] + column[3] + kGap * 3;
        if (inner < wideWidth)
            inner = wideWidth;

        int y = kPadY;
        for (size_t i = 0; i < g_specs.size(); ++i)
        {
            RowWidgets& row = g_rows[i];
            const RowSpec& spec = g_specs[i];

            if (spec.wide)
            {
                MyGUI::TextBox* const box = row.cells[0];
                box->setCoord(kPadX, y, inner, g_rowHeight);
                box->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
                box->setTextColour(spec.colour[0]);
                box->setVisible(true);
                for (int c = 1; c < 4; ++c)
                    row.cells[c]->setVisible(false);
            }
            else
            {
                int x = kPadX;
                for (int c = 0; c < 4; ++c)
                {
                    MyGUI::TextBox* const box = row.cells[c];
                    box->setCoord(x, y, column[c], g_rowHeight);
                    box->setTextAlign((c == 0 ? MyGUI::Align::Left : MyGUI::Align::Right)
                                      | MyGUI::Align::VCenter);
                    box->setTextColour(spec.colour[c]);
                    box->setVisible(true);
                    x += column[c] + kGap;
                }
            }

            y += g_rowHeight;
        }

        for (size_t i = g_specs.size(); i < g_rows.size(); ++i)
            for (int c = 0; c < 4; ++c)
                g_rows[i].cells[c]->setVisible(false);

        g_panelWidth = inner + kPadX * 2;
        g_panelHeight = y + kPadY;
    }


    MyGUI::Colour VerdictColour(int v)
    {
        return v > 0 ? g_better : (v < 0 ? g_worse : g_value);
    }


    // Число со знаком без «-0»: изменение меньше половины деления - ноль.
    std::string Signed(float value, const char* unit, int decimals)
    {
        const float step = decimals == 0 ? 0.5f : 0.05f;
        if (std::fabs(value) < step)
            value = 0.0f;

        char text[64];
        sprintf_s(text, decimals == 0 ? "%+.0f%s" : "%+.1f%s", value, unit);
        return text;
    }


    // Собирает строки панели и раскладывает их. Возвращает число строк.
    int Fill(const Summary& next, const Summary* worn, const Comparison* cmp,
             const char* notice)
    {
        EnsureColours();
        g_specs.clear();

        // Заголовок панели - цветом названия вещи в окошке.
        AddWide(worn != NULL
                    ? std::string(Tr("Worn")) + " (" + g_slotLabel + "): " + worn->name
                    : std::string(notice), g_title);

        if (worn != NULL && cmp != NULL)
        {
            AddColumnTitles();

            for (size_t i = 0; i < cmp->lines.size(); ++i)
            {
                const Line& l = cmp->lines[i];
                AddRow(l.label, l.worn, l.next, l.delta, VerdictColour(l.verdict));
            }

            if (cmp->lines.empty())
                AddWide(Tr("No difference"), g_label);

            // Оси - как строки окошка: подпись обычная, значение с цветом.
            const std::string mainName = !next.mainLabel.empty() ? next.mainLabel
                                       : std::string(next.isArmour ? Tr("Protection") : Tr("Damage"));
            std::string mainValue;
            int mainSign = 0;
            if (cmp->mainNew)
            {
                mainValue = next.main > 0.0001f ? Tr("new") : "-";
                mainSign = next.main > 0.0001f ? +1 : 0;
            }
            else
            {
                mainValue = Signed(cmp->mainPercent, "%", 0);
                mainSign = cmp->mainPercent > kSameThreshold ? +1
                         : (cmp->mainPercent < -kSameThreshold ? -1 : 0);
            }
            AddSummary(mainName, mainValue, VerdictColour(mainSign));

            // Счёт строк, а не разница: «+5 / -4» читали как ещё одно число.
            // Теперь словами - «5 лучше / 4 хуже», каждое своим цветом
            // (тегами цвета MyGUI внутри одной ячейки).
            char better[32], worse[32];
            sprintf_s(better, "%d ", cmp->better);
            sprintf_s(worse, "%d ", cmp->worse);
            const std::string props = HexColour(g_better) + better + Tr("better")
                                    + HexColour(g_label) + " / "
                                    + HexColour(g_worse) + worse + Tr("worse");
            AddSummary(Tr("Properties"), props, g_value);

            AddSummary(Tr("Weight, kg"), Signed(cmp->weightDelta, "", 1),
                       VerdictColour(cmp->weightDelta > 0.05f ? -1
                                   : (cmp->weightDelta < -0.05f ? +1 : 0)));

            const float mainForVerdict = cmp->mainNew
                ? (next.main > 0.0001f ? 100.0f : 0.0f) : cmp->mainPercent;

            if (g_showTotal)
            {
                const float total = mainForVerdict
                    + g_totalPerProperty * (cmp->better - cmp->worse)
                    - g_totalPerKg * cmp->weightDelta;
                AddSummary(Tr("Total"), Signed(total, "%", 0),
                           VerdictColour(total > kSameThreshold ? +1
                                       : (total < -kSameThreshold ? -1 : 0)));
            }

            const VerdictKind verdict = Verdict(*cmp, mainForVerdict);
            AddSummary(Tr("Verdict"), VerdictText(verdict),
                       VerdictColour(verdict == V_MIXED ? 0 : static_cast<int>(verdict)));
        }

        Layout();
        return static_cast<int>(g_specs.size());
    }


    void Place(MyGUI::Widget* tipPanel)
    {
        const MyGUI::IntCoord tip = tipPanel->getAbsoluteCoord();
        const MyGUI::IntSize view = MyGUI::RenderManager::getInstance().getViewSize();

        int x = tip.left + tip.width + 4;
        if (x + g_panelWidth > view.width)
            x = tip.left - 4 - g_panelWidth;
        if (x < 0)
            x = 0;

        int y = tip.top;
        if (y + g_panelHeight > view.height)
            y = view.height - g_panelHeight;
        if (y < 0)
            y = 0;

        g_panel->setCoord(x, y, g_panelWidth, g_panelHeight);
    }


    // ---------------------------------------------------------------
    // Что показывает всплывающее окно
    // ---------------------------------------------------------------

    // Окошко описания, которое сейчас показывает снаряжение, и сама вещь.
    //
    // Первая версия ловила ToolTip::showGameData(hand) - и в игре не
    // сработала ни разу: окошко инвентаря этот путь не использует. Оно
    // собирает содержимое в ToolTipInventory::setContent и спрашивает
    // строки у самой вещи - Armour::getTooltipData1 и родня. Там this и
    // есть вещь под курсором. Поэтому: на время setContent поднимаем флаг,
    // и если внутри него позвали getTooltipData1 снаряжения - это она.
    // Не позвали (еда, деньги, чертёж) - вещи нет, панели нет.
    ToolTip* g_tip = NULL;
    ToolTip* g_building = NULL;   // setContent идёт прямо сейчас

    hand& ShownItem()
    {
        static hand h;
        return h;
    }

    // Кеш: перестраиваем панель только когда сменилась пара вещей.
    Item* g_cachedNext = NULL;
    Item* g_cachedWorn = NULL;
    Item* g_probedFor = NULL;     // для какой вещи уже разбирали окошко
    Character* g_cachedOwner = NULL;
    int g_cachedRows = 0;


    void NoteItem(Item* item)
    {
        if (g_building == NULL || item == NULL)
            return;
        g_tip = g_building;
        ShownItem() = item->getHandle();
    }


    typedef void (*TipDataFn)(Item* self, void* lines);

    TipDataFn g_origArmourTip = NULL;
    TipDataFn g_origLockedTip = NULL;
    TipDataFn g_origSwordTip = NULL;
    TipDataFn g_origCrossbowTip = NULL;

    void ArmourTip_hook(Item* self, void* lines)   { g_origArmourTip(self, lines);   NoteItem(self); }
    void LockedTip_hook(Item* self, void* lines)   { g_origLockedTip(self, lines);   NoteItem(self); }
    void SwordTip_hook(Item* self, void* lines)    { g_origSwordTip(self, lines);    NoteItem(self); }
    void CrossbowTip_hook(Item* self, void* lines) { g_origCrossbowTip(self, lines); NoteItem(self); }


    // Окошко удаляют (перезагрузка интерфейса, загрузка сохранения) -
    // забываем его. Иначе следующий кадр разыменовал бы g_tip в уже
    // освобождённой памяти: Update спрашивает у него getVisible и panel.
    void (*g_origTipDestructor)(ToolTip* self) = NULL;

    void TipDestructor_hook(ToolTip* self)
    {
        if (self == g_tip)
        {
            g_tip = NULL;
            ShownItem().setNull();
        }
        if (self == g_building)
            g_building = NULL;
        g_origTipDestructor(self);
    }


    void (*g_origSetContent)(ToolTip* self, MyGUI::Widget* widget) = NULL;

    void SetContent_hook(ToolTip* self, MyGUI::Widget* widget)
    {
        // Новое содержимое: прежняя вещь этого окошка больше не его.
        if (self == g_tip)
            ShownItem().setNull();

        ToolTip* const outer = g_building;
        g_building = self;
        g_origSetContent(self, widget);
        g_building = outer;

        if (g_debug)
        {
            static ToolTip* lastTip = NULL;
            static Item* lastItem = NULL;
            Item* const item = self == g_tip ? ShownItem().getItem() : NULL;
            if (self != lastTip || item != lastItem)
            {
                lastTip = self;
                lastItem = item;
                char tip[32];
                sprintf_s(tip, "%p", static_cast<void*>(self));
                DebugLog(std::string("GearCompare: tooltip ") + tip + " rebuilt, gear: " +
                         (item != NULL ? item->displayName : std::string("none")));
            }
        }
    }


    void Hide()
    {
        if (g_panel != NULL && g_panel->getVisible())
            g_panel->setVisible(false);
        g_cachedNext = g_cachedWorn = NULL;
        g_cachedOwner = NULL;
        g_probedFor = NULL;
    }


    void LogPair(Character* owner, Item* next, Item* worn, bool wornItself)
    {
        if (!g_debug)
            return;

        std::string line = "GearCompare: hovered '" + next->displayName + "'";
        line += owner != NULL ? " for " + owner->displayName : " for nobody";
        line += wornItself ? " (it is worn)" :
                (worn != NULL ? ", worn '" + worn->displayName + "'" : ", nothing of that kind worn");

        if (Armour* const a = next->isArmour())
        {
            char raw[160];
            _snprintf_s(raw, _TRUNCATE, "; raw cut=%.4f blunt=%.4f pierce=%.4f parts=%u slot=%d",
                      a->cutResistance, a->bluntResistance, a->pierceResistance,
                      static_cast<unsigned>(a->bodypartCoverage.size()),
                      static_cast<int>(next->slotType));
            line += raw;
        }
        else if (Weapon* const w = next->isWeapon())
        {
            char raw[160];
            Sword* const s = w->isSword();
            _snprintf_s(raw, _TRUNCATE, "; raw category=%d cut=%.4f blunt=%.4f bleed=%.4f",
                      static_cast<int>(w->getCategory()),
                      s ? s->cutDamage : 0.0f, s ? s->bluntDamage : 0.0f, w->bleedDamage);
            line += raw;
        }

        DebugLog(line);
    }


    // Клавиша считается зажатой, только когда окно игры впереди: иначе
    // ALT+TAB в другое окно показывал бы панель.
    bool HoldKeyDown()
    {
        return HoldKey::Held(g_holdKey);
    }


    void Update()
    {
        if (!HoldKeyDown())
        {
            Hide();
            return;
        }

        if (g_tip == NULL || !g_tip->getVisible())
        {
            Hide();
            return;
        }

        Item* const next = ShownItem().getItem();
        if (next == NULL)
        {
            Hide();
            return;
        }

        Character* owner = OwnerOf(next);
        if (owner == NULL && gui != NULL)
            owner = gui->getSelectedPlayerCharacter().getCharacter();
        if (owner == NULL)
        {
            Hide();
            return;
        }

        const Counterpart pair = FindCounterpart(owner, next);
        const bool wornItself = pair.wornItself;
        Item* const worn = pair.worn;

        EnsurePanel();

        // Шрифт и цвета окошка узнаём, когда в нём уже есть надписи; как
        // только узнали что-то новое - панель перестраиваем. Разбираем раз
        // на новую вещь: пока цвета «лучше/хуже» не встретились, обход
        // всего окошка каждый кадр был бы пустой работой.
        if (next != g_probedFor)
        {
            g_probedFor = next;
            if (ProbeTooltipStyle(g_tip->panel))
                g_cachedNext = NULL;
        }

        if (next != g_cachedNext || worn != g_cachedWorn || owner != g_cachedOwner)
        {
            g_cachedNext = next;
            g_cachedWorn = worn;
            g_cachedOwner = owner;
            g_slotLabel = pair.slot;
            LogPair(owner, next, worn, wornItself);

            Summary nextSummary;
            Describe(next, nextSummary);

            if (wornItself)
            {
                g_cachedRows = 0;   // сравнивать не с чем - это она и есть
            }
            else if (worn == NULL && !g_compareEmpty)
            {
                g_cachedRows = 0;   // сравнивать не с чем - панель не нужна
            }
            else if (worn == NULL)
            {
                // Пусто в слоте - сравниваем с «ничем».
                Summary nothing;
                nothing.main = 0.0f;
                nothing.weight = 0.0f;
                nothing.isArmour = nextSummary.isArmour;
                nothing.name = pair.noSlot ? Tr("no slot for it") : Tr("nothing");

                Comparison cmp;
                Compare(nothing, nextSummary, cmp);
                g_cachedRows = Fill(nextSummary, &nothing, &cmp, "");
            }
            else
            {
                Summary wornSummary;
                Describe(worn, wornSummary);

                Comparison cmp;
                Compare(wornSummary, nextSummary, cmp);
                g_cachedRows = Fill(nextSummary, &wornSummary, &cmp, "");
            }
        }

        if (g_cachedRows == 0)
        {
            if (g_panel->getVisible())
                g_panel->setVisible(false);
            return;
        }

        Place(g_tip->panel);
        if (!g_panel->getVisible())
            g_panel->setVisible(true);
    }


    void (*g_origGuiUpdate)(ForgottenGUI* thisptr) = NULL;

    void GuiUpdate_hook(ForgottenGUI* thisptr)
    {
        g_origGuiUpdate(thisptr);
        Update();
    }
}


// Страница в ModConfigMenu (вкладка MCM в настройках игры), если он есть.
// Описание API - shared/ModConfigMenu.h. У каждой строки - подсказка.
extern "C" __declspec(dllexport) void MCM_Describe(MCM_Api* api)
{
    static const std::string ini = IniPath();
    api->beginMod(api, "GearCompare", "Gear Compare Alt", ini.c_str(), &LoadSettings);
    if (api->version >= 2)
        api->info(api, Tr("Compares the item under the cursor with what is worn: a panel next to the item description."));
    api->section(api, Tr("Panel"));
    api->toggle(api, "GearCompare", "Enabled", Tr("Enabled"), Tr("A panel with what is worn next to the item description."), 1, MCM_RESTART);
    api->hotkey(api, "GearCompare", "HoldKey", Tr("Show while held"), Tr("The panel is shown only while this key is held. Click and press a key; Backspace - no key, the panel is always shown."), "ALT", 0);
    api->toggle(api, "GearCompare", "CompareEmptySlot", Tr("Compare with an empty slot"), Tr("On - compare with an empty slot too, Worn: nothing. Off - no panel when nothing suitable is worn."), 0, 0);
    api->section(api, Tr("Total"));
    api->toggle(api, "GearCompare", "ShowTotal", Tr("Total percentage"), Tr("A total at the bottom of the panel: the main axis, plus a bonus for each better property, minus one for each worse property and for extra weight."), 0, 0);
    api->number(api, "GearCompare", "TotalPerProperty", Tr("Per property, %"), Tr("How many percent each property that got better adds to the total, and each one that got worse takes away."), 3.0f, 0.0f, 20.0f, 1, 0);
    api->number(api, "GearCompare", "TotalPerKg", Tr("Per extra kg, %"), Tr("How many percent each extra kilogram takes away from the total."), 2.0f, 0.0f, 20.0f, 1, 0);
    api->section(api, Tr("Colours"));
    api->colour(api, "GearCompare", "ColourBetter", Tr("Better"), Tr("Colour of what gets better. auto - like the +2 in the game's item tooltip, or #RRGGBB."), "auto", 0);
    api->colour(api, "GearCompare", "ColourWorse", Tr("Worse"), Tr("Colour of what gets worse. auto - like the -1 in the game's item tooltip, or #RRGGBB."), "auto", 0);
    api->section(api, Tr("Diagnostics"));
    api->toggle(api, "GearCompare", "Debug", Tr("Detailed log"), Tr("Details in RE_Kenshi_log.txt: the item under the cursor, for whom, what it is compared with and its raw numbers. Written only when the item changes."), 0, 0);
}


__declspec(dllexport) void startPlugin()
{
    LoadSettings();

    if (!g_enabled)
    {
        DebugLog("GearCompare: disabled in the ini");
        return;
    }

    // Удаление окошка, его содержимое и строки описания снаряжения. Все
    // шесть функций экспортированы; KEP тоже перехватывает getTooltipData1 у
    // клинков и арбалетов - мы зовём исходную первой, цепочка цела.
    struct HookSpec { const char* name; void* target; void* detour; void** original; };
    const HookSpec hooks[] =
    {
        { "ToolTipInventory::~ToolTipInventory",
          reinterpret_cast<void*>(KenshiLib::GetRealAddress(&ToolTipInventory::_DESTRUCTOR)),
          reinterpret_cast<void*>(&TipDestructor_hook),
          reinterpret_cast<void**>(&g_origTipDestructor) },
        { "ToolTipInventory::setContent",
          reinterpret_cast<void*>(KenshiLib::GetRealAddress(&ToolTipInventory::_NV_setContent)),
          reinterpret_cast<void*>(&SetContent_hook),
          reinterpret_cast<void**>(&g_origSetContent) },
        { "Armour::getTooltipData1",
          reinterpret_cast<void*>(KenshiLib::GetRealAddress(&Armour::_NV_getTooltipData1)),
          reinterpret_cast<void*>(&ArmourTip_hook),
          reinterpret_cast<void**>(&g_origArmourTip) },
        { "LockedArmour::getTooltipData1",
          reinterpret_cast<void*>(KenshiLib::GetRealAddress(&LockedArmour::_NV_getTooltipData1)),
          reinterpret_cast<void*>(&LockedTip_hook),
          reinterpret_cast<void**>(&g_origLockedTip) },
        { "Sword::getTooltipData1",
          reinterpret_cast<void*>(KenshiLib::GetRealAddress(&Sword::_NV_getTooltipData1)),
          reinterpret_cast<void*>(&SwordTip_hook),
          reinterpret_cast<void**>(&g_origSwordTip) },
        { "Crossbow::getTooltipData1",
          reinterpret_cast<void*>(KenshiLib::GetRealAddress(&Crossbow::_NV_getTooltipData1)),
          reinterpret_cast<void*>(&CrossbowTip_hook),
          reinterpret_cast<void**>(&g_origCrossbowTip) }
    };

    for (int i = 0; i < sizeof(hooks) / sizeof(hooks[0]); ++i)
    {
        if (KenshiLib::SUCCESS != KenshiLib::AddHook(
                hooks[i].target, hooks[i].detour, hooks[i].original))
        {
            ErrorLog(std::string("GearCompare: could not hook ") + hooks[i].name);
            return;
        }
    }

    if (KenshiLib::SUCCESS != KenshiLib::AddHook(
            reinterpret_cast<void*>(KenshiLib::GetRealAddress(&ForgottenGUI::update)),
            GuiUpdate_hook, &g_origGuiUpdate))
    {
        ErrorLog("GearCompare: could not hook the interface update");
        return;
    }

    DebugLog("GearCompare: installed");
}
