from pathlib import Path
import hashlib,json
HERE=Path(__file__).resolve().parent
files=[HERE.parent/'WorldCohort.h',HERE/'tests.cpp',HERE/'run_tests.py',HERE/'README.md',HERE/'red-missing-api.log']
results={p.name:p.read_text().strip() for p in sorted((HERE/'_generated').glob('*.log')) if p.name.startswith(('normal-','wait-failed-','wait-failed-request-','timeout-wrap-','missing-marker-'))}
out={'contract':'Main-only Idle/WIdle/no-GPU async Stop; known join and marker before delete; bounded quarantine on unknown state','files':{p.relative_to(HERE.parent).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in files},'results':results,'limits':'Real WIC idle worker only. No live game, source/raster job, long-duration leak or FPS proof.'}
(HERE/'verification.json').write_bytes((json.dumps(out,indent=2)+'\n').encode())
print(json.dumps(out,indent=2))
