// Мост: таблица настроек Mod Hub (Emkej) -> страница ModConfigMenu.
//
// Моды Emkej описывают настройки таблицей ModHubClientTableRegistrationV1:
// у каждой строки подпись, описание, пределы и функции чтения/записи,
// которые сами проверяют значение, применяют его и пишут mod-config.json.
// MCM (API v3) зовёт эти функции напрямую: сигнатуры совпадают.
//
// В плагине:
//   1) там, где таблица отдаётся клиенту Mod Hub, обернуть её:
//        config.table_registration = mcm_bridge::Capture(&kModHubRegistration);
//   2) в одном .cpp, где виден Tr (Localization.h со своим KLOC_DOMAIN):
//        #include <McmModHubBridge.h>
//        MCM_MODHUB_BRIDGE("What the mod does, in one sentence.")
//
// Подписи и описания проходят через Tr плагина: перевод - в его
// locale/<язык>/LC_MESSAGES/<домен>.po, msgid - английский текст Emkej.
// Условия «скрыть/выключить, если флажок» (bool_condition_rules) MCM пока
// не поддерживает - такие строки видны всегда.
//
// Старые копии SDK (март-апрель 2026): строка таблицы - {kind, def}, без
// разделов, и видов только пять (флажок, клавиша, целое, дробное, кнопка),
// у части ещё INT_V2. Там перед подключением:
//   #define MCM_BRIDGE_OLD_SDK
//   #define MCM_BRIDGE_HAS_INT_V2     // если в SDK есть EMC_IntSettingDefV2
//
// «Сбросить по умолчанию» (MCM API v5): рядом с DLL мода лежит
// mod-config.defaults.json - авторский mod-config.json из пакета. Ключи
// сравниваются с setting_id строк без учёта регистра и «_» (debug_logging =
// debugLogging); найденное значение уходит в MCM, и сброс идёт через тот же
// setter, что и обычная правка. Строки без пары в файле сброс не трогает.

#pragma once

