#pragma once
// Мультилокализация для плагинов Kenshi. Общий заголовок на все наши плагины.
//
// Механизм перенесён из мода PlayerFactionColor, автор Asur - он передал
// исходники как образец. Это самодельный gettext: плагин сам читает
// locale/<язык>/LC_MESSAGES/<домен>.mo или .po, а текущий язык берёт
// у самой игры через LocaleManager. Внешний libintl не нужен.
//
// Из его реализации взяты без изменений по существу: разбор двоичного .mo
// (сигнатура, порядок байт, таблицы смещений), разбор .po, поиск папки locale
// вглубь и вверх по дереву, таблица соответствий идентификаторов локали на
// девять языков, правило «en_GB грузим первым как опору».
//
// Главное свойство: Tr() при отсутствии перевода возвращает исходную
// английскую строку. Английский не теряется никогда - он и есть ключ словаря.
//
// ИСПОЛЬЗОВАНИЕ. Перед включением задать домен:
//
//     #define KLOC_DOMAIN "loot_all"
//     #include "Localization.h"
//
// Рядом с DLL положить locale/<язык>/LC_MESSAGES/<домен>.po
//
// ВРЕМЯ ЖИЗНИ. Tr() возвращает указатель внутрь словаря. Словарь строится
// один раз за сессию и не перестраивается, поэтому указатель живёт до
// выгрузки DLL - хранить его безопасно. Подробности у EnsureLoaded().
//
// ПОТОКИ. Только поток интерфейса. Ленивая инициализация статических
// переменных в VC10 не потокобезопасна, да и MyGUI трогать из других
// потоков всё равно нельзя.

#ifndef KLOC_DOMAIN
#error "Задайте #define KLOC_DOMAIN \"имя_домена\" перед включением Localization.h"
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#pragma warning(push)
#pragma warning(disable: 4091)
#include <kenshi/LocaleInfo.h>
#pragma warning(pop)

#include <Debug.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

namespace KLoc
{
    // Словарь и состояние держим в функциях-обёртках: заголовок включается
    // в несколько единиц трансляции, и обычные глобальные переменные дали бы
    // по копии на каждую.
    inline std::map<std::string, std::string>& Catalog()
    {
        static std::map<std::string, std::string> catalog;
        return catalog;
    }

    inline std::string& LoadedLocaleId()
    {
        static std::string id;
        return id;
    }

    inline bool& Initialised()
    {
        static bool done = false;
        return done;
    }

    // ---- пути ----

    inline std::string DllDirectory()
    {
        HMODULE module = NULL;
        if (!GetModuleHandleExA(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCSTR)&DllDirectory,
                &module))
        {
            return std::string();
        }

        char path[MAX_PATH] = { 0 };
        const DWORD length = GetModuleFileNameA(module, path, MAX_PATH);
        if (length == 0 || length >= MAX_PATH)
            return std::string();

