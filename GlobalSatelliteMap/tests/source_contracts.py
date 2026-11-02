"""Viewer wiring/catalog contracts; native WARP and worker checks are separate."""
from pathlib import Path
import gettext,json,re
base=Path(__file__).resolve().parents[1]
raw=(base/'GlobalSatelliteMap.cpp').read_text()
s=re.sub(r'\s+','',raw)
assert 'mapImage->setImageTexture' not in s and 'map->mapImage=' not in s
assert 'w->setDepth(1000000)' in s
assert 'w->setNeedMouseFocus(false)' in s and 'w->setNeedKeyFocus(false)' in s
assert 'WidgetReflayer,status,tiles[AtlasView::WindowSlots]' in s
assert 'SaveFileSystem::loadGame' in s and 'SaveFileSystem::newGame' in s
assert 'j->generation==InterlockedCompareExchange' in s
assert 'WaitForSingleObject(job->thread,0)' in s and 'CloseHandle(job->thread)' in s
assert 'tickClock.Due(now,200)' in s and 'lightClock.Due(now,1000)' in s
assert 'tickClock.Reset();lightClock.Reset();if(layer.get())' in s
assert 'AtlasView::WindowGpuBytes()>AtlasView::GpuCap' in s
assert 'plan.Contains(key)&&FindWindow(key)<0' in s
assert 'WindowVictim(keys,age,plan)' in s
assert 'WindowKey' in s and 'cache[i].level!=desiredLevel' not in s
assert 'next=(i+1)%AtlasView::WindowSlots' in s and 'NeedsWindowShade(onScreen,plan.Contains(e.key),e.shaded,waterDirty?0:lightVersion,pass)' in s
assert 'shaded<4' in s and 'manifest.windows.Hashes(key,j->windowHashes)' in s
assert 'GsmAtlas::Run' in s and 'GsmNative::Srv(e.texture.get())' in s
assert 'getActiveSave' not in s and 'manifest.gsm' not in s
assert 'GetModuleFileNameW' in s and 'SharedIniDefaults/MCMuseWindowsACP' in s
assert s.rfind('api->toggle(api,ID,"Debug"')>s.rfind('api->toggle(api,ID,"AmbientLight"')
# Visible warning uses supported v4 custom UI: info is only a list tooltip.
assert 'api->customArea(api,&AttachDiskWarning,&DetachDiskWarning,NULL,15)' in s
assert 'w->setTextColour(MyGUI::Colour(1.0f,0.2f,0.2f))' in s
assert 'w->setEditWordWrap(true)' in s and 'w->setEditStatic(true)' in s and 'diskWarning.get()->getParent()==p' in s
assert 'void__cdeclDetachDiskWarning(void*)' in s and 'diskWarning.reset()' in s
warning='WARNING: Before rendering 4x4 tiles at 8192x8192, allow at least 24 GB free.\\nUncompressed terrain data: about 23 GB.'
assert warning in raw
ini=(base/'GlobalSatelliteMap.ini').read_text()
for key in ('Enabled=0','Sunlight=1','AmbientLight=1','Water=0','Debug=0'):assert key in ini
worker=(base/'GsmAtlasJob.h').read_text()
for token in ('8ULL*1024*1024*1024','65536','512','CurrentSources(job)!=currentSources','seen!=currentSources','GSM::generation','job.read.afterChunk=ObserveGeneration','char error[512]','FILE_ATTRIBUTE_REPARSE_POINT','CompareStringOrdinal','source path control/alias refused'):assert token in worker,token
assert 'Ogre::' not in worker and 'MyGUI::' not in worker and 'activateZone' not in worker
assert 'job.error=e.what' not in worker
native=(base/'GsmNativeLight.h').read_text()
for token in ('ownedViews_[i].p=views[i];views[i]->AddRef()','Resolve_HDR','rt_adaptedLum','GsmSamplerWindows','SharedSkyParams','SharedDeferredParams'):assert token in native,token
keys={''.join(json.loads(t) for t in re.findall(r'"(?:[^"\\]|\\.)*"',e)) for e in re.findall(r'Tr\(\s*((?:"(?:[^"\\]|\\.)*"\s*)+)\)',raw)}
for lang in ('en_GB','ru_RU','zh_CN'):
 with (base/'locale'/lang/'LC_MESSAGES/global_satellite_map.mo').open('rb')as f:catalog=gettext.GNUTranslations(f)
 assert keys<=catalog._catalog.keys(),(lang,keys-catalog._catalog.keys())
 assert all(catalog.gettext(k) for k in keys)
print(f'Viewer source/MCM contracts PASS; {len(keys)} messages x3locales,5live settings,Debuglast')
