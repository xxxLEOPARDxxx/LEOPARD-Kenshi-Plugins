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
                 'setUseRenderingDistance(useDistance)','setUseMinPixelSize(usePixels)'):
    assert property in compact,property
assert 'r->camera=binding;' in compact and 'CameraAccess::Last(c,last)' in compact
assert 'gameWorkspace->getListener()==ourListener' in compact
assert 'ManagerAccess::Contains(manager,shot)' in compact
assert 'if(!OnMain()||!LoadedGuard()' in compact
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
