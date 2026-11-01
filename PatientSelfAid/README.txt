PatientSelfAid — пациент лечит себя сам, пока его лечат

ИЗМЕНЕНИЯ ТЕКУЩЕЙ ВЕРСИИ 4 (29–30.10.2026)

  * Биологический персонаж с повреждёнными протезами теперь может
    помогать ремонтировать себя своим ремкомплектом, пока его лечит
    другой герой. Проверяется медицинская потребность роботизированных
    частей, а не только правила выбора цели у ИИ скелетов.
  * Обычная первая помощь сохраняет приоритет: если есть раны и аптечка,
    сначала отдаётся приказ лечить раны; иначе — чинить роботизированные
    части при наличии ремкомплекта. Один износ, для которого нужна
    ремкровать, не запускает бесполезное самолечение.
  * Условия сохранены: оба персонажа принадлежат игроку, пациент стоит
    и ждёт лекаря, находится в сознании и на ногах. Занятые, дерущиеся,
    уже лечащие себя или других пациенты не получают новый приказ.
    Приказ отдаётся штатным способом; максимум три попытки за одно
    непрерывное лечение, с паузой в две секунды между попытками.
  * Проверка идёт раз в 0,5 с. Цели и позиции для поиска лекаря рядом
    читаются один раз за проход и переиспользуются. Выключенный мод
    сразу выходит из своего обновления, диагностические строки при
    Debug=0 не строятся. Кэш использует дескрипторы с серийным номером,
    удаляет завершённые лечения и сбрасывается при загрузке, новой игре,
    смене настроек и отсутствии мира. Рабочие буферы переиспользуются,
    лишняя ёмкость после сокращения отряда освобождается.
  * Обе настройки MCM применяются сразу. Подробный журнал — последняя
    строка, выключен по умолчанию (Debug=0); ошибки пишутся всегда.
    Личный PatientSelfAid.ini обновление не заменяет.
  * Переводы ru_RU/en_GB/zh_CN; в пакете .mo, исходные .po в репозитории.

VERSION 4 CHANGES (29–30 OCTOBER 2026)

  * Biological characters with damaged prosthetics can now use their
    own repair kit to help repair themselves while another character
    treats them. Robot first-aid need is checked through the medical
    system as well as the AI's robot-target rules.
  * Ordinary wounds take priority when first aid and a medkit are
    available; otherwise damaged robotic parts can receive a repair
    order if a repair kit is available. Wear that requires a repair bed
    alone does not trigger a useless self-treatment order.
  * Both characters must belong to the player. The patient must be
    conscious, standing and waiting for the medic. Busy patients,
    fighters and characters already treating themselves or others are
    left alone. Native orders are limited to three attempts per
    continuous treatment, with a two-second retry interval.
  * The half-second scan reuses each pass's goals and positions.
    Disabled updates exit immediately; Debug=0 does not build log text.
    Treatment state uses serial-bearing handles, removes finished
    treatments and resets on load, new game, settings changes or no
    world. Buffers are reused and excess capacity is released when
    the squad shrinks.
  * Both MCM settings apply immediately. Detailed log is last and off
    by default (Debug=0); errors are always logged. Updates preserve
    PatientSelfAid.ini. UI translations: ru_RU/en_GB/zh_CN; releases
    contain .mo catalogs, with .po sources kept in the repository.

СПРАВОЧНИК И ИСТОРИЯ

Требуется RE_Kenshi.


ЧТО ДЕЛАЕТ

Когда один персонаж оказывает первую помощь другому, лечащийся в
обычной игре просто стоит и ждёт: лекарь велит ему не двигаться. Даже
если в сумке у пациента своя аптечка.

С плагином пациент в это время лечит себя сам, а скелет или биологический персонаж с повреждёнными протезами
чинит роботизированные части своим ремкомплектом. Лекарь и пациент работают над одним телом
одновременно, каждый своим навыком, так что лечение идёт быстрее.

Это ровно то, что можно сделать руками: выделить пациента и указать
курсором лекаря на него самого. Плагин делает это за вас.


КОГДА СРАБАТЫВАЕТ

Все условия сразу:

  * и лекарь, и пациент — ваши персонажи;
  * лекарь лечит пациента — по приказу или работой медика;
  * пациент стоит и ждёт лекаря, а не занят чем-то другим;
  * пациент в сознании и стоит на ногах;
  * ему есть что лечить, и у него есть аптечка или ремкомплект для роботизированных частей.

