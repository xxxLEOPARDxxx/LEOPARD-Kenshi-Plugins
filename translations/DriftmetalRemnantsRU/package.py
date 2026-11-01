"""Package only staged products, verify ZIP CRC and exact file membership."""
import hashlib,json,pathlib,shutil,sys,zipfile
from inspect_sources import ROOT
sys.stdout.reconfigure(encoding='utf8')
OUT=ROOT/'artifacts'
def main():
 verified=json.loads((ROOT/'verification.json').read_text(encoding='utf8'));assert verified['passed']
 for p in verified['products']:
  path=ROOT/p['path'];assert hashlib.sha256(path.read_bytes()).hexdigest()==p['sha256']
 report=[]
 for kind,dirname in [('merged','Driftmetal Remnants'),('separate','Driftmetal Remnants RU'),('compatibility','Driftmetal Remnants Anim Patch')]:
  folder=OUT/kind/dirname
  for name in ['README.md','WALKERS.md']:shutil.copyfile(ROOT/name,folder/name)
  files=sorted(p for p in folder.rglob('*') if p.is_file())
  assert not any(p.suffix.lower() in ['.ini','.log','.po','.mo'] or '99_UserOverride' in str(p) for p in files)
  dest=OUT/('Driftmetal-Remnants-'+kind+'-RU-20261101.zip')
  with zipfile.ZipFile(dest,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as z:
   for p in files:
    info=zipfile.ZipInfo(p.relative_to(folder.parent).as_posix(),date_time=(2026,11,1,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED
    z.writestr(info,p.read_bytes(),compresslevel=6)
  with zipfile.ZipFile(dest) as z:
   assert z.testzip() is None
   assert set(z.namelist())=={p.relative_to(folder.parent).as_posix() for p in files}
   for p in files:assert hashlib.sha256(z.read(p.relative_to(folder.parent).as_posix())).digest()==hashlib.sha256(p.read_bytes()).digest()
  row={'path':str(dest.relative_to(ROOT)),'files':len(files),'size':dest.stat().st_size,'sha256':hashlib.sha256(dest.read_bytes()).hexdigest(),'crc_verified':True,'contents_verified':True};report.append(row);print(row,flush=True)
 (ROOT/'packages.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf8')
if __name__=='__main__':main()
