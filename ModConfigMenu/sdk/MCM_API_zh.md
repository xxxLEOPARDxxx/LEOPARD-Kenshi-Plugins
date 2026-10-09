# ModConfigMenu (MCM) — 为你的插件添加设置页面

MCM 会在游戏的“设置”窗口中添加一个 **MCM** 标签页。左侧是模组列表；右侧是所选模组的
设置，使用游戏自带的选项行（DatapanelGUI）绘制：复选框、带数字输入框的滑块、下拉列表和
文本框。玩家修改数值后，MCM 会把它写入你插件的 ini 文件，并调用你的插件中重新读取设置的
函数。

你的插件**不依赖** MCM。没有 MCM 时，插件照常工作，读取自己的 ini。如果安装了 MCM，它会
自动找到你的插件并为其生成设置页面。

---

## 前提条件

- 基于 KenshiLib 的 RE_Kenshi 插件（x64 DLL，VC10/v100，/MD），与平常一样。
- 设置保存在 ini 文件中，通过 `GetPrivateProfile*` 读取。
- 一个头文件：`ModConfigMenu.h`（与本文件放在一起）。无需链接任何库：由 MCM 调用你的
  插件，你的插件从不调用 MCM。

---

## 三个步骤

### 1. 包含头文件

```cpp
#include "ModConfigMenu.h"
```

### 2. 提供一个重新读取 ini 的函数

你很可能已经有这样的函数：启动时读取 ini 的那个。需要两处修改：

- 调用约定必须是 `__cdecl`（在 x64 上本来就只有这一种，但指针类型必须一致）；
- 函数必须是**幂等的**：再次读取前，清空会累积的内容（列表、集合）。

```cpp
void __cdecl LoadSettings()
{
    const std::string ini = IniPath();
    g_enabled = GetPrivateProfileIntA("MyPlugin", "Enabled", 1, ini.c_str()) != 0;
    g_radius  = GetPrivateProfileIntA("MyPlugin", "Radius", 50, ini.c_str());
    g_words.clear();          // 清空会累积的内容
    // ...
}
```

### 3. 导出 `MCM_Describe`

名称必须正好是 `MCM_Describe`，且不能被修饰，因此需要 `extern "C"`：

