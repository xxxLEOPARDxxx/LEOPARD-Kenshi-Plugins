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
 for component in ['world-window-session','world-write-worker','window-writer512','accepted-window']:paths.add(HERE.parent/component/'verification.json')
 for name in ['cl.exe','link.exe','c1xx.dll','c2.dll','dumpbin.exe']:paths.add(runner.VC/'bin/amd64'/name)
 for name in ['libcmt.lib','libcpmt.lib','oldnames.lib']:paths.add(runner.VC/'lib/amd64'/name)
 paths.add(runner.SDK/'Lib/x64/advapi32.lib');paths.add(runner.SDK/'Lib/x64/kernel32.lib');paths.add(Path(sys.executable))
 return{str(p.resolve()):sha(p)for p in sorted(paths)if not p.resolve().is_relative_to(HERE)}
def record():
 for opt in ['Od','O2']:require('PASS 169 worker-only disk controls; FileCpuBound24B; tiny tile bytes65536'in(OUT/('tests'+opt+'-run.log')).read_text(),'Missing current tiny IO '+opt)
 require('PASS 16 actual namespace retarget race controls'in(OUT/'retarget-race-run.log').read_text(),'Missing actual metadata retarget race')
 require('PASS 169 worker-only disk controls'in(OUT/'short-write-run.log').read_text(),'Missing forced short-write loop control')
 for name,reason in [('path-open-loss','file exists in original source directory after restoring namespace'),('short-loop-loss','positioned write persisted exact bytes'),('tile-bounds-loss','all out-of-range tile indices refuse')]:require('FAIL '+reason in(OUT/(name+'-run.log')).read_text(),'Missing semantic mutant '+name)
 exes=sorted(OUT.glob('*.exe'));require(len(exes)==7,'Unexpected host binary set')
 for exe in exes:
  log=OUT/(exe.stem+'-imports.log');require(log.is_file()and not re.search(r'(?im)^\s*\S*(?:ogre|kenshi|d3d|dxgi|user32|gdi32)\S*\.dll\s*$',log.read_text()),'Missing/unsafe host PE imports')
 require(not list(OUT.glob('fixtures-*')),'Uncleaned fixture directory remains')
 data=dict(scope='Concrete CPU-only writer-worker Factory/Config; exclusive handle-relative NEW directory and16tile files; no Main/GPU/native game APIs or integration',status='author controls PASS; independent review pending',recordedUtc=datetime.datetime.now(datetime.timezone.utc).isoformat(),sourceFiles=own(),dependencies=dependencies(),logs={str(p.relative_to(HERE)):sha(p)for p in sorted(OUT.glob('*.log'))},artifacts={str(p.relative_to(HERE)):sha(p)for p in sorted(OUT.glob('*.exe'))},cleanupManifests={str(p.relative_to(HERE)):sha(p)for p in sorted(OUT.glob('*-cleanup-*.json'))},commands=['python run_tests.py','python freeze.py','python freeze.py --verify'],results=dict(controlsEach=169,unicodeBuilds=['VC10 /Od /MT /DUNICODE /D_UNICODE','VC10 /O2 /MT /DUNICODE /D_UNICODE'],fileCpuBound=24,maximumTiles=16,configMaxCharacters=220,actualRetargetRaceControls=16,metadataOnlyReparseCanMutate=True,handleRelativeFileCreateRefusesRetargetWithoutCreatingTarget=True,forcedShortWrites=True,killedMutants=3,maximumRegularFixtureBytes=4096,noFullGstResize=True,hostImportsInspectedBeforeExecution=True,allFixturesManifestCleaned=True),limits=['Caller chooses fixed intended existing allowed parent; no game path is inferred.','Metadata-only reparse mutation is not prevented by share flags; handle-relative exclusive creation contains namespace redirection.','Own HANDLE adapter explicitly authorized because frozen Win32File has no handle adoption API.','Factory/files created and serialized on existing writer worker; consumers/outer owner must be joined before teardown.','No atomic atlas publication, native game execution, Main integration, install, full-file throughput, total RSS, all OS faults or long-run leak claim.'])
 MANIFEST.write_text(json.dumps(data,indent=2)+'\n');print('PASS frozen world-build-disk',sha(MANIFEST),'own',len(data['sourceFiles']),'deps',len(data['dependencies']),'logs',len(data['logs']),'cleanup',len(data['cleanupManifests']))
def verify():
 data=json.loads(MANIFEST.read_text());require(own()==data['sourceFiles'],'Owned source set/hash changed');check(data['dependencies']);check(data['logs'],HERE);check(data['artifacts'],HERE);check(data['cleanupManifests'],HERE);require(not list(OUT.glob('fixtures-*')),'Fixture directory remains');print('PASS exact world-build-disk sources/dependencies/imports/test/cleanup evidence')
if __name__=='__main__':verify()if'--verify'in sys.argv else record()
