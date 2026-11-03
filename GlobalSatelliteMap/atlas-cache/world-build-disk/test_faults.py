from pathlib import Path
import re,sys
import run_tests as r
HERE=r.HERE;OUT=r.OUT
def absolute(text,root):return re.sub(r'^\s*#include\s+"([^"\n]+)"',lambda m:'#include "'+(root/m[1]).resolve().as_posix()+'"',text,flags=re.M)
source=absolute((HERE/'WorldBuildDisk.h').read_text(),HERE)
prefix='namespace DiskReview{void Before(const wchar_t*);void After(const wchar_t*,const wchar_t*,HANDLE);}\n'
race=source.replace('namespace WorldBuildDisk{',prefix+'namespace WorldBuildDisk{').replace('HANDLE handle=guards_.CreateRelativeFile(leaf);','DiskReview::Before(config_.directory);HANDLE handle=guards_.CreateRelativeFile(leaf);DiskReview::After(config_.directory,leaf,handle);')
def variant(name,header,useRace=False,green=True,reason=None):
 h=OUT/(name+'.h');h.write_text(header);base=OUT/(name+'-fixture.cpp');base.write_text(absolute((HERE/'tests.cpp').read_text(),HERE).replace((HERE/'WorldBuildDisk.h').as_posix(),h.as_posix()));cpp=base
 if useRace:cpp=OUT/(name+'.cpp');cpp.write_text(absolute((HERE/'race_tests.cpp').read_text(),HERE).replace((HERE/'tests.cpp').as_posix(),base.as_posix()))
 exe=r.build(name,cpp);r.execute(name,exe,green,reason)
variant('retarget-race',race,True)
bad=race.replace('HANDLE handle=guards_.CreateRelativeFile(leaf);','HANDLE handle=::CreateFileW((std::wstring(config_.directory)+L"\\\\"+leaf).c_str(),GENERIC_READ|GENERIC_WRITE,0,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);')
variant('path-open-loss',bad,True,False,'file exists in original source directory after restoring namespace')
shortPrefix='namespace DiskWriteReview{static BOOL Write(HANDLE h,const void*p,DWORD n,DWORD*done,OVERLAPPED*o){return ::WriteFile(h,p,n>2?2:n,done,o);}}\n'
short=source.replace('namespace WorldBuildDisk{',shortPrefix+'namespace WorldBuildDisk{').replace('WriteFile(file_,data,n,&done,NULL)','DiskWriteReview::Write(file_,data,n,&done,NULL)')
variant('short-write',short)
variant('short-loop-loss',short.replace('while(n){DWORD done=0;','if(n){DWORD done=0;'),False,False,'positioned write persisted exact bytes')
variant('tile-bounds-loss',source.replace('x>=4||z>=4','x>4||z>4'),False,False,'all out-of-range tile indices refuse')
