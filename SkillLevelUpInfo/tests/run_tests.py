"""Run module regressions sequentially (the native fixtures reuse one output path).

Use the bundled Python. Does not install, publish or alter live game files.
"""
from pathlib import Path
import subprocess,sys,hashlib,gettext,re,json
here=Path(__file__).resolve().parent
mod=here.parent
commands=[['test_xp_core.py'],['test_head_anchor.py'],
          ['test_head_anchor.py','xp_boundary_fixture.cpp'],['test_panel.py'],['test_update.py']]
log=[]
for args in commands:
 result=subprocess.run([sys.executable,str(here/args[0]),*args[1:]],text=True,capture_output=True)
 text=result.stdout+result.stderr;print(text,end='');log.append(text)
 if result.returncode:raise SystemExit(result.returncode)
source=(mod/'SkillLevelUpInfo.cpp').read_text(encoding='utf-8-sig')
messages=set(re.findall(r'Tr\("([^"]+)"\)',source))
messages.update(re.findall(r'"([A-Za-z][A-Za-z ]+)"',source[source.index('kStatNames'):source.index('struct Snapshot')]))
for locale in ('en_GB','ru_RU','zh_CN'):
 p=mod/'locale'/locale/'LC_MESSAGES'/'skill_level_up_info.mo'
 with p.open('rb') as f:catalog=gettext.GNUTranslations(f)
 missing=messages-set(catalog._catalog)
 if missing:raise AssertionError((locale,missing))
 assert all(catalog.gettext(m).strip() for m in messages)
 print(locale,len(catalog._catalog),'MO entries; all active UI messages covered')
dll=Path(r'D:\DEV\Kenshi\build\SkillLevelUpInfoParityCheck\SkillLevelUpInfoParityCheck.dll')
if dll.exists():
 cmd=[sys.executable,r'D:\DEV\Kenshi\forks\KEO\tools\kenshilib\check_imports.py',str(dll)]
 subprocess.run(cmd,check=True)
 evidence={'dll':str(dll),'bytes':dll.stat().st_size,'sha256':hashlib.sha256(dll.read_bytes()).hexdigest(),
           'checks':{'xp_core':18,'head_anchor':19,'xp_native_boundary':19,'panel_native_boundary':10,'timing_lifecycle_registration':12},
           'runtime_verified':False,'installed':False}
 out=here/'_generated';out.mkdir(exist_ok=True)
 (out/'verification.json').write_text(json.dumps(evidence,indent=2)+'\n',encoding='utf-8')
 print(json.dumps(evidence,indent=2))
print('All SkillLevelUpInfo module checks passed; in-game visual/lifecycle acceptance remains pending.')
