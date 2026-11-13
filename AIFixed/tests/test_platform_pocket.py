"""Compile the actual pocket query and lock wrapper against explicit boundaries.

No live game calls. Native result, lock contention and a real Windows SEH fault
are controlled here; this tests fail-closed admission and lock release only.
"""
from pathlib import Path
import os
import subprocess

here = Path(__file__).resolve().parent
runtime = (here.parent / 'UnstuckRuntime.h').read_text('utf-8')
start = runtime.index('    int QueryPlatformPocket(')
code = runtime[start:runtime.index('    bool RejectLanding(', start)]
fixture = r'''
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdio>
#include <cmath>
#include <float.h>
#include <limits>
#include <UnstuckPolicy.h>
bool available=true,held=false,answer=true,fault=false;
int calls=0, faults=0;
namespace boost {
struct shared_mutex {};
const int try_to_lock=0;
template<class M> struct shared_lock {
    bool owner;
    shared_lock(M&,int):owner(available){if(owner)held=true;}
    bool owns_lock(){return owner;}
    ~shared_lock(){if(owner)held=false;}
};
}
struct hkVector4f {struct {float m128_f32[4];}m_quad;};
struct HavokCharacter {unsigned currentFace;float radius;hkVector4f position;};
struct CharMovement {HavokCharacter* havokCharacter;};
struct NavMesh {
    boost::shared_mutex changeMutex;
    bool isCharacterStuck(const hkVector4f&,unsigned,float radius,float distance){
        ++calls;
        if(!held || radius!=.4f || distance!=3.0f) RaiseException(0xE0000001,0,0,NULL);
        if(fault) RaiseException(0xE0000002,0,0,NULL);
        return answer;
    }
};
void Fault(){++faults;}
// @PRODUCTION@
int checks=0,failures=0;
void Check(bool condition,const char* message){++checks;if(!condition){++failures;printf("FAIL %s\n",message);}}
int main(){
    NavMesh nav;HavokCharacter character={0x02000307,.4f,{{1878,26,4772,0}}};
    CharMovement movement={&character};int result=0;
    const AIFixedGate::Point cap(-.49f,.84638f,-.20f);
    Check(ReadPlatformPocket(&nav,&movement,cap,result)&&result==1&&!held,"native pocket accepted and lock released");
    answer=false;Check(!ReadPlatformPocket(&nav,&movement,cap,result)&&result==0&&!held,"connected face rejected");answer=true;
    const int before=calls;
    available=false;Check(!ReadPlatformPocket(&nav,&movement,cap,result)&&result==-1&&calls==before&&!held,"busy writer is not blocked");available=true;
    Check(!ReadPlatformPocket(&nav,&movement,AIFixedGate::Point(0,1,0),result)&&result==-2&&calls==before,"flat post never queries");
    Check(!ReadPlatformPocket(&nav,&movement,AIFixedGate::Point(0,.59f,.8f),result)&&result==-2&&calls==before,"steep footing uses existing branch");
    character.currentFace=~0u;Check(!ReadPlatformPocket(&nav,&movement,cap,result)&&calls==before,"missing face rejected");character.currentFace=0x02000307;
    character.radius=4;Check(!ReadPlatformPocket(&nav,&movement,cap,result)&&calls==before,"world-unit radius cannot be passed as HK radius");character.radius=.4f;
    character.position.m_quad.m128_f32[1]=std::numeric_limits<float>::quiet_NaN();
    Check(!ReadPlatformPocket(&nav,&movement,cap,result)&&calls==before,"invalid HK position rejected");character.position.m_quad.m128_f32[1]=26;
    fault=true;Check(!ReadPlatformPocket(&nav,&movement,cap,result)&&result==-1&&!held&&faults==1,"native SEH fault releases nav lock and suspends recovery");
    printf("Platform pocket production wrapper: %s (%d checks)\n",failures?"FAIL":"PASS",checks);
    return failures?1:0;
}
'''
out = here / '_generated'
out.mkdir(exist_ok=True)
source = out / 'platform_pocket.cpp'
source.write_text(fixture.replace('// @PRODUCTION@', code), 'utf-8')
vc = Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
sdk = Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env = os.environ.copy()
env['PATH'] = str(vc / 'bin/amd64') + ';' + env['PATH']
env['INCLUDE'] = str(vc / 'include') + ';' + str(sdk / 'Include')
env['LIB'] = str(vc / 'lib/amd64') + ';' + str(sdk / 'Lib/x64')
exe = out / 'platform_pocket.exe'
subprocess.run([str(vc / 'bin/amd64/cl.exe'), '/nologo', '/EHsc', '/MD', '/Od',
                '/I' + str(here.parent), str(source), '/Fe' + str(exe),
                '/Fo' + str(out / 'platform_pocket.obj')], cwd=out, env=env, check=True)
subprocess.run([str(exe)], check=True)
