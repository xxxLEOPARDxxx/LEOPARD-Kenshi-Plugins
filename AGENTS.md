# Kenshi: стенд разработки

Перед работой прочитать C:/Users/User/.codex/memories/kenshi/KENSHI_CONTEXT.md. Свои исходники - ours; форки - forks/LEOPARD-Kenshi-Forks; игра - D:/SteamLibrary/steamapps/common/Kenshi.

## Подготовка выпуска модов (прямые указания пользователя, 29.10.2026)

- У каждого мода/форка актуальный README в исходниках и пакете; перед выпуском обновлять описания Nexus и Steam по принятому стилю существующих модов. Steam-описание агент готовит самостоятельно.
- В релизе один рабочий каталог на язык. Для модов на Localization.h - только .mo; .po остаётся в исходниках. Не класть .po и .mo рядом в архиве. У upstream-плагинов с PO-only загрузчиком сохранять требуемый .po до миграции.
- После успешного создания и проверки нового ZIP удалять старые версии этого же мода из workshop/nexus, не трогая архивы других модов.
- Новому моду/форку сразу создавать workshop/images/<точное имя мода> для скриншотов. Изображения не выдумывать и не подменять пустышками.
- Единый проход подготовки: bundled Python -> D:/DEV/Kenshi/workshop/prepare_release.py <Mod ...>. Он синхронизирует README, создаёт папки изображений, обновляет Steam/Nexus описания и ZIP. Все пакеты проверить по CRC и составу, личные INI/99_UserOverride/журналы не раздавать.
- Локализация ru_RU/en_GB/zh_CN. Подробный журнал всегда последней настройкой. MCM_RESTART и звёздочка только для реального требования перезапуска.
- Сначала пользователь загружает Nexus и присылает ссылки, затем агент загружает соответствующие версии в Steam. До этого публикаций нет.

## Git and continued work (user,29.10.2026)

Commit tested changes by coherent scope and push requested commits to origin. Keep unfinished DynamicFactions changes separate; this work will continue together with the user. Version release-tool sources in ours/tools/workshop and synchronize their working copy in D:/DEV/Kenshi/workshop. Git push permission does not replace the Nexus-first/Steam-after-links release order.
