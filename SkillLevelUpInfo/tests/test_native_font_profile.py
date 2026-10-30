"""Verify real getter profile in a local helper process; never opens Kenshi."""
from pathlib import Path
import os,subprocess
p=Path(__file__).resolve().parent
vc=Path(r'C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC')
sdk=Path(r'C:\Program Files\Microsoft SDKs\Windows\v7.1')
kl=Path(r'D:\DEV\Kenshi\KenshiLib-src\Include')
out=p/'_generated';out.mkdir(exist_ok=True)
env=os.environ.copy();env['PATH']=str(vc/'bin/amd64')+';'+r'D:\SteamLibrary\steamapps\common\Kenshi'+';'+env['PATH']
env['INCLUDE']=';'.join(map(str,[vc/'include',sdk/'Include',kl,kl/'ogre',p.parent,p.parent.parent/'shared',Path(r'D:\DEV\Kenshi\deps\boost_1_60_0')]))
env['LIB']=';'.join(map(str,[vc/'lib/amd64',sdk/'Lib/x64']))
exe=out/'native_font_profile.exe'
subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/DNDEBUG',str(p/'native_font_profile.cpp'),'/Fo'+str(out)+'\\','/Fe'+str(exe)],cwd=out,env=env,check=True)
subprocess.run([str(exe)],env=env,check=True)
