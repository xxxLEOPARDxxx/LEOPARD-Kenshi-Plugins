"""Copy built-in ImageGen outputs and record provenance; never alter raster content."""
import hashlib, html, json, pathlib, shutil, sys
from PIL import Image

ROOT = pathlib.Path(r'D:/DEV/Kenshi/workshop')
HERE = pathlib.Path(__file__).resolve().parent
MANIFEST = HERE / '2026-10-31-steam-squares.json'
def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()

originals = {c['mod']: c for c in json.loads((HERE/'2026-10-30-covers.json').read_text(encoding='utf-8'))['covers']}
entries = {c['mod']: c for c in json.loads(MANIFEST.read_text(encoding='utf-8'))['covers']} if MANIFEST.exists() else {}
for incoming in json.load(sys.stdin):
    mod = incoming['mod']
    if mod == 'DynamicFactions' or mod not in originals: raise ValueError('Not a published cover: '+mod)
    original = originals[mod]
    landscape = ROOT/original['image']
    if digest(landscape) != original['sha256']: raise ValueError('Landscape changed: '+mod)
    source = pathlib.Path(incoming['source']).resolve()
    if not source.is_relative_to(pathlib.Path.home()/'.codex/generated_images'): raise ValueError('Unexpected generated source')
    with Image.open(source) as image:
        dimensions = [image.width, image.height]
        if image.width != image.height or image.width < 1024: raise ValueError('Not a full square: '+mod)
    relative = pathlib.Path('steam-covers')/mod/'000_LEOPARD_Steam_Cover.png'
    destination = ROOT/relative
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)
    entries[mod] = dict(mod=mod, title=original['title'], mcm_badge=original['mcm_badge'],
        image=relative.as_posix(), sha256=digest(destination), bytes=destination.stat().st_size,
        dimensions=dimensions, source_image=original['image'], source_sha256=original['sha256'],
        generated_source=str(source), generation_prompt=incoming['prompt'])
document = dict(date='2026-10-31', tool='built-in image_gen', covers=sorted(entries.values(),key=lambda x:x['mod']))
MANIFEST.write_text(json.dumps(document,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
folder=ROOT/'steam-covers';folder.mkdir(exist_ok=True)
(folder/'manifest.json').write_text(MANIFEST.read_text(encoding='utf-8'),encoding='utf-8')
cards=''.join('<figure><img src="'+html.escape(c['mod'])+'/000_LEOPARD_Steam_Cover.png"><figcaption>'+html.escape(c['title'])+'</figcaption></figure>' for c in document['covers'])
(folder/'gallery.html').write_text('<!doctype html><meta charset="utf-8"><title>Steam square covers</title><style>body{background:#201b17;color:#eed9ac;font:16px sans-serif;display:grid;grid-template-columns:repeat(auto-fit,minmax(250px,1fr));gap:20px;padding:20px}figure{margin:0}img{width:100%}figcaption{padding:8px}</style>'+cards,encoding='utf-8')
print('Saved '+str(len(entries))+'/37 square covers; landscape originals verified unchanged.')
