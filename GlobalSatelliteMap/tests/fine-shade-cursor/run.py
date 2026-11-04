from pathlib import Path
import os,subprocess,sys
here=Path(__file__).resolve().parent;out=here/'_generated';out.mkdir(exist_ok=True)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
red='--red' in sys.argv
for opt in (['Od'] if red else ['Od','O2']):
 exe=out/('cursor-'+opt+'.exe');r=subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/'+opt,'/MT']+(['/DBASELINE_FIXED_START'] if red else[])+[str(here/'cursor.cpp'),'/Fo'+str(out/('cursor-'+opt+'.obj')),'/Fe'+str(exe)],cwd=out,env=env,capture_output=True,text=True);(out/('build-'+opt+'.log')).write_text(r.stdout+r.stderr);assert not r.returncode,r.stdout+r.stderr
 r=subprocess.run([str(exe)],capture_output=True,text=True);(out/('run-'+opt+('-red'if red else'')+'.log')).write_text(r.stdout+r.stderr);print(r.stdout+r.stderr,end='')
 if red:assert r.returncode and 'FAIL epoch changes' in r.stdout
 else:assert not r.returncode
