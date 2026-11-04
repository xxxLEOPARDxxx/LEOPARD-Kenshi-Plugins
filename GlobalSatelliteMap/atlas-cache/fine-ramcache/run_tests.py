from pathlib import Path
import importlib.util,subprocess,sys,json,uuid
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
spec=importlib.util.spec_from_file_location('worker_tests',HERE.parent/'fine-worker/run_tests.py');runner=importlib.util.module_from_spec(spec);spec.loader.exec_module(runner)
def run(opt,red=False,source='tests.cpp'):
    name=Path(source).stem+'-'+opt[1:];exe=OUT/(name+'.exe')
    p=subprocess.run([str(runner.VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MT',opt,'/W3','/DUNICODE','/D_UNICODE',str(HERE/source),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),'/link','advapi32.lib'],cwd=HERE,env=runner.ENV,capture_output=True,text=True)
    (OUT/(name+'-build.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='');assert not p.returncode
    root=runner.OUT/('fixtures-'+uuid.uuid4().hex);root.mkdir()
    p=subprocess.run([str(exe),str(root)],cwd=HERE,env=runner.ENV,capture_output=True,text=True);(OUT/(name+('-red'if red else'')+'-run.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='')
    runner.cleanup(root,'ram-'+name)
    if red:assert p.returncode and ('FAIL 32GiB profile is lazy' if red=='limit' else 'FAIL successful Put warms prepared RAM tile') in p.stderr
    else:assert not p.returncode
if __name__=='__main__':
    if '--limit-red'in sys.argv:
        run('/Od','limit','lru.cpp');raise SystemExit(0)
    run('/Od','--red'in sys.argv)
    if '--red'not in sys.argv:
        run('/O2')
        for source in ['lru.cpp','allocation.cpp','quarantine.cpp']:
            run('/Od',source=source);run('/O2',source=source)
