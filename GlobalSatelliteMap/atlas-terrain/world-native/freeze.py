"""Freeze only observed adapter sources, compiler dependency closure and evidence.

run_tests.py records a full successful session; then --record writes the manifest.
--verify never rebuilds or silently refreshes pins. No game DLL is loaded here.
"""
from pathlib import Path
import datetime,hashlib,json,os,re,subprocess,sys
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';MANIFEST=HERE/'verification.json'
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
ENV=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include;D:/DEV/Kenshi/KenshiLib-src/Include;D:/DEV/Kenshi/deps/boost_1_60_0',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64;D:/DEV/Kenshi/deps/KenshiLib/Libraries;D:/DEV/Kenshi/deps/boost_1_60_0/stage/lib')
def require(ok,message):
 if not ok:raise RuntimeError(message)
def sha(path):
 h=hashlib.sha256()
 with Path(path).open('rb')as stream:
  for block in iter(lambda:stream.read(1024*1024),b''):h.update(block)
 return h.hexdigest()
def pins(paths):return{str(p):sha(p)for p in sorted(set(Path(p).resolve()for p in paths))}
def check(values):
 for name,digest in values.items():require(Path(name).is_file()and sha(name)==digest,'Changed or missing: '+name)
def own():return{p.name:sha(p)for p in sorted(HERE.iterdir())if p.is_file()and p.name!='verification.json'}
def quoted():
 seen=set()
 def visit(path):
  path=path.resolve()
  if path in seen:return
  require(path.is_file(),'Missing quoted include: '+str(path));seen.add(path)
  for relative in re.findall(r'^\s*#include\s+"([^\"]+)"',path.read_text(),re.M):visit(path.parent/relative)
 for name in ['SourceFactory.h','StageAdapter.h','NativeProvider.h','tests.cpp','native_compile.cpp']:visit(HERE/name)
 return{p for p in seen if not p.is_relative_to(HERE)}
def includes(text):
 files=set()
 for line in text.splitlines():
  match=re.search(r'Note: including file:\s*(.+?)\s*$',line)
  if match:files.add(Path(match[1]).resolve())
 require(files,'Compiler /showIncludes produced no dependency paths')
 return files
def snapshot():
 OUT.mkdir(exist_ok=True)
 args=[str(VC/'bin/amd64/cl.exe'),'/nologo','/Zs','/EHsc','/MD','/O2','/DNDEBUG','/DWIN32','/showIncludes',str(HERE/'native_compile.cpp')]
 p=subprocess.run(args,cwd=HERE,env=ENV,capture_output=True,text=True);(OUT/'dependencies-scan.log').write_text(p.stdout+p.stderr)
 require(p.returncode==0,'Dependency syntax scan failed; see dependencies-scan.log')
 paths=includes(p.stdout+p.stderr)|quoted()
 anchor=HERE.parent/'source-receipts/sdk-pins.json';anchorPins=json.loads(anchor.read_text());check(anchorPins);paths.add(anchor)
 paths.update(Path(name)for name in anchorPins)
 paths.update([VC/'bin/amd64/cl.exe',VC/'bin/amd64/link.exe',VC/'lib/amd64/msvcrt.lib',VC/'lib/amd64/msvcprt.lib',SDK/'Lib/x64/AdvAPI32.Lib',Path('D:/DEV/Kenshi/deps/KenshiLib/Libraries/OgreMain_x64.lib')])
 deps=pins(p for p in paths if not p.resolve().is_relative_to(HERE))
 (OUT/'dependencies-before.json').write_text(json.dumps(dict(sourceFiles=own(),dependencies=deps,compileIncludes=sorted(str(p)for p in includes(p.stdout+p.stderr)),syntaxScanCommand=args),indent=2)+'\n')
 print('PASS snapshot',len(deps),'exact dependency files,',len(own()),'owned source files')
def checked_snapshot():
 before=json.loads((OUT/'dependencies-before.json').read_text());check(before['dependencies']);require(own()==before['sourceFiles'],'Owned sources changed during verification');return before
