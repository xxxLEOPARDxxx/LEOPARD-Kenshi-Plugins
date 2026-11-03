from pathlib import Path
import os,subprocess,sys
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
def run(name,cpp,opt='/O2',green=True):
 exe=OUT/(name+'.exe');p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT',opt,'/W3',str(cpp),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),'/link','advapi32.lib'],cwd=HERE,env=env,capture_output=True,text=True);(OUT/(name+'-build.log')).write_text(p.stdout+p.stderr);assert not p.returncode,(name,p.stdout,p.stderr)
 p=subprocess.run([str(exe)],cwd=HERE,capture_output=True,text=True);(OUT/(name+'-run.log')).write_text(p.stdout+p.stderr);print(name,(p.stdout+p.stderr).strip(),flush=True);assert (p.returncode==0 if green else p.returncode==1 and 'FAIL 'in p.stderr),(name,p.returncode)
if __name__=='__main__':
 if (HERE/'verification.json').exists():subprocess.run([sys.executable,str(HERE/'freeze.py'),'--verify'],cwd=HERE,check=True)
 for opt in ['/O2','/Od']:run('accepted-'+opt[1:],HERE/'tests.cpp',opt)
 for script in ['test_faults.py','test_mips.py']+([]if'--skip-mutants'in sys.argv else['test_mutants.py']):
  p=subprocess.run([sys.executable,str(HERE/script)],cwd=HERE,capture_output=True,text=True);(OUT/(script+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='',flush=True);assert not p.returncode,script
