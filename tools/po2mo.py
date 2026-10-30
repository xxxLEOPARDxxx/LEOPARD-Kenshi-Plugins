# -*- coding: utf-8 -*-
"""Компиляция .po в двоичный .mo для всех наших плагинов.

Зачем. .po - формат для правки человеком, .mo - то, что читает код. Так
принято в gettext, и Asur отдельно на это указал. Наш загрузчик
(shared\\Localization.h) пробует сначала .mo и только потом .po, так что
скомпилированный файл подхватится сам, а .po останется лежать рядом как
исходник перевода.

Плюс не только в приличиях: разбор .mo не зависит от того, как переводчик
оформил файл. Многострочные записи, которые пишет Poedit, наш разбор .po не
читает, а через .mo они доезжают правильно, потому что msgfmt их уже склеил.

Формат .mo: сигнатура 0x950412de, ревизия, число строк, смещения двух таблиц
(длина, смещение), сами строки и хеш-таблица GNU gettext для совместимости
с загрузчиком переводов игры.

    python po2mo.py           скомпилировать все каталоги
    python po2mo.py <файл.po> один файл
"""
import io, os, struct, sys

MODS = r"D:\SteamLibrary\steamapps\common\Kenshi\mods"

# Только НАШИ домены. Без этого списка обход задевает чужие моды: первый
# прогон создал main.mo в «Russian Translation Fix Plus», где его отродясь
# не было, и мог изменить поведение чужого перевода. Файл удалён, мод
# возвращён в исходное состояние.
OURS = ("character_inspector", "loot_all", "better_looting", "kill_counter", "stat_colours", "xp_bars")


def _unescape(s):
    out, i = [], 0
    while i < len(s):
        if s[i] == '\\' and i + 1 < len(s):
            i += 1
            out.append({'n': '\n', 't': '\t', 'r': '\r'}.get(s[i], s[i]))
        else:
            out.append(s[i])
        i += 1
    return ''.join(out)


def _quoted(line):
    a, b = line.find('"'), line.rfind('"')
    return _unescape(line[a + 1:b]) if b > a else ''


def parse_po(path):
    """Разбор .po: многострочные записи, множественные формы, пометка fuzzy.

    Что здесь важного, помимо очевидного.

    * Служебная запись msgid "" НЕ выбрасывается: в ней объявлена кодировка
      (Content-Type: charset=UTF-8). Без неё gettext считает каталог
      однобайтовым и падает на первой кириллической букве — с таким файлом
      Kenshi не запускается вовсе.
    * Записи, помеченные "#, fuzzy", пропускаются, как это делает msgfmt.
      Это черновые переводы: переводчик пометил их как требующие проверки,
      и в готовый каталог они попадать не должны. В main.po их 65.
    * Множественные формы (msgid_plural / msgstr[N]) в .mo хранятся
      склейкой через нулевой байт: ключ "ед\0мн", значение "форма0\0форма1…".
      Без этого 10 записей главного каталога просто терялись.
    """
    entries = {}
    msgid = plural = None
    forms = {}
    buf = None          # список кусков текущей строки
    target = None       # 'id' | 'plural' | int (номер формы)
    fuzzy = False
    pending_fuzzy = False

    def flush():
        if msgid is None:
            return
        if fuzzy:
            return
        if plural is not None:
            values = [forms.get(i, '') for i in range(max(forms) + 1)] if forms else []
            if any(values):
                entries[msgid + '\0' + plural] = '\0'.join(values)
        else:
            text = forms.get(0, '')
            if text:
                entries[msgid] = text

    for raw in io.open(path, encoding='utf-8-sig').read().splitlines():
        line = raw.strip()

        if line.startswith('#'):
            # Флаги записи идут перед ней: "#, fuzzy, c-format"
            if line.startswith('#,') and 'fuzzy' in line:
                pending_fuzzy = True
            continue

        if not line:
            continue

        if line.startswith('msgid_plural'):
            if buf is not None and target == 'id':
                msgid = ''.join(buf)
            plural = _quoted(line)
            buf, target = [plural], 'plural'
            continue

        if line.startswith('msgid'):
            # начало новой записи — предыдущую закрываем
            if buf is not None:
                if target == 'plural':
                    plural = ''.join(buf)
                elif isinstance(target, int):
                    forms[target] = ''.join(buf)
                elif target == 'id':
                    msgid = ''.join(buf)
            flush()
            msgid = _quoted(line)
            plural = None
            forms = {}
            fuzzy = pending_fuzzy
            pending_fuzzy = False
            buf, target = [msgid], 'id'
            continue

        if line.startswith('msgstr'):
            if target == 'id':
                msgid = ''.join(buf)
            elif target == 'plural':
                plural = ''.join(buf)
            elif isinstance(target, int):
                forms[target] = ''.join(buf)
            idx = 0
            if line.startswith('msgstr['):
                close = line.find(']')
                if close > 7:
                    try:
                        idx = int(line[7:close])
                    except ValueError:
                        idx = 0
            buf, target = [_quoted(line)], idx
            continue

        if line.startswith('"') and buf is not None:
            buf.append(_quoted(line))

    if buf is not None:
        if target == 'plural':
            plural = ''.join(buf)
        elif isinstance(target, int):
            forms[target] = ''.join(buf)
        elif target == 'id':
            msgid = ''.join(buf)
    flush()
    return entries


