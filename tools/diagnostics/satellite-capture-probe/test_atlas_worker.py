"""Extract actual file worker/Pixels; real GST2 writer, recording I/O, no Ogre/game."""
from pathlib import Path
import hashlib
import json
import os
import subprocess

HERE=Path(__file__).resolve().parent
OUT=HERE/'build/atlas-worker'
OUT.mkdir(parents=True,exist_ok=True)
source=(HERE/'AtlasRun.h').read_text()

def extract(marker):
    start=source.index(marker);begin=source.index('{',start);depth=0
    for at in range(begin,len(source)):
        if source[at]=='{':depth+=1
        elif source[at]=='}':
            depth-=1
            if depth==0:return source[start:at+1]
    raise AssertionError('unbalanced source '+marker)

pixels=extract('struct Pixels {')
worker=extract('static DWORD WINAPI WritePixels(')
assert all(s not in worker for s in ('Ogre::','Log(','CreateThread','au->','ou->'))
prefix=r'''
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>
#include <stdexcept>
#include <new>
#include "AtlasCapturePolicy.h"
#include "AtlasPixels.h"
#include "Gst2Writer.h"
static bool denyNew=false;
void* operator new(size_t bytes) {if(denyNew)throw std::bad_alloc();void* p=std::malloc(bytes);if(!p)throw std::bad_alloc();return p;}
void operator delete(void* p){std::free(p);}
void* operator new[](size_t bytes){return ::operator new(bytes);}
void operator delete[](void* p){::operator delete(p);}
static unsigned checks=0;
static void Check(bool value,const char* why){++checks;if(!value){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
static unsigned Packed(unsigned i){return 0x5ac37e19u^i;}
static float SourceHeight(unsigned i){return 500.0f+((i%2048+i/2048)%127)*25.0f;}
static unsigned __int64 Get64(const unsigned char* p){unsigned __int64 n=0;for(unsigned j=0;j<8;++j)n|=static_cast<unsigned __int64>(p[j])<<(8*j);return n;}
struct Recording : Gst2::RandomFile {
 unsigned char header[1024];unsigned __int64 size,dataWrites,byPlane[3];int failPlane;unsigned blockX,blockZ;bool mismatch;
 Recording():size(0),dataWrites(0),failPlane(-1),blockX(6144),blockZ(4096),mismatch(false){std::memset(header,0,sizeof(header));std::memset(byPlane,0,sizeof(byPlane));}
 bool Resize(unsigned __int64 n){size=n;return true;}
 bool Flush(){return true;}
 bool WriteAt(unsigned __int64 off,const void* data,DWORD bytes){
  if(off>size||bytes>size-off){mismatch=true;return false;}
  if(off<1024){if(off+bytes>1024){mismatch=true;return false;}std::memcpy(header+size_t(off),data,bytes);return true;}
  int plane=-1;for(unsigned p=0;p<3;++p)if(off>=Gst2::PlaneOffset(0,p)&&off<Gst2::PlaneOffset(0,p)+Gst2::PlaneBytes(0,p))plane=p;
  if(plane<0){mismatch=true;return false;}++dataWrites;++byPlane[plane];if(plane==failPlane)return false;
  const unsigned stride=Gst2::PlaneStride(plane);unsigned __int64 texel=(off-Gst2::PlaneOffset(0,plane))/stride;
  unsigned row=unsigned(texel/8192),column=unsigned(texel%8192);
  if(column!=blockX||row<blockZ||row>=blockZ+2048||bytes!=Gst2::BlockPitch(plane)){mismatch=true;return false;}
  unsigned sourceRow=row-blockZ;
  for(unsigned x=0;x<2048;++x){unsigned i=sourceRow*2048+x;
   if(plane==0){const unsigned* v=static_cast<const unsigned*>(data);if(v[x*2]!=Packed(i)||v[x*2+1]!=0)mismatch=true;}
   if(plane==1){const unsigned* v=static_cast<const unsigned*>(data);if(v[x]!=0xff563412u)mismatch=true;}
   if(plane==2){const float* v=static_cast<const float*>(data);if(!_finite(v[x])||std::fabs(v[x]-SourceHeight(i))>.02f)mismatch=true;}
  }
  return !mismatch;
 }
};
struct ActualWorker {
'''
suffix=r'''
};
static void Fill(ActualWorker::Pixels& p,Gst2::Writer& writer){
 const unsigned count=2048u*2048u;p.writer=&writer;Check(AtlasCapture::Select(11,p.block),"actual bounded block selection");
 p.camera.width=18432;p.camera.height=18432;p.camera.farClip=120000;p.camera.y=16000;p.camera.rightY=0;p.camera.upY=0;p.camera.backY=1;
 p.packed.resize(count);p.normal.resize(count);p.depth.resize(count);
 for(unsigned z=0;z<2048;++z)for(unsigned x=0;x<2048;++x){unsigned i=z*2048+x;p.packed[i]=Packed(i);p.normal[i]=0x7f123456u;
  float lx=((x+.5f)/2048-.5f)*18432,ly=((z+.5f)/2048-.5f)*-18432,dy=16000-SourceHeight(i);
  p.depth[i]=std::sqrt(lx*lx+ly*ly+dy*dy)/120000;
 }
}
static void Run(int failPlane,int invalid,bool oom){
 const unsigned count=2048u*2048u;Recording io;Gst2::Writer writer;Check(writer.Begin(io,0,0,true),"actual raw GST2 Begin");io.failPlane=failPlane;
 ActualWorker::Pixels p;Fill(p,writer);
 if(invalid==1){unsigned nan=0x7fc00000u;std::memcpy(&p.depth[count/2],&nan,4);}
 if(invalid==2)p.depth[count/2]=0;
 if(invalid==3)p.depth.pop_back();
 denyNew=oom;DWORD returned=ActualWorker::WritePixels(&p);denyNew=false;
 Check(returned==0,"worker always returns through WINAPI boundary");Check(!io.mismatch,"actual packed/padding/normal/height/row bytes exact");
 Check(std::strlen(p.error)<sizeof(p.error),"bounded fixed error buffer");
 if(invalid){Check(!p.ok&&p.error[0],"invalid/short input refused");Check(io.dataWrites==0&&writer.Mask()==0,"no plane I/O or ledger on invalid geometry");
  if(invalid!=3){Check(p.valid==count-1&&p.invalid==1,"all radial samples prevalidated and accounted");}
  Check(!writer.Poisoned(),"invalid source did not touch writer");Check(!p.packed.empty(),"unwritten source storage preserved");return;}
 Check(p.valid==count&&p.invalid==0,"complete geometry checked before I/O");
 if(oom){Check(!p.ok&&p.error[0],"bad_alloc contained without allocating error string");Check(io.dataWrites==0&&writer.Mask()==0,"failed32MiB allocation earns no bits");return;}
 unsigned __int64 expected=0;
 for(unsigned plane=0;plane<3;++plane)if(failPlane<0||int(plane)<failPlane)expected|=1ULL<<(plane*16+11);
 Check(writer.Mask()==expected&&Get64(io.header+48)==expected,"real writer persists only successful plane block bits");
 if(failPlane>=0){Check(!p.ok&&p.error[0]&&writer.Poisoned(),"plane failure caught and writer poisoned");Check(io.byPlane[failPlane]==1,"injected first row failure stops plane");
  for(unsigned plane=unsigned(failPlane+1);plane<3;++plane)Check(io.byPlane[plane]==0,"later planes never touched after failure");
 }else{Check(p.ok&&!p.error[0],"complete worker success");Check(io.dataWrites==6144,"exactly three2048row writes");Check(!writer.BaseComplete(),"one capture block is never a complete primary");}
 Check(!p.packed.empty()==(failPlane==0),"raw copy released only after successful material write");
}
int main(){
 Run(-1,0,false);Run(0,0,false);Run(1,0,false);Run(2,0,false);
 Run(-1,1,false);Run(-1,2,false);Run(-1,3,false);Run(-1,0,true);
 std::printf("PASS %u extracted production AtlasRun::WritePixels checks;8 full-size cases; no Ogre/game calls\n",checks);return 0;
}
'''
generated=OUT/'actual_atlas_worker.cpp'
generated.write_text(prefix+pixels+';\n'+worker+suffix)
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
cache=HERE.parents[2]/'GlobalSatelliteMap/atlas-cache'
env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include;{HERE};{cache}',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
exe=OUT/'actual_atlas_worker.exe'
subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/W4',str(generated),f'/Fo{OUT}/actual_atlas_worker.obj',f'/Fe{exe}'],env=env,check=True)
run=subprocess.run([str(exe)],capture_output=True,text=True,check=True)
print(run.stdout.strip())
report=dict(workerSourceSha256=hashlib.sha256((HERE/'AtlasRun.h').read_bytes()).hexdigest(),extractedWorkerSha256=hashlib.sha256(worker.encode()).hexdigest(),pixelsSourceSha256=hashlib.sha256((HERE/'AtlasPixels.h').read_bytes()).hexdigest(),cacheWriterSha256=hashlib.sha256((cache/'Gst2Writer.h').read_bytes()).hexdigest(),result=run.stdout.strip(),limits='Recording RandomFile I/O, actual writer+worker+2048²payload; no real atlas/native GPU/process action')
(OUT/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
