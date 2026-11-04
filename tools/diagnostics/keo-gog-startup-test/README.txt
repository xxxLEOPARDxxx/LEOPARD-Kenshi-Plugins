KEO GOG STARTUP TEST - ПРОВЕРКА ЗАПУСКА И MCM
Тестовая сборка 1, 05.11.2026. НЕ БОЕВАЯ ВЕРСИЯ KEO.

Это настоящий RE_Kenshi-плагин для проверки загрузки DLL, KenshiLib и страницы
MCM на GOG. Все оптимизации KEO ВЫКЛЮЧЕНЫ. Патфаиндинг, спавн NPC, потоки,
изменение игровых функций и исправления KEO в этой DLL отсутствуют.
Успешный запуск НЕ означает, что перенос полного KEO на GOG завершён.

ТРЕБУЕТСЯ
- Ваша законная GOG Kenshi 1.0.65.
- Установленные RE_Kenshi с KenshiLib 0.5.0 или совместимой новее и Mod Config Menu.
- Если игра запускается через RE_Kenshi-лаунчер, продолжайте запускать через него.
Ни EXE игры, ни чужие DLL заменять не нужно.

УСТАНОВКА И ПРОВЕРКА
1. Закройте игру. На время чистой проверки выключите другие версии KEO
   в списке модов, не удаляя их папки и личные настройки.
2. Скопируйте папку KEOGogStartupTest из архива в Kenshi/mods/.
   Должен получиться Kenshi/mods/KEOGogStartupTest/KEOGogStartupTest.mod.
3. Включите KEOGogStartupTest в лаунчере и запустите игру через RE_Kenshi.
4. Откройте Настройки -> MCM -> KEO GOG Startup Test.
5. На странице должны быть статус успешного запуска DLL/KenshiLib,
   строка версии платформы и явная надпись «Оптимизации: ВЫКЛ».
   Выберите Русский или 中文 в Language при необходимости.
6. Включите последнюю настройку «Подробный журнал». Это запишет два сообщения
   KEO GOG Startup Test в RE_Kenshi_log.txt. Перезапуск для переключателя не нужен.
7. Сообщите, дошла ли игра до меню/сохранения и появилась ли эта страница.
   Пришлите скрин страницы и RE_Kenshi_log.txt. Если мод не загрузился,
   особенно важен этот журнал и точный текст ошибки.

Язык страницы и подробный журнал хранятся только в памяти этого теста.
INI основного KEO не читается и не меняется. После перезапуска журнал выключен.
Удаление: выключить KEOGogStartupTest, закрыть игру и убрать только его папку.
Сохранения, экономика, NPC и мир этот тест не изменяет.

Полный KEO создан jacobericson / Jakematt2004. Спасибо автору оригинала.
Оригинальные исходники: https://github.com/jacobericson/KEO
Эта маленькая тестовая DLL не содержит движковых оптимизаций оригинала.

-----------------------------------------------------------------------
KEO GOG STARTUP TEST - DLL / SDK / MCM CHECK
Test build 1, 2026-11-05. NOT A PRODUCTION KEO BUILD.

This is a real RE_Kenshi plugin to test DLL loading, KenshiLib and MCM on GOG.
All KEO optimizations are OFF. It contains no pathfinding/spawn fixes, workers,
engine hooks or game-function patches. Successful startup does NOT certify a
completed GOG port of the full KEO plugin.

REQUIREMENTS
- Your legitimate GOG Kenshi 1.0.65.
- Installed RE_Kenshi with KenshiLib 0.5.0 or compatible newer, and Mod Config Menu.
- Launch through your normal RE_Kenshi launcher. Replace no game EXE or SDK DLL.

INSTALL / TEST
1. Close Kenshi. Disable other KEO versions for this isolated test; keep their
   folders and personal settings.
2. Copy the KEOGogStartupTest folder into Kenshi/mods/.
3. Enable KEOGogStartupTest in the launcher and start Kenshi through RE_Kenshi.
4. Open Options -> MCM -> KEO GOG Startup Test.
5. Check for DLL/KenshiLib startup success, platform/version and the explicit
   Optimizations: OFF message. Language offers English, Russian and Chinese.
6. Enable the final Detailed log option. It writes two startup-test messages
   to RE_Kenshi_log.txt. No restart is needed for this switch.
7. Report whether the game reaches its menu/save and this MCM page appears.
   Send a page screenshot and RE_Kenshi_log.txt. If loading fails, include the
   exact error text and the log.

Page language/debug values are memory-only and reset next launch. Main KEO INI
is never read or written. To uninstall: disable the test mod, close the game,
remove only KEOGogStartupTest. No save, NPC or world changes are made.

Original full KEO author: jacobericson / Jakematt2004. Thank you to the author.
Original sources: https://github.com/jacobericson/KEO
This small startup-test DLL contains none of the original engine optimizations.
