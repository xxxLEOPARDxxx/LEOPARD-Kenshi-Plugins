"""Build only the standalone startup test; no KEO engine sources are linked."""
import argparse,os,pathlib,subprocess
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--out',required=True,type=pathlib.Path)
args=parser.parse_args()
here=pathlib.Path(__file__).resolve().parent
out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
vc=pathlib.Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
sdk=pathlib.Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
kl=pathlib.Path(os.environ.get('KENSHILIB','D:/DEV/Kenshi/deps/KenshiLib_keo'))
boost=os.environ.get('BOOST_ROOT','D:/DEV/Kenshi/deps/boost_1_60_0')
mcm=here.parents[2]/'ModConfigMenu/sdk'
env=os.environ.copy();env['INCLUDE']=str(vc/'include')+';'+str(sdk/'Include');env['LIB']=str(vc/'lib/amd64')+';'+str(sdk/'Lib/x64');env['PATH']=str(vc/'bin/amd64')+';'+str(sdk/'Bin')+';'+env.get('PATH','')
cl=str(vc/'bin/amd64/cl.exe')
common=[cl,'/nologo','/EHsc','/MD','/O2','/GL','/DWIN32','/D_WINDOWS','/D_WIN64','/D_CRT_SECURE_NO_WARNINGS','/I'+str(mcm)]
commands=[common+[str(here/'StartupHostTests.cpp'),'/FoStartupHostTests.obj','/FeStartupHostTests.exe','/link','/LTCG'],
          [str(out/'StartupHostTests.exe')],
          common+['/LD','/I'+str(kl/'Include'),'/I'+str(kl/'Include/ogre'),'/I'+str(kl/'Include/mygui'),'/I'+boost,str(here/'GogStartupTest.cpp'),'/FoGogStartupTest.obj','/link','/LTCG','/OPT:REF','/OPT:ICF','/MACHINE:X64','/LIBPATH:'+str(kl/'Libraries/KenshiLib'),'KenshiLib.lib','/OUT:KEOGogStartupTest.dll','/IMPLIB:KEOGogStartupTest.lib']]
for command in commands:
 subprocess.run(command,cwd=out,env=env,check=True)
