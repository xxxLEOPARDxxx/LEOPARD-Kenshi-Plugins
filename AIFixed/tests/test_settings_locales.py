"""Validate actual MCM/defaults and compiled three-language catalogs; no install."""
from pathlib import Path
import configparser,gettext,json,re,subprocess,sys
base=Path(__file__).resolve().parents[1]
source=(base/'AIFixed.cpp').read_text(encoding='utf-8-sig')
mcm=source[source.index('extern "C" __declspec(dllexport) void MCM_Describe'):source.index('__declspec(dllexport) void startPlugin')]
calls=re.findall(r'api->(?:toggle|integer)\(api,\s*"AIFixed",\s*"([^"]+)".*?\);',mcm,re.S)
assert len(calls)==18 and len(set(calls))==18,calls
assert calls[-1]=='Debug'
for match in re.finditer(r'api->(?:toggle|integer)\(api,\s*"AIFixed",\s*"([^"]+)".*?\);',mcm,re.S):
    assert ('MCM_RESTART' in match.group(0))==(match.group(1) in ('Enabled','Debug'))
ini=configparser.ConfigParser();ini.read(base/'AIFixed.ini',encoding='utf-8-sig')
assert set(ini['AIFixed'])==set(key.lower() for key in calls)
for key in ('AutoSleep','RepairHealth','GatePassage','GateOpenLocked','Debug'):
    assert ini['AIFixed'][key]=='0',key
assert ini['AIFixed']['OwnBedsOnly']=='1'
messages={json.loads(m) for m in re.findall(r'Tr\(("(?:[^"\\]|\\.)*")\)',mcm)}
checks=27
for language in ('en_GB','ru_RU','zh_CN'):
    po=base/'locale'/language/'LC_MESSAGES/ai_fixed.po'
    subprocess.run([sys.executable,str(base.parent/'tools/po2mo.py'),str(po)],check=True)
    with po.with_suffix('.mo').open('rb') as stream:catalog=gettext.GNUTranslations(stream)
    for message in messages:
        assert message in catalog._catalog and catalog.gettext(message).strip(),(language,message)
        checks+=1
print(f'AIFixed settings/locales: PASS ({checks} checks;18 settings,13 new;{len(messages)} messages/language)')
