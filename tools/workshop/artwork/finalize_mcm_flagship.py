"""Install the explicitly requested Nexus companion to the flagship Steam cover."""
import hashlib,json,pathlib,shutil,sys
from PIL import Image
root=pathlib.Path(r'D:/DEV/Kenshi/workshop');here=pathlib.Path(__file__).resolve().parent
incoming=json.load(sys.stdin);source=pathlib.Path(incoming['source']).resolve()
if not source.is_relative_to(pathlib.Path.home()/'.codex/generated_images'):raise ValueError('Unexpected source')
with Image.open(source) as image:
    dimensions=[image.width,image.height]
    if abs(image.width/image.height-16/9)>.01:raise ValueError('Nexus companion must be 16:9')
manifest=here/'2026-10-30-covers.json';document=json.loads(manifest.read_text(encoding='utf-8'))
entry=next(c for c in document['covers'] if c['mod']=='ModConfigMenu');destination=root/entry['image']
archive=root/'cover-concepts/2026-10-31-mcm-core/previous_landscape.png';archive.parent.mkdir(parents=True,exist_ok=True)
if not archive.exists():shutil.copy2(destination,archive)
shutil.copy2(source,destination)
entry.update(sha256=hashlib.sha256(destination.read_bytes()).hexdigest(),bytes=destination.stat().st_size,dimensions=dimensions,
    generated_source=str(source),edit_reference='steam-covers/ModConfigMenu/000_LEOPARD_Steam_Cover.png',updated='2026-10-31')
entry.setdefault('edit_prompts',[]).append(incoming['prompt'])
manifest.write_text(json.dumps(document,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
shutil.copy2(manifest,root/'cover-concepts/2026-10-30/manifest.json')
squares=here/'2026-10-31-steam-squares.json';sq=json.loads(squares.read_text(encoding='utf-8'))
record=next(c for c in sq['covers'] if c['mod']=='ModConfigMenu')
record['source_image']=archive.relative_to(root).as_posix()
squares.write_text(json.dumps(sq,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
shutil.copy2(squares,root/'steam-covers/manifest.json')
print('MCM Nexus flagship installed:',destination,dimensions)