#include <ModConfigMenu.h>
#include <Windows.h>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace mcm_bridge
{
    // Коды видов строк - как в emc::ModHubClientSettingKind (числа во всех
    // версиях SDK одни и те же, а имён в старых нет).
    enum
    {
        KIND_BOOL = 0, KIND_KEYBIND = 1, KIND_INT = 2, KIND_FLOAT = 3, KIND_ACTION = 4,
        KIND_INT_V2 = 5, KIND_SELECT = 6, KIND_TEXT = 7, KIND_COLOR = 8, KIND_BOOL_V2 = 9,
        KIND_KEYBIND_V2 = 10, KIND_SELECT_V2 = 11, KIND_TEXT_V2 = 12, KIND_ACTION_V2 = 13
    };

    inline const emc::ModHubClientTableRegistrationV1*& Table()
    {
        static const emc::ModHubClientTableRegistrationV1* table = 0;
        return table;
    }

    // Запомнить таблицу и вернуть её же - чтобы оборачивать на месте.
    inline const emc::ModHubClientTableRegistrationV1* Capture(
        const emc::ModHubClientTableRegistrationV1* table)
    {
        Table() = table;
        return table;
    }

    inline const char* Text(const char* s)
    {
        return (s != 0 && *s != 0) ? Tr(s) : "";
    }

    // Описание: у V2 бывает пустым при заполненной подсказке - берём что есть.
    inline const char* Tip(const char* description, const char* hint)
    {
        if (description != 0 && *description != 0)
            return Tr(description);
        return Text(hint);
    }

    // --- значения по умолчанию (MCM API v5) ---

    struct DefaultValue
    {
        std::string text;
        bool isString;
    };

    inline std::string Norm(const std::string& s)
    {
        std::string r;
        for (size_t i = 0; i < s.size(); ++i)
        {
            const unsigned char c = static_cast<unsigned char>(s[i]);
            if (isalnum(c))
                r += static_cast<char>(tolower(c));
        }
        return r;
    }

    // Плоский JSON «"ключ": значение» - так устроены mod-config.json Emkej.
    inline std::map<std::string, DefaultValue>& Defaults()
    {
        static std::map<std::string, DefaultValue> values;
        static bool loaded = false;
        if (loaded)
            return values;
        loaded = true;

        HMODULE self = 0;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCSTR>(&Defaults), &self);
        char path[MAX_PATH] = {};
        GetModuleFileNameA(self, path, MAX_PATH);
        std::string file(path);
        const std::string::size_type slash = file.find_last_of("\\/");
        file = (slash == std::string::npos ? std::string() : file.substr(0, slash + 1)) + "mod-config.defaults.json";

        FILE* f = 0;
        if (fopen_s(&f, file.c_str(), "rb") != 0 || f == 0)
            return values;
        std::string body;
        char buf[4096];
        size_t n = 0;
        while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
            body.append(buf, n);
        fclose(f);

        size_t i = 0;
        while (true)
        {
            const size_t q = body.find('"', i);
            if (q == std::string::npos)
                break;
            const size_t qe = body.find('"', q + 1);
            if (qe == std::string::npos)
                break;
            const std::string key = body.substr(q + 1, qe - q - 1);
            size_t c = qe + 1;
            while (c < body.size() && isspace(static_cast<unsigned char>(body[c])))
                ++c;
            if (c >= body.size() || body[c] != ':')
            {
                i = qe + 1;
                continue;
            }
            ++c;
            while (c < body.size() && isspace(static_cast<unsigned char>(body[c])))
                ++c;
            DefaultValue v;
            if (c < body.size() && body[c] == '"')
            {
                size_t e = c + 1;
                while (e < body.size() && body[e] != '"')
                {
                    if (body[e] == '\\' && e + 1 < body.size())
                        ++e;
                    v.text += body[e];
                    ++e;
                }
                v.isString = true;
                i = e + 1;
            }
            else
            {
                size_t e = c;
                while (e < body.size() && body[e] != ',' && body[e] != '}' && body[e] != '\n' && body[e] != '\r')
                    ++e;
                v.text = body.substr(c, e - c);
                while (!v.text.empty() && isspace(static_cast<unsigned char>(v.text[v.text.size() - 1])))
                    v.text.erase(v.text.size() - 1);
                v.isString = false;
                i = e;
            }
            values[Norm(key)] = v;
        }
        return values;
    }

    inline const DefaultValue* FindDefault(const char* settingId)
    {
        if (settingId == 0)
            return 0;
        std::map<std::string, DefaultValue>::const_iterator it = Defaults().find(Norm(settingId));
        return it == Defaults().end() ? 0 : &it->second;
    }

    // Значение по умолчанию последней добавленной строки - строкой для MCM.
    inline void GiveDefault(MCM_Api* api, const char* settingId, int kind)
    {
        if (api == 0 || api->version < 5)
            return;
        const DefaultValue* const d = FindDefault(settingId);
        if (d == 0)
            return;
        std::string v = d->text;
        if (kind == KIND_BOOL || kind == KIND_BOOL_V2)
            v = (v == "true" || v == "1") ? "1" : "0";
        else if (kind == KIND_COLOR && v.size() == 9 && v[0] == '#')
            v = v.substr(0, 7);           // #RRGGBBAA: прозрачность оставляет сам мод
        api->defaultValue(api, v.c_str());
    }

#ifndef MCM_BRIDGE_OLD_SDK
    template <typename Def>
    inline void Select(MCM_Api* api, const Def* d)
    {
        std::vector<int> values;
        std::vector<const char*> labels;
        for (uint32_t k = 0; k < d->option_count && d->options != 0; ++k)
        {
            values.push_back(d->options[k].value);
            labels.push_back(Text(d->options[k].label));
        }
        api->choiceFn(api, Text(d->label), Text(d->description),
                      reinterpret_cast<MCM_GetIntFn>(d->get_value),
                      reinterpret_cast<MCM_SetIntFn>(d->set_value), d->user_data,
                      values.empty() ? 0 : &values[0], labels.empty() ? 0 : &labels[0],
                      static_cast<int>(values.size()), 0);
        const DefaultValue* const def = FindDefault(d->setting_id);
        if (def == 0 || api->version < 5)
            return;
        if (!def->isString)
        {
            api->defaultValue(api, def->text.c_str());
            return;
        }
        for (uint32_t k = 0; k < d->option_count && d->options != 0; ++k)
            if (d->options[k].label != 0 && Norm(d->options[k].label) == Norm(def->text))
            {
                char buf[16];
                sprintf_s(buf, "%d", d->options[k].value);
                api->defaultValue(api, buf);
                return;
            }
    }

