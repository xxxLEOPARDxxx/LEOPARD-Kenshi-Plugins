ITEM MARKERS - OPEN GRADES / SDK v1

РУССКИЙ

Два независимых способа добавить собственное качество без пересборки ItemMarkers:

1. ФАЙЛЫ ПРАВИЛ (DLL НЕ НУЖНА)
Положите профиль .ini в grades рядом с ItemMarkers.dll. Скопируйте
backpack-example.ini.disabled, укажите настоящие строковые ID из FCS и
переименуйте файл в .ini. Нажмите «Перезагрузить правила качества» в MCM
или перезапустите игру. ItemMarkers не изменяет характеристики предметов.

[Grade.любое-ASCII-имя] задаёт Rank=0..5 и Colour=#RRGGBB. Именованных
грейдов может быть больше шести: Rank - только порог отображения в MCM,
а не ID качества. Необязательный Palette=ItemGrade4 берёт цвет из текущей
палитры интерфейса; Colour остаётся запасным значением.

[Rule.любое-ASCII-имя] выбирает Grade из того же файла. Условия ItemID и
MaterialID сравниваются с точными строковыми ID, без переведённых названий.
Оба условия вместе означают И. Необязательный Type - числовой itemType из
Kenshi SDK (например ITEM=4, ARMOUR=3). Хотя бы один из ItemID, MaterialID,
Type обязателен. LevelMin/LevelMax разрешены только для настоящего штатного
Gear: рюкзак с неизвестным уровнем не считается уровнем 0. Priority от
-10000 до 10000: выше выигрывает. Равные приоритеты - первый профиль по
имени файла, затем первое правило. Имена секций и ключи чувствительны к регистру.

Лимиты: 64 файла по 64 КиБ, до 128 определений грейдов на файл, суммарно
512 правил; строка до 511 байт, ID до 127 байт, имя грейда до 63 байт.
Неизвестные ключи, дубли, неверные цвета и обрезанные файлы отклоняются.
Если хоть один файл ошибочен, предыдущий набор правил остаётся целиком.
После удаления профиля перезагрузите правила. Файлы не читаются каждый кадр.

2. ДИНАМИЧЕСКОЕ КАЧЕСТВО ЭКЗЕМПЛЯРА (C API)
ItemMarkersApi.h - публичный ABI без STL и зависимости от игровых заголовков.
Клиенту не нужна библиотека импорта. Найдите экспорт ItemMarkers_GetApi через
GetProcAddress. Передайте версию 1 и sizeof(IM_Api). Если ItemMarkers ещё не
загружен, повторите подключение позднее в своём GUI-обновлении; порядок модов
не должен влиять на поддержку. Смотрите компилируемый GradeProviderExample.cpp.

register_provider копирует описание; имя уникально, максимум 32 обработчика.
Приоритет API выше файлов; среди обработчиков выигрывает больший Priority,
при равном - более ранняя регистрация. Возврат 0 передаёт решение дальше,
1 возвращает Rank и RGB. Любой другой ответ, неверные данные, C++-исключение
или перехваченное SEH-исключение отключает обработчик до новой регистрации.
После API идут файлы правил, затем неизменённые штатные цвета Gear/Meitou.

IM_ItemView.item - временный Item* для чтения только во время callback.
ItemMarkers не знает устройство чужого качества: его читает владелец мода.
Нельзя хранить view, строки или Item* для последующего вызова. Не изменяйте
инвентарь, предметы, GUI или мир из callback. Он вызывается в потоке GUI;
должен быть быстрым, неблокирующим, без IO и ожидания других API-потоков.
Реестр сериализован: ожидание потока, который вызывает API, вызовет deadlock.
Регистрация/удаление из callback возвращает IM_BUSY. Контекст живёт у клиента.

notify_changed(handle) вызывайте, когда качество действительно поменялось,
не каждый кадр. Уведомления объединяются: видимые открытые иконки проверяются
не чаще раза в 0,4 с, неизменённые виджеты не пересоздаются. Новые и вновь
показанные иконки получают актуальное качество. Цвет, стиль и прозрачность
следуют настройкам пользователя. Результат API не меняет tooltip или stats.

При регистрации ItemMarkers удерживает DLL обработчика. Перед освобождением
контекста или выгрузкой DLL обязательно успешно unregister_provider.
Делайте это вне DllMain, после callback. IM_BUSY означает: отложить выгрузку,
контекст пока не освобождать. Это НЕ механизм hot reload игровых плагинов.
Все передаваемые указатели обязаны быть действительными; API не является
песочницей для недоверенного нативного кода. Не передавайте STL между DLL.

ENGLISH

CONFIGURATION-ONLY MODS
Place .ini profiles in grades beside ItemMarkers.dll; copy the disabled
backpack example, supply real FCS string IDs and rename it to .ini. Use the
MCM reload action or restart. Profiles only affect markers, never item stats.

[Grade.name] defines Rank=0..5 and Colour=#RRGGBB. There can be many named
grades; rank is the player's display threshold, not the grade identity.
Optional Palette=ItemGrade4 resolves the active UI theme, with Colour as fallback.
[Rule.name] references a Grade in that file. ItemID, MaterialID and optional
numeric Type (Kenshi itemType) are AND conditions; at least one is required.
LevelMin/LevelMax only match genuine supported native Gear, never nongear -1.
Priority -10000..10000 wins descending; ties use filename then section order.
Keys and section names are case-sensitive, ASCII; comments can be UTF-8.
Limits:64 files/64KiB each,128 grades per file,512 compiled rules total,
511-byte lines,127-byte item/material IDs,63-byte grade names. Invalid,
truncated, duplicated or unknown fields reject the whole reload transaction;
the previous rules remain active. Deleted profiles disappear after reload.
There is no per-frame filesystem/INI polling.

RUNTIME PER-INSTANCE PROVIDERS
Discover ItemMarkers_GetApi with GetProcAddress; request version1 and
sizeof(IM_Api). Retry later if load order means it is not available yet.
Use ItemMarkersApi.h and the compilable GradeProviderExample.cpp fragment.
No import library, STL ABI or game headers are required by the API itself.

register_provider copies the descriptor,32 providers max, unique name.
Providers precede file rules, then unchanged native Gear/Meitou handling.
Higher priority wins; ties use registration order. Return0 to pass,1 with
valid size/rank/RGB to handle. Invalid replies or caught C++/SEH faults
disable that provider until re-registration, with an always-visible error.

Callbacks run only on the GUI thread. All view/item/string pointers are
borrowed for this call. Never retain them. Your mod owns its instance-quality
logic; ItemMarkers must not cast backpacks to armour/weapons. Do not change
inventory, items, GUI or world during a callback. No blocking/IO, no waiting
for another thread using this API: the registry is serialized. Reentrant
registration/unregistration returns IM_BUSY. Keep caller context alive.

Notify only actual grade changes. Notifications coalesce into a400ms visible
icon check. Unchanged appearances do not recreate widgets. Newly created or
shown icons use current results. Player marker style/opacity/minimum rank
still apply. Item stats and tooltip text are the owning mod's responsibility.

The provider DLL is held while registered. Successfully unregister OUTSIDE
DllMain before freeing context or unloading your module. IM_BUSY means defer;
do not free/unload yet. This is not general hot reload for game plugins.
Pointers supplied to the API must be valid. Native providers are trusted code,
not a sandbox. Do not pass STL objects across DLL boundaries.
