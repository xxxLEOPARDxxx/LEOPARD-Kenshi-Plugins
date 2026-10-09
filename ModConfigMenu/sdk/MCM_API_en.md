# ModConfigMenu (MCM) — adding your plugin's settings

MCM adds an **MCM** tab to the game's Options window. On the left is a list
of mods; on the right are the selected mod's settings, drawn with the game's
own option lines (DatapanelGUI): check boxes, sliders with a number field,
drop-down lists and text fields. When the player changes a value, MCM writes
it to your plugin's ini and calls your function that re-reads the settings.

Your plugin does **not depend** on MCM. Without MCM it works as before and
reads its own ini. If MCM is installed, it finds the plugin by itself and
builds a page for it.

---

## Requirements

- A RE_Kenshi plugin built on KenshiLib (x64 DLL, VC10/v100, /MD), as usual.
- Settings in an ini file read with `GetPrivateProfile*`.
- One header: `ModConfigMenu.h` (next to this file). There is nothing to
  link: MCM calls your plugin; your plugin never calls MCM.

---

## Three steps

### 1. Include the header

```cpp
#include "ModConfigMenu.h"
```

### 2. Have a function that re-reads the ini

You most likely already have one: the one that reads the ini at start-up.
Two changes are needed:

- the calling convention must be `__cdecl` (on x64 it is the only one
  anyway, but the pointer type has to match);
- the function must be **idempotent**: clear anything that accumulates
  (lists, sets) before reading again.

```cpp
void __cdecl LoadSettings()
{
    const std::string ini = IniPath();
    g_enabled = GetPrivateProfileIntA("MyPlugin", "Enabled", 1, ini.c_str()) != 0;
    g_radius  = GetPrivateProfileIntA("MyPlugin", "Radius", 50, ini.c_str());
    g_words.clear();          // clear what accumulates
    // ...
}
```

### 3. Export `MCM_Describe`

The name must be exactly `MCM_Describe`, undecorated, so `extern "C"` is
required:

```cpp
extern "C" __declspec(dllexport) void MCM_Describe(MCM_Api* api)
{
    static const std::string ini = IniPath();   // must outlive the call
    const char* const sec = "MyPlugin";

    api->beginMod(api, "MyPlugin", "My Plugin", ini.c_str(), &LoadSettings);
    if (api->version >= 2)
        api->info(api, Tr("What the mod does, in one or two sentences."));

    api->section(api, "General");
    api->toggle (api, sec, "Enabled", "Enabled", "Turns the plugin on and off.", 1, 0);
    api->integer(api, sec, "Radius", "Search radius, m", NULL, 50, 10, 500, 0);
    api->number (api, sec, "Scale", "Scale", NULL, 1.0f, 0.5f, 3.0f, 2, 0);

    static const char* const modes[]  = { "fast", "normal", "precise" };
    static const char* const labels[] = { "Fast", "Normal", "Precise" };
    api->choice (api, sec, "Mode", "Mode", NULL, "normal", modes, labels, 3, 0);

    api->section(api, "Advanced");
    api->hotkey (api, sec, "Hotkey", "Hotkey", NULL, "SHIFT+B", 0);
    api->colour (api, sec, "Colour", "Marker colour", NULL, "#E04030", 0);
    api->text   (api, sec, "Ignore", "Ignored words", NULL, "the,of", MCM_RESTART);
}
```

That is all. Rebuild the plugin, open Options and the **MCM** tab.

---

## Reference

All strings are UTF-8. MCM copies them during the call; you do not need to
keep them. Pass labels already translated (with your own `Tr()` or similar).
The game language changes only with a restart, so translating once is
enough.

