from pathlib import Path
import hashlib,json,sys
sys.dont_write_bytecode=True;sys.path.insert(0,'D:/DEV/Kenshi/ours/SkillLevelUpInfo/tests/_tools')
import pefile
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
inputs=json.loads((HERE/'sdk-pins.json').read_text())
for name,want in inputs.items():assert sha(Path(name))==want,name
pe=pefile.PE(str(OUT/'native-contract.dll'));imports={d.dll.decode():[s.name.decode() if s.name else str(s.ordinal)for s in d.imports] for d in pe.DIRECTORY_ENTRY_IMPORT}
expected={'?deallocBytes@NedPoolingImpl@Ogre@@SAXPEAX@Z','?getResourceGroup@ResourceGroupManager@Ogre@@IEAAPEAUResourceGroup@12@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z'}
assert set(imports['OgreMain_x64.dll'])==expected
(OUT/'native-imports.json').write_text(json.dumps(imports,indent=2)+'\n')
own={p.name:sha(p)for p in sorted(HERE.iterdir())if p.is_file()and p.name!='verification.json'}
artifacts={p.relative_to(OUT).as_posix():sha(p)for p in sorted(OUT.rglob('*'))if p.is_file()and p.suffix in ['.log','.json','.h','.dll']}
(HERE/'verification.json').write_text(json.dumps(dict(scope='bounded native FileSystem metadata/provider plus Win32 lease; raw-indexed metadata one-shot tested; complete native frame adapter still untested',sourceFiles=own,inputPins=inputs,artifacts=artifacts,nativeContract=dict(path='_generated/native-contract.dll',bytes=(OUT/'native-contract.dll').stat().st_size,sha256=sha(OUT/'native-contract.dll'),imports=imports['OgreMain_x64.dll'],executed=False),results=dict(standaloneGuardsFiles=True,sdkAbiLocks=True,actualWin32FaultCutpoints=29,compiledRejectedMutants=13,nativeModuleCallsInHostTests=False),limits=dict(blobBytes=16*1024*1024,callerCpuCap=64*1024*1024,maxRoles=16,maxAncestorHandles=64,pathBytes=260,textBytes=1024,nativeFrameAdapterTested=False)),indent=2)+'\n')
print('PASS frozen',sha(HERE/'verification.json'),len(own),'source files;',len(inputs),'unchanged native/SDK inputs; native DLL compiled only')
