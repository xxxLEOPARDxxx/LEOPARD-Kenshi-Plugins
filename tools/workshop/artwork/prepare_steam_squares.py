"""Stage only square preview files; preserve content, item metadata and descriptions."""
import hashlib,importlib.util,json,pathlib,sys
from PIL import Image
root=pathlib.Path(r'D:/DEV/Kenshi/workshop')
spec=importlib.util.spec_from_file_location('workshop_build',root/'build.py')
build=importlib.util.module_from_spec(spec);spec.loader.exec_module(build)
manifest=json.loads((root/'steam-covers/manifest.json').read_text(encoding='utf-8'))
baseline=json.loads((root/'cover-concepts/2026-10-30/steam-preview-remote-verification.json').read_text(encoding='utf-8'))
expected={c['mod']:c for c in baseline}
if len(manifest['covers'])!=37 or set(expected)!={c['mod'] for c in manifest['covers']}:raise ValueError('Published cover set mismatch')
report=[]
for c in manifest['covers']:
    mod=c['mod']
    if mod=='DynamicFactions':raise ValueError('Unpublished project')
    folder=root/mod;metadata=folder/'item.json';before=metadata.read_bytes();meta=json.loads(before)
    if str(meta['workshop_id'])!=str(expected[mod]['id']):raise ValueError('Workshop ID mismatch: '+mod)
    source=pathlib.Path(build.steam_preview_source(mod,str(root/c['source_image'])))
    if hashlib.sha256(source.read_bytes()).hexdigest()!=c['sha256']:raise ValueError('Asset hash mismatch: '+mod)
    filename=build.prepare_image(str(source),str(folder/'_square_stage'))
    staged=folder/filename
    with Image.open(staged) as image:
        if image.width!=image.height:raise ValueError('Non-square output')
        size=[image.width,image.height]
    if staged.stat().st_size>build.MAX_IMG:raise ValueError('Steam size limit')
    target=folder/('preview'+staged.suffix);staged.replace(target)
    for suffix in ('.png','.jpg'):
        other=folder/('preview'+suffix)
        if other!=target and other.exists():other.unlink()
    if metadata.read_bytes()!=before:raise ValueError('Unexpected metadata mutation')
    report.append(dict(mod=mod,id=meta['workshop_id'],preview=str(target),dimensions=size,
        bytes=target.stat().st_size,sha256=hashlib.sha256(target.read_bytes()).hexdigest(),metadata_sha256=hashlib.sha256(before).hexdigest()))
(root/'steam-covers/staged-previews.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print('Staged',len(report),'square Steam previews; item metadata untouched.')