| Function | Written to the ini | Control |
|---|---|---|
| `beginMod(api, id, title, iniPath, apply)` | — | Starts a mod page. `id` — Latin letters, unique. `title` — text in the mod list: **the mod's original English name**, untranslated. If someone wants the names translated, the translation goes into MCM's own catalog (`locale/<language>/LC_MESSAGES/mod_config_menu.po`, the msgid is the name). `iniPath` — **full** path to the ini. `apply` — re-read function, may be `NULL`. |
| `info(api, text)` | — | Since version 2. What the mod does, in brief: the tooltip at the bottom of the window when the mod is hovered in the list. Already translated. |
| `section(api, title)` | — | Sub-heading inside the page. |
| `toggle(api, sec, key, label, tip, def, flags)` | `0` / `1` | Check box. |
| `integer(api, sec, key, label, tip, def, min, max, flags)` | integer | Slider with a number field. |
| `number(api, sec, key, label, tip, def, min, max, decimals, flags)` | decimal, `decimals` places, dot | Slider with a number field. |
| `choice(api, sec, key, label, tip, def, values, labels, count, flags)` | one of `values` | A button with the current choice: click - next, SHIFT+click - previous (the game's drop-down list did not report the choice in game). `labels` (what is shown) may be `NULL`; then the `values` themselves are shown. |
| `text(api, sec, key, label, tip, def, flags)` | the string as is | Text field. |
| `hotkey(...)` | a string (`"SHIFT+B"`, `"F12"`, `"ALT"`, `"NONE"`) | Capture: click the button, then press a key or a combination; a modifier alone is a key too; Backspace - `NONE`. To parse the string in your plugin - `shared/HoldKey.h` (`HoldKey::Parse`, `HoldKey::Held`). |
| `colour(...)` | a string (`"#RRGGBB"`) | A palette (25 colours, as in Mod Hub) and a Hex button to type the code; a default word (`auto`) gets its own palette button. |

- `sec` / `key` — ini section and key (`[sec]` / `key=`). Settings can live
  in different sections of one file.
- `tip` — tooltip shown when hovering over the label, may be `NULL`.
- `def` — default value: used when the key is missing from the ini, and by
  the "Reset to defaults" button.

**Flags**

| Flag | Meaning |
|---|---|
| `MCM_RESTART` | The value takes effect only after the game is restarted. An asterisk is added to the label and a note to the tooltip. |

---

## How MCM behaves

- **When `MCM_Describe` is called.** Once, when the game first creates the
  Options window (`OptionsWindow::create`). By then every RE_Kenshi plugin is
  loaded, so mod load order does not matter. MCM goes through the loaded DLLs
  and calls `MCM_Describe` in each one that has it. The call is wrapped in
  `__try`: if a plugin crashes there, MCM logs it and carries on.
- **Several pages.** Every `beginMod` starts a new page. One plugin may
  describe several mods if that suits it.
- **Reading.** Values are re-read from the ini each time a page is opened
  (`GetPrivateProfileStringA`). A trailing `; ...` comment is dropped from
  non-text values. Numbers are clamped to `min`/`max`.
- **Writing.** With `WritePrivateProfileStringA`: only that key's line
  changes; comments and other keys stay as they are. Check boxes and lists
  are written at once. A slider is written after it has been still for
  0.4 s. Text is written 0.8 s after the last edit. Anything still pending is
  written when the Options window closes.
- **`apply`.** Called after a write, on the game's main thread (from the
  Options window update). Re-read the ini there and apply what you can on the
  fly. Do not put heavy work there. If `apply` crashes, MCM logs it.
- **"Reset to defaults".** A button at the bottom of the page. It writes
  `def` to every key of the page and calls `apply`.
- **Ini without a BOM.** The `*PrivateProfile*A` functions read bytes as
  they are. If you need non-Latin text, store UTF-8 without a BOM.

---

## Tips

- **Apply whatever you can on the fly.** Mark with `MCM_RESTART` what needs
  a restart (hooks installed only at start-up, tables built once). A common
  trick: always install the hooks and check the `Enabled` flag inside them.
  Then the switch works without a restart.
- **The ini keys must match.** The keys you give MCM must be the ones your
  plugin reads, including the section's case.
- **Do not keep the `api` pointer.** It is valid only during the call.
- **Do not throw C++ exceptions across the DLL boundary.**
- **Use `api->version`.** If you need something from a newer API version,
  check `api->version`. `MCM_Api` fields are only ever added at the end; old
  ones never change.

---

## Settings without an ini: rows on functions (version 3)

If a plugin stores its settings itself (JSON, its own format) and validates
them itself, MCM can leave the files alone and call the plugin's functions.
Start the page with `beginMod` and `iniPath = NULL`, and use the `Fn` rows:

