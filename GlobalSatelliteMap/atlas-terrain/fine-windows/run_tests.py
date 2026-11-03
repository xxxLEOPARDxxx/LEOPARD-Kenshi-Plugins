from pathlib import Path
import os,subprocess,sys,json,hashlib
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
def verify():
 for name,want in json.loads((HERE/'dependencies.json').read_text()).items():
  assert hashlib.sha256((HERE.parents[2]/name).read_bytes()).hexdigest()==want,name
 print('PASS unchanged policy dependency pins',flush=True)
verify()
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
ENV=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
def run(name,args):
 p=subprocess.run(args,cwd=HERE,env=ENV,capture_output=True,text=True);(OUT/(name+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='',flush=True);return p.returncode
for mode in ['O2','Od']:
 exe=OUT/('planner_'+mode+'.exe')
 code=run('build-'+mode,[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/'+mode,'/MT','/W3',str(HERE/'planner_tests.cpp'),'/Fo'+str(OUT/('planner_'+mode+'.obj')),'/Fe'+str(exe)])
 if code:sys.exit(code)
 code=run('run-'+mode,[str(exe)])
 if code:sys.exit(code)
if '--skip-mutants' not in sys.argv:
 code=run('mutants',[sys.executable,str(HERE/'test_mutants.py')])
 if code:sys.exit(code)
verify()
