from pathlib import Path
import re,json
here=Path(__file__).resolve().parent
native=(here/'NativeCensus.h').read_text(encoding='utf-8-sig');route=(here/'CensusRoute.h').read_text(encoding='utf-8-sig')
pins=(here.parent/'_generated/ModelProfile.h').read_text();checks=0
for prefix in re.findall(r'NativeModels::(?:Get<[^;\n]*?|Pin)\([^\n;]*?"([?][^"]+)"',native):
 assert prefix in pins,prefix;checks+=1
for banned in ['_getTexturePtr','GetDevice','AddRef','Release(', 'createEntity','loadEntity','loadParts','makeInstance','makeEntity','setVisible','_getFullTransformUpdated']:
 assert banned not in native,banned;checks+=1
assert 'NativeModels::Profile();' in native and 'CensusPins[i].bytes,24' in native;checks+=1
assert 'manager_->town!=town_' in native and 'NativeModels::Registered(collection_,e)' in native;checks+=1
assert 'Physical(s.building,s.entity)!=s.physical' in native and 's.material[j]' in native;checks+=1
assert native.index('if(!Current())return E_ABORT')<native.index('manager_->buildingEntities');checks+=1
assert 'out=next;candidate=chosen;' in route and 'if(!ops.End(chosen))return E_ABORT' in route;checks+=1
p=json.loads((here/'_generated/profile.json').read_text());assert len(p['windows'])==14 and '0x92fd2b' in p['windows'];checks+=1
print('PASS',checks,'production source/profile/side-effect contracts')
