#pragma once

// Палитра игры: цвета обычных надписей берём у Kenshi, а не зашиваем.
//
// ЗАЧЕМ. Kenshi держит именованные цвета интерфейса в
// data/gui/colours/kenshi_colours.xml, и моды на интерфейс этот файл
// перекрывают. Плагин, который берёт цвета оттуда, меняет вид вместе с
// темой; зашитый - спорит с ней. Просьба Asur.
//
// ПОЧЕМУ ЧИТАЕМ ФАЙЛ, А НЕ СПРАШИВАЕМ MyGUI. Asur предложил брать
// значения у движка: палитра загружена в MyGUI::LanguageManager, и
// «#{Main}» разворачивается вызовом replaceTags. По сути это правильнее
// - файл уже разобран игрой, со всеми модами и порядком загрузки, - и я
// так и сделал. Но такой вызов **валит игру**, если приходит из нашего
// покадрового хука (GameWorld::_NV_mainLoop_GPUSensitiveStuff).
// Проверено трижды, с переносом во всё более поздние точки:
//
//   * из startPlugin - падение сразу, журнал обрывается на строке
//     загрузки плагина;
//   * из первого кадра - загрузка сейва доходит до «In-game» и падает;
//   * из создания первой строки полос - падает, как только персонаж
//     сдвинулся и пошёл опыт.
//
// При этом Character Inspector тот же вызов делает благополучно: он
// обращается к палитре из обработчика интерфейса, а не из игрового
// цикла. Похоже, что хук живёт в потоке отрисовки, а MyGUI к обращениям
// оттуда не готов. Разбираться дальше дороже, чем прочитать тот же файл
// самим: разбора здесь десяток строк, и он работает всегда.
//
// Если возвращаться к варианту Asur - звать его можно только из кода
// интерфейса, и это надо проверять отдельно.
//
// ПЕРЕКРЫТИЕ ищем как игра: идём по включённым модам в порядке mods.cfg
// и берём последний, у которого свой файл палитры есть.
//
// ЧТО ЕЩЁ ВАЖНО (от Asur). Если у скина виджета уже задан TextColour
// или Colour - не перекрывать его из кода вовсе: скиновый цвет самый
// совместимый. Лезть сюда стоит только когда виджету нужен именно
// СМЫСЛОВОЙ цвет интерфейса.
//
// Ключи: Main (основной текст), Secondary (тёмный второстепенный),
// Title (заголовочный; у части модов темнее обычного), Good, Bad,
// Special, Greyed.
//
// Использование:
//     #include <GameTheme.h>
//     box->setTextColour(GameTheme::ReadableColour("Main", "#AFA68B"));

#include <Windows.h>

#include <cstdio>
#include <fstream>
#include <map>
#include <string>

#include <mygui/MyGUI_Colour.h>

extern "C" IMAGE_DOS_HEADER __ImageBase;


namespace GameTheme
{
    inline bool ValidHex(const std::string& text)
    {
        if (text.size() != 7 || text[0] != '#')
            return false;

        for (int i = 1; i < 7; ++i)
        {
            const char c = text[i];
            const bool hex = (c >= '0' && c <= '9')
                          || (c >= 'a' && c <= 'f')
                          || (c >= 'A' && c <= 'F');
            if (!hex)
                return false;
        }

        return true;
    }


    // Корень игры ищем не отсчётом уровней, а по признаку: там лежит
    // data/mods.cfg. Плагин может стоять в папке любой глубины.
    inline std::string FindGameRoot()
    {
        char modulePath[MAX_PATH] = {};

        const DWORD length = GetModuleFileNameA(
            reinterpret_cast<HMODULE>(&__ImageBase), modulePath, MAX_PATH);

        if (length == 0 || length >= MAX_PATH)
            return std::string();

        std::string path(modulePath, length);

        for (int up = 0; up < 8; ++up)
        {
            const std::string::size_type slash = path.find_last_of("\\/");
            if (slash == std::string::npos)
                break;
            path.resize(slash);

            std::ifstream probe((path + "\\data\\mods.cfg").c_str());
            if (probe.is_open())
                return path + "\\";
        }

        return std::string();
    }


