"""Compile real helper/worker mutants; each must fail an actual behavior check."""
from pathlib import Path
import json,subprocess,sys
P=Path(__file__).resolve().parents[1];OUT=P/'build/cached-town-job-tests';OUT.mkdir(parents=True,exist_ok=True)
helper=(P/'GsmCachedTownJob.h').read_text(encoding='utf-8');job=(P/'GsmAtlasJob.h').read_text(encoding='utf-8')
cases=[
 ('expand-cap','helper','const SIZE_T MaxBytes=PrivateTownLod::MaxBytes;','const SIZE_T MaxBytes=1024*1024;','oversize refused before payload IO'),
 ('allow-writer','helper','GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING','GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING','source denies concurrent writer throughout actual read'),
 ('omit-path-check','helper','!FinalPath(handle,path)','false','final path mismatch refuses before payload IO'),
 ('skip-parser','helper','if(!PrivateTownLod::Parser(&out.bytes[0],out.bytes.size()).Read(out.info))','if(false)','actual parser bounds/signature returned'),
 ('late-cancel','helper','if(Stopped(cancel,context)){state=Discard(out);ok=false;break;}','','midread cancel clears partial payload at chunk boundary'),
 ('retain-failed-payload','helper','{out.Clear();out.state=Refused;','{out.state=Refused;','final path rechecked after read before publication'),
 ('skip-worker-mode','job','if(job.cachedTown){ReadCachedTown(job);}','if(false){ReadCachedTown(job);}','actual worker distinct cached-town dispatch'),
 ('allow-mixed-worker','job','if(unsigned(job.validation)+unsigned(job.waterValidation)+unsigned(job.modelValidation)+unsigned(job.cachedTown)>1)','if(false)','cached and terrain modes exclusive before read'),
 ('ignore-generation','job','if(job.generation!=InterlockedCompareExchange(&GSM::generation,0,0))','if(false)','stale generation discarded before read'),
]
results=[]
for name,target,old,new,reason in cases:
    text=helper if target=='helper' else job;assert old in text,name
    mutated=text.replace(old,new)
    native=OUT/(name+'-helper.h');native.write_text(mutated if target=='helper' else helper,encoding='utf-8')
    worker=OUT/(name+'-job.h');worker.write_text((job if target=='helper' else mutated).replace('#include "GsmCachedTownJob.h"','#include "'+native.as_posix()+'"'),encoding='utf-8')
    p=subprocess.run([sys.executable,str(P/'tests/cached_town_job_tests.py'),'--case-name',name,'--case-header',str(worker)],capture_output=True,text=True)
    log=p.stdout+p.stderr;(OUT/(name+'-expected-red.log')).write_text(log,encoding='utf-8')
    assert p.returncode and 'FAIL '+reason in log,(name,log)
    print('Expected RED',name,'->',reason);results.append(dict(name=name,reason=reason))
(OUT/'mutants.json').write_text(json.dumps(results,indent=2)+'\n',encoding='utf-8')
print('PASS',len(cases),'compiled cached-town worker/helper functional mutants')
