from pathlib import Path
import os,re,subprocess,json
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated'/'mutants';OUT.mkdir(parents=True,exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');ENV=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
header=(HERE/'FineJob.h').read_text()
for relative in ['../world-layout/WorldLayout.h','../world-cohort/WorldCohort.h']:header=header.replace(relative,(HERE/relative).resolve().as_posix())
test=(HERE/'tests.cpp').read_text()
for relative in ['../compact-material/_generated/Shaders.h','../staged-material/_generated/StagedShaders.h']:test=test.replace(relative,(HERE/relative).resolve().as_posix())
cases={'live-bank-upload':('&&gate.producerReservedGpu==0',''),'missing-cpu-floor':('b.producerCpu>=ProducerCpuFloor()','b.producerCpu>0')};result={}
for name,(old,new) in cases.items():
 assert old in header
 folder=OUT/name;folder.mkdir(exist_ok=True);(folder/'FineJob.h').write_text(header.replace(old,new));(folder/'tests.cpp').write_text(test)
 exe=folder/'test.exe';p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT','/O2','/W3',str(folder/'tests.cpp'),'/Fo'+str(folder/'test.obj'),'/Fe'+str(exe),'/link','advapi32.lib'],cwd=folder,env=ENV,capture_output=True,text=True);(folder/'compile.log').write_text(p.stdout+p.stderr);assert p.returncode==0
 p=subprocess.run([str(VC/'bin/amd64/dumpbin.exe'),'/imports',str(exe)],capture_output=True,text=True);(folder/'imports.log').write_text(p.stdout+p.stderr);assert p.returncode==0 and not re.search(r'(?im)^\s*\S*(?:kenshi|ogre)\S*\.dll\s*$',p.stdout)
 p=subprocess.run([str(exe)],capture_output=True,text=True);(folder/'run.log').write_text(p.stdout+p.stderr);assert p.returncode!=0 and 'FAIL ' in p.stdout;result[name]=p.stdout.strip()
(OUT/'result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
