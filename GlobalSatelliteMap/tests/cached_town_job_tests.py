"""Bounded real native mesh reads and owned Win32 edge fixtures, no game calls."""
from pathlib import Path
import argparse,os,subprocess
P=Path(__file__).resolve().parents[1]
OUT=P/'build/cached-town-job-tests';OUT.mkdir(parents=True,exist_ok=True)
GAME=Path('D:/SteamLibrary/steamapps/common/Kenshi')
parser=argparse.ArgumentParser();parser.add_argument('--town-id',type=int,default=2);parser.add_argument('--case-header',type=Path);parser.add_argument('--case-name',default='actual');a=parser.parse_args()
root=OUT/'game';(root/'data/meshes/distant').mkdir(parents=True,exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include;D:/DEV/Kenshi/deps/boost_1_60_0;{P}',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
cmd=[VC/'bin/amd64/cl.exe','/nologo','/EHsc','/MD','/O2','/W3']
if a.case_header:cmd+=['/DCACHED_JOB_HEADER="'+a.case_header.as_posix()+'"']
cmd+=['/Fo'+str(OUT/(a.case_name+'.obj')),'/Fe'+str(OUT/(a.case_name+'.exe')),P/'tests/cached_town_job_tests.cpp','/link','advapi32.lib','user32.lib']
p=subprocess.run(list(map(str,cmd)),env=env,capture_output=True,text=True)
(OUT/(a.case_name+'-compile.log')).write_text(p.stdout+p.stderr,encoding='utf-8')
assert p.returncode==0,p.stdout+p.stderr
linked=OUT/'linked-game'
if linked.exists():linked.rmdir() # Removes only this owned directory junction, not its target.
created=subprocess.run(['cmd','/d','/c','mklink','/J',str(linked),str(root)],capture_output=True,text=True)
assert created.returncode==0,created.stdout+created.stderr
try:p=subprocess.run([str(OUT/(a.case_name+'.exe')),GAME.as_posix()+'/',root.as_posix()+'/',str(a.town_id),linked.as_posix()+'/'],capture_output=True,text=True)
finally:linked.rmdir()
(OUT/(a.case_name+'-run.log')).write_text(p.stdout+p.stderr,encoding='utf-8');print(p.stdout+p.stderr,end='');assert p.returncode==0
