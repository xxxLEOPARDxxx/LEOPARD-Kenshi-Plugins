"""Functional controls against the compiled production maximum-zoom policy."""
from pathlib import Path
import os, subprocess
BASE=Path(__file__).resolve().parents[1]
OUT=BASE/'build/zoom-policy-tests';OUT.mkdir(exist_ok=True)
HEADER=(BASE/'GsmZoomPolicy.h').read_text(encoding='utf-8')
TEST=(BASE/'tests/zoom_tests.cpp').read_text(encoding='utf-8').replace('../GsmZoomPolicy.h','GsmZoomPolicy.h')
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include;{OUT}',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
for name,old,new in (
 ('baseline',None,None),
 ('compound_on_reload','double(peak)*2*multiplier','double(maximum)*multiplier'),
 ('forget_original_limit','(saved_>minimum?saved_:minimum)','double(peak)*2'),
 ('fight_external_owner','if(owned_&&maximum!=applied_)','if(false)'),
 ('never_restore','if(!extend&&!owned_)return Unchanged;','if(!extend)return Unchanged;'),
 ('lose_coordinate_bound','>999999','>1e30'),
 ('repeat_writes','if(maximum==value)return Unchanged;',''),
):
    code=HEADER if old is None else HEADER.replace(old,new)
    if old is not None:assert old in HEADER,name
    (OUT/'GsmZoomPolicy.h').write_text(code,encoding='utf-8');(OUT/'case.cpp').write_text(TEST,encoding='utf-8')
    c=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2',str(OUT/'case.cpp'),f'/Fe{OUT}/case.exe',f'/Fo{OUT}/case.obj'],env=env,capture_output=True,text=True)
    if c.returncode:raise RuntimeError(c.stdout+c.stderr)
    r=subprocess.run([str(OUT/'case.exe')],capture_output=True,text=True)
    assert (r.returncode==0)==(old is None),(name,r.stdout)
    print(name,r.stdout.strip())
print('6 compiled zoom-policy controls rejected')
