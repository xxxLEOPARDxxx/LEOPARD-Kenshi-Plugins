from pathlib import Path
import os,subprocess
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
def run(name,source,opt,red=False):
 exe=OUT/(name+'.exe');p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT',opt,str(source),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe)],env=env,capture_output=True,text=True)
 (OUT/(name+'-build.log')).write_text(p.stdout+p.stderr);assert p.returncode==0,p.stdout+p.stderr
 p=subprocess.run([str(exe)],env=env,capture_output=True,text=True);(OUT/(name+'-run.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='');assert (p.returncode!=0)==red
header=(HERE/'GeoRootChain.h').read_text();old=header.replace('zone!=slot','slot!=game');assert old!=header
old=old.replace('"../source-receipts/ReceiptPolicy.h"','"'+(HERE.parent/'source-receipts/ReceiptPolicy.h').as_posix()+'"');mutant=OUT/'GeoRootChainLegacy.h';mutant.write_text(old)
source=OUT/'root-chain-legacy.cpp';source.write_text((HERE/'root_chain_tests.cpp').read_text().replace('"GeoRootChain.h"','"'+mutant.as_posix()+'"'))
run('root-chain-original-equality-red',source,'/Od',True)
run('root-chain-Od',HERE/'root_chain_tests.cpp','/Od');run('root-chain-O2',HERE/'root_chain_tests.cpp','/O2')
