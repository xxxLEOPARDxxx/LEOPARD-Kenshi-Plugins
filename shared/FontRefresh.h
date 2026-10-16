// FontRefresh.h - text widgets survive the game rebuilding its fonts.
//
// Kenshi rebuilds its fonts in ForgottenGUI::changeFontSize (the font size
// option, its preview in the Options window, closing the window). Each font
// keeps its object but gets a new texture; the old texture is destroyed.
// A text widget created before still holds the old texture, and the next
// frame crashes in MyGUI rendering (lock xadd on the freed texture's
// counter - dumps 13-14.10.2026: "opened the options - crash", "changed a
// setting - crash", LiveXpBars on screen). The game's own windows are
// rebuilt by the game; plug-in widgets are not.
//
// After the game's call every text widget of the interface gets its font
// assigned again: MyGUI then takes the font's current texture. It walks the
// whole interface, all mods' widgets included, so one installed plug-in is
// enough; several installed walk several times - it happens only on a font
// change.
//
//   FontRefresh::Install();   // in startPlugin, once
#pragma once

#include <windows.h>
#include <string>
#include <vector>

#include <core/Functions.h>
#include <kenshi/gui/ForgottenGUI.h>
#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_ISubWidgetText.h>
#include <mygui/MyGUI_TextBox.h>
#include <mygui/MyGUI_Widget.h>

#include <Debug.h>

namespace FontRefresh
{
    // One widget. A TextBox (and EditBox, Button...) goes through its own
    // setFontName; any other widget whose skin has a text part (a plain
    // Widget with a caption) is handled on that text part directly - a
    // TextBox-only walk missed those (dump 16.10.2026: Asur, latest MCM and
    // LiveXpBars, still a stale font texture on opening the options).
    // stage: 1 - reading the font name, 2 - assigning it. A fault at 1 is a
    // text part without a font (MyGUI reads the name through a null font):
    // it has no texture to go stale and needs nothing.
    inline void RebindOne(MyGUI::Widget* widget, int& stage)
    {
        stage = 1;
        MyGUI::TextBox* const box = widget->castType<MyGUI::TextBox>(false);
        if (box != NULL)
        {
            const std::string font = box->getFontName();
            if (!font.empty())
            {
                stage = 2;
                const int height = box->getFontHeight();
                box->setFontName(font);
                if (height > 0 && box->getFontHeight() != height)
                    box->setFontHeight(height);
            }
            return;
        }
        MyGUI::ISubWidgetText* const text = widget->getSubWidgetText();
        if (text == NULL)
            return;
        const std::string font = text->getFontName();
        if (font.empty())
            return;
        stage = 2;
        const int height = text->getFontHeight();
        text->setFontName(font);
        if (height > 0 && text->getFontHeight() != height)
            text->setFontHeight(height);
    }

    // SEH apart from the function with C++ objects (C2712). One broken
    // widget is skipped, the walk goes on (before, the first fault ended the
    // whole walk and every widget after it kept the old texture).
    inline bool SafeRebindOne(MyGUI::Widget* widget, int& stage)
    {
        __try
        {
            RebindOne(widget, stage);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    struct Faults
    {
        int noFont;     // stage 1 - harmless, see RebindOne
        int assign;     // stage 2 - the widget may keep the old texture
        int walk;       // children could not be listed
        Faults() : noFont(0), assign(0), walk(0) {}
    };

    inline void RebindTree(MyGUI::Widget* widget, int depth, Faults& faults)
    {
        if (widget == NULL || depth > 64)
            return;
        int stage = 0;
        if (!SafeRebindOne(widget, stage))
        {
            if (stage == 1)
                ++faults.noFont;
            else
                ++faults.assign;
        }
        __try
        {
            const size_t count = widget->getChildCount();
            for (size_t i = 0; i < count; ++i)
                RebindTree(widget->getChildAt(i), depth + 1, faults);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ++faults.walk;
        }
    }

    // Roots are collected first: the enumerator is a C++ object, and the
    // walk itself runs under SEH.
    inline Faults RebindAll()
    {
        MyGUI::Gui* const gui = MyGUI::Gui::getInstancePtr();
        if (gui == NULL)
            return Faults();
        std::vector<MyGUI::Widget*> roots;
        MyGUI::EnumeratorWidgetPtr it = gui->getEnumerator();
        while (it.next())
            roots.push_back(it.current());
        Faults faults;
        for (size_t i = 0; i < roots.size(); ++i)
            RebindTree(roots[i], 0, faults);
        return faults;
    }

    typedef void (*ChangeFontSizeFn)();

    inline ChangeFontSizeFn& Original()
    {
        static ChangeFontSizeFn original = NULL;
        return original;
    }

    inline void ChangeFontSize_hook()
    {
        if (Original() != NULL)
            Original()();
        const Faults faults = RebindAll();
        if (faults.assign > 0 || faults.walk > 0)
            ErrorLog("FontRefresh: fonts not re-applied - " + std::to_string((long long)faults.assign)
                + " widget(s) failed, " + std::to_string((long long)faults.walk) + " child list(s) unreadable");
        if (faults.noFont > 0)
            DebugLog("FontRefresh: " + std::to_string((long long)faults.noFont) + " text part(s) without a font skipped");
    }

    inline bool Install()
    {
        static bool installed = false;
        if (installed)
            return true;
        if (KenshiLib::SUCCESS != KenshiLib::AddHook(
                KenshiLib::GetRealAddress(&ForgottenGUI::changeFontSize),
                &ChangeFontSize_hook, &Original()))
        {
            ErrorLog("FontRefresh: could not hook ForgottenGUI::changeFontSize");
            return false;
        }
        installed = true;
        return true;
    }
}
