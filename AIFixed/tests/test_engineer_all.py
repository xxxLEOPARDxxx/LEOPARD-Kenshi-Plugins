"""Offline verification only: host fixtures and file/native contracts; no game run."""
from pathlib import Path
import hashlib,json,subprocess,sys,time

here=Path(__file__).resolve().parent
root=here.parent
out=here/'_generated/engineer-final-checks'
out.mkdir(parents=True,exist_ok=True)
production=[root/'AIFixed.cpp',root/'AIFixed.ini',*sorted(root.glob('Engineer*.h')),
            *sorted(root.glob('Unstuck*.h')),
            *sorted((root/'locale').glob('*/LC_MESSAGES/ai_fixed.po'))]
def hashes():
    return {str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in production}
before=hashes()
tests=sorted(p for p in here.glob('test_engineer_*.py') if p.name!='test_engineer_all.py')
tests += [here/name for name in ('test_policies.py','test_bed_selection.py',
    'test_delay_settings.py','test_gate_runtime.py','test_lifetime_debug.py',
    'test_settings_locales.py','test_landing_runtime.py','test_physical_cap.py','test_platform_pocket.py',
    'test_guard_trace.py')]
report={'started_utc':time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()),
        'game_started':False,'source_hashes':before,'tests':[]}
for script in tests:
    started=time.perf_counter()
    run=subprocess.run([sys.executable,str(script)],cwd=root,capture_output=True,text=True,encoding='utf8',errors='replace',timeout=120)
    log=run.stdout+run.stderr
    (out/(script.stem+'.log')).write_text(log,encoding='utf8')
    report['tests'].append({'name':script.name,'exit_code':run.returncode,'seconds':round(time.perf_counter()-started,3)})
    print(f'{script.name}: {"PASS" if run.returncode==0 else "FAIL"}',flush=True)
    if run.returncode:
        print(log[-14000:])
        (out/'result.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf8')
        raise SystemExit(run.returncode)
assert hashes()==before,'Production changed during verification; rerun on a stable candidate'
report['status']='PASS'
report['finished_utc']=time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime())
(out/'result.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf8')
print(f'Offline engineer integration and existing AI Fixed regressions: PASS ({len(tests)} suites)')
