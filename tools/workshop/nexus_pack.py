# -*- coding: utf-8 -*-
"""Prepare Nexus ZIPs. Full release workflow: prepare_release.py [Mod ...]."""
import os,sys,zipfile,re,hashlib
from pathlib import Path
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
import build
OUT=os.path.join(build.HERE,"nexus")
def pack(item):
    mod=item["mod"];src=build.mod_dir(mod);src_ini=build.defaults_ini(mod)
    out=Path(OUT).resolve();out.mkdir(parents=True,exist_ok=True)
    path=out/(mod+"-"+item["version"]+".zip");temporary=path.with_suffix(".zip.tmp")
    n=0;expected={}
    with zipfile.ZipFile(temporary,"w",zipfile.ZIP_DEFLATED) as z:
        for folder,dirs,names in os.walk(src):
            dirs[:]=[d for d in dirs if d.lower() not in build.SKIP_DIRS and not d.lower().endswith("-source")]
            for name in sorted(names):
                if not build.runtime_file_allowed(mod,folder,name):continue
                if folder==src and (build.is_player_ini(mod,name) or (src_ini and name.lower().endswith(".defaults.ini"))):continue
                full=build.runtime_source_path(mod,folder,name);rel=os.path.relpath(os.path.join(folder,name),src).replace(os.sep,"/")
                member=mod+"/"+rel
                z.write(full,member);expected[member]=hashlib.sha256(Path(full).read_bytes()).hexdigest();n+=1
        if src_ini:
            base="CharacterInspector" if mod=="Character Inspector" else mod
            member=mod+"/"+base+".defaults.ini"
            z.write(src_ini,member);expected[member]=hashlib.sha256(Path(src_ini).read_bytes()).hexdigest();n+=1
    with zipfile.ZipFile(temporary) as z:
        if z.testzip() is not None:raise RuntimeError("Archive CRC failed: "+str(temporary))
        names=set(z.namelist())
        if len(z.namelist())!=len(expected) or names!=set(expected):raise RuntimeError("Archive composition differs from candidate")
        for name,digest in expected.items():
            if hashlib.sha256(z.read(name)).hexdigest()!=digest:raise RuntimeError("Archive payload differs from candidate: "+name)
        if any(x.endswith(".po") and x[:-3]+".mo" in names for x in names):raise RuntimeError("Duplicate locale formats")
        if os.environ.get('KENSHI_RELEASE_INPUT_ROOT'):
            if any(x.lower().endswith('.po') for x in names):raise RuntimeError('Clean candidate must contain MO runtime catalogs only')
            catalogs=[x for x in names if x.endswith('.mo')]
            languages={x.split('/locale/')[1].split('/')[0] for x in catalogs if '/locale/' in x}
            if len(catalogs)!=3 or languages!={'ru_RU','en_GB','zh_CN'}:raise RuntimeError('Candidate must contain exactly three runtime languages')
            if not any(Path(x).name.lower() in ('readme.md','readme.txt') for x in names):raise RuntimeError('Candidate README missing')
            source=build.source_dir(mod)
            if source:
                for license_file in Path(source).iterdir():
                    if license_file.is_file() and license_file.name.upper().startswith(('LICENSE','COPYING')) and not any(Path(x).name==license_file.name for x in names):
                        raise RuntimeError('Source license omitted from candidate: '+license_file.name)
            if any(x.lower().endswith('.log') or '99_useroverride' in x.lower() or Path(x).name=='mod-config.json' for x in names):raise RuntimeError('Private runtime files in candidate archive')
            for x in names:
                if x.lower().endswith('.ini') and not x.lower().endswith('.defaults.ini') and not ('/SourceData/' in x and Path(x).name in {'00_Vanilla.ini','50_OfficialExtensions.ini','90_ModSupport.ini'}):
                    raise RuntimeError('Personal INI in candidate archive: '+x)
                if x.lower().endswith('.defaults.ini') and re.search(r'(?mi)^Debug\s*=\s*(1|true)\s*$',z.read(x).decode('utf-8-sig',errors='replace')):
                    raise RuntimeError('Detailed logging is enabled in factory defaults: '+x)
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