        std::string full(path, length);
        const size_t slash = full.find_last_of("\\/");
        if (slash == std::string::npos)
            return std::string();
        return full.substr(0, slash);
    }

    inline std::string Join(const std::string& left, const std::string& right)
    {
        if (left.empty())
            return right;
        const char last = left[left.size() - 1];
        if (last == '\\' || last == '/')
            return left + right;
        return left + "\\" + right;
    }

    inline bool FileExists(const std::string& path)
    {
        const DWORD attrs = GetFileAttributesA(path.c_str());
        return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
    }

    inline bool DirectoryExists(const std::string& path)
    {
        const DWORD attrs = GetFileAttributesA(path.c_str());
        return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    inline std::string ParentDirectory(const std::string& path)
    {
        const size_t slash = path.find_last_of("\\/");
        if (slash == std::string::npos)
            return std::string();
        return path.substr(0, slash);
    }

    // Папку locale ищем, а не вычисляем: раскладка модов разная, DLL может
    // лежать и в корне мода, и в подпапке. Логика Asur'а - сначала вглубь
    // от корня плагина, потом вверх по родителям.
    inline bool LocaleBaseHasDomain(const std::string& localeBase)
    {
        const std::string messages = Join(Join(Join(localeBase, "en_GB"), "LC_MESSAGES"), KLOC_DOMAIN);
        return FileExists(messages + ".mo") || FileExists(messages + ".po");
    }

    inline std::string FindLocaleBaseRecursive(const std::string& root, int depth)
    {
        if (root.empty() || depth < 0)
            return std::string();

        const std::string direct = Join(root, "locale");
        if (DirectoryExists(direct) && LocaleBaseHasDomain(direct))
            return direct;

        WIN32_FIND_DATAA data;
        const std::string mask = Join(root, "*");
        HANDLE find = FindFirstFileA(mask.c_str(), &data);
        if (find == INVALID_HANDLE_VALUE)
            return std::string();

        std::string result;
        do
        {
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                continue;
            if (strcmp(data.cFileName, ".") == 0 || strcmp(data.cFileName, "..") == 0)
                continue;

            const std::string child = Join(root, data.cFileName);
            if (strcmp(data.cFileName, "locale") == 0 && LocaleBaseHasDomain(child))
            {
                result = child;
                break;
            }

            result = FindLocaleBaseRecursive(child, depth - 1);
            if (!result.empty())
                break;
        }
        while (FindNextFileA(find, &data));

        // Один выход с одним FindClose: в исходнике закрытие дублировалось
        // в трёх ветках, и любая новая ветка легко утекла бы хендлом.
        FindClose(find);
        return result;
    }

    inline std::string FindPluginRoot()
    {
        std::string current = DllDirectory();
        for (int i = 0; i < 6 && !current.empty(); ++i)
        {
            if (DirectoryExists(Join(current, "locale"))
                || DirectoryExists(Join(current, "gui"))
                || FileExists(Join(current, "RE_Kenshi.json")))
            {
                return current;
            }
            current = ParentDirectory(current);
        }
        return DllDirectory();
    }

    inline std::string FindLocaleBase()
    {
        std::string found = FindLocaleBaseRecursive(FindPluginRoot(), 4);
        if (!found.empty())
            return found;

        std::string current = DllDirectory();
        for (int i = 0; i < 6 && !current.empty(); ++i)
        {
            found = FindLocaleBaseRecursive(current, 2);
            if (!found.empty())
                return found;
            current = ParentDirectory(current);
        }
        return std::string();
    }

    // ---- разбор .po ----

    inline std::string UnescapePo(const std::string& value)
    {
        std::string result;
        result.reserve(value.size());
        for (size_t i = 0; i < value.size(); ++i)
        {
            if (value[i] == '\\' && i + 1 < value.size())
            {
                ++i;
                if (value[i] == 'n')
                    result += '\n';
                else if (value[i] == 't')
                    result += '\t';
                else
                    result += value[i];
            }
            else
            {
                result += value[i];
            }
        }
        return result;
    }

    inline bool PoQuoted(const std::string& line, std::string* value)
    {
        const size_t first = line.find('"');
        const size_t last = line.find_last_of('"');
        if (first == std::string::npos || last == std::string::npos || last <= first)
            return false;
        *value = UnescapePo(line.substr(first + 1, last - first - 1));
        return true;
    }

    inline bool LoadPo(const std::string& path)
    {
        std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
        if (!file.good())
            return false;

        std::string line;
        std::string currentId;
        bool any = false;
        while (std::getline(file, line))
        {
            // .po от переводчиков часто приходит с BOM и в CRLF - снимаем
            // и то и другое, иначе первый ключ и все значения не совпадут.
            if (!line.empty() && line[line.size() - 1] == '\r')
                line.erase(line.size() - 1);
            if (line.size() >= 3
                && (unsigned char)line[0] == 0xEF
                && (unsigned char)line[1] == 0xBB
                && (unsigned char)line[2] == 0xBF)
            {
                line.erase(0, 3);
            }

            if (line.compare(0, 6, "msgid ") == 0)
            {
                PoQuoted(line, &currentId);
            }
            else if (line.compare(0, 7, "msgstr ") == 0)
            {
                std::string text;
                if (!currentId.empty() && PoQuoted(line, &text) && !text.empty())
                {
                    Catalog()[currentId] = text;
                    any = true;
                }
            }
        }
        return any;
    }

    // ---- разбор .mo (двоичный формат gettext) ----

    inline unsigned int MoU32(const std::vector<char>& data, size_t offset, bool bigEndian)
    {
        if (offset + 4 > data.size())
            return 0;
        const unsigned char b0 = (unsigned char)data[offset + 0];
        const unsigned char b1 = (unsigned char)data[offset + 1];
        const unsigned char b2 = (unsigned char)data[offset + 2];
        const unsigned char b3 = (unsigned char)data[offset + 3];
        if (bigEndian)
            return ((unsigned int)b0 << 24) | ((unsigned int)b1 << 16) | ((unsigned int)b2 << 8) | (unsigned int)b3;
        return ((unsigned int)b3 << 24) | ((unsigned int)b2 << 16) | ((unsigned int)b1 << 8) | (unsigned int)b0;
    }

    inline bool LoadMo(const std::string& path)
    {
        std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
        if (!file.good())
            return false;

        std::vector<char> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (data.size() < 28)
            return false;

        const unsigned int magic = MoU32(data, 0, false);
        bool bigEndian = false;
        if (magic == 0x950412de)
            bigEndian = false;
        else if (magic == 0xde120495)
            bigEndian = true;
        else
            return false;

        const unsigned int count = MoU32(data, 8, bigEndian);
        const unsigned int originals = MoU32(data, 12, bigEndian);
        const unsigned int translations = MoU32(data, 16, bigEndian);

        // Проверки границ считаем в 64 битах: count приходит из файла, и
        // count * 8 на 32 битах переполнилось бы, пропустив битый файл дальше.
        const unsigned __int64 need = (unsigned __int64)count * 8;
        const unsigned __int64 size = (unsigned __int64)data.size();
        if ((unsigned __int64)originals + need > size || (unsigned __int64)translations + need > size)
            return false;

        bool any = false;
        for (unsigned int i = 0; i < count; ++i)
        {
            const unsigned int oLen = MoU32(data, originals + i * 8, bigEndian);
            const unsigned int oOff = MoU32(data, originals + i * 8 + 4, bigEndian);
            const unsigned int tLen = MoU32(data, translations + i * 8, bigEndian);
            const unsigned int tOff = MoU32(data, translations + i * 8 + 4, bigEndian);
            if ((unsigned __int64)oOff + oLen > size || (unsigned __int64)tOff + tLen > size)
                continue;
            if (oLen == 0 || tLen == 0)
                continue;

            std::string original(&data[oOff], &data[oOff] + oLen);
            std::string translation(&data[tOff], &data[tOff] + tLen);
            const size_t oNull = original.find('\0');
            const size_t tNull = translation.find('\0');
            if (oNull != std::string::npos)
                original = original.substr(0, oNull);
            if (tNull != std::string::npos)
                translation = translation.substr(0, tNull);
            if (!original.empty() && !translation.empty())
            {
                Catalog()[original] = translation;
                any = true;
            }
        }
        return any;
    }

    // ---- выбор языка ----

    inline std::string CurrentLocaleId()
    {
        LocaleManager* manager = LocaleManager::getInstance();
        if (manager == NULL)
            return std::string();
        LocaleInfo* locale = manager->getCurrentLocale();
        if (locale == NULL || locale->id.empty())
            return std::string();
        return locale->id;
    }

    inline std::string LowerAscii(const std::string& value)
    {
        std::string out = value;
        for (size_t i = 0; i < out.size(); ++i)
        {
            if (out[i] >= 'A' && out[i] <= 'Z')
                out[i] = (char)(out[i] - 'A' + 'a');
        }
        return out;
    }

    inline void AddCandidate(std::vector<std::string>& candidates, const std::string& localeId)
    {
        if (localeId.empty())
            return;
        for (size_t i = 0; i < candidates.size(); ++i)
        {
            if (candidates[i] == localeId)
                return;
        }
        candidates.push_back(localeId);
    }

    // Kenshi отдаёт идентификатор локали в разном виде: "ru", "ru_RU",
    // "russian". Таблица и разбор двухбуквенного префикса - из
    // PlayerFactionColor, чтобы новый язык сводился к появлению папки.
    inline void AddCandidates(std::vector<std::string>& candidates, const std::string& localeId)
    {
        AddCandidate(candidates, localeId);

        // Русский, английский, китайский (упрощённый; игра зовёт его
        // zh_CN). Добавить язык = дописать строку сюда и положить папку;
        // сам механизм ни в чём больше не завязан на список.
        static const char* kNames[][3] = {
            { "ru", "russian", "ru_RU" },
            { "en", "english", "en_GB" },
            { "zh", "chinese", "zh_CN" },
        };
        const size_t count = sizeof(kNames) / sizeof(kNames[0]);
        const std::string lower = LowerAscii(localeId);

        for (size_t i = 0; i < count; ++i)
        {
            if (lower == kNames[i][0] || lower == kNames[i][1])
                AddCandidate(candidates, kNames[i][2]);
        }

        if (localeId.size() >= 2)
        {
            const std::string prefix = LowerAscii(localeId.substr(0, 2));
            for (size_t i = 0; i < count; ++i)
            {
                if (prefix == kNames[i][0])
                    AddCandidate(candidates, kNames[i][2]);
            }
        }
    }

    inline bool LoadDomain(const std::string& localeBase, const std::string& localeId)
    {
        const std::string stem = Join(Join(Join(localeBase, localeId), "LC_MESSAGES"), KLOC_DOMAIN);
        if (FileExists(stem + ".mo") && LoadMo(stem + ".mo"))
            return true;
        return FileExists(stem + ".po") && LoadPo(stem + ".po");
    }

    inline void Reload()
    {
        const std::string localeId = CurrentLocaleId();
        Catalog().clear();

        const std::string localeBase = FindLocaleBase();
        if (localeBase.empty())
        {
            LoadedLocaleId() = localeId;
            DebugLog(KLOC_DOMAIN ": locale folder not found, staying English");
            return;
        }

        // en_GB первым как опора: если в языковом файле строки нет, останется
        // английский оригинал - он же ключ словаря.
        LoadDomain(localeBase, "en_GB");

        std::vector<std::string> candidates;
        AddCandidates(candidates, localeId);
        for (size_t i = 0; i < candidates.size(); ++i)
        {
            if (candidates[i] != "en_GB" && LoadDomain(localeBase, candidates[i]))
                break;
        }

        LoadedLocaleId() = localeId;

        char message[256];
        sprintf_s(message, KLOC_DOMAIN ": locale id='%s', strings loaded=%d",
            localeId.empty() ? "<empty>" : localeId.c_str(), (int)Catalog().size());
        DebugLog(message);
    }

    // Словарь загружается ОДИН раз за сессию и больше не перестраивается.
    //
    // Так сделано намеренно, по двум причинам.
    //
    // Первая - висячие указатели. Tr() отдаёт указатель внутрь строки в
    // словаре. Очистка словаря обесценила бы все ранее выданные указатели, а
    // выражения вида Tr("a") + x + Tr("b") вычисляют аргументы в
    // неопределённом порядке: перезагрузка внутри второго Tr() испортила бы
    // результат первого ещё до склейки. Пока словарь не трогают, указатели
    // живут до выгрузки DLL и проблема не существует.
    //
    // Вторая - цена. Иначе пришлось бы опрашивать локаль игры на каждую
    // надпись либо городить проверку по таймеру.
    //
    // Терять нечего: смена языка в Kenshi всё равно требует перезапуска игры.
    inline void EnsureLoaded()
    {
        if (Initialised())
            return;
        Initialised() = true;
        Reload();
    }

    // Набор спецификаторов формата в строке: %s, %d, %.0f и так далее.
    // %% пропускаем - это литеральный процент, аргумента не требует.
    inline std::string FormatSpecs(const char* text)
    {
        std::string specs;
        for (const char* p = text; *p; ++p)
        {
            if (*p != '%')
                continue;
            ++p;
            // Строка кончилась сразу после '%'. Именно break, а не continue:
            // continue отдал бы управление инкременту цикла, тот шагнул бы
            // ЗА нулевой байт, и дальше пошло бы чтение чужой памяти.
            if (*p == '\0')
                break;
            if (*p == '%')
                continue;
            while (*p && !strchr("diouxXeEfgGaAcspn", *p))
                ++p;
            if (*p == '\0')
                break;
            specs += *p;
        }
        return specs;
    }
}

