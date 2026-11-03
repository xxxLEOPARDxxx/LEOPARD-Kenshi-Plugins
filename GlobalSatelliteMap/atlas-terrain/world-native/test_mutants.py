from pathlib import Path
import os,re,subprocess,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated/mutants';OUT.mkdir(parents=True,exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
ENV=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
mutants={
 'wrong-group':{'SourceFactory.h':[("!std::strcmp(r.group,group)",'true')]},
 'canonical-alias':{'SourceFactory.h':[("&&!std::strcmp(r.resource,name)",'&&(!std::strcmp(r.resource,name)||!std::strcmp(r.resource,canonicalPath))')]},
 'slot-zero-routing':{'StageAdapter.h':[("return stages_[r.role.slot].Capture(r,out)",'return stages_[0].Capture(r,out)')]},
 # An unsafe adapter might truncate a combined64-role bank to its first SID.
 'truncate-full64-bank':{'StageAdapter.h':[("count!=InputsPerSid",'count<InputsPerSid'),('for(unsigned i=0;i<count;++i){const SourceReceipts::WorkerRequest&r=s[i].request','for(unsigned i=0;i<InputsPerSid;++i){const SourceReceipts::WorkerRequest&r=s[i].request'),('stages_[slot].BeginBank(s,count,scope)','stages_[slot].BeginBank(s,InputsPerSid,scope)')]},
}
for name,edits in mutants.items():
 folder=OUT/name;folder.mkdir(exist_ok=True)
 for file in ['SourceFactory.h','StageAdapter.h','tests.cpp']:
  text=(HERE/file).read_text()
  for old,new in edits.get(file,[]):
   assert text.count(old)==1,(name,file,old,text.count(old));text=text.replace(old,new)
  if file=='SourceFactory.h':
   text=re.sub(r'#include "(\.\./[^\"]+)"',lambda m:'#include "'+(HERE/m[1]).resolve().as_posix()+'"',text)
  (folder/file).write_text(text)
 exe=folder/'test.exe'
 p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/W3',str(folder/'tests.cpp'),'/Fo'+str(folder/'test.obj'),'/Fe'+str(exe),'/link','advapi32.lib'],env=ENV,cwd=HERE,capture_output=True,text=True)
 (folder/'build.log').write_text(p.stdout+p.stderr);assert p.returncode==0,(name,p.stdout,p.stderr)
 p=subprocess.run([str(exe)],capture_output=True,text=True);(folder/'run.log').write_text(p.stdout+p.stderr)
 assert p.returncode==1 and 'FAIL 'in p.stderr,(name,p.returncode,p.stdout,p.stderr)
 print('PASS killed',name,':',p.stderr.strip())
