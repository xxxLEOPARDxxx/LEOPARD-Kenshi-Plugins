from pathlib import Path
import os,subprocess,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=os.environ.copy();base=str(VC/'include')+';'+str(SDK/'Include')+';'+str(HERE);env['INCLUDE']=base;env['LIB']=str(VC/'lib/amd64')+';'+str(SDK/'Lib/x64')
native=(HERE/'NativePrivateLod.h').read_text();policy=(HERE/'BlobPolicy.h').read_text()
cases=[
 ('omit-staging-reservation','policy','return GpuBytes()*3;','return GpuBytes();','native primary shadow temp payload reservation'),
 ('premature-loader-free','policy','ops.ClearPacket();if(owned.mesh)','ops.DropLoader();ops.ClearPacket();if(owned.mesh)','packet unload destructor loader exact retirement order'),
 ('unload-wrong-owner','policy','if(!ops.Identity())return false;','if(!ops.Identity()){}','wrong creator identity never unloads or frees loader'),
 ('unbounded-after-fault','policy','if(poisoned||(owner&&owner!=p))','if(owner&&owner!=p)','uncertain native ownership blocks all future loads without growth'),
 ('ignore-payload-budget','policy','return candidate<=cap&&other<=cap-candidate;','return true;','inclusive candidate GPU reservation plus1 refusal'),
 ('wrong-native-layout','policy','semantic==7&&type==1&&offset==24','semantic==7&&type==1&&offset==28','all282 actual cached meshes structurally admitted'),
 ('borrowed-address','native','mesh_!=reinterpret_cast<Ogre::Mesh*>(&storage_)||','','borrowed original address cannot be adopted or destroyed'),
 ('wrong-creator','native','reinterpret_cast<Creator>(f_[FCreator])(mesh_)==NULL','true','normal native load has no shared-manager creator'),
 ('wrong-constructor-arguments','native','mesh_,NULL,name_,0,group_,true,&loader_','mesh_,reinterpret_cast<void*>(1),name_,0,group_,true,&loader_','explicit own-storage NULL-creator constructor arguments'),
 ('free-input-bytes','native','bytes_.size(),false,true','bytes_.size(),true,true','native stream wraps exact owned read-only bytes without freeing'),
 ('own-stack-stream','native','Ogre::SPFM_NONE','Ogre::SPFM_DELETE','stream smart pointer never owns stack stream'),
]
def call(args,name,allow=False):
 p=subprocess.run([str(a) for a in args],env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True);(OUT/(name+'.log')).write_text(p.stdout)
 if p.returncode and not allow:raise RuntimeError(name+': '+p.stdout[-1300:])
 return p
try:
 for name,kind,old,new,reason in cases:
  text=policy if kind=='policy' else native;assert text.count(old)==1,(name,text.count(old));text=text.replace(old,new);path=OUT/(name+'.h');path.write_text(text)
  compiled_native=text if kind=='native' else native.replace('"BlobPolicy.h"','"'+path.as_posix()+'"')
  compiled_native=compiled_native.replace('"../NativeModel.h"','"'+(HERE.parent/'NativeModel.h').as_posix()+'"').replace('"_generated/PrivateProfile.h"','"'+(OUT/'PrivateProfile.h').as_posix()+'"').replace('"BlobPolicy.h"','"'+(HERE/'BlobPolicy.h').as_posix()+'"');adapter=OUT/(name+'-adapter.h');adapter.write_text(compiled_native)
  env['INCLUDE']=base+';D:/DEV/Kenshi/KenshiLib-src/Include;D:/DEV/Kenshi/KenshiLib-src/Include/ogre;D:/DEV/Kenshi/deps/boost_1_60_0;D:/DEV/Kenshi/shared'
  call([VC/'bin/amd64/cl.exe','/nologo','/c','/EHsc','/MD','/O2','/W1','/DUNICODE','/D_UNICODE','/DPRIVATE_LOD_NATIVE_HEADER="'+adapter.as_posix()+'"','/Fo'+str(OUT/(name+'-adapter.obj')),HERE/'compile_adapter.cpp'],name+'-adapter-build')
  env['INCLUDE']=base
  if kind=='native':
   call([sys.executable,HERE/'extract_control.py','--source',path],name+'-extract');source=HERE/'control_tests.cpp';defines=[];args=[]
  else:source=HERE/'policy_tests.cpp';defines=['/DPRIVATE_BLOB_HEADER="'+path.as_posix()+'"'];args=['D:/SteamLibrary/steamapps/common/Kenshi/data/meshes/distant']
  exe=OUT/(name+'.exe');call([VC/'bin/amd64/cl.exe','/nologo','/EHsc','/MD','/O2','/W1',*defines,'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),source],name+'-fixture-build');p=call([exe,*args],name+'-run',True)
  if not p.returncode or 'FAIL '+reason not in p.stdout:raise RuntimeError('survived or unfair '+name+': '+p.stdout[-1000:])
  print('Expected RED:',name,'->',reason)
finally:call([sys.executable,HERE/'extract_control.py'],'restored-control-extraction')
print('PASS eleven full-adapter-compiled functional private LOD mutants rejected')
