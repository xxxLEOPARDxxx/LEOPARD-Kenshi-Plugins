"""Extract the production Read method for bounded fake-native dispatch seams."""
from pathlib import Path
import argparse
HERE=Path(__file__).resolve().parent
parser=argparse.ArgumentParser();parser.add_argument('--source',type=Path,default=HERE/'NativeWaterSnapshot.h');args=parser.parse_args()
text=args.source.read_text()
def extract(signature):
    start=text.index(signature)
    opening=text.index('{',start);depth=1;end=opening+1
    while depth:
        if text[end]=='{':depth+=1
        elif text[end]=='}':depth-=1
        end+=1
    return text[start:end]+'\n'
out=HERE/'_generated/native-snapshot';out.mkdir(parents=True,exist_ok=True)
(out/'native_snapshot_dispatch.inc').write_text(extract('    HRESULT Read(bool enabled,'))
(out/'native_snapshot_basis.inc').write_text(extract('inline void StockBasis()'))
(out/'native_snapshot_texture.inc').write_text(extract('inline void UnitTexture('))
