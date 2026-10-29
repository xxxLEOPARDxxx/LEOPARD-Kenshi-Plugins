# -*- coding: utf-8 -*-
"""Загрузка наших модов в Steam Workshop Kenshi без FCS.

Через steam_api64.dll из папки игры (плоский API), от имени запущенного
и залогиненного Steam. Версии интерфейсов - те же, что у самой игры
(kenshi_x64.exe): STEAMUGC_INTERFACE_VERSION008, SteamUtils008,
SteamClient017.

    python steam_upload.py check                 - только подключиться и прочитать
    python steam_upload.py upload <Mod> [...]    - создать/обновить предметы
    python steam_upload.py refresh <Mod> [...]   - файлы и описания без записи в «Изменениях»

Данные мода - workshop\\<Mod>\\ (build.py): content\\, preview.png,
screens\\, description.txt, item.json. ID созданного предмета пишется в
item.json, и следующий запуск обновляет тот же предмет. Рядом с модом
в Kenshi\\mods\\<Mod>\\ кладётся _<Mod>.info и _<Mod>.img в формате FCS,
чтобы при желании обновлять и из FCS.
"""
import ctypes
import datetime
import io
import json
import os
import shutil
import sys
import time

KENSHI = r"D:\SteamLibrary\steamapps\common\Kenshi"
HERE = os.path.dirname(os.path.abspath(__file__))
APPID = 233860

c_u64 = ctypes.c_uint64
c_bool = ctypes.c_bool
c_char_p = ctypes.c_char_p
c_void_p = ctypes.c_void_p
c_int = ctypes.c_int
c_uint = ctypes.c_uint32


class CreateItemResult(ctypes.Structure):          # k_iCallback 3403
    _pack_ = 8
    _fields_ = [("result", ctypes.c_int32), ("id", c_u64), ("needs_agreement", c_bool)]


class SubmitItemUpdateResult(ctypes.Structure):    # k_iCallback 3404 (UGC 008)
    _pack_ = 8
    _fields_ = [("result", ctypes.c_int32), ("needs_agreement", c_bool)]


class ParamStringArray(ctypes.Structure):
    _fields_ = [("strings", ctypes.POINTER(c_char_p)), ("count", ctypes.c_int32)]


