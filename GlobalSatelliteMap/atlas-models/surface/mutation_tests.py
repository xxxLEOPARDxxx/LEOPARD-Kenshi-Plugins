from pathlib import Path
import os,subprocess
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
header=(HERE/'AtlasModelSurface.h').read_text()
env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include;{HERE}',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
cases=[
 ('skip-copy','for(unsigned i=0;i<3;++i)c.p->CopyResource(texture_[i].p,input[i].p);','','all bytes exact, including HDR/NaN/coverage/height'),
 ('copy-wrong-direction','c.p->CopyResource(texture_[i].p,input[i].p);','c.p->CopyResource(input[i].p,texture_[i].p);','all bytes exact, including HDR/NaN/coverage/height'),
 ('skip-height','i<3;++i)c.p->CopyResource','i<2;++i)c.p->CopyResource','all bytes exact, including HDR/NaN/coverage/height'),
 ('split-cap','return caller<=AtlasView::GpuCap-PrivateBytes;','return caller<=402653184ULL-PrivateBytes;','inclusive shared192MiB budget'),
 ('skip-copy-budget','!device_.p||!sources||!Fits(caller)','!device_.p||!sources','copy plus1 budget refused'),
 ('own-as-pristine','if(AtlasView::Identity(input[i].p,texture_[j].p))return E_INVALIDARG;','','own output cannot become pristine base'),
 ('lose-read-restore','AtlasView::SavedOutputReads r0(c.p,texture_[0].p),r1(c.p,texture_[1].p),r2(c.p,texture_[2].p);','ID3D11ShaderResourceView*empty=NULL;c.p->VSSetShaderResources(17,1,&empty);c.p->HSSetShaderResources(31,1,&empty);c.p->DSSetShaderResources(63,1,&empty);c.p->GSSetShaderResources(127,1,&empty);c.p->PSSetShaderResources(109,1,&empty);c.p->CSSetShaderResources(121,1,&empty);','all six stage reads, OM/UAV/predicate and all untouched state restored'),
 ('omit-predicate','AtlasWater::SavedPredicate predicate(c.p);','','all bytes exact, including HDR/NaN/coverage/height'),
 ('ignore-write-hazards','if(AtlasView::Bound(c.p,input[i].p,true)||AtlasView::Bound(c.p,texture_[i].p,true))return DXGI_ERROR_INVALID_CALL;','if(false)return DXGI_ERROR_INVALID_CALL;','source RT hazard refused'),
 ('retain-input','return device_.p->GetDeviceRemovedReason();','sources[0]->AddRef();return device_.p->GetDeviceRemovedReason();','no retained source SRV references'),
]
for name,old,new,reason in cases:
    assert old in header,name
    text=header.replace(old,new).replace('"../../atlas-water/AtlasWater.h"','"'+(HERE.parents[1]/'atlas-water/AtlasWater.h').as_posix()+'"')
    path=OUT/(name+'.h');path.write_text(text);exe=OUT/(name+'.exe')
    cmd=[VC/'bin/amd64/cl.exe','/nologo','/EHsc','/MD','/O2','/W1','/DSURFACE_HEADER="'+path.as_posix()+'"','/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),HERE/'warp_tests.cpp','/link','d3d11.lib','dxgi.lib']
    p=subprocess.run([str(a) for a in cmd],env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);(OUT/(name+'-build.log')).write_text(p.stdout)
    if p.returncode:raise RuntimeError(name+': '+p.stdout)
    p=subprocess.run([str(exe)],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);(OUT/(name+'-run.log')).write_text(p.stdout)
    if not p.returncode or 'FAIL '+reason not in p.stdout:raise RuntimeError('survived or unfair '+name+': '+p.stdout)
    print('Expected RED:',name,'->',reason)
print('PASS ten compiled real WARP surface mutants rejected')
