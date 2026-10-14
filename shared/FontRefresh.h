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

#include <core/Functions.h>
#include <kenshi/gui/ForgottenGUI.h>
#include <mygui/MyGUI_Gui.h>
#include <mygui/MyGUI_TextBox.h>
#include <mygui/MyGUI_Widget.h>

#include <Debug.h>

namespace FontRefresh
{
    inline void RebindTree(MyGUI::Widget* widget, int depth)
    {
        if (widget == NULL || depth > 64)
            return;
        MyGUI::TextBox* const text = widget->castType<MyGUI::TextBox>(false);
        if (text != NULL)
        {
            const std::string font = text->getFontName();
            if (!font.empty())
            {
                const int height = text->getFontHeight();
                text->setFontName(font);
                if (height > 0 && text->getFontHeight() != height)
                    text->setFontHeight(height);
            }
        }
        const size_t count = widget->getChildCount();
        for (size_t i = 0; i < count; ++i)
            RebindTree(widget->getChildAt(i), depth + 1);
    }

    inline void RebindAll()
    {
        MyGUI::Gui* const gui = MyGUI::Gui::getInstancePtr();
        if (gui == NULL)
            return;
        MyGUI::EnumeratorWidgetPtr roots = gui->getEnumerator();
        while (roots.next())
            RebindTree(roots.current(), 0);
    }

    // SEH apart from the function with C++ objects (C2712)
    inline bool SafeRebindAll()
    {
        __try
        {
            RebindAll();
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
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
        if (!SafeRebindAll())
            ErrorLog("FontRefresh: re-applying fonts after the game's font change failed");
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
