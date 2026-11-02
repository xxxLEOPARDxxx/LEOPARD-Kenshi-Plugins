"""Compile the real one-shot catalog caller against a value-only source seam."""
from pathlib import Path
import os
import subprocess

base = Path(__file__).resolve().parents[1]
source = (base / 'GlobalSatelliteMap.cpp').read_text(encoding='utf-8')
assert 'TownValueDiagnostic();' in source[source.index('void Draw('):], 'Catalog must not depend on an existing native proxy'
start = source.index('void TownValueDiagnostic()')
opening = source.index('{', start)
depth = 1
end = opening + 1
while depth:
    depth += (source[end] == '{') - (source[end] == '}')
    end += 1
body = source[start:end]
assert 'townValuesLogged=false' in source[source.index('void Clear()'):source.index('void Clear()') + 700]
out = base / 'build/town-value-caller'
out.mkdir(exist_ok=True)
fixture = r'''
#include <Windows.h>
#include <sstream>
#include <string>
#include <vector>
#include <cstdio>
static bool debug=false,ready=false,townValuesLogged=false;
static LONG observedGeneration=7;
namespace GSM {volatile LONG generation=7;}
static const void*shou=reinterpret_cast<void*>(1);
static unsigned queries=0;static HRESULT result=S_OK;static bool changeWorld=false;
static std::vector<std::string>lines;
static void Diagnostic(const std::string&s){lines.push_back(s);}
namespace GsmTownValues {
struct TownValue {unsigned id;float storedXYZ[3];int proxyState;bool hasProxy;};
struct Result {LONG generation;unsigned total,count,nullEntries,otherTypes;TownValue towns[3];Result():generation(7),total(3),count(3),nullEntries(0),otherTypes(0){for(unsigned i=0;i<3;++i){towns[i].id=100+i;towns[i].storedXYZ[0]=float(i+1);towns[i].storedXYZ[1]=float(i+2);towns[i].storedXYZ[2]=float(i+3);towns[i].proxyState=int(i);towns[i].hasProxy=i!=0;}}};
HRESULT Read(bool enabled,bool verified,const void*k,const void*m,LONG gen,volatile LONG*live,Result&r){++queries;if(!enabled||!verified||k!=shou||!m||gen!=7||live!=&GSM::generation)throw 1;if(changeWorld)InterlockedIncrement(live);return result;}
}
BODY
static unsigned checks=0;static void C(bool ok,const char*s){++checks;if(!ok){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}}
int main(){try{
TownValueDiagnostic();C(!queries&&lines.empty(),"OFF has no source work or log");
debug=true;TownValueDiagnostic();C(!queries&&lines.empty(),"unready has no source work or log");
ready=true;TownValueDiagnostic();C(queries==1&&lines.size()==4,"one source read and one copied record per row");
C(lines[1].find("id=100")!=std::string::npos&&lines[3].find("id=102")!=std::string::npos,"actual IDs reach catalog log");
TownValueDiagnostic();C(queries==1&&lines.size()==4,"unchanged generation no repeated census");
townValuesLogged=false;lines.clear();result=E_FAIL;TownValueDiagnostic();C(queries==2&&lines.size()==1,"failure has only status no partial rows");
TownValueDiagnostic();C(queries==2,"failed diagnostic does not retry every frame");
townValuesLogged=false;lines.clear();result=S_OK;changeWorld=true;TownValueDiagnostic();C(queries==3&&lines.empty(),"world change discards all diagnostic publication");
std::printf("town value actual caller: %u checks PASS\n",checks);return 0;
}catch(...){return 1;}}
'''.replace('BODY', body)
vc = Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
sdk = Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env = dict(os.environ, INCLUDE=f'{vc}/include;{sdk}/Include', LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
cpp = out / 'caller.cpp'
cpp.write_text(fixture, encoding='utf-8')
exe = out / 'caller.exe'
with (out / 'compile.log').open('w') as log:
    subprocess.run([str(vc / 'bin/amd64/cl.exe'), '/nologo', '/EHsc', '/MD', '/O2', '/GL', '/Fe' + str(exe), '/Fo' + str(out / 'caller.obj'), str(cpp), '/link', '/LTCG'], env=env, check=True, stdout=log, stderr=subprocess.STDOUT)
subprocess.run([str(exe)], check=True)
