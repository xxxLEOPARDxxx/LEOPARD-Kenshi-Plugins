from pathlib import Path
import hashlib,json,sys
here=Path(__file__).resolve().parent
sys.path.insert(0,str(here.parents[2]/'GlobalSatelliteMap/tests'))
from test_pe_contract import PE
p=json.loads((here/'geography-inputs.json').read_text());game=Path('D:/SteamLibrary/steamapps/common/Kenshi')
assert len(p['files'])==92 and len(p['biomes'])==15 and len(p['cells'])==64
for f in p['files']+[p['blend']]:
    data=(game/f['path']).read_bytes();assert len(data)==f['bytes'] and hashlib.sha256(data).hexdigest()==f['sha256']
pe=PE(game/'RE_Kenshi/Kenshi_x64.exe')
assert hashlib.sha256(pe.data).hexdigest()=='504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e'
assert pe.data[pe.offset(0x9d5840):pe.offset(0x9d5840)+8]==bytes.fromhex('488b8120821600c3')
assert pe.data[pe.offset(0xb0bb0):pe.offset(0xb0bb0)+3]==bytes.fromhex('448928') # Missing float value initialized zero.
q=lambda s:json.dumps(s,ensure_ascii=True)
lines=['struct GeoFile {const char* path;const char* sha256;unsigned bytes,bank,layer,format;};','static const GeoFile GeoFiles[]={']
lines += ['{%s,%s,%su,%su,%su,%su},'%(q(f['path']),q(f['sha256']),f['bytes'],f['bank'],f['layer'],f['format']) for f in p['files']]
lines += ['};','struct GeoBiome {const char* sid;unsigned diffuse[6],normal[6];};','static const GeoBiome GeoBiomes[]={']
lines += ['{%s,{%s},{%s}},'%(q(b['sid']),','.join(map(str,b['diffuse'])),','.join(map(str,b['normal']))) for b in p['biomes']]
lines += ['};','static const unsigned GeoSlots[64][5]={']+['{'+','.join(map(str,c['slots']))+'},' for c in p['cells']]+['};']
lines += ['static const BYTE GeoSelector[256]={'+','.join(str(v) for c in p['cells'] for v in c['selector'])+'};']
lines += ['static const char* GeoTreeHashes[64]={']+[q(c['tree_sha256'])+',' for c in p['cells']]+['};']
f=p['blend'];lines+=['static const GeoFile GeoBlend={%s,%s,%su,0,0,0};'%(q(f['path']),q(f['sha256']),f['bytes'])]
lines += ['static const NativeCheck GeoNativeChecks[]={']
for rva in (0x9d5840,0xb0ba7):
    raw=pe.data[pe.offset(rva):pe.offset(rva)+24]
    lines+=['{0x%x,{%s}},'%(rva,','.join('0x%02x'%b for b in raw))]
lines+=['};']
(here/'build/GeographyInputs.h').write_text('\n'.join(lines)+'\n',encoding='ascii')
print('PASS guarded92DDS/15SID/64coarse manifest and exact native missing-float zero initialization')