class Steam(object):
    def __init__(self):
        os.chdir(KENSHI)   # там steam_appid.txt = 233860
        self.dll = d = ctypes.CDLL(os.path.join(KENSHI, "steam_api64.dll"))

        def fn(name, res, *args):
            f = getattr(d, name)
            f.restype = res
            f.argtypes = list(args)
            return f

        self.Init = fn("SteamAPI_Init", c_bool)
        self.Shutdown = fn("SteamAPI_Shutdown", None)
        self.RunCallbacks = fn("SteamAPI_RunCallbacks", None)
        self.SteamClient = fn("SteamClient", c_void_p)
        self.HUser = fn("SteamAPI_GetHSteamUser", c_int)
        self.HPipe = fn("SteamAPI_GetHSteamPipe", c_int)
        self.GetUGC = fn("SteamAPI_ISteamClient_GetISteamUGC", c_void_p, c_void_p, c_int, c_int, c_char_p)
        self.GetUtils = fn("SteamAPI_ISteamClient_GetISteamUtils", c_void_p, c_void_p, c_int, c_char_p)
        self.IsDone = fn("SteamAPI_ISteamUtils_IsAPICallCompleted", c_bool, c_void_p, c_u64, ctypes.POINTER(c_bool))
        self.GetResult = fn("SteamAPI_ISteamUtils_GetAPICallResult", c_bool, c_void_p, c_u64, c_void_p, c_int, c_int, ctypes.POINTER(c_bool))
        self.FailReason = fn("SteamAPI_ISteamUtils_GetAPICallFailureReason", c_int, c_void_p, c_u64)
        u = "SteamAPI_ISteamUGC_"
        self.CreateItem = fn(u + "CreateItem", c_u64, c_void_p, c_uint, c_int)
        self.StartUpdate = fn(u + "StartItemUpdate", c_u64, c_void_p, c_uint, c_u64)
        self.SetTitle = fn(u + "SetItemTitle", c_bool, c_void_p, c_u64, c_char_p)
        self.SetDescription = fn(u + "SetItemDescription", c_bool, c_void_p, c_u64, c_char_p)
        self.SetLanguage = fn(u + "SetItemUpdateLanguage", c_bool, c_void_p, c_u64, c_char_p)
        self.SetVisibility = fn(u + "SetItemVisibility", c_bool, c_void_p, c_u64, c_int)
        self.SetTags = fn(u + "SetItemTags", c_bool, c_void_p, c_u64, ctypes.POINTER(ParamStringArray))
        self.SetContent = fn(u + "SetItemContent", c_bool, c_void_p, c_u64, c_char_p)
        self.SetPreview = fn(u + "SetItemPreview", c_bool, c_void_p, c_u64, c_char_p)
        self.AddPreviewFile = fn(u + "AddItemPreviewFile", c_bool, c_void_p, c_u64, c_char_p, c_int)
        self.Submit = fn(u + "SubmitItemUpdate", c_u64, c_void_p, c_u64, c_char_p)
        self.Progress = fn(u + "GetItemUpdateProgress", c_int, c_void_p, c_u64, ctypes.POINTER(c_u64), ctypes.POINTER(c_u64))
        self.NumSubscribed = fn(u + "GetNumSubscribedItems", c_uint, c_void_p)

        if not self.Init():
            raise SystemExit("SteamAPI_Init не прошёл: Steam не запущен или не тот аккаунт")
        client = self.SteamClient()
        self.ugc = self.GetUGC(client, self.HUser(), self.HPipe(), b"STEAMUGC_INTERFACE_VERSION008")
        self.utils = self.GetUtils(client, self.HPipe(), b"SteamUtils008")
        if not self.ugc or not self.utils:
            raise SystemExit("Steam не отдал интерфейсы UGC/Utils")

    def wait(self, call, struct, cb, update=None):
        failed = c_bool(False)
        while not self.IsDone(self.utils, call, ctypes.byref(failed)):
            self.RunCallbacks()
            if update is not None:
                done, total = c_u64(0), c_u64(0)
                st = self.Progress(self.ugc, update, ctypes.byref(done), ctypes.byref(total))
                if total.value:
                    sys.stdout.write("\r    этап %d: %d / %d байт   " % (st, done.value, total.value))
                    sys.stdout.flush()
            time.sleep(0.1)
        if update is not None:
            sys.stdout.write("\n")
        res = struct()
        ok = self.GetResult(self.utils, call, ctypes.byref(res), ctypes.sizeof(res), cb, ctypes.byref(failed))
        if not ok or failed.value:
            raise SystemExit("вызов Steam не удался, причина %d" % self.FailReason(self.utils, call))
        return res


def u8(s):
    return s.encode("utf-8")


def change_note(mod, version):
    """Заметка к обновлению (вкладка «Изменения» в Мастерской) - из
    changelog.json: {мод: {версия: текст}}. Полная заметка обязательна."""
    path = os.path.join(HERE, "changelog.json")
    if os.path.exists(path):
        log = json.load(io.open(path, encoding="utf-8"))
        text = log.get(mod, {}).get(str(version))
        if text:
            return text
    raise ValueError("Missing release changelog for %s version %s; prepare the bilingual notes first" % (mod, version))


def preview_path(base):
    """preview.png или preview.jpg (build.py ужимает тяжёлое превью в JPEG)."""
    for ext in (".png", ".jpg"):
        p = os.path.join(base, "preview" + ext)
        if os.path.exists(p):
            return p
    raise SystemExit("нет превью в " + base)


