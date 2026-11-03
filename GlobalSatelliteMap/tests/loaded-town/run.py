from pathlib import Path
import os, subprocess
here=Path(__file__).resolve().parent
out=here/'_generated';out.mkdir(exist_ok=True)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=os.environ.copy();env['INCLUDE']=f'{vc}/include;{sdk}/Include'
env['LIB']=f'{vc}/lib/amd64;{sdk}/Lib/x64'
def run(name,args,environment=env):
    p=subprocess.run(args,cwd=out,env=environment,capture_output=True,text=True)
    text=p.stdout+p.stderr;(out/(name+'.log')).write_text(text)
    print(text,end='',flush=True)
    if p.returncode:raise SystemExit(p.returncode)
cl=str(vc/'bin/amd64/cl.exe')
for opt in ['Od','O2']:
    run('packet-'+opt,[cl,'/nologo','/EHsc','/'+opt,'/MT',str(here/'packet_tests.cpp'),'/Fo'+str(out/'packet.obj'),'/Fe'+str(out/'packet.exe'),'/link','d3d11.lib','dxgi.lib'])
    run('packet-run-'+opt,[str(out/'packet.exe')])
native=env.copy();native['INCLUDE']+=';D:/DEV/Kenshi/KenshiLib-src/Include;D:/DEV/Kenshi/KenshiLib-src/Include/ogre;D:/DEV/Kenshi/deps/boost_1_60_0;D:/DEV/Kenshi/shared'
run('native',[cl,'/nologo','/c','/EHsc','/MD','/O2','/GL','/W1','/DUNICODE','/D_UNICODE',str(here/'native.cpp'),'/Fo'+str(out/'native.obj')],native)
print('PASS actual loaded-town adapter compilation and refresh controls')
