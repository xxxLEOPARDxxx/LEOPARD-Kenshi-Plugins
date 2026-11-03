from pathlib import Path
import os,subprocess,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
ENV=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
def run(name,args):
 p=subprocess.run(args,cwd=HERE,env=ENV,capture_output=True,text=True);(OUT/(name+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='',flush=True);return p.returncode
code=run('dependencies-before',[sys.executable,str(HERE/'freeze.py'),'--dependencies'])
if code:sys.exit(code)
code=run('prepare_fixtures',[sys.executable,str(HERE/'prepare_fixtures.py')])
if code:sys.exit(code)
for test in ['api_tests','warp_tests']:
 code=run(test+'-build',[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT','/W3',str(HERE/(test+'.cpp')),'/Fo'+str(OUT/(test+'.obj')),'/Fe'+str(OUT/(test+'.exe')),'/link','d3d11.lib','dxgi.lib','advapi32.lib'])
 if code:sys.exit(code)
 code=run(test,[str(OUT/(test+'.exe'))])
 if code:sys.exit(code)
code=run('failures',[sys.executable,str(HERE/'test_failures.py')])
if code:sys.exit(code)
sys.exit(run('dependencies-after',[sys.executable,str(HERE/'freeze.py'),'--dependencies']))
