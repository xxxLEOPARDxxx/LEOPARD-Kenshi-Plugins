from pathlib import Path
import os,subprocess,re,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True);SOURCE=HERE.parent/'WorldCohort.h'
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');ENV=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
if '--red' in sys.argv:
 p=subprocess.run(['git','show','HEAD:GlobalSatelliteMap/atlas-terrain/world-cohort/WorldCohort.h'],cwd=HERE,capture_output=True,text=True);assert not p.returncode
 original=p.stdout
else: original=SOURCE.read_text()
text=original.replace('#include "../world-stream/WorldStream.h"','#include "'+(HERE.parent.parent/'world-stream/WorldStream.h').as_posix()+'"').replace('template<class Provider>class Controller{','template<class Provider>class Controller{public: // Test visibility only; method behavior unchanged.')
(OUT/'PublicCohort.h').write_bytes(text.encode())
for mode in ['Od','O2']:
 exe=OUT/f'tests-{mode}.exe';args=[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT','/'+mode,'/W3','/showIncludes',str(HERE/'tests.cpp'),'/Fo'+str(exe.with_suffix('.obj')),'/Fe'+str(exe),'/link','advapi32.lib','ole32.lib','windowscodecs.lib']
 p=subprocess.run(args,cwd=HERE,env=ENV,capture_output=True,text=True);(OUT/('build-'+mode+'.log')).write_text(p.stdout+p.stderr);print('\n'.join(x for x in (p.stdout+p.stderr).splitlines() if 'Note: including file:' not in x),flush=True)
 if '--red' in sys.argv:
  log=p.stdout+p.stderr;assert p.returncode and "'RequestShutdown' : is not a member" in log and 'C2825' not in log and 'C2589' not in log
  (HERE/'red-missing-api.log').write_text('Pre-change HEAD source, corrected test harness; expected missing API only.\n'+log);print('RED confirmed: production async API absent');sys.exit(0)
 if p.returncode:sys.exit(p.returncode)
 p=subprocess.run([str(VC/'bin/amd64/dumpbin.exe'),'/imports',str(exe)],capture_output=True,text=True);(OUT/('imports-'+mode+'.log')).write_text(p.stdout+p.stderr);assert not p.returncode and not re.search(r'(?im)^\s*\S*(?:kenshi|ogre)\S*\.dll\s*$',p.stdout)
 for case in ['normal','wait-failed-request','wait-failed','timeout-wrap','missing-marker']:
  p=subprocess.run([str(exe)]+([] if case=='normal' else [case]),cwd=HERE,capture_output=True,text=True);(OUT/(case+'-'+mode+'.log')).write_text(p.stdout+p.stderr);print(case,p.stdout+p.stderr,flush=True)
  if p.returncode:sys.exit(p.returncode)
