import ast,re
from run_tests import HERE,OUT,run
def absolute(text):return re.sub(r'(^\s*#include\s+")([^"]+)(")',lambda m:m[1]+(HERE/m[2]).resolve().as_posix()+m[3],text,flags=re.M)
source=absolute((HERE/'WorldWindowSession.h').read_text());tests=absolute((HERE/'tests.cpp').read_text());prefix=tests.split('int main()')[0]
tree=ast.parse((HERE/'test_adversarial.py').read_text());body=next(ast.literal_eval(n.value)for n in tree.body if isinstance(n,ast.Assign)and any(isinstance(t,ast.Name)and t.id=='body'for t in n.targets))
functional=[
 ('wrong-origin','+147456)/9','+147447)/9',2),
 ('flipped-z','out.z=z%8192;','out.z=8192-512-z%8192;',1),
 ('height-is-normal','view.Plane(plane),view.Bytes(plane)','(plane==2?view.normal:view.Plane(plane)),view.Bytes(plane)',1),
 ('missing-plane','for(unsigned plane=0;plane<3;++plane)','for(unsigned plane=0;plane<2;++plane)',1),
 ('duplicate-poisons','if(t.done[bit/64]&mask)return Refuse(HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS));','',1),
]
adversarial=[
 ('skip-session-world','!SourceReceipts::SameScope(scope_,now)||','',1),
 ('skip-atlas-material','atlasMaterial!=stamp_.material||','',1),
 ('foreign-worker','if(owner_!=GetCurrentThreadId())return E_ACCESSDENIED;','',1),
 ('underreserve-files','sizeof(Session)+16*bytes:0','sizeof(Session)+bytes:0',1),
 ('skip-last-cancel','if(WorldPixels::Cancelled(cancel))return Poison(HRESULT_FROM_WIN32(ERROR_CANCELLED));\n   t.done','t.done',1),
 ('keep-poison-files','Release();return error_;','return error_;',1),
 ('ignore-io-result','if(!t.writer.WritePlane(plane,destination,view.Plane(plane),view.Bytes(plane),view.Pitch(plane),StopContext::Check,&stop))','if((t.writer.WritePlane(plane,destination,view.Plane(plane),view.Bytes(plane),view.Pitch(plane),StopContext::Check,&stop),false))',1),
]
for group in (functional,adversarial):
 for name,old,new,count in group:
  assert source.count(old)==count,(name,source.count(old));hp=OUT/(name+'.h');hp.write_text(source.replace(old,new));cpp=OUT/(name+'.cpp');cpp.write_text((tests if group is functional else prefix+body).replace('"'+(HERE/'WorldWindowSession.h').as_posix()+'"','"'+hp.as_posix()+'"'));run(name,cpp,green=False)
print('PASS all12 compiled session mutants rejected by runtime assertions')
