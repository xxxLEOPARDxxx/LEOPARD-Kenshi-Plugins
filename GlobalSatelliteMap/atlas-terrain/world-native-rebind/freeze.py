from pathlib import Path
import datetime,hashlib,json,re,sys
import run_tests as runner
HERE=runner.HERE;OUT=runner.OUT;MANIFEST=HERE/'verification.json'
def require(ok,why):
 if not ok:raise RuntimeError(why)
def sha(path):
 h=hashlib.sha256()
 with Path(path).open('rb')as stream:
  for block in iter(lambda:stream.read(1048576),b''):h.update(block)
 return h.hexdigest()
def own():return{p.name:sha(p)for p in sorted(HERE.iterdir())if p.is_file()and p.name!='verification.json'}
def check(values,root=None):
 for name,pin in values.items():
  path=root/name if root else Path(name);require(path.is_file()and sha(path)==pin,'Changed or missing '+str(path))
def dependencies():
 paths=set()
 for log in OUT.glob('*-build.log'):
  for name in re.findall(r'Note: including file:\s*(.+)',log.read_text()):paths.add(Path(name).resolve())
 for component in ['world-cohort','world-stream','world-native','source-receipts','world-bank','world-source','world-raster','world-pixels','png-source']:paths.add(HERE.parent/component/'verification.json')
 paths.add(HERE.parent.parent/'atlas-cache/accepted-window/verification.json')
 for name in ['cl.exe','link.exe','c1xx.dll','c2.dll','dumpbin.exe']:paths.add(runner.VC/'bin/amd64'/name)
 for name in ['msvcrt.lib','msvcprt.lib','oldnames.lib']:paths.add(runner.VC/'lib/amd64'/name)
 for name in ['d3d11','dxgi','advapi32','ole32','windowscodecs','kernel32','uuid']:paths.add(runner.SDK/'Lib/x64'/(name+'.lib'))
 paths.add(Path('D:/DEV/Kenshi/deps/KenshiLib/Libraries/OgreMain_x64.lib'))
 for name in ['thread','system','chrono','date_time','atomic']:paths.add(Path('D:/DEV/Kenshi/deps/boost_1_60_0/stage/lib')/('libboost_'+name+'-vc100-mt-1_60.lib'))
 paths.add(Path(sys.executable));paths.add(Path('D:/SteamLibrary/steamapps/common/Kenshi/OgreMain_x64.dll'));paths.add(Path('D:/SteamLibrary/steamapps/common/Kenshi/data/newland/land/fullmap.tif'))
 for n in range(2):
  for p in range(3):paths.add(HERE.parent/'world-cohort/_generated'/('sequential-%u-accepted-%u.bin'%(n,p)))
 for line in(HERE.parent/'world-cohort/_generated/source-inputs.txt').read_text().splitlines():
  name,size,pin=line.split('\t');path=Path(name);require(path.stat().st_size==int(size)and sha(path)==pin,'Changed selected input '+name);paths.add(path)
 return{str(p.resolve()):sha(p)for p in sorted(paths)if not p.resolve().is_relative_to(HERE)}
def record():
 for opt in ['Od','O2']:require('PASS 310 native rebind controls;64regions, same-scope stale token/request/bank refusal; CPU45071954B'in(OUT/('tests'+opt+'.log')).read_text(),'Missing current host evidence '+opt)
 require(re.search(r'PASS \d+ actual rebind WIC/WARP checks; two cohorts/four views, TIFF authenticated once; CPU45071954B',(OUT/'flowO2.log').read_text()),'Missing actual repeated cohort flow')
 for name,why in [('binding-high64','old same-scope MainToken never reaches freshly bound provider'),('capture-open','old same-scope source request cannot recapture outside owned transaction'),('idle-admission','unreleased CPU/plane view forbids provider rebinding without Pump'),('failed-provider-ready','missing native source invalidates/deletes old and failed candidate'),('provider-peak','old/new provider peak over64MiB refuses before worker allocation')]:require('FAIL '+why in(OUT/'mutants'/(name+'-run.log')).read_text(),'Missing semantic mutant '+name)
 exes=sorted(OUT.rglob('*.exe'));require(len(exes)==8,'Unexpected host executable set')
 for exe in exes:
  log=exe.with_name(exe.stem+'-imports.log');require(log.is_file()and not re.search(r'(?im)^\s*\S*(?:ogre|kenshi)\S*\.dll\s*$',log.read_text()),'Missing/native host imports '+str(exe))
 require('OgreMain_x64.dll'in(OUT/'native-imports.log').read_text(),'Actual native wrapper was not linked to expected imports')
 data=dict(scope='New Main-only owning persistent cohort session with checked replacement of real frozen terminal StageAdapter; host fake stage plus native compile-only, no integration/install',status='author checks PASS; independent review pending',recordedUtc=datetime.datetime.now(datetime.timezone.utc).isoformat(),sourceFiles=own(),dependencies=dependencies(),logs={str(p.relative_to(HERE)):sha(p)for p in sorted(OUT.rglob('*.log'))},artifacts={str(p.relative_to(HERE)):sha(p)for p in sorted(OUT.rglob('*'))if p.is_file()and p.suffix in ['.exe','.dll']},commands=['python run_tests.py --all','python freeze.py','python freeze.py --verify'],results=dict(hostEach=310,regions=64,sameScopeTokenRequestBankReplayRefused=True,bindingHigh64WithEqualLow32=True,actualControllerBoundIdle=True,partialConstructorAndCaptureFailureCleaned=True,foreignBankUnlockRefused=True,copiedCpuWithFakeTerminal=45071954,oldNewProviderPeakIncluded=True,repeatedWarpCohorts=2,repeatedWarpViews=4,heightAuthentications=1,nativeProviderDefaultCompileLinkOnly=True,nativeProviderHostExecuted=False,hostPeImportsCheckedBeforeExecution=True,killedMutants=5),limits=['Only pinned base profile; active native winner/world/material authority remains external.','Session tokens stamp profile as proxy binding ID; original raw snapshots are preserved internally and passed to real provider.','Only known own Idle plus legal caller consumer joins permits rebind; no arbitrary bool or foreign controller admission.','CPU sizeof calculation includes fixed storage plus old/new provider objects; external writer/caller allocations must be added separately.','No native repeated-session execution, FPS, full-world throughput, total RSS, all OS faults, long-run leak or publication claim.'])
 MANIFEST.write_text(json.dumps(data,indent=2)+'\n');print('PASS frozen world-native-rebind',sha(MANIFEST),'own',len(data['sourceFiles']),'deps',len(data['dependencies']),'logs',len(data['logs']))
def verify():
 data=json.loads(MANIFEST.read_text());require(own()==data['sourceFiles'],'Owned source set/hash changed');check(data['dependencies']);check(data['logs'],HERE);check(data['artifacts'],HERE);print('PASS exact world-native-rebind sources/dependencies/logs/artifacts; native compile-only')
if __name__=='__main__':verify()if'--verify'in sys.argv else record()
