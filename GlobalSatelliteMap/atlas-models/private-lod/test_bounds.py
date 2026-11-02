"""Actual production BlobInfo/Parser real-file bounds API and sensitive mutants."""
from pathlib import Path
import os,subprocess,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated/bounds';OUT.mkdir(parents=True,exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=os.environ.copy();env['INCLUDE']=f'{VC}/include;{SDK}/Include;{HERE}';env['LIB']=f'{VC}/lib/amd64;{SDK}/Lib/x64'
def run(label,header=None):
 defines=[] if header is None else ['/DPRIVATE_BLOB_HEADER="'+header.as_posix()+'"']
 exe=OUT/(label+'.exe');cmd=[VC/'bin/amd64/cl.exe','/nologo','/EHsc','/MD','/O2','/W3',*defines,'/Fo'+str(OUT/(label+'.obj')),'/Fe'+str(exe),HERE/'bounds_tests.cpp']
 r=subprocess.run([str(a) for a in cmd],env=env,capture_output=True,text=True);(OUT/(label+'-build.log')).write_text(r.stdout+r.stderr)
 if r.returncode:print(r.stdout+r.stderr);raise SystemExit(r.returncode)
 r=subprocess.run([str(exe),'D:/SteamLibrary/steamapps/common/Kenshi/data/meshes/distant'],capture_output=True,text=True);(OUT/(label+'-run.log')).write_text(r.stdout+r.stderr);return r
if __name__=='__main__':
 r=run('actual');print(r.stdout+r.stderr,end='')
 if r.returncode:raise SystemExit(r.returncode)
 source=(HERE/'BlobPolicy.h').read_text();needle='std::memcpy(out.localBounds,f,24);';assert source.count(needle)==1
 for label,replacement in [('omit-copy',''),('wrong-source','std::memcpy(out.localBounds,f+1,24);'),('omit-height','std::memcpy(out.localBounds,f,24);out.localBounds[1]=out.localBounds[4]=0;')]:
  header=OUT/(label+'.h');header.write_text(source.replace(needle,replacement));r=run(label,header)
  assert r.returncode and 'FAIL actual native bounds copied exactly' in r.stderr,(label,r.stdout,r.stderr)
  print('Expected RED:',label)
 print('PASS three production bounds-copy mutants rejected')