```cpp
extern "C" __declspec(dllexport) void MCM_Describe(MCM_Api* api)
{
    static const std::string ini = IniPath();   // 必须在调用期间保持有效
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

完成。重新编译插件，打开“设置”中的 **MCM** 标签页即可。

---

## 参考

所有字符串均为 UTF-8。MCM 在调用期间会复制它们，你无需保留。请传入已翻译的文字（用你
自己的 `Tr()` 或类似函数）。游戏语言只有重启后才会改变，因此翻译一次即可。

| 函数 | 写入 ini 的内容 | 控件 |
|---|---|---|
| `beginMod(api, id, title, iniPath, apply)` | — | 开始一个模组页面。`id` — 拉丁字母，唯一。`title` — 列表中显示的名称：**模组的英文原名**，不翻译。如需翻译名称，请将译文放入 MCM 自己的目录（`locale/<语言>/LC_MESSAGES/mod_config_menu.po`，msgid 即名称）。`iniPath` — ini 的**完整**路径。`apply` — 重新读取函数，可以为 `NULL`。 |
| `info(api, text)` | — | 版本 2 起提供。简要说明模组的作用：鼠标悬停在列表中的模组上时，在窗口底部显示的提示。需传入已翻译的文字。 |
| `section(api, title)` | — | 页面内的小标题。 |
| `toggle(api, sec, key, label, tip, def, flags)` | `0` / `1` | 复选框。 |
| `integer(api, sec, key, label, tip, def, min, max, flags)` | 整数 | 带数字输入框的滑块。 |
| `number(api, sec, key, label, tip, def, min, max, decimals, flags)` | 小数，`decimals` 位，小数点为“.” | 带数字输入框的滑块。 |
| `choice(api, sec, key, label, tip, def, values, labels, count, flags)` | `values` 之一 | 显示当前选项的按钮：点击 - 下一个，SHIFT+点击 - 上一个（游戏内的下拉列表无法传回选择）。`labels`（显示的文字）可以为 `NULL`，此时直接显示 `values`。 |
| `text(api, sec, key, label, tip, def, flags)` | 原样字符串 | 文本框。 |
| `hotkey(...)` | 字符串（`"SHIFT+B"`、`"F12"`、`"ALT"`、`"NONE"`） | 按键捕获：点击按钮，然后按下按键或组合键；单独的修饰键也算按键；Backspace - `NONE`。在插件中解析字符串 - `shared/HoldKey.h`（`HoldKey::Parse`、`HoldKey::Held`）。 |
| `colour(...)` | 字符串（`"#RRGGBB"`） | 调色板（25 种颜色，与 Mod Hub 相同）及 Hex 按钮用于输入代码；默认词（`auto`）在调色板中单独成按钮。 |

- `sec` / `key` — ini 的节和键（`[sec]` / `key=`）。设置可以分布在同一文件的不同节中。
- `tip` — 鼠标悬停在标签上时显示的提示，可以为 `NULL`。
- `def` — 默认值：ini 中缺少该键时使用，“恢复默认”按钮也使用它。

**标志**

| 标志 | 含义 |
|---|---|
| `MCM_RESTART` | 数值只有在重启游戏后才生效。标签后会加上星号，提示中会加上说明。 |

---

## MCM 的行为

- **何时调用 `MCM_Describe`。** 只调用一次：在游戏第一次创建设置窗口时
  （`OptionsWindow::create`）。此时所有 RE_Kenshi 插件都已加载，因此模组加载顺序无关
  紧要。MCM 遍历已加载的 DLL，对每个包含该函数的 DLL 调用 `MCM_Describe`。调用被包在
  `__try` 中：如果插件在其中崩溃，MCM 会记录日志并继续运行。
- **多个页面。** 每次调用 `beginMod` 都会开始一个新页面。如果需要，一个插件可以描述
  多个模组。
- **读取。** 每次打开页面时都会重新从 ini 读取数值（`GetPrivateProfileStringA`）。非文本
  数值末尾的 `; ...` 注释会被去掉。数字会被限制在 `min`/`max` 之间。
- **写入。** 使用 `WritePrivateProfileStringA`：只修改该键所在的行，注释和其他键保持
  不变。复选框和列表立即写入。滑块在静止 0.4 秒后写入。文本在最后一次编辑 0.8 秒后写入。
  关闭设置窗口时，所有尚未写入的内容都会被写入。
- **`apply`。** 在写入之后、在游戏主线程中（设置窗口的更新中）调用。请在其中重新读取 ini，
  并即时应用能应用的内容。不要在其中做耗时的工作。如果 `apply` 崩溃，MCM 会记录日志。
- **“恢复默认”。** 页面底部的按钮。它把 `def` 写入页面的所有键，然后调用 `apply`。
- **ini 不要带 BOM。** `*PrivateProfile*A` 函数按原样读取字节。如果需要非拉丁文字，请使用
  不带 BOM 的 UTF-8 保存。

---

## 建议

- **能即时生效的都即时生效。** 需要重启才能生效的内容（只在启动时安装的钩子、只构建一次
  的表）请标记 `MCM_RESTART`。常用技巧：总是安装钩子，在钩子内部检查 `Enabled` 标志。这样
  开关无需重启即可生效。
- **ini 的键必须一致。** 传给 MCM 的键必须与插件读取的键相同，包括节名的大小写。
- **不要保存 `api` 指针。** 它只在调用期间有效。
- **不要让 C++ 异常跨越 DLL 边界。**
- **使用 `api->version`。** 如果需要较新版本 API 中的功能，请检查 `api->version`。`MCM_Api`
  的字段只会追加到末尾，旧字段永远不变。

---

## 不用 ini 的设置：基于函数的行（版本 3）

如果插件自己保存设置（JSON 或自定义格式）并自行校验，MCM 可以不读写文件，而是调用插件的函数。
用 `beginMod` 开始页面时传入 `iniPath = NULL`，并使用带 `Fn` 的行：

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

| 函数 | 值 |
|---|---|
| `toggleFn`、`integerFn`、`choiceFn` | 整数（`MCM_GetIntFn` / `MCM_SetIntFn`）；`choiceFn` 接收数值数组 `values` 和标签 |
| `numberFn` | 小数（`MCM_GetFloatFn` / `MCM_SetFloatFn`） |
| `textFn`、`colourFn` | 字符串（`MCM_GetTextFn` / `MCM_SetTextFn`） |
| `hotkeyFn` | 按键 `MCM_Key { keycode（OIS::KeyCode，-1 表示无）, modifiers（MCM_KEY_CTRL/SHIFT/ALT） }`；以文字显示：`CTRL+SHIFT+B`、`NONE` |
| `action` | 按钮；按下后页面会重新读取 |

返回 0 表示接受，其他值表示拒绝：`err` 中的文字写入日志，该行显示 getter 返回的值。
要让此类页面拥有“恢复默认”按钮，请在每一行之后传入其默认值（版本 5）：
`if (api->version >= 5) api->defaultValue(api, "1");`——以 MCM 能理解的字符串表示：0/1、数字、选项的数值、文字、
按键 `CTRL+B`、颜色 `#RRGGBB`。恢复默认时会通过你的 setter 写入。
函数签名与 Mod Hub（EMC_*Callback）相同：基于 Mod Hub 的模组可直接使用现成的桥接 `McmModHubBridge.h`。

