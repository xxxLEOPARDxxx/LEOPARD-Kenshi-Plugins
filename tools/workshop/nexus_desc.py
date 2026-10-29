# -*- coding: utf-8 -*-
"""Описание для Nexus: английское и русское в одном тексте.

    python nexus_desc.py   -> nexus/<Mod>_description.txt

У страницы Nexus одно описание, поэтому языки идут подряд через [line].
Заголовки [h2] из описаний Мастерской Nexus не понимает - заменяются на
[size=4][b]...[/b][/size].
"""
import io
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import build  # noqa: E402


def to_nexus(text):
    # Ссылка на Nexus на самом Nexus не нужна - остаётся только версия.
    text = re.sub(r"(Version|Версия) (\S+) · Nexus: \S+", r"\1 \2", text)
    # Ссылка на MCM - на его страницу Nexus, а не в Мастерскую Steam.
    text = text.replace(build.MCM_STEAM_URL, build.MCM_NEXUS_URL)
    return re.sub(r"\[h2\](.*?)\[/h2\]", r"[size=4][b]\1[/b][/size]", text)


if __name__ == "__main__":
    out = os.path.join(build.HERE, "nexus")
    os.makedirs(out, exist_ok=True)
    only = set(sys.argv[1:])
    for it in build.ITEMS:
        if only and it["mod"] not in only:
            continue
        base = os.path.join(build.HERE, it["mod"])
        if not os.path.isfile(os.path.join(base, "description_en.txt")):
            print("%-16s описание ещё не собрано - пропущен" % it["mod"])
            continue
        en = io.open(os.path.join(base, "description_en.txt"), encoding="utf-8").read().strip()
        ru = io.open(os.path.join(base, "description_ru.txt"), encoding="utf-8").read().strip()
        text = to_nexus(en) + "\n\n[line]\n[size=5][b]Русский[/b][/size]\n\n" + to_nexus(ru) + "\n"
        path = os.path.join(out, it["mod"] + "_description.txt")
        io.open(path, "w", encoding="utf-8", newline="\n").write(text)
        print("%-16s %5d байт" % (it["mod"], len(text.encode("utf-8"))))