```cpp
static int __cdecl GetRadius(void* ud, int* out) { *out = g_radius; return 0; }
static int __cdecl SetRadius(void* ud, int v, char* err, unsigned errSize)
{
    if (v < 10) { strcpy_s(err, errSize, "radius too small"); return 1; }
    g_radius = v; SaveMyJson(); return 0;
}
...
api->beginMod(api, "MyPlugin", "My Plugin", NULL, NULL);
api->integerFn(api, Tr("Radius"), Tr("Search radius, m"), &GetRadius, &SetRadius, NULL, 10, 500, 0);
```

| Function | Value |
|---|---|
| `toggleFn`, `integerFn`, `choiceFn` | integer (`MCM_GetIntFn` / `MCM_SetIntFn`); `choiceFn` takes an array of numbers `values` and labels |
| `numberFn` | decimal (`MCM_GetFloatFn` / `MCM_SetFloatFn`) |
| `textFn`, `colourFn` | string (`MCM_GetTextFn` / `MCM_SetTextFn`) |
| `hotkeyFn` | key `MCM_Key { keycode (OIS::KeyCode, -1 - none), modifiers (MCM_KEY_CTRL/SHIFT/ALT) }`; shown as text `CTRL+SHIFT+B`, `NONE` |
| `action` | a button; the page is re-read after it is pressed |

Return 0 when the value is accepted, anything else to refuse it: the text in
`err` goes to the log, and the row shows what the getter returns. To give such a page a "Reset to
defaults" button, pass each row's default right after the row (version 5):
`if (api->version >= 5) api->defaultValue(api, "1");` - as a string MCM
understands: 0/1, a number, the number of a choice, text, a key `CTRL+B`, a
colour `#RRGGBB`. The reset hands it to your setter. The signatures are the same as Mod Hub's (EMC_*Callback):
for Mod Hub mods there is a ready bridge, `McmModHubBridge.h`.

## Your own area on the page (version 4)

When a plugin needs to show data rather than a setting (a list, a table), it
asks MCM for an area at the top of the page and draws into it itself:

```cpp
static void __cdecl Attach(void* ud, void* parent) { BuildMyList((MyGUI::Widget*)parent); }
static void __cdecl Detach(void* ud) { DestroyMyList(); }
...
if (api->version >= 4)
    api->customArea(api, &Attach, &Detach, NULL, 60);   // 60% of the page height
```

`attach` is called when the page is shown and every time the Options window
is opened again (re-read your data there); `detach` when the page is left -
the plugin removes its own widgets inside `parent`. The setting rows go
below the area.

Open the Options window right on your page (for example, from your own key):

```cpp
MCM_OpenPageFn open = (MCM_OpenPageFn)GetProcAddress(GetModuleHandleA("ModConfigMenu.dll"), "MCM_OpenPage");
if (open) open("MyPlugin");   // the id from beginMod
```

---

## Before you publish

- **Page title.** While your page is selected, the `title` from
  `beginMod` is shown large in the settings window caption, on a plate.
  A long title stretches the plate - keep it short.
- **Tooltips.** MCM drops a trailing full stop and escapes `#` itself
  (otherwise MyGUI would take `#RRGGBB` for a colour change). Use `\n`
  for a line break.
- **Characters.** The game fonts (especially in the Russian build, which
  replaces them) contain only ASCII and Cyrillic. An em dash `—`,
  guillemets `«»`, an ellipsis `…` and `№` are drawn as blanks. Write `-`,
  `"`, `...`, `No.`.
- **The dependency is optional.** In the Steam / Nexus description it is
  enough to say "in-game settings with ModConfigMenu (optional)". Do not
  add MCM to "Required items".
- **Check.** Set `Debug=1` in `ModConfigMenu.ini`: the log must contain
  `described by ...\YourPlugin.dll`. Then go through the page: change every
  row, close the window, open it again - the value is still there and in
  the ini.

## Debugging

Set `Debug=1` in `ModConfigMenu.ini`. Then `RE_Kenshi_log.txt` gets lines
like:

```
ModConfigMenu: described by ...\MyPlugin.dll
ModConfigMenu: 3 mod(s) with settings
ModConfigMenu: MyPlugin [MyPlugin] Radius=120
```

If your plugin is not in the list, check that `MCM_Describe` is exported
undecorated: `dumpbin /exports MyPlugin.dll` must show exactly
`MCM_Describe`.
