"""Pin this isolated cohort's current evidence; no game/native calls."""
from pathlib import Path
import datetime,hashlib,json,re,sys
import run_tests as runner
HERE=runner.HERE;OUT=runner.OUT;MANIFEST=HERE/'verification.json'
def require(ok,reason):
 if not ok:raise RuntimeError(reason)
def sha(path):
 h=hashlib.sha256()
 with Path(path).open('rb')as stream:
  for block in iter(lambda:stream.read(1024*1024),b''):h.update(block)
 return h.hexdigest()
def own():return{p.name:sha(p)for p in sorted(HERE.iterdir())if p.is_file()and p.name!='verification.json'}
def check(values,root=None):
 for name,pin in values.items():
  path=root/name if root else Path(name);require(path.is_file()and sha(path)==pin,'Changed or missing '+str(path))
def dependencies():
 paths=set()
 for log in OUT.glob('*-build.log'):
  for name in re.findall(r'Note: including file:\s*(.+)',log.read_text()):paths.add(Path(name).resolve())
 for component in ['world-stream','world-raster','world-bank','world-source','source-receipts','png-source','world-native','world-pixels']:
  paths.add(HERE.parent/component/'verification.json')
 paths.add(HERE.parent.parent/'atlas-cache/accepted-window/verification.json')
 for name in ['cl.exe','link.exe','c1xx.dll','c2.dll']:paths.add(runner.VC/'bin/amd64'/name)
 for name in ['libcmt.lib','libcpmt.lib','oldnames.lib']:paths.add(runner.VC/'lib/amd64'/name)
 for name in ['d3d11','dxgi','advapi32','ole32','windowscodecs','kernel32','uuid']:paths.add(runner.SDK/'Lib/x64'/(name+'.lib'))
 paths.add(runner.VC/'bin/amd64/dumpbin.exe');paths.add(Path(sys.executable));paths.add(Path('D:/SteamLibrary/steamapps/common/Kenshi/data/newland/land/fullmap.tif'))
 for line in (OUT/'source-inputs.txt').read_text().splitlines():
  name,size,pin=line.split('\t');path=Path(name);require(path.stat().st_size==int(size)and sha(path)==pin,'Selected encoded source pin changed '+name);paths.add(path)
 return{str(p.resolve()):sha(p)for p in sorted(paths)if not p.resolve().is_relative_to(HERE)}
def record():
 contracts={}
 for opt in ['Od','O2']:
  text=(OUT/('contract'+opt+'-run.log')).read_text();match=re.search(r'PASS (\d+) bounded contract checks; CPU44514346B',text);require(match is not None and int(match[1])>=448,'Missing current contract '+opt);contracts[opt]=int(match[1])
 require(re.search(r'PASS \d+ actual cohort checks; four sequential baselines, K2/K4 six accepted views,1572864 independent heights; CPU44514346B',(OUT/'flowO2-run.log').read_text()),'Missing completed WIC/WARP flow')
 for scenario in ['cancel','lease','opened','decoded','complete','released','send','borrow-budget','quarantine']:require(re.search(r'PASS \d+ actual cohort fault checks: '+scenario,(OUT/'faults'/ (scenario+'.log')).read_text()),'Missing fault '+scenario)
 for name,reason in [('off-result','OFF refuses complete accepted result'),('exit-error-latch','explicit Cancel cannot hide abnormal worker failure'),('quarantine-debt','quarantine debt includes all four rasters'),('quarantine-destructor','quarantined destructor releases no GPU owner'),('raw-borrow-budget','inconsistent raw owner capacity refuses CPU borrow')]:require('FAIL '+reason in(OUT/'faults'/(name+'-run.log')).read_text(),'Missing mutant '+name)
 exes=sorted(OUT.rglob('*.exe'));require(len(exes)==9,'Unexpected host artifact set')
 for exe in exes:
  log=exe.with_name(exe.stem+'-imports.log')if exe.parent==OUT else exe.with_name('imports.log'if exe.stem=='fault'else exe.stem+'-imports.log')
  require(log.is_file()and not re.search(r'(?im)^\s*\S*(?:ogre|kenshi)\S*\.dll\s*$',log.read_text()),'Missing or native host imports '+str(exe))
 data=dict(scope='Isolated two/four-view same-cell terrain cohort; shared frozen plain worker/sourcebank, serial accepted output and CPU borrow; no Main/native integration or writer',status='author controls PASS; independent review pending',recordedUtc=datetime.datetime.now(datetime.timezone.utc).isoformat(),sourceFiles=own(),dependencies=dependencies(),logs={str(p.relative_to(HERE)):sha(p)for p in sorted(OUT.rglob('*.log'))},artifacts={str(p.relative_to(HERE)):sha(p)for p in sorted(OUT.rglob('*'))if p.is_file()and p.suffix in ['.exe','.bin','.u16','.txt']},commands=['python run_tests.py','python fault_tests.py --only borrow-budget','python fault_tests.py --mutant raw-borrow-budget','python freeze.py','python freeze.py --verify'],results=dict(contractChecks=contracts,copiedCpuBytes=44514346,gpuK2=137571380,gpuK4=187062500,sequentialBaselines=4,cohortViews=6,independentHeightPixels=1572864,sourcePreparesEachCohortSid=1,cancellationBoundaries=7,abnormalWorkerBoundaries=['opened','decoded','complete','released'],allFourOwnersQuarantined=True,cpuBorrow=True,borrowedAdapterWorker=True,borrowBudgetRefusal=True,killedMutants=5,hostPeImportsInspectedBeforeExecution=True),limitations=['Native source/world/material authority remains caller supplied; only pinned base profile.','Normal alpha0 remains native; adapter integration is exercised in a host consumer only.','Caller must join every borrowing consumer before Pump/ReleaseView/Cancel/Shutdown/destruction.','Explicit payload caps do not bound opaque WIC/driver/RSS or per-operation latency.','No native API execution, full-world throughput, FPS, long-run leak or publication claim.'])
 MANIFEST.write_text(json.dumps(data,indent=2)+'\n');print('PASS frozen world-cohort',sha(MANIFEST),'sources',len(data['sourceFiles']),'dependencies',len(data['dependencies']),'logs',len(data['logs']))
def verify():
 data=json.loads(MANIFEST.read_text());require(own()==data['sourceFiles'],'Owned source set/hash changed');check(data['dependencies']);check(data['logs'],HERE);check(data['artifacts'],HERE);print('PASS exact frozen world-cohort sources/dependencies/evidence/artifacts; host-only evidence')
if __name__=='__main__':verify()if'--verify'in sys.argv else record()
