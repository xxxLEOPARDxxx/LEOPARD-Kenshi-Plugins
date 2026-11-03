"""Stage exact base-fixture inputs beside a build; never install or access a process."""
from pathlib import Path
import argparse,json,hashlib,shutil
HERE=Path(__file__).resolve().parent
COMPACT=HERE.parents[2]/'GlobalSatelliteMap/atlas-terrain/compact-material'
GAME=Path('D:/SteamLibrary/steamapps/common/Kenshi')
parser=argparse.ArgumentParser();parser.add_argument('--output',type=Path,default=HERE/'build/fixed-terrain-inputs');args=parser.parse_args()
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
contract=COMPACT/'_generated/contract.json'
assert sha(contract)=='afe79b4152ba35cc364e4f7c92a59771a428d4dec2d63217d442bfba2993f0b9'
j=json.loads(contract.read_text());args.output.mkdir(parents=True,exist_ok=True);pins={}
def copy(source,name,expected):
 assert sha(source)==expected,(source,'input pin mismatch')
 out=args.output/name;shutil.copyfile(source,out);assert sha(out)==expected
 pins[name]=dict(bytes=out.stat().st_size,sha256=expected,source=str(source))
for i,a in enumerate(j['dds']):copy(GAME/a['path'],f'dds{i:02}.bin',a['sha256'])
for i,a in enumerate(j['rgba']):copy(COMPACT/f'_generated/image{i}.rgba',f'image{i}.rgba',a['decodedRgbaSha256'])
copy(COMPACT/'_generated/height.u16','height.u16','4c2943629387364ee0bfd9f630656affd94fdc7671ab0754b5199e8f3109d3f2')
(args.output/'source-pins.json').write_text(json.dumps(dict(scope='prepared fixed base fixture only; no active resource-winner proof',materialContractSha256=sha(contract),inputs=pins),indent=2)+'\n')
print('PASS staged',len(pins),'exact fixed inputs;',sum(p['bytes'] for p in pins.values()),'bytes;',args.output)
