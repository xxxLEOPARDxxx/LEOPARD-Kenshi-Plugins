"""Extract actual native backend methods; fixtures supply only owned data/getter seams."""
from pathlib import Path
import argparse
HERE=Path(__file__).resolve().parent
parser=argparse.ArgumentParser();parser.add_argument('--source',type=Path,default=HERE/'NativeLocalLightSnapshot.h');args=parser.parse_args();text=args.source.read_text()
def extract(signature):
    start=text.index(signature);opening=text.index('{',start);at=opening+1;depth=1
    while depth:
        if text[at]=='{':depth+=1
        elif text[at]=='}':depth-=1
        at+=1
    return text[start:at]+'\n'
out=HERE/'_generated';out.mkdir(parents=True,exist_ok=True)
for name,signature in [('fault','inline int NativeFault('),('sin_import','    bool SinImport()'),('type','    bool LightType('),('light_raw','    bool ReadLightRaw('),('profile','    bool Profile(LONG generation)'),('scene','    const void*Scene()'),('state','    bool ReadState('),('pointers','    bool CopyPointers('),('light','    bool ReadLight(const void*')]:
    (out/('backend_'+name+'.inc')).write_text(extract(signature))
