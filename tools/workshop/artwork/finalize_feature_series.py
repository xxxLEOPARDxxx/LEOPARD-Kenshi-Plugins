"""Validate and stage the approved feature-first cover series; never upload code."""
from pathlib import Path
import hashlib, html, json, shutil, zipfile
from PIL import Image

W = Path('D:/DEV/Kenshi/workshop')
SERIES = W / 'cover-series/2026-10-31-features'
CANON = Path(__file__).resolve().parent
PLAN = json.loads((SERIES / 'plan.json').read_text(encoding='utf-8'))
PROTOTYPES = json.loads((W / 'cover-concepts/2026-10-31-alternatives/manifest.json').read_text(encoding='utf-8'))
PROTOTYPES = {c['mod']: c for c in PROTOTYPES['covers']}

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def write(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')

def main():
    jobs = PLAN['jobs']
    assert len(jobs) == 37 and len({j['mod'] for j in jobs}) == 37
    assert not any(j['mod'] == 'DynamicFactions' for j in jobs)
    assets = []
    # Complete validation before replacing any posting asset.
    for job in jobs:
        mod = job['mod']
        row = {k: job[k] for k in ['mod', 'title', 'mcm_badge', 'palette']}
        row['assets'] = {}
        for fmt in ['steam', 'nexus']:
            src = SERIES / mod / (fmt + '.png')
            with Image.open(src) as im:
                size = [im.width, im.height]
                assert (im.width == im.height) if fmt == 'steam' else (abs(im.width / im.height - 16 / 9) < .02), (mod, fmt, size)
                im.verify()
            meta = src.with_suffix('.json')
            if meta.exists():
                generation = json.loads(meta.read_text(encoding='utf-8'))
                assert generation['sha256'] == digest(src), (mod, fmt)
            else:
                assert fmt == 'steam' and mod in PROTOTYPES
                prototype = PROTOTYPES[mod]
                assert digest(src) == prototype['sha256']
                generation = dict(source=prototype['source'], prompt=prototype['prompt'], approved_prototype=True)
            row['assets'][fmt] = dict(image=str(src.relative_to(W)).replace('\\', '/'), sha256=digest(src), dimensions=size, bytes=src.stat().st_size, generation=generation)
        assets.append(row)
    assert sum(c['mcm_badge'] for c in assets) == 36
    assert not next(c for c in assets if c['mod'] == 'Dark UI Advanced')['mcm_badge']
    backup = W / 'cover-concepts/2026-10-31-before-feature-series'
    before = []
    for row in assets:
        mod = row['mod']
        for fmt, parent, filename in [('steam', 'steam-covers', '000_LEOPARD_Steam_Cover.png'), ('nexus', 'images', '000_LEOPARD_Cover.png')]:
            dst = W / parent / mod / filename
            archived = backup / fmt / mod / filename
            archived.parent.mkdir(parents=True, exist_ok=True)
            if dst.exists() and not archived.exists():
                shutil.copy2(dst, archived)
            before.append(dict(mod=mod, format=fmt, backup=str(archived), previous_sha256=digest(archived)))
            dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(SERIES / mod / (fmt + '.png'), dst)
            assert digest(dst) == row['assets'][fmt]['sha256']
    write(backup / 'manifest.json', before)
    manifest = dict(date='2026-10-31', tool='built-in image_gen', status='74 assets visually reviewed and staged; Steam publication checked separately', rules=PLAN['rules'], fidelity_review=['Dark UI Advanced has no MCM badge', 'Vital Sense uses status icons, no overhead HP bars', 'Recruit leader star has actual followers', 'Equipment grade uses one coloured star, no repair-kit grade', 'Live XP shows percentages, no raw XP totals', 'Ranged Damage XP uses an armoured character, not a wooden dummy', 'Skeleton repair portrait uses a wrench', 'MCM retains its distinct central management-hub identity'], covers=assets)
    write(SERIES / 'manifest.json', manifest)
    write(CANON / '2026-10-31-feature-series.json', manifest)
    squares = []
    for row in assets:
        a = row['assets']['steam']
        squares.append(dict(mod=row['mod'], title=row['title'], mcm_badge=row['mcm_badge'], image='steam-covers/' + row['mod'] + '/000_LEOPARD_Steam_Cover.png', source_image='images/' + row['mod'] + '/000_LEOPARD_Cover.png', sha256=a['sha256'], bytes=a['bytes'], dimensions=a['dimensions'], generation_prompt=a['generation']['prompt'], generated_source=a['generation']['source']))
    square_manifest = dict(date='2026-10-31', tool='built-in image_gen', series='feature-first diverse palettes', covers=squares)
    write(W / 'steam-covers/manifest.json', square_manifest)
    write(CANON / '2026-10-31-steam-squares.json', square_manifest)
    ready = SERIES / 'Nexus-ready'
    ready.mkdir(exist_ok=True)
    for row in assets:
        shutil.copy2(SERIES / row['mod'] / 'nexus.png', ready / (row['mod'] + '.png'))
    archive = SERIES / 'Nexus-Covers-2026-10-31.zip'
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_STORED) as z:
        for row in assets:
            z.write(ready / (row['mod'] + '.png'), row['mod'] + '.png')
    with zipfile.ZipFile(archive) as z:
        assert len(z.namelist()) == 37 and z.testzip() is None
    cards = []
    for row in assets:
        title = html.escape(row['title'])
        mod = html.escape(row['mod'], quote=True)
        cards.append('<article><h2>' + title + '</h2><div class="pair"><a href="' + mod + '/steam.png"><img loading="lazy" src="' + mod + '/steam.png" alt="Steam: ' + title + '"></a><a href="' + mod + '/nexus.png"><img loading="lazy" src="' + mod + '/nexus.png" alt="Nexus: ' + title + '"></a></div><p>Steam 1:1 / Nexus 16:9</p></article>')
    page = '<!doctype html><html lang="ru"><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>Kenshi: 37 пар обложек</title><style>body{background:#141819;color:#eee;font:16px system-ui;margin:24px}h1{font-size:28px}h2{font-size:18px}main{display:grid;grid-template-columns:repeat(auto-fit,minmax(440px,1fr));gap:24px}article{background:#202627;padding:16px;border-radius:8px}.pair{display:grid;grid-template-columns:1fr 1.78fr;align-items:center;gap:12px}img{display:block;width:100%;height:auto}a{color:#71d4d2}p{color:#bbc4c4}@media(max-width:520px){main{grid-template-columns:1fr}.pair{grid-template-columns:1fr}}</style><h1>37 модов: Steam и Nexus</h1><p>Новая серия с отдельными палитрами и наглядными функциями. Нажми изображение для полного размера.</p><p><a href="Nexus-Covers-2026-10-31.zip">Все 37 обложек Nexus одним архивом</a></p><main>' + ''.join(cards) + '</main></html>'
    (SERIES / 'gallery.html').write_text(page, encoding='utf-8')
    print('PASS: 37 pairs / 74 PNGs; square/16:9, hashes, backups, posting folders and 37-file Nexus ZIP CRC verified.')

if __name__ == '__main__':
    main()
