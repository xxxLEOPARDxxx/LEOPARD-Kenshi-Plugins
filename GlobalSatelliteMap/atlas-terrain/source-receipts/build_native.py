from pathlib import Path
import os,subprocess,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include;D:/DEV/Kenshi/KenshiLib-src/Include;D:/DEV/Kenshi/deps/boost_1_60_0',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64;D:/DEV/Kenshi/deps/KenshiLib/Libraries;D:/DEV/Kenshi/deps/boost_1_60_0/stage/lib')
p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/LD','/EHsc','/MD','/O2','/W3','/DNDEBUG','/DWIN32',str(HERE/'native_compile.cpp'),'/Fo'+str(OUT/'native.obj'),'/Fe'+str(OUT/'native-contract.dll'),'/link','OgreMain_x64.lib','advapi32.lib'],cwd=HERE,env=env,capture_output=True,text=True);(OUT/'native-build.log').write_text(p.stdout+p.stderr);print(p.stdout+p.stderr);sys.exit(p.returncode)
