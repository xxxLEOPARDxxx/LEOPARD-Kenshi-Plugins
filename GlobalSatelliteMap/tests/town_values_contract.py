from pathlib import Path
import hashlib,os,re,subprocess
from test_pe_contract import PE
base=Path(__file__).resolve().parents[1];header=(base/'GsmTownValues.h').read_text();out=base/'build/town-values-review';out.mkdir(exist_ok=True)
p=PE(Path('D:/SteamLibrary/steamapps/common/Kenshi/RE_Kenshi/Kenshi_x64.exe'))
assert hashlib.sha256(p.data).hexdigest()=='504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e'
pins=re.findall(r'\{0x([0-9a-f]+),\{([^}]+)\}\}',header);assert len(pins)==2
for r,values in pins:
    code=bytes(int(v,0) for v in values.split(','));assert len(code)==24;at=p.offset(int(r,16));assert p.data[at:at+24]==code
assert p.string(0x1685d90)=='distant_' and p.string(0x1685d88)=='.mesh'
for banned in ['getPosition(', 'CreateFile','WriteFile','GetModuleHandle','NativeModels','NativeWater','std::string','new ','->load','setVisible']:
    assert banned not in header,banned
print('Town values exact504b filename/id/stored-position source contract PASS')
fixture=(base/'tests/town_values_tests.cpp').read_text();vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
mutants=[
('wrong-id-field','identity.town+0x64','identity.town+0x60'),
('skip-field-profile','||!Fields(engine)',''),
('ignore-coordinate-finiteness','if(!_finite(value.storedXYZ[i]))return false;',''),
('omit-value-postcheck','if(record>=next.count||!SameValue(identity,next.towns[record++]))return E_ABORT;','++record;'),
('omit-array-postcheck','if(GsmTownDiagnostic::Pointer(reinterpret_cast<const BYTE*>(first.array+i))!=identity.town)return E_ABORT;',''),
('ignore-world-generation','return InterlockedCompareExchange(live,0,0)==expected;','return true;'),
('ignore-proxy-owner','if(GsmTownDiagnostic::Pointer(identity.proxy)!=identity.town)return false;',''),
]
for name,before,after in mutants:
    assert before in header,name
    changed=header.replace(before,after).replace('#include "GsmTownDiagnostic.h"','#include "'+(base/'GsmTownDiagnostic.h').as_posix()+'"')
    if name=='ignore-proxy-owner':changed=changed.replace('GsmTownDiagnostic::Pointer(identity.proxy)==identity.town&&','')
    path=out/(name+'.h');path.write_text(changed);cpp=out/(name+'.cpp');cpp.write_text(fixture.replace('#include "../GsmTownValues.h"','#include "'+path.as_posix()+'"'));exe=out/(name+'.exe')
    with (out/(name+'-compile.log')).open('w') as log:
        subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/GL','/W3','/Fe'+str(exe),'/Fo'+str(out/(name+'.obj')),str(cpp),'/link','/LTCG'],env=env,check=True,stdout=log,stderr=subprocess.STDOUT)
    ran=subprocess.run([str(exe)],capture_output=True,text=True);(out/(name+'-run.log')).write_text(ran.stdout+ran.stderr)
    assert ran.returncode and 'FAIL ' in ran.stderr,(name,ran.stdout,ran.stderr)
    print('Town value compiled functional mutant rejected:',name)