def imports(path):
 sys.path.insert(0,'D:/DEV/Kenshi/ours/SkillLevelUpInfo/tests/_tools');import pefile
 pe=pefile.PE(str(path));return{d.dll.decode():sorted(i.name.decode()if i.name else'#'+str(i.ordinal)for i in d.imports)for d in pe.DIRECTORY_ENTRY_IMPORT}
def record():
 before=checked_snapshot();session=json.loads((OUT/'run-session.json').read_text());require(session['fullRun'],'Manifest requires full host/native/mutant run')
 require(all(c['exitCode']==0 for c in session['commands']),'Run contains failed command')
 for name in ['hostOd','hostO2']:
  text=(OUT/(name+'.log')).read_text();require('PASS 122105 source factory / production adapter checks;64x16 cells,1999 SIDs,29985 source requests'in text,'Unexpected host evidence: '+name)
  require(not any('ogre'in n.lower()or'kenshi'in n.lower()for n in imports(OUT/(name+'.exe'))),'Host imports game DLL')
 required=['wrong-group','canonical-alias','slot-zero-routing','truncate-full64-bank'];mutants=(OUT/'mutants.log').read_text()
 for name in required:require('PASS killed '+name+' :'in mutants,'Missing killed mutant: '+name)
 afterIncludes=includes((OUT/'native-build.log').read_text());require(afterIncludes==set(Path(n)for n in before['compileIncludes']),'Native include closure changed between syntax scan and build')
 artifacts={}
 for file in ['hostOd.exe','hostO2.exe','native-contract.dll']:
  path=OUT/file;artifacts[file]=dict(sha256=sha(path),bytes=path.stat().st_size,imports=imports(path),executedByHost=file!='native-contract.dll')
 require(any(n.lower()=='ogremain_x64.dll'for n in artifacts['native-contract.dll']['imports']),'Native contract did not link real Ogre imports')
 evidence={str(p.relative_to(HERE)):sha(p)for p in sorted(OUT.rglob('*'))if p.is_file()and(p.suffix=='.log'or p.name in ['dependencies-before.json','run-session.json']or(p.parent.parent==OUT/'mutants'and p.suffix in ['.h','.cpp']))}
 result=dict(scope='Isolated Main-only raw indexed NativeMetadata stage adapter and plain source factory; no world-stream/Main edits',status='implementer checks PASS; independent review in progress',recordedUtc=datetime.datetime.now(datetime.timezone.utc).isoformat(),sourceFiles=own(),dependencies=before['dependencies'],evidence=evidence,artifacts=artifacts,commands=session['commands'],results=dict(hostChecksEach=122105,hostBuilds=['VC10 /Od','VC10 /O2'],regions=64,cellsPerRegion=16,selectedSidStages=1999,sourceRequests=29985,killedMutants=required,nativeCompileAndLink=True,nativeContractHostExecuted=False),limitations=['Fake-stage host tests prove source/request/routing contract, not a real native file receipt or native GPU equivalence.','NativeProvider contract DLL was compiled and linked only; no native callback was executed by these tests.','No GPU rendering, Main integration, FPS, long-run memory or final feature completion claim.'])
 MANIFEST.write_text(json.dumps(result,indent=2)+'\n');print('PASS frozen manifest',sha(MANIFEST),'dependencies',len(before['dependencies']),'evidence',len(evidence))
def verify():
 data=json.loads(MANIFEST.read_text());require(own()==data['sourceFiles'],'Owned source set/hash changed');check(data['dependencies']);check({str(HERE/name):digest for name,digest in data['evidence'].items()})
 for name,artifact in data['artifacts'].items():
  path=OUT/name;require(path.is_file()and path.stat().st_size==artifact['bytes']and sha(path)==artifact['sha256'],'Artifact changed: '+name);require(imports(path)==artifact['imports'],'PE imports changed: '+name)
 print('PASS frozen world-native exact sources/dependencies/evidence/native imports; no native execution')
if __name__=='__main__':
 if '--snapshot'in sys.argv:snapshot()
 elif '--check-snapshot'in sys.argv:checked_snapshot();print('PASS unchanged source and dependency snapshot')
 elif '--record'in sys.argv:record()
 elif '--verify'in sys.argv:verify()
 else:sys.exit('Use --snapshot, --check-snapshot, --record or --verify')
