"""Execute actual placement header against native ownership transitions."""
from pathlib import Path
import os,subprocess,sys
here=Path(__file__).resolve().parent
vc=Path(r'C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC')
sdk=Path(r'C:\Program Files\Microsoft SDKs\Windows\v7.1')
kl=Path(r'D:\DEV\Kenshi\KenshiLib-src\Include')
out=here/'_generated';out.mkdir(exist_ok=True)
source=Path(sys.argv[1])if len(sys.argv)>1 else here/'placement_fixture.cpp'
exe=out/(source.stem+'.exe')
env=os.environ.copy();env['PATH']=str(vc/'bin/amd64')+';'+r'D:\SteamLibrary\steamapps\common\Kenshi'+';'+env['PATH']
env['INCLUDE']=';'.join(map(str,[vc/'include',sdk/'Include',kl,kl/'ogre',here.parent,here,Path(r'D:\DEV\Kenshi\deps\boost_1_60_0')]))
env['LIB']=';'.join(map(str,[vc/'lib/amd64',sdk/'Lib/x64',Path(r'D:\DEV\Kenshi\deps\KenshiLib\Libraries')]))
subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2',str(source),'/Fo'+str(out)+'\\','/Fe'+str(exe),'OgreMain_x64.lib'],env=env,check=True)
subprocess.run([str(exe)],env=env,check=True)