def write_fcs_info(meta):
    mod = meta["mod"]
    # Папка мода - Kenshi/mods или папка Мастерской (build.mod_dir): часть
    # модов пользователь держит там. Нигде нет - файлов FCS не пишем.
    import build
    d = build.mod_dir(mod)
    if d is None:
        print("  папки мода нет - _%s.info не записан" % mod)
        return
    tags ="".join("    <string>%s</string>\n" % t for t in meta["tags"])
    now = datetime.datetime.now().astimezone().isoformat()
    xml = ('<?xml version="1.0"?>\n'
           '<ModData xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" '
           'xmlns:xsd="http://www.w3.org/2001/XMLSchema">\n'
           '  <id>%d</id>\n  <mod>%s</mod>\n  <title>%s</title>\n  <tags>\n%s  </tags>\n'
           '  <visibility>%d</visibility>\n  <lastUpdate>%s</lastUpdate>\n</ModData>\n'
           % (meta["workshop_id"], mod, meta["title"], tags, meta["visibility"], now))
    io.open(os.path.join(d, "_%s.info" % mod), "w", encoding="utf-8").write(xml)
    shutil.copy2(preview_path(os.path.join(HERE, mod)), os.path.join(d, "_%s.img" % mod))


def upload(st, mod, note):
    base = os.path.join(HERE, mod)
    meta_path = os.path.join(base, "item.json")
    meta = json.load(io.open(meta_path, encoding="utf-8"))
    new = not meta.get("workshop_id")
    if new:
        r = st.wait(st.CreateItem(st.ugc, APPID, 0), CreateItemResult, 3403)
        if r.result != 1:
            raise SystemExit("%s: CreateItem вернул EResult %d" % (mod, r.result))
        meta["workshop_id"] = r.id
        json.dump(meta, io.open(meta_path, "w", encoding="utf-8"), indent=1, ensure_ascii=False)
        print("  создан предмет %d%s" % (r.id, " (НУЖНО ПРИНЯТЬ СОГЛАШЕНИЕ МАСТЕРСКОЙ)" if r.needs_agreement else ""))
    h = st.StartUpdate(st.ugc, APPID, meta["workshop_id"])
    # Основной язык предмета - английский; русский идёт вторым
    # обновлением (SetItemUpdateLanguage), его видят клиенты Steam на русском.
    desc = io.open(os.path.join(base, "description_en.txt"), encoding="utf-8").read()
    tags = (c_char_p * len(meta["tags"]))(*[u8(t) for t in meta["tags"]])
    arr = ParamStringArray(tags, len(meta["tags"]))
    checks = [
        ("title", st.SetTitle(st.ugc, h, u8(meta["title"]))),
        ("description", st.SetDescription(st.ugc, h, u8(desc))),
        ("visibility", st.SetVisibility(st.ugc, h, int(meta["visibility"]))),
        ("tags", st.SetTags(st.ugc, h, ctypes.byref(arr))),
        ("content", st.SetContent(st.ugc, h, u8(os.path.join(base, "content")))),
        ("preview", st.SetPreview(st.ugc, h, u8(preview_path(base)))),
    ]
    if new:   # скриншоты добавляем один раз, иначе при обновлении задвоятся
        sdir = os.path.join(base, "screens")
        for f in sorted(os.listdir(sdir)):
            checks.append(("screen " + f, st.AddPreviewFile(st.ugc, h, u8(os.path.join(sdir, f)), 0)))
    bad = [n for n, ok in checks if not ok]
    if bad:
        raise SystemExit("%s: Steam отверг %s" % (mod, ", ".join(bad)))
    r = st.wait(st.Submit(st.ugc, h, u8(note) if note else None), SubmitItemUpdateResult, 3404, update=h)
    if r.result != 1:
        raise SystemExit("%s: SubmitItemUpdate вернул EResult %d" % (mod, r.result))

    # Русский перевод названия и описания.
    desc_ru = io.open(os.path.join(base, "description_ru.txt"), encoding="utf-8").read()
    h = st.StartUpdate(st.ugc, APPID, meta["workshop_id"])
    checks = [
        ("language", st.SetLanguage(st.ugc, h, b"russian")),
        ("title ru", st.SetTitle(st.ugc, h, u8(meta.get("title_ru", meta["title"])))),
        ("description ru", st.SetDescription(st.ugc, h, u8(desc_ru))),
    ]
    bad = [n for n, ok in checks if not ok]
    if bad:
        raise SystemExit("%s: Steam отверг %s" % (mod, ", ".join(bad)))
    # Без заметки: иначе во вкладке «Изменения» у версии две одинаковые записи.
    r2 = st.wait(st.Submit(st.ugc, h, None), SubmitItemUpdateResult, 3404)
    if r2.result != 1:
        raise SystemExit("%s: русский перевод: EResult %d" % (mod, r2.result))
    print("  русский перевод загружен")
    write_fcs_info(meta)
    print("  готово: https://steamcommunity.com/sharedfiles/filedetails/?id=%d%s"
          % (meta["workshop_id"], " (НУЖНО ПРИНЯТЬ СОГЛАШЕНИЕ МАСТЕРСКОЙ)" if r.needs_agreement else ""))


