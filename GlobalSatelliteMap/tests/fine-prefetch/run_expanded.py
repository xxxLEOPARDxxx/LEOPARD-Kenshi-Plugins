from pathlib import Path
import os,subprocess
here=Path(__file__).resolve().parent;out=here/'_generated';out.mkdir(exist_ok=True)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
for mode in ('Od','O2'):
    exe=out/'expanded.exe'
    p=subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT','/'+mode,str(here/'expanded.cpp'),'/Fo'+str(out/'expanded.obj'),'/Fe'+str(exe),'/link','advapi32.lib'],cwd=out,env=env,capture_output=True,text=True)
    assert p.returncode==0,p.stdout+p.stderr
    p=subprocess.run([str(exe)],capture_output=True,text=True)
    print(p.stdout+p.stderr,end='');(out/('expanded-'+mode+'.log')).write_text(p.stdout+p.stderr)
    assert p.returncode==0