def hash_string(s):
    """Хеш gettext (алгоритм PJW/hashpjw), как в GNU gettext."""
    h = 0
    for c in s:
        h = (h << 4) + c
        g = h & 0xF0000000
        if g:
            h ^= g >> 24
            h ^= g
        h &= 0xFFFFFFFF
    return h


def next_prime(n):
    def prime(x):
        if x < 2:
            return False
        i = 2
        while i * i <= x:
            if x % i == 0:
                return False
            i += 1
        return True
    while not prime(n):
        n += 1
    return n


def build_hash_table(keys):
    """Таблица для поиска по хешу. Эталонные каталоги игры её содержат, и
    полагаться на то, что чужая реализация gettext переживёт hashsz=0,
    не стоит: цена ошибки - игра не стартует."""
    size = next_prime(max(3, len(keys) * 4 // 3))
    table = [0] * size
    for idx, k in enumerate(keys):
        # Хешируется только единственное число - до первого нулевого
        # байта, как в GNU gettext (там это C-строка). Первая версия
        # хешировала весь ключ «ед\0мн», и все 10 записей с
        # множественными формами игра по хешу не находила: в окне машины
        # висело «INPUT», в очереди исследований - «hrs» (30.09.2026).
        b = k.encode('utf-8').split(b'\x00')[0]
        h = hash_string(b)
        pos = h % size
        step = 1 + h % (size - 2)
        while table[pos]:
            pos = (pos + step) % size
        table[pos] = idx + 1        # в gettext индексы с единицы
    return table


def write_mo(entries, path):
    # Служебная запись с кодировкой обязана идти первой.
    items = sorted(entries.items(), key=lambda kv: kv[0].encode('utf-8'))
    ids = b'\x00'.join(k.encode('utf-8') for k, _ in items) + (b'\x00' if items else b'')
    strs = b'\x00'.join(v.encode('utf-8') for _, v in items) + (b'\x00' if items else b'')

    n = len(items)
    table = build_hash_table([k for k, _ in items])
    head = 7 * 4
    otab = head
    ttab = otab + n * 8
    htab = ttab + n * 8
    ostart = htab + len(table) * 4
    tstart = ostart + len(ids)

    out = [struct.pack('<Iiiiiii', 0x950412de, 0, n, otab, ttab, len(table), htab)]

    off = ostart
    for k, _ in items:
        b = k.encode('utf-8')
        out.append(struct.pack('<ii', len(b), off))
        off += len(b) + 1
    off = tstart
    for _, v in items:
        b = v.encode('utf-8')
        out.append(struct.pack('<ii', len(b), off))
        off += len(b) + 1

    out.append(b''.join(struct.pack('<I', v) for v in table))
    out.append(ids)
    out.append(strs)
    io.open(path, 'wb').write(b''.join(out))
    return n


OURS = tuple(OURS) + ("skill_level_up_info", "multi_xp_mining", "kenshi_rotate")


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('-')]
    if args:
        targets = args
    else:
        targets = []
        for root, _dirs, files in os.walk(MODS):
            if os.path.basename(root) != 'LC_MESSAGES':
                continue
            for f in files:
                if f.endswith('.po') and f[:-3] in OURS:
                    targets.append(os.path.join(root, f))

    total = bad = 0
    for po in sorted(targets):
        mo = po[:-3] + '.mo'
        entries = parse_po(po)
        n = write_mo(entries, mo)
        total += 1
        short = po.replace(MODS + os.sep, '')

        # Проверяем написанное сторонним разбором. Битый .mo не просто
        # не переводит - Kenshi с ним не запускается вовсе, а причина по
        # симптому не читается. Один раз уже наступили.
        note = "строк: %d" % n
        try:
            import gettext
            t = gettext.GNUTranslations(io.open(mo, 'rb'))
            if len(t._catalog) < 1:
                raise ValueError("пустой каталог")
        except Exception as e:
            bad += 1
            note += "  ПРОВЕРКА НЕ ПРОЙДЕНА: %s: %s" % (type(e).__name__, e)
            os.remove(mo)
            note += " (файл удалён, чтобы не сломать игру)"
        print("  %-64s %s" % (short, note))

    print("скомпилировано файлов: %d, битых: %d" % (total, bad))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
