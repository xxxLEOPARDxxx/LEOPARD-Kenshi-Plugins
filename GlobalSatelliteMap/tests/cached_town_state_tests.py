from pathlib import Path
import os,subprocess,sys,argparse
parser=argparse.ArgumentParser();parser.add_argument('--skip-mutants',action='store_true');args=parser.parse_args()
HERE=Path(__file__).resolve().parent;MODULE=HERE.parent;OUT=MODULE/'build/cached-town-state-tests';OUT.mkdir(parents=True,exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=os.environ.copy();env['INCLUDE']=f'{VC}/include;{SDK}/Include;{MODULE}';env['LIB']=f'{VC}/lib/amd64;{SDK}/Lib/x64'
exe=OUT/'state.exe';r=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/W3',str(HERE/'cached_town_state_tests.cpp'),'/Fo'+str(OUT/'state.obj'),'/Fe'+str(exe),'/link','/STACK:8388608'],env=env,capture_output=True,text=True);(OUT/'build.log').write_text(r.stdout+r.stderr)
if r.returncode:print(r.stdout+r.stderr);raise SystemExit(r.returncode)
r=subprocess.run([str(exe),'D:/SteamLibrary/steamapps/common/Kenshi/data/meshes/distant/distant_2.mesh'],capture_output=True,text=True);(OUT/'run.log').write_text(r.stdout+r.stderr);print(r.stdout+r.stderr,end='')
if r.returncode:raise SystemExit(r.returncode)
env['INCLUDE']+=';D:/DEV/Kenshi/KenshiLib-src/Include;D:/DEV/Kenshi/KenshiLib-src/Include/ogre;D:/DEV/Kenshi/deps/boost_1_60_0;D:/DEV/Kenshi/shared'
r=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/c','/EHsc','/MD','/O2','/W1','/DUNICODE','/D_UNICODE',str(HERE/'cached_town_state_native.cpp'),'/Fo'+str(OUT/'native.obj')],env=env,capture_output=True,text=True);(OUT/'native-build.log').write_text(r.stdout+r.stderr)
if r.returncode:print(r.stdout+r.stderr);raise SystemExit(r.returncode)
print('Complete native cached State adapter compile PASS (not invoked)')
subprocess.run([sys.executable,str(HERE/'cached_town_native_extract_tests.py')],check=True)
if not args.skip_mutants:subprocess.run([sys.executable,str(HERE/'cached_town_state_mutants.py')],check=True)
