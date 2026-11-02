from pathlib import Path
import os,subprocess,sys,hashlib,json
HERE=Path(__file__).resolve().parent
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
pins=json.loads((HERE/'fixture-pins.json').read_text())
for name,pin in pins['fixtures'].items():
 data=(HERE/name).read_bytes();assert len(data)==pin['bytes'] and hashlib.sha256(data).hexdigest()==pin['sha256'],name
out=HERE/'build';out.mkdir(exist_ok=True)
def run(name,header,green,opt="/O2"):
 folder=out/name;folder.mkdir(exist_ok=True)
 (folder/'geometry_tests.cpp').write_bytes((HERE/'geometry_tests.cpp').read_bytes())
 if header is not None:(folder/'FixedTerrainGeometry.h').write_text(header)
 p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo',opt,'/EHsc','/W3',str(folder/'geometry_tests.cpp'),'/Fo'+str(folder/'test.obj'),'/Fe'+str(folder/'test.exe')],env=env,cwd=folder,capture_output=True,text=True)
 (folder/'build.log').write_text(p.stdout+p.stderr)
 if header is None:
  assert p.returncode!=0 and 'FixedTerrainGeometry.h' in p.stdout;print('pre-helper expected RED: missing implementation');return
 assert p.returncode==0,(name,p.stdout,p.stderr)
 p=subprocess.run([str(folder/'test.exe')],cwd=HERE,capture_output=True,text=True);(folder/'run.log').write_text(p.stdout+p.stderr)
 assert (p.returncode==0)==green,(name,p.returncode,p.stdout[:1600])
 print(name+': '+p.stdout.strip().splitlines()[-1])
if '--pre-helper-red' in sys.argv:
 run('pre-helper-red',None,False);sys.exit(0)
header=(HERE/'FixedTerrainGeometry.h').read_text();run('actual',header,True);run('debug',header,True,'/Od')
for name,old,new in [
 ('fine-normal-substitute','Vec3 result={v.value[6],v.value[4],v.value[5]};','Vec3 result={v.value[3],v.value[4],v.value[5]};'),
 ('single-halo','x+66','x+65'),
 ('wrong-global-parity','if(gx&1u)','if((gx+1)&1u)'),
 ('missing-corner-diagonal','if((gx&1u)&&(gz&1u))','if(false)'),
 ('wrong-strip-columns','z*65+x','z*65+(64-x)'),
 ('clamp-world-edge','if(x<0||z<0||x>16384||z>16384)return 0.0f;','if(x<0)x=0;if(z<0)z=0;if(x>16384)x=16384;if(z>16384)z=16384;')]:
 assert old in header,name;run(name,header.replace(old,new),False)
