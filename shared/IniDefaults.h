#pragma once
// Настройки игрока не теряются при обновлении мода.
//
// Зачем (11.10.2026). Раньше в архиве и в Мастерской лежал <Мод>.ini, и
// каждое обновление затирало им настройки игрока - жаловались. Теперь в
// раздаче только <Мод>.defaults.ini (значения по умолчанию с пояснениями),
// а <Мод>.ini принадлежит игроку: его не раздаём и не перезаписываем.
//
// Как. Prepare(путь к ini) - один раз за запуск для каждого файла:
//   - ini нет - копия <Мод>.defaults.ini, с пояснениями;
//   - ini есть - дописываются ключи, которых в нём нет (новые настройки
//     из обновления), со значениями из defaults. Что игрок поменял - не
//     трогаем; пояснения к новым ключам - в defaults и в README.
// Нет defaults (старая раскладка, ручная установка) - ничего не делаем:
// плагин берёт значения по умолчанию из кода, как и раньше.
//
// Ini без BOM: иначе GetPrivateProfile* не узнаёт первую секцию.

#include <Windows.h>

#include <set>
#include <string>
#include <vector>

namespace IniDefaults
{
    inline std::string DefaultsPath(const std::string& ini)
    {
        const std::string::size_type dot = ini.rfind('.');
        return dot == std::string::npos ? std::string()
                                        : ini.substr(0, dot) + ".defaults.ini";
    }


    inline bool Exists(const std::string& path)
    {
        return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
    }


    // Список из GetPrivateProfile*: строки через \0, в конце ещё один \0.
    inline std::vector<std::string> SplitList(const std::vector<char>& buf,
                                              DWORD length)
    {
        std::vector<std::string> out;
        const char* p = &buf[0];
        const char* const end = p + length;
        while (p < end && *p != '\0')
        {
            const std::string item(p);
            out.push_back(item);
            p += item.size() + 1;
        }
        return out;
    }


    inline void AddMissingKeys(const std::string& ini,
                               const std::string& defaults)
    {
        std::vector<char> buf(64 * 1024);
        const DWORD size = static_cast<DWORD>(buf.size());

        const std::vector<std::string> sections = SplitList(
            buf, GetPrivateProfileSectionNamesA(&buf[0], size, defaults.c_str()));

        for (size_t s = 0; s < sections.size(); ++s)
        {
            const char* const section = sections[s].c_str();
            const std::vector<std::string> keys = SplitList(
                buf, GetPrivateProfileStringA(section, NULL, "", &buf[0], size,
                                              defaults.c_str()));

            for (size_t k = 0; k < keys.size(); ++k)
            {
                const char* const key = keys[k].c_str();

                // Есть ли ключ у игрока: метка, которой в значении не бывает.
                char probe[4] = {};
                GetPrivateProfileStringA(section, key, "\x01", probe,
                                         sizeof(probe), ini.c_str());
                if (probe[0] != '\x01')
                    continue;

                char value[1024] = {};
                GetPrivateProfileStringA(section, key, "", value,
                                         sizeof(value), defaults.c_str());
                WritePrivateProfileStringA(section, key, value, ini.c_str());
            }
        }
    }


    // Возвращает тот же путь - удобно оборачивать IniPath().
    inline std::string Prepare(const std::string& ini)
    {
        static std::set<std::string> done;
        if (ini.empty() || !done.insert(ini).second)
            return ini;

        const std::string defaults = DefaultsPath(ini);
        if (defaults.empty() || !Exists(defaults))
            return ini;

        if (!Exists(ini))
            CopyFileA(defaults.c_str(), ini.c_str(), TRUE);
        else
            AddMissingKeys(ini, defaults);

        return ini;
    }

    // mod-config.json форков Emkej - то же правило: файл игрока не
    // раздаём, в раздаче mod-config.defaults.json. Нет файла - копия
    // defaults (раньше плагин писал в журнал «could not read
    // mod-config.json» и брал значения из кода, а defaults не читал никто;
    // жалоба из сборки Second Life of Kenshi, 14.10.2026).
    inline std::string PrepareJson(const std::string& json)
    {
        if (!json.empty() && !Exists(json))
        {
            const std::string::size_type slash = json.find_last_of("\\/");
            const std::string defaults =
                (slash == std::string::npos ? std::string() : json.substr(0, slash + 1)) +
                "mod-config.defaults.json";
            if (Exists(defaults))
                CopyFileA(defaults.c_str(), json.c_str(), TRUE);
        }
        return json;
    }
}
