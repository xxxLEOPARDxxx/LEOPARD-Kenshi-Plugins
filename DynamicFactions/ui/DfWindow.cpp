#define WIN32_LEAN_AND_MEAN
#define KLOC_DOMAIN "dynamic_factions"
#include <Localization.h>
#include <GameTheme.h>
#include <WidgetRef.h>

#include "ui/DfWindow.h"

#include <cmath>
#include <cstdio>
#include <string>

#include <mygui/MyGUI_EditBox.h>
#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_RenderManager.h>
#include <mygui/MyGUI_Window.h>

namespace
{
    const int kWidth = 440;
    const int kHeight = 520;
    const int kPad = 10;

    WidgetRef* g_winRef = NULL;         // не удаляется: см. WidgetRef.h
    MyGUI::EditBox* g_text = NULL;      // живёт, пока жив g_winRef
    bool g_wantClose = false;
    std::string g_shown;                // что сейчас в окне - чтобы не ставить то же

    MyGUI::Window* Win()
    {
        return g_winRef != NULL ? static_cast<MyGUI::Window*>(g_winRef->get()) : NULL;
    }

    // Ваниль - пергамент, текст тёмный; мод на интерфейс - тёмное окно, текст светлый.
    MyGUI::Colour TextColour()
    {
        if (!GameTheme::PaletteFromMod())
            return MyGUI::Colour(0.20f, 0.15f, 0.11f);
        return GameTheme::ReadableColour("Main", "#AFA68B");
    }

    // '#' в MyGUI - начало цветового тега.
    std::string EscapeTags(const std::string& v)
    {
        std::string r;
        for (size_t k = 0; k < v.size(); ++k)
        {
            r += v[k];
            if (v[k] == '#')
                r += '#';
        }
        return r;
    }

    std::string Format(const char* fmt, int a)
    {
        char buf[256];
        sprintf_s(buf, sizeof(buf), fmt, a);
        return buf;
    }

    std::string Format(const char* fmt, int a, int b)
    {
        char buf[256];
        sprintf_s(buf, sizeof(buf), fmt, a, b);
        return buf;
    }

    std::string Format(const char* fmt, int a, int b, int c)
    {
        char buf[256];
        sprintf_s(buf, sizeof(buf), fmt, a, b, c);
        return buf;
    }

    // Игровое время: день с 1, час.
    std::string When(double hour)
    {
        const int h = static_cast<int>(std::floor(hour));
        return Format(Tr("Day %d, %02d:00"), h / 24 + 1, h % 24);
    }

    // События хранятся английскими ключами (core/DfBrain.cpp) - перевод здесь.
    std::string EventText(const std::string& key)
    {
        if (key == "found")
            return Tr("Founded in the Hub");
        if (key == "recruit")
            return Tr("Hired a recruit in town");
        int food=0;
        if(sscanf_s(key.c_str(),"food %d",&food)==1)
            return Format(Tr("Bought food for %d cats"),food);
        if (key == "extinct")
            return Tr("All members are dead - the faction is extinct");
        int sold=0,paid=0;
        if (sscanf_s(key.c_str(), "sale %d %d", &sold, &paid)==2)
            return Format(Tr("Sold %d units of ore for %d cats"), sold, paid);
        int level = 0;
        if (sscanf_s(key.c_str(), "level %d", &level) == 1)
            return Format(Tr("Reached tech level %d"), level);
        return key;
    }

    std::string RaceText(const std::string& race)
    {
        if (race == "human")
            return Tr("human");
        if (race == "shek")
            return Tr("shek");
        if (race == "skeleton")
            return Tr("skeleton");
        if (race == "hive")
            return Tr("hive");
        return race;
    }

