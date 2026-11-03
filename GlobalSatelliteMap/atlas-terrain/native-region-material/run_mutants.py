from pathlib import Path
import os,subprocess,re,json
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated'/'mutants';OUT.mkdir(parents=True,exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');ENV=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
header=(HERE/'RegionMaterial.h').read_text()
for relative in ['../world-raster/WorldTerrainRaster.h','../source-receipts/ReceiptPolicy.h']:header=header.replace(relative,(HERE/relative).resolve().as_posix())
tests=(HERE/'tests.cpp').read_text().replace('"_generated/Fixture.h"','"'+(HERE/'_generated/Fixture.h').as_posix()+'"')
cases={'wrong-slope-scale':('((row>=3&&row<=5)?0.01f:1.0f)','((row>=3&&row<=5)?0.1f:1.0f)'), 'missing-tree-hash':('Helpers::Hash(mask)!=region.cells[c].treeSha256','false'),'stale-publication':('||!access.Current(scope))return false;\n  out=result;',' )return false;\n  out=result;')};killed={}
for name,(old,new) in cases.items():
 assert header.count(old)==1,(name,header.count(old));folder=OUT/name;folder.mkdir(exist_ok=True);(folder/'RegionMaterial.h').write_bytes(header.replace(old,new).encode());(folder/'tests.cpp').write_bytes(tests.encode());exe=folder/'test.exe'
 p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2',str(folder/'tests.cpp'),'/Fo'+str(folder/'test.obj'),'/Fe'+str(exe),'/link','advapi32.lib'],cwd=HERE/'_generated',env=ENV,capture_output=True,text=True);(folder/'build.log').write_text(p.stdout+p.stderr);assert p.returncode==0,(name,p.stdout,p.stderr)
 p=subprocess.run([str(VC/'bin/amd64/dumpbin.exe'),'/imports',str(exe)],capture_output=True,text=True);(folder/'imports.log').write_text(p.stdout+p.stderr);assert not p.returncode and not re.search(r'(?im)^\s*\S*(?:kenshi|ogre)\S*\.dll\s*$',p.stdout)
 p=subprocess.run([str(exe)],cwd=HERE/'_generated',capture_output=True,text=True);(folder/'run.log').write_text(p.stdout+p.stderr);assert p.returncode!=0 and 'FAIL ' in p.stderr,(name,p.returncode,p.stdout,p.stderr);killed[name]=p.stderr.strip()
(OUT/'result.json').write_text(json.dumps(killed,indent=2)+'\n');print(json.dumps(killed,indent=2))
