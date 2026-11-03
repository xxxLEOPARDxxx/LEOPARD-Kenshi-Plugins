from pathlib import Path
import re
from run_tests import HERE,OUT,run
def absolute(text):return re.sub(r'(^\s*#include\s+")([^"]+)(")',lambda m:m[1]+(HERE/m[2]).resolve().as_posix()+m[3],text,flags=re.M)
source=absolute((HERE/'AcceptedWindow.h').read_text());tests=absolute((HERE/'tests.cpp').read_text())
mutants=[
('coverage-zero','dst[3]=255;','dst[3]=0;'),
('rgb-incomplete','std::memcpy(dst,src,3);','std::memcpy(dst,src,2);dst[2]=0;'),
('wrong-material','out.material=v.material;','out.material=v.normal;'),
('missing-height','out.height=v.height;','out.height=NULL;'),
('omit-accepted-charge','required,p.OwnedBytes(),w.OwnedBytes()','required,0,w.OwnedBytes()'),
('omit-raw-charge','sizeof(w),l.retainedRaw,l.other','sizeof(w),0,l.other'),
('skip-getter-budget','if(!Budget(p,w,v.side,ledger)){Fail(E_OUTOFMEMORY);return false;}',''),
('skip-source-identity','||!WorldHeight::Same(height_,v.heightSource)',''),
('skip-owner-content','||std::memcmp(v.normal+at,normal_+at,3)',''),
('retain-failed-copy','HRESULT Fail(HRESULT hr){Clear();error_=hr;return hr;}','HRESULT Fail(HRESULT hr){error_=hr;return hr;}'),
('ignore-off','Clear();if(!enabled)return Fail(S_FALSE);','Clear();'),
]
for name,old,new in mutants:
 assert source.count(old)==1,(name,source.count(old));hp=OUT/(name+'.h');hp.write_text(source.replace(old,new));cpp=OUT/(name+'.cpp');cpp.write_text(tests.replace('"'+(HERE/'AcceptedWindow.h').as_posix()+'"','"'+hp.as_posix()+'"'));run(name,cpp,green=False)
print('PASS all11 functional mutants rejected by actual runtime assertions')
