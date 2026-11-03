from pathlib import Path
import hashlib,json,datetime,re
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
deps={}
for mode in ['Od','O2']:
 assert 'PASS 462 canonicalgroup/physicalcollision productioncontrols' in (OUT/('run-'+mode+'.log')).read_text()
 for line in (OUT/('build-'+mode+'.log')).read_text().splitlines():
  m=re.match(r'Note: including file:\s*(.+)',line)
  if m:
   p=Path(m.group(1).strip());assert p.is_file(),p;deps[str(p.resolve())]=sha(p)
files={p.name:sha(p) for p in sorted(HERE.iterdir()) if p.is_file() and p.name!='verification.json'}
game=Path('D:/SteamLibrary/steamapps/common/Kenshi');physical={}
for relative in ['resources.cfg','data/newland/Assets/Textures/BoneLoop_DIF_WHITE.dds','data/newland/land/textures/BoneLoop_DIF_WHITE.dds']:
 p=game/relative;physical[relative]={'bytes':p.stat().st_size,'sha256':sha(p)}
evidence={str(p.relative_to(HERE)):sha(p) for p in sorted(OUT.glob('*.log'))}
manifest={'scope':'Canonical resource group routing and real same-size physical DDS collision; fake metadata namespaces, actual production receipt/worker code, no native game API execution','status':'author controls PASS; independent review pending','recordedUtc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'sourceFiles':files,'SourceFactorySha256':sha(HERE.parent/'SourceFactory.h'),'NativeMetadataUnchangedSha256':sha(HERE.parent.parent/'source-receipts/NativeMetadata.h'),'dependencies':dict(sorted(deps.items())),'physicalInputs':physical,'evidence':evidence,'checksPerOptimization':462,'existingAdapterManifestSha256':sha(HERE.parent/'verification.json')}
(HERE/'verification.json').write_bytes((json.dumps(manifest,indent=2)+'\n').encode());print('Frozen canonical-source manifest',sha(HERE/'verification.json'))
