KEO GOG - ДИАГНОСТИКА СОВМЕСТИМОСТИ / COMPATIBILITY DIAGNOSTICS
Диагностический пакет 1 / Diagnostic package 1

ЭТО НЕ РАБОЧАЯ GOG-ВЕРСИЯ KEO И НЕ ИСПРАВЛЕНИЕ ИГРЫ.
Этот небольшой пакет нужен, чтобы подготовить настоящий перенос KEO на GOG.
Он не устанавливает плагин и не проверяет игру на FPS/поведение NPC.

КАК ЗАПУСТИТЬ
1. Полностью распакуйте ZIP в отдельную папку, можно рядом с Kenshi_x64.exe.
   Не запускайте из окна архиватора. Игра может быть закрыта.
2. Откройте Collect-GOG-Profile.cmd. Python и права администратора не нужны.
   Если игра рядом не найдена, вставьте путь к папке вашей GOG Kenshi.
   Либо перетащите папку игры на CMD-файл.
3. Откройте созданную подпапку reports и передайте автору только новый JSON.
   Ничего не отправляется автоматически. Сам EXE, DLL и сохранения не копируются.

ЧТО ЧИТАЕТСЯ
- Kenshi_x64.exe и, если существует, RE_Kenshi/Kenshi_x64.exe:
  SHA256, MD5, размер и заголовки PE64.
- RE_Kenshi/RVAs/GOG_1.0.65.br: размер, число записей и SHA256.
- KenshiLib.dll, если есть: только размер и SHA256 для сверки пары с таблицей.
- Только на распознанном GOG 1.0.65: короткие 32-байтовые окна инструкций
  по уже известным официальным адресам. Неизвестные адреса не угадываются.

Скрипт ничего не меняет в папке игры, не запускает её и не подключается к её
процессу. Не скачивает и не отправляет файлы. Создаётся только новый JSON
в папке reports рядом со скриптом. Раньше созданные отчёты не перезаписываются.
PowerShell ExecutionPolicy Bypass применяется только к этому локальному запуску;
постоянные настройки системы не меняются.

СТАТУСЫ
- gog_image_and_known_table_identified: найдены известные EXE и GOG-таблица.
  Это ещё НЕ означает работоспособность KEO на GOG.
- required_gog_inputs_unverified: версия EXE/таблицы неизвестна или это Steam.
- diagnostic_input_refused: не найден файл, неверный формат либо другая ошибка.
Во всех случаях пришлите JSON, если он создан. Он не содержит сохранений,
игровых персонажей и учётных данных. Просмотрите отчёт перед передачей.
Таблица точек — исторический срез KEO 5772425, а не полный профиль новых сборок.
Поддерживаемая целевая версия: GOG Kenshi 1.0.65. Для нового EXE потребуется
новый проверенный профиль. Не подменяйте EXE и не отключайте защиту KEO.

-----------------------------------------------------------------------

THIS IS NOT A WORKING GOG BUILD OF KEO OR A GAME FIX.
This small package collects evidence needed for the GOG port. It installs
no plugin and does not test FPS, AI or NPC spawning.

HOW TO RUN
1. Extract the entire ZIP to a separate folder, optionally next to
   Kenshi_x64.exe. Do not run it inside the archive viewer. Kenshi may be closed.
2. Run Collect-GOG-Profile.cmd. No Python or administrator rights are needed.
   If the nearby game is not found, enter your GOG Kenshi game directory.
   Alternatively, drag the game folder onto the CMD file.
3. Send only the new JSON file from the reports subfolder to the maintainer.
   Nothing is uploaded automatically. EXE/DLL/save files are not copied.

WHAT IT READS
- Kenshi_x64.exe and RE_Kenshi/Kenshi_x64.exe, if present: hashes, size, PE64.
- RE_Kenshi/RVAs/GOG_1.0.65.br: size, entry count and SHA256.
- KenshiLib.dll, if present: size/SHA256 only, to identify its table pairing.
- Only for a recognized GOG 1.0.65 executable: 32-byte instruction windows
  at already known official addresses. Unknown addresses are never guessed.

The script changes no game file, starts no game and accesses no running game
process. It downloads/uploads nothing. Only a new JSON is created next to the
script in reports; existing reports are never overwritten. ExecutionPolicy
Bypass applies only to this local PowerShell launch, not system settings.

RESULTS
- gog_image_and_known_table_identified: known GOG image/table identified.
  This does NOT certify KEO compatibility.
- required_gog_inputs_unverified: unknown executable/table or a Steam image.
- diagnostic_input_refused: missing file, malformed input or another error.
Send the JSON in every case if created. It contains no saves, characters or
account credentials; review it before sending. The address list is the historical 5772425 snapshot, not a full profile of later builds.
Target version: GOG Kenshi
1.0.65. A changed executable requires another audited profile. Do not replace
the game executable or bypass KEO's build gate.
