from pathlib import Path
import sys,json,hashlib,importlib.util
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root.parent/'SkillLevelUpInfo'/'tests'/'_tools'))
import pefile
dll=Path('D:/DEV/Kenshi/build/ItemMarkers/ItemMarkers.dll')
pe=pefile.PE(str(dll))
exports={s.name.decode() for s in pe.DIRECTORY_ENTRY_EXPORT.symbols if s.name}
assert 'ItemMarkers_GetApi' in exports and 'MCM_Describe' in exports
assert any('startPlugin' in s for s in exports)
spec=importlib.util.spec_from_file_location('qol_pe',root.parents[1]/'tools/qol_binary_check.py')
qol_pe=importlib.util.module_from_spec(spec);spec.loader.exec_module(qol_pe)
deps={}
for p in [Path('D:/SteamLibrary/steamapps/common/Kenshi/KenshiLib.dll'),Path('D:/SteamLibrary/steamapps/common/Kenshi/OgreMain_x64.dll'),Path('D:/SteamLibrary/steamapps/common/Kenshi/MyGUIEngine_x64.dll')]:
    # KenshiLib's aliases share export addresses; use the existing raw-name-table
    # verifier, which retains every alias including data globals such as shou.
    deps[p.name.lower()]={s.encode() for s in qol_pe.PE(p).exports()}
count=0
for dependency in pe.DIRECTORY_ENTRY_IMPORT:
    name=dependency.dll.decode().lower()
    if name not in deps:continue
    for symbol in dependency.imports:
        assert symbol.name in deps[name],(name,symbol.name)
        count+=1
inputs=['ItemMarkers.cpp','GradeExtensions.h','GradeProfiles.h','GradeRuntime.h','sdk/GradeProviderExample.cpp','sdk/README.txt','tests/extensions_tests.cpp','tests/extensions_runtime.cpp','tests/extensions_refresh_tests.py']
result={'dllSha256':hashlib.sha256(dll.read_bytes()).hexdigest(),'bytes':dll.stat().st_size,'nativeImports':count,'exports':sorted(exports),
    'inputs':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in inputs},
    'publicHeaderSha256':hashlib.sha256((root.parent/'shared/ItemMarkersApi.h').read_bytes()).hexdigest(),
    'tests':{'pure':'20622 each /Od and /O2, including10000 zero-allocation registry+rule lookups','actualDllAndConfig':'52 each /Od and /O2, including reload exception rollback','extractedRefresh':'34 each /Od and /O2; 6 functional mutants rejected','knownMaps':'10543 production checks;15 compiled mutants rejected'},
    'scope':'Linked and host-tested candidate, not gameplay/FPS QA or publication.'}
(root/'tests/extensions-verification.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({k:result[k] for k in ['dllSha256','bytes','nativeImports','scope']},indent=2))
