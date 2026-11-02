from pathlib import Path
import os,subprocess,sys,argparse
parser=argparse.ArgumentParser();parser.add_argument('--skip-mutants',action='store_true');args=parser.parse_args()
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=os.environ.copy();env['INCLUDE']=f'{VC}/include;{SDK}/Include;{HERE}';env['LIB']=f'{VC}/lib/amd64;{SDK}/Lib/x64'
log=[]
def run(name,cmd,environment=env):
 r=subprocess.run(cmd,env=environment,capture_output=True,text=True,cwd=OUT);text=r.stdout+r.stderr;(OUT/(name+'.log')).write_text(text);log.append(text);print(text,end='',flush=True)
 if r.returncode:(OUT/'full.log').write_text('\n'.join(log));raise SystemExit(r.returncode)
run('generate',[sys.executable,str(HERE/'generate.py')])
run('rgb-oracle',[sys.executable,str(HERE/'prepare_rgb_oracle.py')])
run('rgb-contract',[sys.executable,str(HERE/'test_rgb_contract.py')])
for name in ['policy','admission','warp','rgb_warp']:
 exe=OUT/(name+'.exe');run(name+'-build',[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT','/W3',str(HERE/(name+'_tests.cpp')),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib']);run(name+'-run',[str(exe)])
run('native-extracted',[sys.executable,str(HERE/'test_native_extracted.py')])
sdkEnv=env.copy();sdkEnv['INCLUDE']+=';D:/DEV/Kenshi/KenshiLib-src/Include;D:/DEV/Kenshi/KenshiLib-src/Include/ogre;D:/DEV/Kenshi/deps/boost_1_60_0;D:/DEV/Kenshi/shared'
run('native-build',[str(VC/'bin/amd64/cl.exe'),'/nologo','/c','/EHsc','/MD','/O2','/GL','/W1','/DUNICODE','/D_UNICODE',str(HERE/'native_compile.cpp'),'/Fo'+str(OUT/'native.obj')],sdkEnv)
if not args.skip_mutants:
 run('mutants',[sys.executable,str(HERE/'test_mutants.py')])
 run('rgb-mutants',[sys.executable,str(HERE/'test_rgb_mutants.py')])
(OUT/'full.log').write_text('\n'.join(log));print('PASS complete isolated instanced-building suite (native adapter compiled, not invoked)')
