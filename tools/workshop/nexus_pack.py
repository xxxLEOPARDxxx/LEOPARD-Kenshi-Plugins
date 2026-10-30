# -*- coding: utf-8 -*-
"""Prepare Nexus ZIPs. Full release workflow: prepare_release.py [Mod ...]."""
import os,sys,zipfile,re
from pathlib import Path
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
import build
OUT=os.path.join(build.HERE,"nexus")
def pack(item):
    mod=item["mod"];src=build.mod_dir(mod);src_ini=build.defaults_ini(mod)
    out=Path(OUT).resolve();out.mkdir(parents=True,exist_ok=True)
    path=out/(mod+"-"+item["version"]+".zip");temporary=path.with_suffix(".zip.tmp")
    n=0
    with zipfile.ZipFile(temporary,"w",zipfile.ZIP_DEFLATED) as z:
        for folder,dirs,names in os.walk(src):
            dirs[:]=[d for d in dirs if d.lower() not in build.SKIP_DIRS and not d.lower().endswith("-source")]
            for name in sorted(names):
                if not build.runtime_file_allowed(mod,folder,name):continue
                if folder==src and (build.is_player_ini(mod,name) or (src_ini and name.lower().endswith(".defaults.ini"))):continue
                full=build.runtime_source_path(mod,folder,name);rel=os.path.relpath(os.path.join(folder,name),src).replace(os.sep,"/")
                z.write(full,mod+"/"+rel);n+=1
        if src_ini:
            base="CharacterInspector" if mod=="Character Inspector" else mod
            z.write(src_ini,mod+"/"+base+".defaults.ini");n+=1
    with zipfile.ZipFile(temporary) as z:
        if z.testzip() is not None:raise RuntimeError("Archive CRC failed: "+str(temporary))
        names=set(z.namelist())
        if any(x.endswith(".po") and x[:-3]+".mo" in names for x in names):raise RuntimeError("Duplicate locale formats")
    os.replace(temporary,path)
    pattern=re.compile(r"^"+re.escape(mod)+r"-[0-9][^/\\]*\.zip$")
    for old in out.iterdir():
        if old==path or not pattern.fullmatch(old.name):continue
        if old.resolve().parent!=out or old.suffix.lower()!=".zip":raise RuntimeError("Unsafe old archive path")
        old.unlink()
        print("Removed superseded archive:",old.name)
    return str(path),n
if __name__=="__main__":
    only=set(sys.argv[1:])
    for item in build.ITEMS:
        if only and item["mod"] not in only:continue
        if not build.mod_dir(item["mod"]):continue
        path,n=pack(item);print(item["mod"],n,os.path.basename(path))
