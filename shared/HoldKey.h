#pragma once
// Клавиша «показывать, пока зажата» - в том виде, как её записывает MCM
// (захват нажатия, 07.10.2026): «ALT», «CTRL», «SHIFT», «F», «CTRL+B»,
// «NUM5», «NONE» (= клавиши нет, показывать всегда).
//
// Раньше AssignedWorkers и GearCompare понимали только ALT/CTRL/SHIFT/NONE
// и выбирались выпадающим списком MCM, а он в игре выбор не отдавал.
//
// Использование:
//     HoldKey::Key g_hold;                       // по умолчанию ALT
//     g_hold = HoldKey::Parse(textFromIni);      // в LoadSettings
//     if (HoldKey::Held(g_hold)) ...             // каждый кадр
#include <Windows.h>
#include <cctype>
#include <cstring>
#include <string>

namespace HoldKey
{
    struct Key
    {
        int vk;             // 0 - клавиши нет: «зажата» всегда
        bool ctrl, shift, alt;
        Key() : vk(VK_MENU), ctrl(false), shift(false), alt(false) {}
    };

    inline int VkFromName(const std::string& n)
    {
        if (n.size() == 1)
        {
            const char c = n[0];
            if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
                return c;
            switch (c)
            {
            case '`': return VK_OEM_3;
            case '-': return VK_OEM_MINUS;
            case '=': return VK_OEM_PLUS;
            case '[': return VK_OEM_4;
            case ']': return VK_OEM_6;
            case ';': return VK_OEM_1;
            case '\'': return VK_OEM_7;
            case ',': return VK_OEM_COMMA;
            case '.': return VK_OEM_PERIOD;
            case '/': return VK_OEM_2;
            case '\\': return VK_OEM_5;
            default: return 0;
            }
        }
        if (n.size() >= 2 && n.size() <= 3 && n[0] == 'F' && isdigit(static_cast<unsigned char>(n[1])))
        {
            const int f = atoi(n.c_str() + 1);
            return (f >= 1 && f <= 12) ? VK_F1 + f - 1 : 0;
        }
        if (n.size() == 4 && n.compare(0, 3, "NUM") == 0 && isdigit(static_cast<unsigned char>(n[3])))
            return VK_NUMPAD0 + (n[3] - '0');
        struct Named { const char* name; int vk; };
        static const Named named[] = {
            { "ALT", VK_MENU }, { "CTRL", VK_CONTROL }, { "SHIFT", VK_SHIFT },
            { "LALT", VK_LMENU }, { "RALT", VK_RMENU }, { "LCTRL", VK_LCONTROL },
            { "RCTRL", VK_RCONTROL }, { "LSHIFT", VK_LSHIFT }, { "RSHIFT", VK_RSHIFT },
            { "SPACE", VK_SPACE }, { "TAB", VK_TAB }, { "ENTER", VK_RETURN },
            { "INSERT", VK_INSERT }, { "DELETE", VK_DELETE }, { "HOME", VK_HOME },
            { "END", VK_END }, { "PAGEUP", VK_PRIOR }, { "PAGEDOWN", VK_NEXT },
            { "UP", VK_UP }, { "DOWN", VK_DOWN }, { "LEFT", VK_LEFT }, { "RIGHT", VK_RIGHT },
            { "BACKSPACE", VK_BACK }, { "ESC", VK_ESCAPE },
        };
        for (size_t i = 0; i < sizeof(named) / sizeof(named[0]); ++i)
            if (n == named[i].name)
                return named[i].vk;
        return 0;
    }

    // Пустое, «NONE» и непонятное - клавиши нет (показывать всегда), как
    // и раньше было с «NONE и прочим».
    inline Key Parse(const std::string& text)
    {
        std::string t;
        for (size_t i = 0; i < text.size(); ++i)
            if (text[i] != ' ')
                t += static_cast<char>(toupper(static_cast<unsigned char>(text[i])));
        Key k;
        k.vk = 0;
        if (t.empty() || t == "NONE")
            return k;
        size_t start = 0;
        while (true)
        {
            const size_t plus = t.find('+', start);
            if (plus == std::string::npos || plus + 1 >= t.size())
                break;
            const std::string mod = t.substr(start, plus - start);
            if (mod == "CTRL") k.ctrl = true;
            else if (mod == "SHIFT") k.shift = true;
            else if (mod == "ALT") k.alt = true;
            start = plus + 1;
        }
        k.vk = VkFromName(t.substr(start));
        if (k.vk == 0)
            k.ctrl = k.shift = k.alt = false;
        return k;
    }

    inline bool Down(int vk)
    {
        return (GetAsyncKeyState(vk) & 0x8000) != 0;
    }

    // Зажата ли - только когда окно игры впереди: иначе ALT+TAB в другое
    // окно показывал бы панель.
    inline bool Held(const Key& k)
    {
        if (k.vk == 0)
            return true;
        // Сначала дешёвое - сама клавиша; окно на переднем плане - только
        // когда она нажата (зовётся каждый кадр в нескольких плагинах).
        if (!Down(k.vk))
            return false;
        const HWND front = GetForegroundWindow();
        if (front == NULL)
            return false;
        DWORD pid = 0;
        GetWindowThreadProcessId(front, &pid);
        if (pid != GetCurrentProcessId())
            return false;
        return Down(k.vk) && (!k.ctrl || Down(VK_CONTROL)) && (!k.shift || Down(VK_SHIFT)) &&
               (!k.alt || Down(VK_MENU));
    }
}
