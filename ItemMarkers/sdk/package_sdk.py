"""Create a small standalone author SDK; no game DLL or player files."""
from pathlib import Path
import zipfile,hashlib,json
root=Path(__file__).resolve().parents[1]
destination=Path('D:/DEV/Kenshi/workshop/sdk/ItemMarkers-SDK-v1.zip')
destination.parent.mkdir(parents=True,exist_ok=True)
sources={
    'ItemMarkers-SDK-v1/ItemMarkersApi.h':root.parent/'shared'/'ItemMarkersApi.h',
    'ItemMarkers-SDK-v1/README.txt':root/'sdk'/'README.txt',
    'ItemMarkers-SDK-v1/GradeProviderExample.cpp':root/'sdk'/'GradeProviderExample.cpp',
    'ItemMarkers-SDK-v1/backpack-example.ini.disabled':root/'grades'/'backpack-example.ini.disabled',
}
with zipfile.ZipFile(destination,'w',compression=zipfile.ZIP_DEFLATED) as archive:
    for name,path in sources.items():archive.write(path,name)
with zipfile.ZipFile(destination) as archive:
    assert archive.testzip() is None and set(archive.namelist())==set(sources)
    for name,path in sources.items():assert archive.read(name)==path.read_bytes()
print(json.dumps({'sdk':str(destination),'bytes':destination.stat().st_size,'sha256':hashlib.sha256(destination.read_bytes()).hexdigest(),'files':list(sources)},indent=2))