// Перевод строки интерфейса. Нет перевода - возвращается английский оригинал.
inline const char* Tr(const char* english)
{
    if (english == NULL)
        return "";
    KLoc::EnsureLoaded();
    const std::map<std::string, std::string>& catalog = KLoc::Catalog();
    const std::map<std::string, std::string>::const_iterator it = catalog.find(english);
    if (it == catalog.end())
        return english;
    return it->second.c_str();
}

// То же для строк, которые идут в sprintf. Файлы .po правятся людьми, и
// опечатка в спецификаторе ("%d" вместо "%s", потерянный "%s") превратилась бы
// в чтение мусора со стека и вылет. Поэтому набор спецификаторов в переводе
// сверяется с оригиналом, и при расхождении берётся английский.
inline const char* TrFmt(const char* english)
{
    if (english == NULL)
        return "";
    KLoc::EnsureLoaded();
    const std::map<std::string, std::string>& catalog = KLoc::Catalog();
    const std::map<std::string, std::string>::const_iterator it = catalog.find(english);
    if (it == catalog.end())
        return english;

    if (KLoc::FormatSpecs(it->second.c_str()) != KLoc::FormatSpecs(english))
    {
        char message[512];
        sprintf_s(message,
            KLOC_DOMAIN ": format mismatch in translation of \"%.160s\", using English",
            english);
        DebugLog(message);
        return english;
    }
    return it->second.c_str();
}
