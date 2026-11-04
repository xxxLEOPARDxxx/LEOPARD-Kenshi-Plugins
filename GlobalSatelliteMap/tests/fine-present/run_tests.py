"""Extract real FinePresent/visibility/binding code; count projections/UI writes."""
from pathlib import Path
import os,subprocess,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
source=(HERE.parent.parent/'GsmFineViewer.h').read_text()
def block(start):
 begin=source.index(start);opening=source.index('{',begin);depth=1;end=opening+1
 while depth:
  depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[begin:end]
functions='\n'.join(block(s) for s in ['void FineDetachWidget(', 'void FineVisibility(', 'void FinePresent('])
helper=''
if '// BEGIN FINE PRESENT CACHE' in source:helper=source.split('// BEGIN FINE PRESENT CACHE',1)[1].split('// END FINE PRESENT CACHE',1)[0]
code=(HERE/'controls.cpp').read_text().replace('// EXTRACTED_CACHE',helper).replace('// EXTRACTED_FUNCTIONS',functions)
code=code.replace('// RESET_CACHE','finePresentation.Reset();' if helper else '')
cpp=OUT/'controls.cpp';cpp.write_text(code);exe=OUT/'controls.exe'
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
cmd=[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT','/W3',str(cpp),'/Fo'+str(OUT/'controls.obj'),'/Fe'+str(exe),'/link','advapi32.lib']
p=subprocess.run(cmd,cwd=OUT,env=env,capture_output=True,text=True);(OUT/'build.log').write_text(p.stdout+p.stderr)
if p.returncode:print(p.stdout+p.stderr);sys.exit(p.returncode)
p=subprocess.run([str(exe)],cwd=OUT,env=env,capture_output=True,text=True);(OUT/('run.log' if helper else 'legacy-red.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='');sys.exit(p.returncode)
