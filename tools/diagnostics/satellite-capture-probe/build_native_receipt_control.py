from pathlib import Path
import os,subprocess,sys
here=Path(__file__).resolve().parent;out=here/'build/native-receipt-control';out.mkdir(parents=True,exist_ok=True)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include;D:/DEV/Kenshi/KenshiLib-src/Include;D:/DEV/Kenshi/deps/boost_1_60_0',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64;D:/DEV/Kenshi/deps/KenshiLib/Libraries;D:/DEV/Kenshi/deps/boost_1_60_0/stage/lib')
p=subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/LD','/EHsc','/MD','/O2','/W3','/DNDEBUG','/DWIN32',str(here/'native_receipt_control_compile.cpp'),'/Fo'+str(out/'control.obj'),'/Fe'+str(out/'control.dll'),'/link','OgreMain_x64.lib','advapi32.lib'],cwd=here,env=env,capture_output=True,text=True);(out/'build.log').write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='');sys.exit(p.returncode)