    inline void ParseFile(const std::string& file,
                          std::map<std::string, std::string>& out)
    {
        std::ifstream in(file.c_str());
        if (!in.is_open())
            return;

        std::string line;
        while (std::getline(in, line))
        {
            const std::string::size_type k = line.find("key=\"");
            if (k == std::string::npos)
                continue;
            const std::string::size_type ke = line.find('"', k + 5);
            if (ke == std::string::npos)
                continue;

            const std::string::size_type v = line.find("value=\"", ke);
            if (v == std::string::npos)
                continue;
            const std::string::size_type ve = line.find('"', v + 7);
            if (ve == std::string::npos)
                continue;

            const std::string key = line.substr(k + 5, ke - k - 5);
            const std::string value = line.substr(v + 7, ve - v - 7);

            if (!key.empty() && ValidHex(value))
                out[key] = value;
        }
    }


    inline bool& ModPaletteFlag();

    inline const std::map<std::string, std::string>& Table()
    {
        static std::map<std::string, std::string> table;
        static bool loaded = false;

        if (loaded)
            return table;
        loaded = true;

        const std::string root = FindGameRoot();
        if (root.empty())
            return table;

        const std::string suffix = "gui\\colours\\kenshi_colours.xml";
        std::string chosen = root + "data\\" + suffix;

        std::ifstream cfg((root + "data\\mods.cfg").c_str());
        if (cfg.is_open())
        {
            std::string line;
            while (std::getline(cfg, line))
            {
                while (!line.empty() &&
                       (line[line.size() - 1] == '\r' ||
                        line[line.size() - 1] == ' '))
                    line.resize(line.size() - 1);

                const std::string::size_type dot = line.rfind(".mod");
                if (dot == std::string::npos || dot + 4 != line.size())
                    continue;

                const std::string candidate =
                    root + "mods\\" + line.substr(0, dot) + "\\" + suffix;

                std::ifstream probe(candidate.c_str());
                if (probe.is_open())
                    chosen = candidate;     // побеждает последний
            }
        }

        ParseFile(chosen, table);
        ModPaletteFlag() = chosen != root + "data\\" + suffix;
        return table;
    }


    // Палитру перекрыл мод на интерфейс (true) или она ванильная (false).
    // У ванили окна - светлый пергамент, текст на них тёмный; у тёмных
    // модов (Russian Dark UI) - наоборот. QuickStartSelect 07.10.2026: на
    // ванили светлые надписи на пергаменте не читались.
    inline bool& ModPaletteFlag()
    {
        static bool fromMod = false;
        return fromMod;
    }

    inline bool PaletteFromMod()
    {
        Table();
        return ModPaletteFlag();
    }


    // Цвет по ключу в виде «#RRGGBB». Нет ключа - вернётся запасной.
    inline const char* Hex(const char* key, const char* fallback)
    {
        if (key == NULL)
            return fallback;

        const std::map<std::string, std::string>& table = Table();
        const std::map<std::string, std::string>::const_iterator it =
            table.find(key);

        return (it == table.end()) ? fallback : it->second.c_str();
    }


    inline MyGUI::Colour Parse(const char* hex, const MyGUI::Colour& other)
    {
        unsigned r = 0, g = 0, b = 0;

        if (hex == NULL || *hex != '#' ||
            sscanf_s(hex + 1, "%2x%2x%2x", &r, &g, &b) != 3)
            return other;

        return MyGUI::Colour(r / 255.0f, g / 255.0f, b / 255.0f);
    }


    inline MyGUI::Colour Colour(const char* key, const char* fallback)
    {
        return Parse(Hex(key, fallback), MyGUI::Colour::White);
    }


    // Яркость цвета по восприятию.
    inline float Luma(const MyGUI::Colour& c)
    {
        return 0.2126f * c.red + 0.7152f * c.green + 0.0722f * c.blue;
    }


    // Цвет темы, но не темнее читаемого.
    //
    // Следовать теме вслепую нельзя, когда надпись лежит на тёмной
    // панели. Своим окнам моды задают цвет скином, а ключи палитры при
    // этом бывают почти чёрными: у «Russian Dark UI» Main = #767676,
    // Greyed = #555555. Character Inspector один раз уже получил из-за
    // этого чёрные надписи.
    //
    // Обратный случай тоже есть: в ванили Title = #492620, тёмная охра,
    // а у того же мода Title = #b7a074, светлый. «Всегда светлого»
    // ключа не существует - об этом предупреждал и Asur.
    //
    // Порог подобран так, чтобы ванильный Main (#afa68b, 0.65) проходил,
    // а #767676 (0.46) - нет.
    inline MyGUI::Colour ReadableColour(const char* key, const char* fallback,
                                        float minLuma = 0.5f)
    {
        const MyGUI::Colour themed = Colour(key, fallback);

        if (Luma(themed) >= minLuma)
            return themed;

        return Parse(fallback, themed);
    }
}
