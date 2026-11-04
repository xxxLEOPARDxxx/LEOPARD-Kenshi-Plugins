"""Execute the unchanged production shutdown bodies against bounded native states."""
from pathlib import Path
import os, subprocess, sys
sys.dont_write_bytecode=True
here=Path(__file__).resolve().parent; root=here.parents[1]; out=here/'_generated'; out.mkdir(exist_ok=True)
source=(root/'atlas-terrain/fine-coordinator/FineCoordinator.h').read_text()
def body(signature):
    a=source.index(signature); a=source.index('{',a); b=a+1; level=1
    while level:
        level+=(source[b]=='{')-(source[b]=='}'); b+=1
    return source[a+1:b-1]
request=body(' HRESULT RequestShutdown()'); poll=body(' HRESULT PollShutdown()'); queue=body(' bool Queue(')
template=(here/'shutdown.cpp').read_text().replace('REQUEST_BODY',request).replace('POLL_BODY',poll).replace('QUEUE_BODY',queue)
(out/'actual.cpp').write_text(template)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'); sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=os.environ.copy(); env['INCLUDE']=f'{vc}/include;{sdk}/Include';env['LIB']=f'{vc}/lib/amd64;{sdk}/Lib/x64'
for opt in ['Od','O2']:
    args=[str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/'+opt,'/MT',str(out/'actual.cpp'),'/Fo'+str(out/'actual.obj'),'/Fe'+str(out/'actual.exe')]
    p=subprocess.run(args,cwd=out,env=env,capture_output=True,text=True);(out/('build-'+opt+'.log')).write_text(p.stdout+p.stderr)
    if p.returncode:print(p.stdout+p.stderr);raise SystemExit(p.returncode)
    p=subprocess.run([str(out/'actual.exe')],cwd=out,capture_output=True,text=True);(out/('run-'+opt+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='')
    if '--red' in sys.argv:
        assert p.returncode and 'FAIL historical ERROR_BUSY known join is not quarantined' in p.stdout
    elif p.returncode:raise SystemExit(p.returncode)
