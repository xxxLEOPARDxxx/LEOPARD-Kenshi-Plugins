"""Compile and execute sensitive mutations of the actual caller/header bodies."""
from pathlib import Path
import os,subprocess,sys
HERE=Path(__file__).resolve().parent;MODULE=HERE.parent;OUT=MODULE/'build/cached-town-state-tests'
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=os.environ.copy();env['INCLUDE']=f'{VC}/include;{SDK}/Include;{MODULE}';env['LIB']=f'{VC}/lib/amd64;{SDK}/Lib/x64'
source=(MODULE/'GsmCachedTownState.h').read_text()
controls=[
 ('late-native-destroy','~ExplicitOwner(){}','~ExplicitOwner(){Clear();}','late owner destructor makes zero native cleanup calls'),
 ('eager-owner','ExplicitOwner():value_(NULL)','ExplicitOwner():value_(new T)','OFF owner allocates and calls nothing'),
 ('lost-quarantine','if(!value_->Clear())return false;','if(!value_->Clear()){delete value_;value_=NULL;return false;}','uncertain cleanup retains stable owner and native graph'),
 ('unlaunched','if(!pending_||!launched_||!Same(request,selected_))return E_ABORT;','if(!pending_||!Same(request,selected_))return E_ABORT;','unlaunched completion refused'),
 ('wrong-camera','double(camera[0])+origin[0]','double(camera[0])-origin[0]','nearest uses raw camera plus proven origin'),
 ('unresolved-y','storedXYZ[1]<=1','storedXYZ[1]<=0','unresolved Y and proxy excluded before queries'),
 ('unbounded-radius','d>8192.0*8192.0','false','outside8192 excluded before queries'),
 ('five-checks','&&checks<ChecksPerSelect','', 'active validation shares four-check budget with candidates'),
 ('wrong-metadata','||std::memcmp(&parsed,&result.info,sizeof(parsed))','','worker metadata must match reparsed bytes'),
 ('ignore-budget','if(!PrivateTownLod::Budget(otherOwnedGpu,parsed.GpuReservation(),AtlasView::GpuCap))','if(false)','inclusive GPU bound plus1 refuses before import'),
 ('no-post-source','||Inspect(request)!=S_OK','','proxy appearance during import clears new source'),
 ('stale-valid','&&Current(context_)&&ops_.Valid(generation)','&&ops_.Valid(generation)','generation invalidates display immediately while owner remains charged'),
 ('quarantine-retry','quarantined_=true;','quarantined_=false;','quarantine refuses new native work'),
 ('drop-old','if(result.state!=GsmCachedTown::Ready){','if(result.state!=GsmCachedTown::Ready){Retire();','unavailable replacement keeps ready old slot'),
 ('wrong-world','double(request.storedXYZ[i])-origin[i]','double(request.storedXYZ[i])+origin[i]','identity pose uses absolute minus origin'),
 ('no-pre-source','HRESULT inspect=Inspect(request);if(inspect!=S_OK){if(FAILED(inspect))Retire();return inspect;}','HRESULT inspect=S_OK;','proxy appearing before adoption prevents duplicate model'),
 ('memo-growth','if(!Room()){Error("cached town attempt memo full");return S_FALSE;}','','64 memo limit refuses growth and repeated queries'),
]
native=[
 ('native-king','&&c.king==shou','', 'wrong SharedKing prevents queries'),
 ('native-proxy','if(NativeLoadedCensus::Pointer(p+0x4f8)||std::memcmp(p+0x48,r.storedXYZ,12))','if(std::memcmp(p+0x48,r.storedXYZ,12))','native proxy blocks duplicate fallback'),
 ('native-empty','if(!count)return S_FALSE;','','empty registry cannot count as loaded'),
 ('native-limit','if(count>NativeLoadedCensus::EntryLimit)','if(false)','4097 registry limit refused'),
 ('native-origin','||std::memcmp(origin,c.origin,12)','','current origin must match proven source frame'),
 ('native-post-world','return ContextCurrent(c)&&','return true&&','post-getter source and world reread rejects mutation'),
 ('native-post-size','&&m->buildingEntities.size()==count','','post-getter source and world reread rejects mutation'),
 ('native-root-basis','if(rootWorld[i]!=expected)return E_INVALIDARG;','','nonidentity root basis refused before pose shortcut'),
]
for label,needle,replacement,reason in controls+native:
 assert source.count(needle)==(2 if label=='unresolved-y' else 1),(label,source.count(needle))
 header=OUT/(label+'.h');header.write_text(source.replace(needle,replacement))
 if (label,needle,replacement,reason) in native:
  r=subprocess.run([sys.executable,str(HERE/'cached_town_native_extract_tests.py'),'--header',str(header),'--label',label],capture_output=True,text=True)
 else:
  exe=OUT/(label+'.exe');cmd=[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/W1','/DGSM_CACHED_TOWN_HEADER="'+header.as_posix()+'"',str(HERE/'cached_town_state_tests.cpp'),'/Fo'+str(OUT/(label+'.obj')),'/Fe'+str(exe),'/link','/STACK:8388608']
  c=subprocess.run(cmd,env=env,capture_output=True,text=True);(OUT/(label+'-build.log')).write_text(c.stdout+c.stderr)
  if c.returncode:raise RuntimeError(label+' compile failed: '+c.stdout+c.stderr)
  r=subprocess.run([str(exe),'D:/SteamLibrary/steamapps/common/Kenshi/data/meshes/distant/distant_2.mesh'],capture_output=True,text=True)
 (OUT/(label+'-run.log')).write_text(r.stdout+r.stderr)
 assert r.returncode and 'FAIL '+reason in r.stdout+r.stderr,(label,r.returncode,r.stdout,r.stderr)
 print('Expected RED:',label,'->',reason)
print('PASS %d compiled cached-town State/native inspector mutants rejected'%(len(controls)+len(native)))
