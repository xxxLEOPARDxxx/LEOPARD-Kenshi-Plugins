from pathlib import Path
import os,subprocess,sys,json,hashlib,gettext
root=Path(__file__).resolve().parents[2];out=Path(__file__).resolve().parent/'_generated';out.mkdir(exist_ok=True)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
inc=[vc/'include',sdk/'Include',Path('D:/DEV/Kenshi/KenshiLib-src/Include'),Path('D:/DEV/Kenshi/KenshiLib-src/Include/ogre'),Path('D:/DEV/Kenshi/deps/boost_1_60_0'),Path('D:/DEV/Kenshi/shared')]+[p for p in root.parent.iterdir()if p.is_dir()]
env=dict(os.environ,INCLUDE=';'.join(map(str,inc)))
cmd=[str(vc/'bin/amd64/cl.exe'),'/nologo','/c','/EHsc','/MD','/O2','/GS-','/GL','/W1','/DUNICODE','/D_UNICODE','/DNDEBUG','/DWIN32','/D_WINDOWS','/D_USRDLL','/Fo'+str(out/'map-info-native.obj'),str(root/'GlobalSatelliteMap.cpp')]
p=subprocess.run(cmd,cwd=root,env=env,capture_output=True,text=True);(out/'native-build.log').write_text(p.stdout+p.stderr);print((p.stdout+p.stderr)[-5000:]);sys.exit(p.returncode)
