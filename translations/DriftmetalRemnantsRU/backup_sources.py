import datetime,hashlib,json,pathlib,shutil,sys
from inspect_sources import ROOT,SOURCES
sys.stdout.reconfigure(encoding='utf8')
stamp='20261101-driftmetal'
BACKUP=pathlib.Path('D:/DEV/Kenshi/backup')/stamp
def manifest(folder):
 return {p.relative_to(folder).as_posix():{'size':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in sorted(folder.rglob('*')) if p.is_file()}
out={}
for label,key in [('installed-merged-ru','installed_ru'),('installed-animation-patch','animation_patch'),('upstream-english','upstream')]:
 src=SOURCES[key].parent;dst=BACKUP/label
 if not dst.exists():shutil.copytree(src,dst)
 a,b=manifest(src),manifest(dst);assert a==b,(label,'backup mismatch')
 out[label]={'source':str(src),'backup':str(dst),'files':a,'total_bytes':sum(x['size'] for x in a.values())}
 print(label,len(a),'files',out[label]['total_bytes'],'bytes verified')
(ROOT/'backup-manifest.json').write_text(json.dumps(out,ensure_ascii=False,indent=2),encoding='utf8')
old=out['installed-merged-ru']['files'];new=out['upstream-english']['files']
delta={k:[p for p in sorted(set(old)|set(new)) if (p not in old if k=='added' else p not in new if k=='removed' else p in old and p in new and old[p]!=new[p])] for k in ['added','removed','changed']}
(ROOT/'asset-diff.json').write_text(json.dumps(delta,ensure_ascii=False,indent=2),encoding='utf8')
print('assets', {k:len(v) for k,v in delta.items()})
