from pathlib import Path
import os,subprocess,hashlib,json
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=os.environ.copy();env['INCLUDE']=f'{VC}/include;{SDK}/Include';env['LIB']=f'{VC}/lib/amd64;{SDK}/Lib/x64'
header=HERE.parents[1]/'GsmFineSeamReadback.h'
source=header.read_text().replace('#include "atlas-view/AtlasRelight.h"','#include "'+(HERE.parents[1]/'atlas-view/AtlasRelight.h').as_posix()+'"')
fault=source.replace('context_.p->GetData(query_.p,&done,sizeof(done),D3D11_ASYNC_GETDATA_DONOTFLUSH)','TestQuery(context_.p,query_.p,&done,sizeof(done),D3D11_ASYNC_GETDATA_DONOTFLUSH)').replace('context_.p->Map(stage_[i].p,0,D3D11_MAP_READ,D3D11_MAP_FLAG_DO_NOT_WAIT,&maps.value[i])','TestMap(context_.p,stage_[i].p,0,D3D11_MAP_READ,D3D11_MAP_FLAG_DO_NOT_WAIT,&maps.value[i])').replace('c->Unmap(t[i],0)','TestUnmap(c,t[i],0)').replace('new(std::nothrow)BYTE[PayloadBytes]','TestAllocate()')
fault=fault.replace('d->CreateTexture2D(&td,NULL,&stage_[0].p)','TestCreate(d,&td,&stage_[0].p)').replace('d->CreateTexture2D(&td,NULL,&stage_[1].p)','TestCreate(d,&td,&stage_[1].p)').replace('d->CreateQuery(&qd,&query_.p)','TestCreateQuery(d,&qd,&query_.p)')
assert fault!=source
faultpath=OUT/'fault-header.h';faultpath.write_text(fault)
for opt in ['Od','O2']:
 for mode in ['native','faults']:
  label='readback-'+mode+'-'+opt;exe=OUT/(label+'.exe');extra=['/DTEST_FAULTS','/DSEAM_READBACK_HEADER="'+faultpath.as_posix()+'"'] if mode=='faults' else []
  p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT','/'+opt]+extra+[str(HERE/'tests.cpp'),'/Fo'+str(OUT/(label+'.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib'],cwd=OUT,env=env,capture_output=True,text=True);print(p.stdout+p.stderr,end='');assert p.returncode==0
  p=subprocess.run([str(exe)],cwd=OUT,capture_output=True,text=True,timeout=30);print(mode+': '+p.stdout+p.stderr,end='');assert p.returncode==0
mutants={
 'row-copy':('size_t(y)*maps.value[i].RowPitch','size_t(0)*maps.value[i].RowPitch'),
 'partial-unmap':('if(mapped[i])TestUnmap(c,t[i],0)','if(false)TestUnmap(c,t[i],0)'),
 'fence-error-latch':('if(FAILED(hr))return Fail(hr);if(!done)','if(FAILED(hr))return hr;if(!done)'),
}
for name,(old,new) in mutants.items():
 assert fault.count(old)==1,(name,fault.count(old))
 path=OUT/('mutant-'+name+'.h');path.write_text(fault.replace(old,new));exe=OUT/('mutant-'+name+'.exe')
 p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT','/O2','/DTEST_FAULTS','/DSEAM_READBACK_HEADER="'+path.as_posix()+'"',str(HERE/'tests.cpp'),'/Fo'+str(OUT/('mutant-'+name+'.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib'],cwd=OUT,env=env,capture_output=True,text=True);assert p.returncode==0,p.stdout+p.stderr
 p=subprocess.run([str(exe)],cwd=OUT,capture_output=True,text=True,timeout=30);assert p.returncode!=0 and 'FAIL ' in p.stdout,(name,p.stdout,p.stderr);print('KILLED '+name+': '+p.stdout.strip())
record={'productionSha256':hashlib.sha256(header.read_bytes()).hexdigest(),'testsSha256':hashlib.sha256((HERE/'tests.cpp').read_bytes()).hexdigest(),'runnerSha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'nativeControlsPerOptimization':26,'faultControlsPerOptimization':41,'optimizations':['Od','O2'],'mutantsKilled':list(mutants),'fixture':'Actual D3D11 WARP, two distinct continuous RGBA8 source planes; forced fault calls preserve real D3D otherwise.'}
(HERE/'verification.json').write_text(json.dumps(record,indent=2)+'\n')
