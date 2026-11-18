from pathlib import Path
import os,subprocess
here=Path('D:/DEV/Kenshi/ours/AIFixed/tests');out=here/'_generated/engineer_surplus';out.mkdir(parents=True,exist_ok=True)
p=out/'compile_runtime.cpp';p.write_text('#include <kenshi/Character.h>\n#include <EngineerSurplusRuntime.h>\n')
vc=Path(r'C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path(r'C:/Program Files/Microsoft SDKs/Windows/v7.1');dep=Path('D:/DEV/Kenshi/deps')
e=os.environ.copy();e['PATH']=str(vc/'bin/amd64')+';'+e['PATH'];e['INCLUDE']=';'.join(map(str,[vc/'include',sdk/'Include',here.parent,Path('D:/DEV/Kenshi/KenshiLib-src/Include'),Path('D:/DEV/Kenshi/KenshiLib-src/Include/ogre'),dep/'boost_1_60_0']));e['LIB']=str(vc/'lib/amd64')+';'+str(sdk/'Lib/x64')
r=subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/Od','/DUNICODE','/D_UNICODE','/DNOMINMAX','/DWIN32_LEAN_AND_MEAN','/c',str(p),'/Fo'+str(out)+'/'],env=e,cwd=out,capture_output=True,text=True);print(r.stdout,r.stderr);raise SystemExit(r.returncode)
