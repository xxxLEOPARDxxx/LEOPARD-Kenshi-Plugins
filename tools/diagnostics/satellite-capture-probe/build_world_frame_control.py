"""Build/run the FakeProvider host only; reject game imports before execution."""
from pathlib import Path
import datetime,hashlib,json,os,re,subprocess,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'build/world-frame-control';OUT.mkdir(parents=True,exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
ENV=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include;D:/DEV/Kenshi/KenshiLib-src/Include;D:/DEV/Kenshi/deps/boost_1_60_0',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
sys.path.insert(0,'D:/DEV/Kenshi/ours/SkillLevelUpInfo/tests/_tools');import pefile
MANIFEST=HERE/'world-frame-verification.json'
OWN=['WorldFrameControl.h','world_frame_tests.cpp','build_world_frame_control.py','world_frame_faults.py']
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def sourcePins():return{name:sha(HERE/name)for name in OWN}
def require(ok,message):
 if not ok:raise RuntimeError(message)
def importsOf(path):return{d.dll.decode():sorted(i.name.decode()if i.name else'#'+str(i.ordinal)for i in d.imports)for d in pefile.PE(str(path)).DIRECTORY_ENTRY_IMPORT}
def verify():
 data=json.loads(MANIFEST.read_text());require(sourcePins()==data['sourceFiles'],'Private source changed')
 for section,base in [('dependencies',None),('evidence',HERE)]:
  for name,digest in data[section].items():require(sha(base/name if base else name)==digest,'Changed '+name)
 for name,value in data['artifacts'].items():
  path=HERE/name;require(sha(path)==value['sha256']and importsOf(path)==value['imports'],'Changed binary/imports '+name)
 print('PASS frozen private frame sources/dependencies/evidence/imports; host only')
if '--verify'in sys.argv:verify();sys.exit(0)
beforeSources=sourcePins();commands=[]
def run(name,args):
 p=subprocess.run(args,cwd=HERE,env=ENV,capture_output=True,text=True);(OUT/(name+'.log')).write_text(p.stdout+p.stderr);commands.append(dict(name=name,args=args,exitCode=p.returncode));print('\n'.join(line for line in(p.stdout+p.stderr).splitlines()if'Note: including file:'not in line),flush=True)
 if p.returncode:sys.exit(p.returncode)
for opt in ['/Od','/O2']:
 name='host'+opt[1:];exe=OUT/(name+'.exe')
 run(name+'-build',[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD',opt,'/W3','/DNDEBUG','/DWIN32','/DBOOST_ALL_NO_LIB','/showIncludes',str(HERE/'world_frame_tests.cpp'),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib','advapi32.lib','ole32.lib','windowscodecs.lib'])
 imports=importsOf(exe)
 if any('ogre'in name.lower()or'kenshi'in name.lower()for name in imports):raise RuntimeError('Host imports a game/native module; will not execute')
 (OUT/(name+'-imports.json')).write_text(json.dumps(imports,indent=2)+'\n');print('PASS host imports contain no Ogre/Kenshi before execution',flush=True)
 run(name,[str(exe)])
if '--skip-faults'not in sys.argv:run('faults',[sys.executable,str(HERE/'world_frame_faults.py')])
if '--skip-faults'not in sys.argv:
 require(sourcePins()==beforeSources,'Private sources changed during tests')
 dependencies=set()
 for opt in ['Od','O2']:
  for line in(OUT/('host'+opt+'-build.log')).read_text().splitlines():
   match=re.search(r'Note: including file:\s*(.+?)\s*$',line)
   if match:dependencies.add(Path(match[1]).resolve())
 require(dependencies,'Missing compiler include closure')
 for suffix in ['-material.rgba16f','-normal.rgba8']:
  dependencies.add((HERE/('../../../GlobalSatelliteMap/atlas-terrain/raw-rgb/_generated/control3'+suffix)).resolve())
 for name in ['world-stream','world-pixels','world-native']:
  dependencies.add((HERE/f'../../../GlobalSatelliteMap/atlas-terrain/{name}/verification.json').resolve())
 dependencies.add((HERE/'../../../GlobalSatelliteMap/atlas-terrain/world-raster/_generated/height-two.u16').resolve())
 dependencies.add(Path('D:/SteamLibrary/steamapps/common/Kenshi/data/newland/land/fullmap.tif'))
 dependencies.update([VC/'bin/amd64/cl.exe',VC/'bin/amd64/link.exe',VC/'lib/amd64/msvcrt.lib',VC/'lib/amd64/msvcprt.lib'])
 for name in ['d3d11','dxgi','advapi32','ole32','windowscodecs']:dependencies.add(SDK/('Lib/x64/'+name+'.lib'))
 ownPaths={(HERE/name).resolve()for name in OWN};deps={str(path):sha(path)for path in sorted(dependencies)if path.resolve()not in ownPaths}
 evidence={str(path.relative_to(HERE)):sha(path)for path in sorted(OUT.rglob('*'))if path.is_file()and(path.suffix=='.log'or path.name.endswith('-imports.json')or path.name=='imports.json')}
 artifacts={str(path.relative_to(HERE)):dict(sha256=sha(path),imports=importsOf(path))for path in sorted(OUT.rglob('*.exe'))}
 for artifact in artifacts.values():require(not any('ogre'in name.lower()or'kenshi'in name.lower()for name in artifact['imports']),'Game-importing host binary in evidence')
 result=dict(scope='Private one-window FakeProvider/WARP accepted-frame export owner; no native provider execution or installation',status='implementer PASS; independent review pending',recordedUtc=datetime.datetime.now(datetime.timezone.utc).isoformat(),sourceFiles=sourcePins(),dependencies=deps,evidence=evidence,artifacts=artifacts,commands=commands,results=dict(hostChecksEach=65609,independentTriangleHeightPixels=65536,borrowedWriterCancelChecks=10,unknownWriterWaitChecks=10,abnormalEofChecks=5,writeFailureChecks=6,killedMutants=4,hostImportsCheckedBeforeEveryExecution=True),limits=['Tick requires current caller Scope and exact Frame plus admission; caller establishes native active-world/material/source authority.','GWSF0001 is a private diagnostic format, never GST/GSI or an atlas-completion record.','Worker may have completed a private forensic frame.gwf before a later world/frame/device/token change; such a file may remain, but Tick refuses success acknowledgment.','materialSha covers900 parameter floats only; distortion60 and complete native material semantics are not hashed by that field.','No Main file I/O; writer owns only copied header/path/scalars and borrows accepted CPU planes until known join.','Unknown writer waits retain whole provider/stream/export Owner and conservative CPU/GPU debt until restart.','No actual native provider execution, game install, FPS, full-world throughput or long-run leak-free claim.'])
 MANIFEST.write_text(json.dumps(result,indent=2)+'\n');print('PASS frozen private frame',sha(MANIFEST),'dependencies',len(deps),'artifacts',len(artifacts))
