"""Private, pinned KEO path candidate; never publishes or changes personal INI."""
from pathlib import Path
import argparse, hashlib, json, os, shutil, subprocess, sys, zipfile
from datetime import datetime, timezone, timedelta

ROOT = Path('D:/DEV/Kenshi/forks/KEO')
GAME = Path('D:/SteamLibrary/steamapps/common/Kenshi')
OUT = Path('D:/DEV/Kenshi/workshop/testing/KEO')
COMMIT = '3e5a11d41a9e54462067a36b5efb21a7227b6094'
DLL = 'f8074f907b3527d4d1848ae68ffd6824d9b9ad8d2d7f0be2944c2748316d65b6'
BASE_ZIP = '8dac3fce6779f22b6906482eb644e55be9085af4c374b85896599f82b69e6f69'

def sha(data):
    return hashlib.sha256(data).hexdigest()

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--install', action='store_true')
    args = parser.parse_args()
    sys.stdout.reconfigure(encoding='utf-8')
    commit = subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], text=True).strip()
    assert commit == COMMIT, commit
    assert not subprocess.check_output(['git', '-C', str(ROOT), 'diff', '--name-only'], text=True).strip()
    data = (ROOT/'build/KEO_step4_prod/KEO.dll').read_bytes()
    assert sha(data) == DLL
    sys.path.insert(0, 'D:/DEV/Kenshi/tools')
    from qol_binary_check import PE
    pe = PE(ROOT/'build/KEO_step4_prod/KEO.dll')
    assert {'?startPlugin@@YAXXZ', 'MCM_Describe'} <= pe.exports()
    imports = {}
    for name, symbols in pe.imports().items():
        if name.lower() in ('kenshilib.dll', 'myguiengine_x64.dll', 'ogremain_x64.dll', 'ois_x64.dll'):
            assert set(symbols) <= PE(GAME/name).exports(), name
            imports[name] = len(symbols)
    base = OUT/'KEO-jobs-test-20261105.zip'
    assert sha(base.read_bytes()) == BASE_ZIP
    with zipfile.ZipFile(base) as z:
        assert z.testzip() is None and len(z.namelist()) == 13
        files = {n: z.read(n) for n in z.namelist()}
    files['KEO/KEO.dll'] = data
    files['KEO/PATH_TEST_NOTES.md'] = (ROOT/'leopard/path-ownership-maintenance-20261105.md').read_bytes()
    files['TEST_README_RU_EN.txt'] = '''KEO: закрытая тестовая сборка 3e5a11d, 05.11.2026. Не публичный выпуск.
Новые изменения:
- Новый приказ группе больше 30 человек снимает у остальных выбранных персонажей старую привязку к построению и маршруту.
- Если маршрут отменён или заменён во время расчёта промежуточной точки, старые координаты не подменяют штатный ответ игры.
Проверки кода, сборка и независимое ревью пройдены. Игровые маршруты, FPS и рост памяти новой сборки ещё не проверены.
Три необязательных задания из предыдущей сборки сохранены. Создание двух точек циклического маршрута и сохранение/загрузка подтверждены игроком на предыдущей сборке df07c25.
Сбор продукции и пополнение прилавка требуют отдельной игровой проверки; пополнение переносит вещи и не совершает денежную сделку.
Проверяйте в отдельном слоте. Не включайте оригинальный KenshiQOL одновременно. Личный KEO.ini не заменяйте шаблоном.
Проверка GOG игроком ожидается; отдельный KEO-GOG-test-20261105.zip не изменён.

KEO private test build 3e5a11d, 05 Nov 2026. Not a public release.
- A new MOVE for more than 30 selected characters retires excess actors' previous formation and route ownership.
- A route cancelled/replaced during intermediate-point snapping no longer publishes its retired coordinates over the native answer.
Host controls, build and independent review passed. New-build gameplay, FPS and memory measurements remain pending.
The three optional jobs remain available. Two-point cyclic movement and save/load were accepted on previous build df07c25.
Gather Output and Stock Counter need separate physical-delivery acceptance. Stocking transfers items, not money.
Use a separate test save. Do not run original KenshiQOL simultaneously or overwrite personal KEO.ini with its template.
Real GOG tester acceptance is pending; the separate GOG startup archive is unchanged.
'''.encode('utf-8')
    assert not any(n.lower().endswith(('.po', '.exe', '.log')) or '..' in Path(n).parts for n in files)
    assert 'KEO/KEO.ini' not in files
    path = OUT/'KEO-path-test-20261105.zip'
    temp = path.with_suffix('.zip.partial')
    with zipfile.ZipFile(temp, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for name, content in files.items():
            z.writestr(name, content)
    with zipfile.ZipFile(temp) as z:
        assert z.testzip() is None and set(z.namelist()) == set(files)
        assert all(z.read(n) == b for n, b in files.items())
    os.replace(temp, path)
    now = datetime.now(timezone(timedelta(hours=3)))
    receipt = dict(time_msk=now.isoformat(), source_commit=COMMIT, dll_sha256=DLL,
        archive=str(path), archive_sha256=sha(path.read_bytes()), archive_bytes=path.stat().st_size,
        files=len(files), imports=imports, crc='passed', status='prepared_private_not_published',
        tests={'order_ownership_per_optimization':345, 'snap_freshness_per_optimization':186,
               'optimizations':['Od','O2'], 'old_behavior_negative_controls':'rejected'},
        runtime_acceptance='pending; UI automation initialization failed; no game launch or FPS measurement',
        changes_ru=['При новом приказе большой группе снимается старая привязка у выбранных персонажей сверх первых 30.',
                    'При отмене или замене маршрута во время расчёта точки сохраняется штатный ответ игры.'])
    if args.install:
        count = subprocess.check_output(['powershell', '-NoProfile', '-Command',
            '@(Get-Process Kenshi* -ErrorAction SilentlyContinue).Count'], text=True).strip()
        assert count == '0', 'Kenshi is running; no installation attempted'
        personal = [GAME/'mods/KEO'/n for n in ('KEO.ini','KEOCompat.ini','RE_Kenshi.json')]+[GAME/'data/mods.cfg']
        preserved = {str(p):p.read_bytes() for p in personal if p.exists()}
        target = GAME/'mods/KEO/KEO.dll'
        assert target.exists()
        backup = Path('D:/DEV/Kenshi/backup')/('keo-path-'+now.strftime('%Y%m%d-%H%M%S'))
        backup.mkdir(parents=True, exist_ok=False)
        shutil.copy2(target, backup/'KEO.dll')
        target.write_bytes(data)
        assert sha(target.read_bytes()) == DLL
        assert all(Path(p).read_bytes() == b for p,b in preserved.items())
        receipt.update(status='installed_local_private_not_published', backup=str(backup),
            preserved_sha256={p:sha(b) for p,b in preserved.items()}, game_process_count_before_install=0)
        (backup/'installation.json').write_text(json.dumps(receipt, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    for destination in (OUT/'KEO-path-test-20261105.json', Path(__file__).with_suffix('.json')):
        destination.write_text(json.dumps(receipt, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(receipt, ensure_ascii=False, indent=2))

if __name__ == '__main__':
    main()
