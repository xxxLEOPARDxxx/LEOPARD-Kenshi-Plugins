from pathlib import Path
import json,hashlib
HERE=Path(__file__).resolve().parent
OUT=HERE/'_generated'
assert (OUT/'shader-contract.json').is_file(),'ordered staged material shader proof is missing'
c=json.loads((OUT/'shader-contract.json').read_text())
assert c['nativeSha256']=='f9326a91f29e1f1644fc0acbb33bc126bce04590faef2cc7b89dabd1292fd9a1'
assert c['slots']==[0,1,2,3] and c['liveFields']==['albedo.rgba','normal.rgb','absorbance']
assert c['nativeCoreByteExact'] and c['nativeTailByteExact'] and c['nativeVertexUnchanged']
assert c['stageBindings']=={'cb':'b1','previousAlbedo':'t8','previousNormalAbsorb':'t9'}
assert c['carryBytes']==4194304 and c['stageConstantBytes']==16
for name in ['Partial','Tail']:
    assert (OUT/(name+'.cso')).stat().st_size>0
    reflection=(OUT/(name+'.reflection.txt')).read_text()
    assert 'float4 satBiome[225]' in reflection and 'Offset:  240 Size:  3600' in reflection
print('PASS ordered staged shader extraction, untouched native core/tail/VS and reflected explicit ABI')
