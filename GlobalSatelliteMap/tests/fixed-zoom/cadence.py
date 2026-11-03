from pathlib import Path
import os,subprocess,sys,re,json,hashlib
here=Path(__file__).resolve().parent;root=here.parents[1];out=here/'_generated';out.mkdir(exist_ok=True);source=(root/'GlobalSatelliteMap.cpp').read_text(encoding='utf-8-sig');tick=source[source.index('void Tick(MapScreen*'):source.index('bool RelativeZoomEnabled')];gate=re.search(r'if\(!tickClock\.Due\(now,.*?\)\)return;',tick).group()
program=r'''
#include <cstdio>
namespace AtlasView{enum{DetailSlots=128};}bool ready=false;void*job=NULL,*completed=NULL;unsigned capacity=128;unsigned long long producer=0;unsigned BaseDetailCapacity(){return capacity;}unsigned long long ProducerGpuBytes(){return producer;}struct Clock{unsigned period;bool Due(unsigned,unsigned p){period=p;return false;}}tickClock;
void Tick(){unsigned now=1;__GATE__}
unsigned checks=0;void C(unsigned want,const char*why){Tick();++checks;if(tickClock.period!=want){printf("FAIL %s got%u want%u\n",why,tickClock.period,want);throw 1;}}
int main(){try{ready=true;job=reinterpret_cast<void*>(1);C(50,"ordinary disk read pending accelerates completion polling");job=NULL;completed=reinterpret_cast<void*>(1);C(50,"completed ordinary upload accelerates");completed=NULL;C(200,"loaded idle restores200ms");job=reinterpret_cast<void*>(1);ready=false;C(200,"unvalidated state stays200ms");ready=true;capacity=0;C(200,"fine production retains200ms budget");capacity=2;C(200,"fine startup retains200ms");capacity=128;producer=1;C(200,"GPU source draining retains200ms");producer=0;C(50,"ordinary LOD with idle fine owner may accelerate");printf("PASS %u actual Tick cadence controls\n",checks);return 0;}catch(...){return 1;}}
'''.replace('__GATE__',gate);(out/'cadence.cpp').write_text(program);vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');env=dict(os.environ,INCLUDE=f'{vc}/include',LIB=f'{vc}/lib/amd64;C:/Program Files/Microsoft SDKs/Windows/v7.1/Lib/x64');results={}
for mode in ('Od','O2'):
 exe=out/('cadence-'+mode+'.exe');p=subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT','/'+mode,str(out/'cadence.cpp'),'/Fo'+str(out/('cadence-'+mode+'.obj')),'/Fe'+str(exe)],env=env,capture_output=True,text=True);(out/('cadence-build-'+mode+'.log')).write_text(p.stdout+p.stderr)
 if p.returncode:print(p.stdout+p.stderr);sys.exit(p.returncode)
 p=subprocess.run([str(exe)],capture_output=True,text=True);(out/('cadence-run-'+mode+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr)
 if p.returncode:sys.exit(p.returncode)
 results[mode]=p.stdout.strip()
(here/'cadence-verification.json').write_text(json.dumps({'actualTickGate':gate,'gateSha256':hashlib.sha256(gate.encode()).hexdigest(),'results':results,'scope':'Actual extracted Tick interval choice; no game/FPS claim.'},indent=2)+'\n')