## 页面上的自定义区域（版本 4）

如果插件要显示的不是设置而是数据（列表、表格），可以向 MCM 申请页面顶部的一块区域，自行绘制：

```cpp
static void __cdecl Attach(void* ud, void* parent) { BuildMyList((MyGUI::Widget*)parent); }
static void __cdecl Detach(void* ud) { DestroyMyList(); }
...
if (api->version >= 4)
    api->customArea(api, &Attach, &Detach, NULL, 60);   // 占页面高度的 60%
```

显示页面时以及每次重新打开设置窗口时都会调用 `attach`（可在此重新读取数据）；离开页面时调用 `detach`——
插件需自行移除 `parent` 中的控件。设置行显示在该区域下方。

直接打开设置窗口并定位到自己的页面（例如通过自己的快捷键）：

```cpp
MCM_OpenPageFn open = (MCM_OpenPageFn)GetProcAddress(GetModuleHandleA("ModConfigMenu.dll"), "MCM_OpenPage");
if (open) open("MyPlugin");   // beginMod 中的 id
```

---

## 发布之前

- **页面标题。** 选中你的页面时，`beginMod` 中的 `title` 会以大字显示在
  设置窗口的标题栏里（带底板）。标题过长会拉长底板，请尽量简短。
- **提示文字。** 末尾的句号由 MCM 自动去掉，`#` 也由 MCM 自动转义（否则
  MyGUI 会把 `#RRGGBB` 当作颜色切换）。换行用 `\n`。
- **字符。** 游戏字体（尤其是替换了字体的俄文版）只包含 ASCII 和西里尔
  字母。长破折号 `—`、书名号 `«»`、省略号 `…`、`№` 会显示为空白。请改用
  `-`、`"`、`...`、`No.`。（中文版字体另有中文字符。）
- **依赖是可选的。** 在 Steam / Nexus 的描述中写明“游戏内设置需要
  ModConfigMenu（可选）”即可，不要把 MCM 加进 “Required items”。
- **检查。** 在 `ModConfigMenu.ini` 中设置 `Debug=1`：日志里应出现
  `described by ...\YourPlugin.dll`。然后逐行检查页面：修改每一项，关闭
  窗口再打开——数值仍在，并已写入 ini。

## 调试

在 `ModConfigMenu.ini` 中设置 `Debug=1`。之后 `RE_Kenshi_log.txt` 中会出现如下行：

```
ModConfigMenu: described by ...\MyPlugin.dll
ModConfigMenu: 3 mod(s) with settings
ModConfigMenu: MyPlugin [MyPlugin] Radius=120
```

如果你的插件不在列表中，请检查 `MCM_Describe` 是否以未修饰的名称导出：
`dumpbin /exports MyPlugin.dll` 应该正好显示 `MCM_Describe`。
