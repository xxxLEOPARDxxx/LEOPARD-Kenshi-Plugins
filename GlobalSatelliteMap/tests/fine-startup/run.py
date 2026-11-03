"""Small actual-header startup regression. No native calls, worker or fixtures.

Native sizes use actual NativeStage/Session types; static reservation bodies,
controller Owner fields/Extra and MakeBudget are extracted unchanged except
qualifying names and replacing the native object's static accessor. This checks
the observed CPU ledger without starting an engine-dependent native session.
"""
from pathlib import Path
import os,re,subprocess,sys
sys.dont_write_bytecode=True
here=Path(__file__).resolve().parent;root=here.parents[1];out=here/'_generated';out.mkdir(exist_ok=True)
coordinator=(root/'atlas-terrain/fine-coordinator/FineCoordinator.h').read_text()
rebind=(root/'atlas-terrain/world-native-rebind/NativeRebind.h').read_text()
cohort=(root/'atlas-terrain/world-cohort/WorldCohort.h').read_text()
def body(text,signature):
    a=text.index(signature);a=text.index('{',a);level=1;b=a+1
    while level:
        level+=(text[b]=='{')-(text[b]=='}');b+=1
    return text[a+1:b-1]
owner=coordinator.split(' struct Owner {',1)[1].split('  Owner():',1)[0]
native=body(cohort,' U64 CpuReservation()const').replace('sizeof(Controller)','sizeof(WorldCohort::Controller<ActualSession>)')
native=native.replace('sizeof(GpuOwners)','sizeof(WorldCohort::GpuOwners)').replace('sizeof(Work)','sizeof(WorldCohort::Work)')
extra=body(rebind,' U64 ExtraCpuPeakBytes()const').replace('sizeof(Session)','sizeof(ActualSession)').replace('sizeof(cohort_)','sizeof(WorldCohort::Controller<ActualSession>)').replace('sizeof(Owned)','sizeof(ActualOwned)')
make=body(coordinator,' bool MakeBudget(')
overhead=body(coordinator,' static U64 Extra()').replace('sizeof(Controller)','sizeof(FineCoordinator::Controller<ActualSession>)').replace('sizeof(Owner)','sizeof(ActualOwner)').replace('sizeof(NativeSession)','sizeof(ActualSession)')
legacy='r.viewerGpu+r.fineGpu+r.pendingGpu+r.retiredGpu<=FineJob::ViewerAllowance'
startup='return FineCoordinator::StartupFits(r);' if 'inline bool StartupFits(' in coordinator else 'return '+legacy+';'
legacy32='--legacy32' in sys.argv
if legacy32:startup='return r.viewerGpu+r.fineGpu+r.pendingGpu+r.retiredGpu<=32ULL*1024*1024;'
generated='''#pragma once
typedef WorldNative::StageAdapter<WorldNative::NativeStage> ActualOwned;
typedef WorldNativeRebind::Session<ActualOwned> ActualSession;
typedef ActualSession NativeSession;
struct ActualOwner {OWNER};
inline FineJob::U64 ActualCohortBytes(){NATIVE}
inline FineJob::U64 ActualExtraBytes(){EXTRA}
inline FineJob::U64 ActualNativeBytes(){return ActualCohortBytes()+ActualExtraBytes();}
inline bool TestStartup(const FineCoordinator::Residency&r){STARTUP}
struct ActualBudget {
 typedef FineCoordinator::U64 U64;
 struct NativeStatic{U64 CpuReservation()const{return ActualNativeBytes();}};
 struct OwnerStub{NativeStatic native;}owned;
 OwnerStub*owner_;bool quarantined_;
 ActualBudget():owner_(&owned),quarantined_(false){}
 bool Main()const{return true;}
 static bool Add(U64&a,U64 b,U64 cap){if(b>cap||a>cap-b)return false;a+=b;return true;}
 static U64 Extra(){OVERHEAD}
 bool MakeBudget(const FineCoordinator::Residency&r,const FineStore::Ledger&external,FineJob::Budget&out)const{MAKE}
};
'''
for key,value in dict(OWNER=owner,NATIVE=native,EXTRA=extra,STARTUP=startup,OVERHEAD=overhead,MAKE=make).items():generated=generated.replace(key,value)
(out/'actual_budget.h').write_text(generated)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=os.environ.copy();env['INCLUDE']=f'{vc}/include;{sdk}/Include;D:/DEV/Kenshi/KenshiLib-src/Include;D:/DEV/Kenshi/KenshiLib-src/Include/ogre;D:/DEV/Kenshi/deps/boost_1_60_0;D:/DEV/Kenshi/shared';env['LIB']=f'{vc}/lib/amd64;{sdk}/Lib/x64'
for opt in ['Od','O2']:
    p=subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/'+opt,'/MT',str(here/'startup.cpp'),'/Fo'+str(out/'startup.obj'),'/Fe'+str(out/'startup.exe'),'/link','d3d11.lib','dxgi.lib'],cwd=out,env=env,capture_output=True,text=True)
    (out/('build-'+opt+'.log')).write_text(p.stdout+p.stderr)
    if p.returncode:print(p.stdout+p.stderr);raise SystemExit(p.returncode)
    p=subprocess.run([str(out/'startup.exe')],cwd=out,capture_output=True,text=True)
    (out/(('legacy32-' if legacy32 else 'run-')+opt+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='')
    if legacy32:
        assert p.returncode==1 and 'FAIL actual town viewer plus retained fine cache starts' in p.stdout
        print('Expected RED: previous32MiB startup rejects observed town viewer even with zero fine windows')
    elif p.returncode:raise SystemExit(p.returncode)
