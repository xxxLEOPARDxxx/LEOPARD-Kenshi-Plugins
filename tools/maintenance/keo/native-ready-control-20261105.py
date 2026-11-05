"""Single-variable native-readiness control; no DLL/mod-list changes."""
from pathlib import Path
import hashlib,json,re,subprocess,sys
from datetime import datetime,timezone,timedelta
G=Path('D:/SteamLibrary/steamapps/common/Kenshi');D=G/'_codex_work/20261105-squin-second-load'
ini=G/'mods/KEO/KEO.ini';backup=D/'KEO.ini.before-native-ready-control'
state=D/'native-ready-control.json';sha=lambda b:hashlib.sha256(b).hexdigest()
assert len(sys.argv)==2 and sys.argv[1]in('apply','restore')
assert subprocess.check_output(['powershell','-NoProfile','-Command','@(Get-Process Kenshi* -ErrorAction SilentlyContinue).Count'],text=True).strip()=='0','Close Kenshi first; no file changed'
if sys.argv[1]=='apply':
 assert not state.exists(),'Control already recorded; do not overwrite baseline'
 before=ini.read_bytes();assert not backup.exists()
 matches=list(re.finditer(rb'(?mi)^[ \t]*readinessOverrides[ \t]*=[^\r\n]*',before));assert len(matches)<=1
 after=re.sub(rb'(?mi)^([ \t]*readinessOverrides[ \t]*=)[^\r\n]*',rb'\g<1>false',before)if matches else before+b'\r\n; Temporary single-variable Squin reload control\r\nreadinessOverrides=false\r\n'
 assert after!=before,'Already native readiness; control would change no variable'
 backup.write_bytes(before);ini.write_bytes(after);assert ini.read_bytes()==after
 receipt=dict(status='native_ready_control_applied',time_msk=datetime.now(timezone(timedelta(hours=3))).isoformat(),before_sha256=sha(before),control_sha256=sha(after),only_changed_setting='readinessOverrides=false',dll_or_mod_list_changed=False)
 state.write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8')
else:
 receipt=json.loads(state.read_text(encoding='utf-8'));assert sha(ini.read_bytes())==receipt['control_sha256'],'User changed INI; refuse blind restore'
 before=backup.read_bytes();assert sha(before)==receipt['before_sha256'];ini.write_bytes(before);assert ini.read_bytes()==before
 receipt['status']='original_ini_restored';state.write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8')
print(json.dumps(receipt))
