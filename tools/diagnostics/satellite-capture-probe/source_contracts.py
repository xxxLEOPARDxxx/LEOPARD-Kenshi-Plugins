from pathlib import Path
import re
p=Path(__file__).resolve().parent
source=(p/'CaptureProbe.cpp').read_text(encoding='utf-8')
compact=re.sub(r'\s+','',source)
assert 'setProjectionType(Ogre::PT_ORTHOGRAPHIC)' in source
assert 'getTerrainHeight' not in source
assert 'const int Side = 512;' in source
assert 'activateZone' not in source and 'showDistantTown(' not in source
assert 'setEnabled(false)' in source
assert 'ExposureScopeexposure;ProxyScopeproxy;exposure.Apply();proxy.Apply();' in compact
for property in ('setPosition(position)','setOrientation(orientation)','setOrthoWindow(width,height)',
                 'setProjectionType(type)','setFOVy(fov)','setNearClipDistance(nearZ)',
                 'setFarClipDistance(farZ)','setAutoAspectRatio(autoAspect)',
                 'setUseRenderingDistance(useDistance)','nativeApi.setMin(c,usePixels)'):
    assert property in compact,property
assert 'r->camera=binding;' in compact and 'nativeApi.notify(c,last)' in compact
for obsolete in ('CameraAccess::', '->setUseMinPixelSize(', '->getUseMinPixelSize(',
                 '->getLastViewport(', '->getCullingFrustum(', 'terrain->mCamera', 'terrain->mPos'):
    assert obsolete not in source, obsolete
assert 'EnsureRenderAbi();' in source and '!RenderAbiMatches()' in source
assert 'NativeTerrainCache::Matches' in source and 'restorationSafe=false;chainSafe=false;' in compact
assert 'gameWorkspace->getListener()==ourListener' in compact
assert 'ManagerAccess::Contains(manager,shot)' in compact
assert 'if(!restorationSafe||!chainSafe||!OnMain()||!LoadedGuard()' in compact
assert 'if(!au||au->terrain!=terrain||!ReadTerrainBlood(terrain,after)' in compact
assert '!NativeTerrainCache::BloodMatches(blood,after,mainCamera)||!WriteTerrainCache(terrain,state)' in compact
assert 'unsafe borrowed camera at cleanup' in source
assert 'Terrain helper must already borrow main LOD camera' in source
assert compact.index('constructionrestorationfailed') < compact.rindex('gameWorkspace=renderer->workspace;')
assert 'EnableRequestFile",0' in source and 'existing token latched without replay' in source
assert 'main/probe GBuffer aliasing refused' in source
assert '(GetAsyncKeyState' in compact and 'press=down&&!keyHeld' in compact
assert 'NativeReconstructedX' in (p/'ProbePolicy.h').read_text()
assert 'ProbeOwnership::AttachOwnedCamera(own,c,ownNode)' in compact
assert 'ownNode->attachObject(own)' not in compact
assert 'ownNodeId=ownNode->getId();' in compact
assert 'cleanup=true;' in compact
assert 'findCameraNoThrow("LEOPARD_PrivateCaptureCamera")==own' in compact
assert not list(p.glob('*.bmp')),'No capture was invoked'
print('Capture bounds/restoration/source contracts passed; tests do not invoke the renderer')

# Verify the built PE, not only the source declaration. RE_Kenshi uses the C++ name.
import sys
sys.path.insert(0, str(p.parents[2] / 'GlobalSatelliteMap/tests'))
from test_pe_contract import verify
verify(p/'build/SatelliteCaptureProbe.dll', imports_root='D:/SteamLibrary/steamapps/common/Kenshi')
rejected = 0
for name in ('GlobalSatelliteMap', 'SatelliteCaptureProbe'):
    bad = p/'build'/f'rejected-C-startup-{name}.dll'
    if not bad.exists():
        continue
    try:
        verify(bad, mcm=name == 'GlobalSatelliteMap')
    except ValueError as error:
        assert 'missing exact export ?startPlugin@@YAXXZ' in str(error)
        rejected += 1
    else:
        raise AssertionError(f'historical broken DLL was accepted: {bad}')
print(f'Actual PE startup contract passed; {rejected} historical C-startup DLLs rejected')

experiment=(p/'TerrainExperiment.h').read_text()
ec=re.sub(r'\s+','',experiment)
assert 'api.createObject(scene,"Terrain",api.memory(scene,Ogre::SCENE_STATIC),NULL)' in ec
assert 'api.scale(terrain,294912,9800)' in ec and 'api.limits(terrain,32,8)' in ec
assert ec.index('api.destroyObject(scene,terrain)') < ec.index('scene->destroyCamera(camera)') < ec.index('api.destroyScene(root,scene)')
assert 'setMaterialCallbacks' not in experiment and 'new Terrain' not in experiment
assert 'activation' not in experiment and 'loadGame' not in experiment
assert 'IsolatedTerrain",0' in source and 'terrainExperiment.root' in source
assert 'fresh==au->render->scene' in experiment and 'object==au->terrain' in experiment
assert 'mainGb.textures[i]==gb.textures[i]' in experiment
assert ec.index('worker.StopAndJoin(before.request,5000)') < ec.index('api.destroyObject(scene,terrain)') < ec.index('worker.Complete(closed)')
worker=(p/'NativeWorkerHandle.h').read_text()
assert 'GetProcessIdOfThread' in worker and 'DuplicateHandle' in worker
assert '_InterlockedExchange8' in worker and 'TerminateThread' not in worker
assert 'typedefTerrainExperimentPolicy::Stats*(__fastcall*Info)(constTerrain*,TerrainExperimentPolicy::Stats*)' in ec
print('Private native Terrain ownership/aggregate ABI/default-off contracts passed')

assert 'LogResourceTelemetry("before-private-terrain")' in source
assert 'LogResourceTelemetry("after-private-terrain")' in source
assert 'NativeResourceBytesName' in source and 'HasPrivateSceneName' in source
assert 'telemetry only; no resource eviction' in source
print('Private resource telemetry uses exact exports without cache eviction')
