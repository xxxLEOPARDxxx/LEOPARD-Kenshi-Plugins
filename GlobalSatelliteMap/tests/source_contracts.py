"""Integration contracts complement native policy tests; they are not live UI proof."""
from pathlib import Path
import gettext
import json
import re
base = Path(__file__).resolve().parents[1]
raw_source = (base / 'GlobalSatelliteMap.cpp').read_text()
source = re.sub(r'\s+', '', raw_source)
assert 'mapImage->setImageTexture' not in source
assert 'map->mapImage =' not in source
assert 'w->setDepth(1000000)' in source
assert 'w->setNeedMouseFocus(false)' in source and 'w->setNeedKeyFocus(false)' in source
assert 'WidgetReflayer,status,tiles[16]' in source
assert 'Clear();session.Reset()' in source
assert 'SaveFileSystem::loadGame' in source and 'SaveFileSystem::newGame' in source
assert 'job->generation==InterlockedCompareExchange' in source
assert 'WaitForSingleObject(job->thread,0)' in source
assert 'CloseHandle(job->thread)' in source
assert 'nextTick=now+200' in source and 'booluploaded=false' in source
assert 'cacheBytes<=GSM::CacheBudget-bytes' in source
assert 'api->toggle(api,ID,"Debug"' in source
assert source.rfind('api->toggle(api,ID,"Debug"') > source.rfind('api->toggle(api,ID,"Enabled"')
assert 'Enabled=0' in (base / 'GlobalSatelliteMap.ini').read_text()
assert 'Debug=0' in (base / 'GlobalSatelliteMap.ini').read_text()
worker = re.sub(r'\s+', '', (base / 'CaptureValidation.cpp').read_text())
assert 'activateZone' not in worker + source
assert 'Ogre::' not in worker and 'MyGUI::' not in worker
assert '512ull*1024*1024' in worker and 'files.size()>=4096' in worker
assert 'NativeSave(j,after)' in worker
assert not (base / 'captures').exists(), 'No fabricated capture assets are bundled'
keys = {
    ''.join(json.loads(token) for token in re.findall(r'"(?:[^"\\]|\\.)*"', expression))
    for expression in re.findall(r'Tr\(\s*((?:"(?:[^"\\]|\\.)*"\s*)+)\)', raw_source)
}
for lang in ('en_GB', 'ru_RU', 'zh_CN'):
    with (base / 'locale' / lang / 'LC_MESSAGES' / 'global_satellite_map.mo').open('rb') as f:
        catalog = gettext.GNUTranslations(f)
    assert keys <= catalog._catalog.keys(), (lang, keys - catalog._catalog.keys())
    assert all(catalog.gettext(k) for k in keys)
print(f'Integration contracts passed; {len(keys)} strings verified in each of 3 compiled locales')