def update_descriptions(st, mod, note):
    """Только описания (англ. основное + русский перевод), без файлов и картинок."""
    base = os.path.join(HERE, mod)
    meta = json.load(io.open(os.path.join(base, "item.json"), encoding="utf-8"))
    if not meta.get("workshop_id"):
        raise SystemExit("%s: предмета в Мастерской ещё нет" % mod)
    for lang, fname in ((None, "description_en.txt"), (b"russian", "description_ru.txt")):
        h = st.StartUpdate(st.ugc, APPID, meta["workshop_id"])
        desc = io.open(os.path.join(base, fname), encoding="utf-8").read()
        ok = True
        if lang:
            ok = st.SetLanguage(st.ugc, h, lang) and st.SetTitle(st.ugc, h, u8(meta.get("title_ru", meta["title"])))
        else:
            ok = st.SetTitle(st.ugc, h, u8(meta["title"]))     # переименование (09.10.2026)
        ok = ok and st.SetDescription(st.ugc, h, u8(desc))
        if not ok:
            raise SystemExit("%s: Steam отверг описание %s" % (mod, fname))
        # Без заметки: смена текста описания - не версия мода, во вкладке
        # «Обновления» ей не место (Submit без заметки записи не создаёт).
        r = st.wait(st.Submit(st.ugc, h, None), SubmitItemUpdateResult, 3404)
        if r.result != 1:
            raise SystemExit("%s: %s: EResult %d" % (mod, fname, r.result))
        print("  %s обновлено" % fname)


def update_preview(st, mod):
    """Только главная картинка (preview), без файлов и без записи в «Обновлениях»."""
    base = os.path.join(HERE, mod)
    meta = json.load(io.open(os.path.join(base, "item.json"), encoding="utf-8"))
    if not meta.get("workshop_id"):
        raise SystemExit("%s: предмета в Мастерской ещё нет" % mod)
    h = st.StartUpdate(st.ugc, APPID, meta["workshop_id"])
    if not st.SetPreview(st.ugc, h, u8(preview_path(base))):
        raise SystemExit("%s: Steam отверг превью" % mod)
    r = st.wait(st.Submit(st.ugc, h, None), SubmitItemUpdateResult, 3404)
    if r.result != 1:
        raise SystemExit("%s: превью: EResult %d" % (mod, r.result))
    write_fcs_info(meta)            # _<Mod>.img - та же картинка, как у FCS
    print("  превью обновлено")


if __name__ == "__main__":
    st = Steam()
    if sys.argv[1] == "preview":
        for mod in sys.argv[2:]:
            print("==", mod)
            update_preview(st, mod)
    if sys.argv[1] == "desc":
        for mod in sys.argv[2:]:
            print("==", mod)
            update_descriptions(st, mod, u"Описание")
    if sys.argv[1] == "check":
        print("Steam подключён, интерфейсы получены; подписок на моды Kenshi:",
              st.NumSubscribed(st.ugc))
    elif sys.argv[1] == "refresh":
        # Файлы и описания без записи во вкладке «Изменения»: версия та же
        # (09.10.2026 - убрать из Steam старые <Mod>-source.zip у форков и
        # обновить шапку описаний; заметка прошлой версии второй раз не нужна).
        for mod in sys.argv[2:]:
            print("==", mod)
            upload(st, mod, None)
    elif sys.argv[1] == "upload":
        for mod in sys.argv[2:]:
            meta = json.load(io.open(os.path.join(HERE, mod, "item.json"), encoding="utf-8"))
            print("==", mod)
            upload(st, mod, change_note(mod, meta["version"]))
    st.Shutdown()