Пациента, который сам кого-то лечит или дерётся, плагин не трогает.


ЕСЛИ НЕ РАБОТАЕТ

Включите Debug=1 в PatientSelfAid.ini и посмотрите RE_Kenshi_log.txt:
там по каждому пациенту видно, почему приказ не отдан или не прижился.
Если игра упорно сбрасывает приказ, плагин отступается после трёх
попыток за одно лечение.


КАК СДЕЛАНО

Раз в полсекунды плагин перебирает отряд игрока. Лекарь опознаётся по
текущей цели (FIRST_AID_ORDER, FIRST_AID_ROBOT, JOB_MEDIC,
JOB_REPAIR_ROBOT). Пациента он находит по цели, выполняемой подзадаче
или, как запасной вариант, по соседству:

  * по цели лекаря — так видно лечение по приказу;
  * по текущей подзадаче медика или ремонта роботов — пациент лежит
    в currentSubTarget выполняемой задачи;
  * по соседству — персонаж стоит смирно (STAND_STILL), а в пределах
    6 метров работает лекарь из отряда. Это запасной путь, если цель и
    текущая подзадача не дали пациента. Приказ «лечить себя» отдаётся
тем же вызовом, что и
клик игрока (OrdersReceiver::addOrder), и из того же места — из
обновления интерфейса.


НАСТРОЙКИ В ИГРЕ

Если установлен мод ModConfigMenu, все настройки есть прямо в игре:
окно «Настройки», вкладка MCM, страница «Patient Self Aid». У каждой строки
подсказка внизу окна, изменения сразу пишутся в ini и применяются.
Выключатель и подробный журнал применяются без перезапуска игры.

ФАЙЛ НАСТРОЕК И ОБНОВЛЕНИЯ МОДА

PatientSelfAid.ini — ваши настройки: обновление мода его не трогает.
В моде лежит PatientSelfAid.defaults.ini — значения по умолчанию с пояснениями.
Нет PatientSelfAid.ini — плагин при запуске сделает его копией defaults.
Появились в обновлении новые настройки — плагин допишет их в ваш файл
со значениями по умолчанию, остальное оставит как было.

29.10.2026: учтены повреждённые протезы у биологических персонажей.
Износ, устраняемый только ремкроватью, не считается поводом лечиться аптечкой.

29.10.2026, проверка производительности: выключенный мод сразу выходит
из хука. Раз в полсекунды цели и позиции для поиска соседнего лекаря
читаются однократно и переиспользуются. При выключенном Debug текст
журнала не строится. Кэш текущих пациентов хранится по игровым
дескрипторам с серийным номером, удаляет завершённые лечения на ближайшем
проходе и сбрасывается при загрузке, новой игре, смене настроек или
отсутствии игрового мира. Буферы переиспользуются, лишняя ёмкость после
сокращения отряда освобождается. Три попытки на одно непрерывное лечение
и пауза в две секунды между ними сохранены.


Localization diagnostics / Журнал локализации (2026-11-01)
Locale and catalogue-size summaries are written only while this plugin's Detailed log is enabled. Enabling it after translations have loaded reports the existing catalogue once. Loading/format errors remain visible with detailed logging off.
Сводка языка и числа переводов записывается только при включённом подробном журнале этого плагина. Включение журнала после загрузки переводов выводит сводку существующего каталога один раз. Ошибки загрузки и формата видны и при выключенном подробном журнале.

Prepared release 5 / Подготовленный выпуск 5 (2026-11-02)

English

- Optional language diagnostics. Language and translation-catalogue summaries follow this plugin's Detailed log setting, which is off by default.
- Useful errors remain visible. Translation loading and format errors are still reported with Detailed log disabled. Enabling the setting after loading reports the saved summary once without reloading translations.
- Clean update package. English, Russian and Simplified Chinese each use one compiled runtime catalogue. Personal settings and gameplay saves are preserved.

Русский

- Необязательная диагностика языка. Сводки языка и каталога переводов подчиняются настройке подробного журнала этого плагина, выключенной по умолчанию.
- Ошибки остаются видимыми. Ошибки загрузки и формата переводов записываются и при выключенном подробном журнале. Включение настройки после загрузки выводит сохранённую сводку один раз без перезагрузки переводов.
- Чистый пакет обновления. Английский, русский и упрощённый китайский используют по одному скомпилированному рабочему каталогу. Личные настройки и игровые сохранения сохраняются.