    std::string Describe(const DfFactionState& s, const DfSnapshot& snap, const std::string& hire, const std::string& work,
                         const DfRules& r)
    {
        std::string t = s.name + "\n";
        t += std::string(Tr("Race:")) + " " + RaceText(s.race) + "\n";
        if (!s.founded)
            t += std::string(Tr("Not founded yet - waits until the Hub is loaded")) + "\n";
        else
            t += std::string(Tr("Founded:")) + " " + When(s.foundedHour) + "\n";
        if (snap.members >= 0)
        {
            t += Format(Tr("Members: %d of %d"), snap.members, r.maxMembers) + "\n";
            t += Format(Tr("Money: %d"), snap.money) + "\n";
            if (snap.members < r.maxMembers && snap.recruitPrice >= 0)
                t += Format(Tr("Cheapest recruit in town: %d (needs %d)"), snap.recruitPrice,
                            snap.recruitPrice + kDfReserve) + "\n";
        }
        if (!hire.empty())
            t += std::string(Tr("Hiring:")) + " " + hire + "\n";
        if (!work.empty()) t += work + "\n";
        if (r.techPointCost == 0)
            t += std::string(Tr("Research is paused until real research work is implemented.")) + "\n";
        else
        if (s.techLevel >= r.maxTech)
            t += Format(Tr("Tech: level %d (maximum)"), s.techLevel) + "\n";
        else
            t += Format(Tr("Tech: level %d, %d of %d points"), s.techLevel, s.techPoints,
                        100 * (s.techLevel + 1)) + "\n";
        t += std::string(Tr("Status:")) + " " + (s.extinct ? Tr("extinct") : s.founded ? Tr("active") : Tr("waiting")) + "\n";

        if (!s.events.empty())
        {
            t += std::string("\n") + Tr("Recent events:") + "\n";
            const size_t shown = s.events.size() < 10 ? s.events.size() : 10;
            for (size_t i = s.events.size(); i > s.events.size() - shown; --i)
            {
                const DfEvent& e = s.events[i - 1];
                t += When(e.hour) + "  " + EventText(e.text) + "\n";
            }
        }
        return t;
    }

    void OnWindowButton(MyGUI::Window*, const std::string& name)
    {
        if (name == "close")
            g_wantClose = true;
    }

    void Open()
    {
        const MyGUI::IntSize view = MyGUI::RenderManager::getInstance().getViewSize();
        MyGUI::Window* const win = MyGUI::Gui::getInstance().createWidget<MyGUI::Window>(
            "Kenshi_WindowCX", MyGUI::IntCoord(view.width - kWidth - 40, 120, kWidth, kHeight),
            MyGUI::Align::Default, "Overlapped", "DynamicFactions_Window");
        win->setCaption(Tr("Dynamic Factions"));
        win->eventWindowButtonPressed += MyGUI::newDelegate(&OnWindowButton);
        if (g_winRef == NULL)
            g_winRef = new WidgetRef();
        g_winRef->set(win);

        const MyGUI::IntCoord client = win->getClientCoord();
        g_text = win->createWidget<MyGUI::EditBox>("Kenshi_WordWrapEmpty",
            MyGUI::IntCoord(kPad, kPad, client.width - kPad * 2, client.height - kPad * 2),
            MyGUI::Align::Stretch);
        g_text->setEditMultiLine(true);
        g_text->setEditWordWrap(true);
        g_text->setEditStatic(true);
        g_text->setNeedMouseFocus(false);
        g_text->setTextColour(TextColour());
        g_text->setCaption(Tr("Waiting for the game..."));
        g_shown.clear();
        g_wantClose = false;
    }

    void Close()
    {
        if (MyGUI::Window* const win = Win())
            MyGUI::Gui::getInstance().destroyWidget(win);
        g_text = NULL;
        g_shown.clear();
        g_wantClose = false;
    }
}

void DfWindowToggle()
{
    if (Win() != NULL)
        Close();
    else
        Open();
}

bool DfWindowIsOpen()
{
    return Win() != NULL;
}

void DfWindowUpdate(const std::vector<DfFactionState>& states, const std::vector<DfSnapshot>& snaps,
                    const std::vector<std::string>& hires, const std::vector<std::string>& work, const DfRules& rules)
{
    if (Win() == NULL || g_text == NULL)
        return;
    std::string text;
    for (size_t i = 0; i < states.size() && i < snaps.size(); ++i)
    {
        if (i > 0)
            text += "\n";
        text += Describe(states[i], snaps[i], i < hires.size() ? hires[i] : std::string(), i < work.size() ? work[i] : std::string(), rules);
    }
    if (states.empty())
        text = Tr("No dynamic factions in this game yet.");
    if (text == g_shown)
        return;
    g_shown = text;
    g_text->setCaption(EscapeTags(text));
}

void DfWindowFrame()
{
    if (g_wantClose)
        Close();
}