#endif

    inline void Describe(MCM_Api* api, const char* info)
    {
        const emc::ModHubClientTableRegistrationV1* const t = Table();
        if (api == 0 || api->version < 3 || t == 0 || t->mod_desc == 0)
            return;
        const EMC_ModDescriptorV1* const mod = t->mod_desc;
        // Название - оригинальное английское (переводит его, если нужно,
        // каталог самого MCM).
        api->beginMod(api, mod->mod_id, mod->mod_display_name, 0, 0);
        if (info != 0)
            api->info(api, Tr(info));

        // Порядок строк - по разделам, в порядке первого появления раздела:
        // в таблицах Emkej строки одного раздела бывают разбросаны (Vital-Read:
        // Core, Advanced, снова Core), и раздел показывался бы дважды.
        std::vector<uint32_t> order;
#ifndef MCM_BRIDGE_OLD_SDK
        std::vector<const char*> sections;
        for (uint32_t i = 0; i < t->row_count; ++i)
        {
            const char* const name = t->rows[i].section_display_name;
            bool known = false;
            for (size_t k = 0; k < sections.size() && !known; ++k)
                known = (sections[k] == 0 && name == 0) ||
                        (sections[k] != 0 && name != 0 && strcmp(sections[k], name) == 0);
            if (!known)
                sections.push_back(name);
        }
        for (size_t k = 0; k < sections.size(); ++k)
            for (uint32_t i = 0; i < t->row_count; ++i)
            {
                const char* const name = t->rows[i].section_display_name;
                if ((sections[k] == 0 && name == 0) ||
                    (sections[k] != 0 && name != 0 && strcmp(sections[k], name) == 0))
                    order.push_back(i);
            }
#else
        for (uint32_t i = 0; i < t->row_count; ++i)
            order.push_back(i);
#endif

        const char* section = 0;
        for (size_t n = 0; n < order.size(); ++n)
        {
            const emc::ModHubClientSettingRowV1& r = t->rows[order[n]];
            if (r.def == 0)
                continue;
#ifndef MCM_BRIDGE_OLD_SDK
            if (r.section_display_name != 0 && *r.section_display_name != 0 &&
                (section == 0 || strcmp(section, r.section_display_name) != 0))
            {
                api->section(api, Text(r.section_display_name));
                section = r.section_display_name;
            }
#endif

            switch (r.kind)
            {
            case KIND_BOOL:
            {
                const EMC_BoolSettingDefV1* d = static_cast<const EMC_BoolSettingDefV1*>(r.def);
                api->toggleFn(api, Text(d->label), Text(d->description),
                              reinterpret_cast<MCM_GetIntFn>(d->get_value),
                              reinterpret_cast<MCM_SetIntFn>(d->set_value), d->user_data, 0);
                GiveDefault(api, d->setting_id, r.kind);
                break;
            }
#ifndef MCM_BRIDGE_OLD_SDK
            case KIND_BOOL_V2:
            {
                const EMC_BoolSettingDefV2* d = static_cast<const EMC_BoolSettingDefV2*>(r.def);
                api->toggleFn(api, Text(d->label), Tip(d->description, d->hover_hint),
                              reinterpret_cast<MCM_GetIntFn>(d->get_value),
                              reinterpret_cast<MCM_SetIntFn>(d->set_value), d->user_data, 0);
                GiveDefault(api, d->setting_id, r.kind);
                break;
            }
#endif
            case KIND_KEYBIND:
            {
                const EMC_KeybindSettingDefV1* d = static_cast<const EMC_KeybindSettingDefV1*>(r.def);
                api->hotkeyFn(api, Text(d->label), Text(d->description),
                              reinterpret_cast<MCM_GetKeyFn>(d->get_value),
                              reinterpret_cast<MCM_SetKeyFn>(d->set_value), d->user_data, 0);
                GiveDefault(api, d->setting_id, r.kind);
                break;
            }
#ifndef MCM_BRIDGE_OLD_SDK
            case KIND_KEYBIND_V2:
            {
                const EMC_KeybindSettingDefV2* d = static_cast<const EMC_KeybindSettingDefV2*>(r.def);
                api->hotkeyFn(api, Text(d->label), Tip(d->description, d->hover_hint),
                              reinterpret_cast<MCM_GetKeyFn>(d->get_value),
                              reinterpret_cast<MCM_SetKeyFn>(d->set_value), d->user_data, 0);
                GiveDefault(api, d->setting_id, r.kind);
                break;
            }
#endif
            case KIND_INT:
            {
                const EMC_IntSettingDefV1* d = static_cast<const EMC_IntSettingDefV1*>(r.def);
                api->integerFn(api, Text(d->label), Text(d->description),
                               reinterpret_cast<MCM_GetIntFn>(d->get_value),
                               reinterpret_cast<MCM_SetIntFn>(d->set_value), d->user_data,
                               d->min_value, d->max_value, 0);
                GiveDefault(api, d->setting_id, r.kind);
                break;
            }
#if !defined(MCM_BRIDGE_OLD_SDK) || defined(MCM_BRIDGE_HAS_INT_V2)
            case KIND_INT_V2:
            {
                const EMC_IntSettingDefV2* d = static_cast<const EMC_IntSettingDefV2*>(r.def);
                api->integerFn(api, Text(d->label), Text(d->description),
                               reinterpret_cast<MCM_GetIntFn>(d->get_value),
                               reinterpret_cast<MCM_SetIntFn>(d->set_value), d->user_data,
                               d->min_value, d->max_value, 0);
                GiveDefault(api, d->setting_id, r.kind);
                break;
            }
#endif
            case KIND_FLOAT:
            {
                const EMC_FloatSettingDefV1* d = static_cast<const EMC_FloatSettingDefV1*>(r.def);
                api->numberFn(api, Text(d->label), Text(d->description),
                              reinterpret_cast<MCM_GetFloatFn>(d->get_value),
                              reinterpret_cast<MCM_SetFloatFn>(d->set_value), d->user_data,
                              d->min_value, d->max_value,
                              static_cast<int>(d->display_decimals > 6 ? 6 : d->display_decimals), 0);
                GiveDefault(api, d->setting_id, r.kind);
                break;
            }
#ifndef MCM_BRIDGE_OLD_SDK
            case KIND_SELECT:
                Select(api, static_cast<const EMC_SelectSettingDefV1*>(r.def));
                break;
#endif
#ifndef MCM_BRIDGE_OLD_SDK
            case KIND_SELECT_V2:
                Select(api, static_cast<const EMC_SelectSettingDefV2*>(r.def));
                break;
#endif
#ifndef MCM_BRIDGE_OLD_SDK
            case KIND_TEXT:
            {
                const EMC_TextSettingDefV1* d = static_cast<const EMC_TextSettingDefV1*>(r.def);
                api->textFn(api, Text(d->label), Text(d->description),
                            reinterpret_cast<MCM_GetTextFn>(d->get_value),
                            reinterpret_cast<MCM_SetTextFn>(d->set_value), d->user_data, 0);
                GiveDefault(api, d->setting_id, r.kind);
                break;
            }
#endif
#ifndef MCM_BRIDGE_OLD_SDK
            case KIND_TEXT_V2:
            {
                const EMC_TextSettingDefV2* d = static_cast<const EMC_TextSettingDefV2*>(r.def);
                api->textFn(api, Text(d->label), Tip(d->description, d->hover_hint),
                            reinterpret_cast<MCM_GetTextFn>(d->get_value),
                            reinterpret_cast<MCM_SetTextFn>(d->set_value), d->user_data, 0);
                GiveDefault(api, d->setting_id, r.kind);
                break;
            }
#endif
#ifndef MCM_BRIDGE_OLD_SDK
            case KIND_COLOR:
            {
                const EMC_ColorSettingDefV1* d = static_cast<const EMC_ColorSettingDefV1*>(r.def);
                api->colourFn(api, Text(d->label), Text(d->description),
                              reinterpret_cast<MCM_GetTextFn>(d->get_value),
                              reinterpret_cast<MCM_SetTextFn>(d->set_value), d->user_data, 0);
                GiveDefault(api, d->setting_id, r.kind);
                break;
            }
#endif
            case KIND_ACTION:
            {
                const EMC_ActionRowDefV1* d = static_cast<const EMC_ActionRowDefV1*>(r.def);
                api->action(api, Text(d->label), Text(d->description),
                            reinterpret_cast<MCM_ActionFn>(d->on_action), d->user_data, 0);
                break;
            }
#ifndef MCM_BRIDGE_OLD_SDK
            case KIND_ACTION_V2:
            {
                const EMC_ActionRowDefV2* d = static_cast<const EMC_ActionRowDefV2*>(r.def);
                api->action(api, Text(d->label), Tip(d->description, d->hover_hint),
                            reinterpret_cast<MCM_ActionFn>(d->on_action), d->user_data, 0);
                break;
            }
#endif
            default:
                break;
            }
        }
    }
}

// Экспорт страницы MCM. INFO - английская фраза о моде (идёт через Tr).
#define MCM_MODHUB_BRIDGE(INFO) \
    extern "C" __declspec(dllexport) void MCM_Describe(MCM_Api* api) \
    { \
        mcm_bridge::Describe(api, INFO); \
    }

// То же, но таблица берётся прямо здесь - для модов, у которых она видна по
// имени, а отдаётся Mod Hub только внутри его регистрации (Wall-B-Gone)
// или строится при запуске (Vital-Sense).
#define MCM_MODHUB_BRIDGE_TABLE(TABLE, INFO) \
    extern "C" __declspec(dllexport) void MCM_Describe(MCM_Api* api) \
    { \
        mcm_bridge::Capture(TABLE); \
        mcm_bridge::Describe(api, INFO); \
    }
