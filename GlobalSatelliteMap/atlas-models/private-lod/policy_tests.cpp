#ifndef PRIVATE_BLOB_HEADER
#define PRIVATE_BLOB_HEADER "BlobPolicy.h"
#endif
#include PRIVATE_BLOB_HEADER
#include <cstdio>
#include <vector>
#include <stdexcept>
using namespace PrivateTownLod;
static unsigned checks=0;
static void Check(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}}
struct Ops {
 std::vector<unsigned>calls;unsigned fail;bool same;Ops():fail(0),same(true){}void Call(unsigned n){calls.push_back(n);if(fail==n)throw std::runtime_error("injected cleanup fault");}
 void ClearPacket(){Call(1);}bool Identity(){Call(2);return same;}void Unload(){Call(3);}void DestroyMesh(){Call(4);}void DropLoader(){Call(5);}
};
static void Lifecycle(){Ownership own;own.mesh=true;Ops ops;Check(Retire(ops,own)&&!own.mesh,"owned resource retirement succeeds");const unsigned order[]={1,2,3,4,5};Check(ops.calls.size()==5&&!std::memcmp(&ops.calls[0],order,sizeof(order)),"packet unload destructor loader exact retirement order");
 for(unsigned fail=2;fail<=4;++fail){Ownership f;f.mesh=true;Ops o;o.fail=fail;bool threw=false;try{Retire(o,f);}catch(...){threw=true;}Check(threw&&o.calls.back()==fail,"native cleanup exception stops later destructive steps");Check(o.calls.front()==1,"packet retired before any cleanup failure");}
 Ownership borrowed;borrowed.mesh=true;Ops wrong;wrong.same=false;Check(!Retire(wrong,borrowed)&&wrong.calls.size()==2&&borrowed.mesh,"wrong creator identity never unloads or frees loader");
 Ownership partial;Ops p;Check(Retire(p,partial)&&p.calls.size()==2&&p.calls[0]==1&&p.calls[1]==5,"unconstructed partial storage cleanup skips native calls");
 Gate gate;const void*a=reinterpret_cast<void*>(1),*b=reinterpret_cast<void*>(2);Check(gate.Acquire(a)&&!gate.Acquire(b),"single global live mesh slot");gate.Release(a);Check(gate.Acquire(b),"successful retirement releases slot");gate.Poison();gate.Release(b);Check(!gate.Acquire(a)&&!gate.Acquire(b)&&gate.owner==b,"uncertain native ownership blocks all future loads without growth");}
int main(int argc,char**argv){try{Check(argc==2,"asset directory argument");char pattern[MAX_PATH];_snprintf_s(pattern,sizeof(pattern),_TRUNCATE,"%s/distant_*.mesh",argv[1]);WIN32_FIND_DATAA fd;HANDLE h=FindFirstFileA(pattern,&fd);Check(h!=INVALID_HANDLE_VALUE,"actual native mesh folder");unsigned count=0;SIZE_T disk=0,gpu=0;std::vector<BYTE>sample;
 do{char file[MAX_PATH];_snprintf_s(file,sizeof(file),_TRUNCATE,"%s/%s",argv[1],fd.cFileName);FILE*f=NULL;fopen_s(&f,file,"rb");Check(f!=NULL,"read actual mesh fixture");fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);std::vector<BYTE>b(n);Check(n>0&&fread(&b[0],1,n,f)==SIZE_T(n),"read exact mesh bytes");fclose(f);BlobInfo info;Check(Parser(&b[0],b.size()).Read(info),"all282 actual cached meshes structurally admitted");Check(info.vertices&&info.indices&&info.GpuBytes()<=b.size(),"preflight GPU bound from actual native bytes");disk+=b.size();gpu+=info.GpuBytes();if(sample.empty())sample=b;++count;}while(FindNextFileA(h,&fd));FindClose(h);Check(count==282&&disk==7182137&&gpu==7123782,"exact current282 file and GPU payload inventory");
 const unsigned __int64 cap=192ULL*1024*1024;Check(Budget(cap-337122,337122,cap)&&!Budget(cap-337121,337122,cap)&&!Budget(0,337122,337121),"inclusive candidate GPU reservation plus1 refusal");BlobInfo reserved;Check(Parser(&sample[0],sample.size()).Read(reserved)&&reserved.GpuReservation()==reserved.GpuBytes()*3,"native primary shadow temp payload reservation");BlobInfo info;Check(!Parser(NULL,32).Read(info)&&!Parser(reinterpret_cast<void*>(1),MaxBytes+1).Read(info),"null oversize blob refused before read");std::vector<BYTE>b=sample;b[1]=0;Check(!Parser(&b[0],b.size()).Read(info),"wrong native serializer header refused");b=sample;for(unsigned i=0;i<8;++i){Check(!Parser(&b[0],b.size()-1-i).Read(info),"truncated chunk boundary refused");}b=sample;b.push_back(0);Check(!Parser(&b[0],b.size()).Read(info),"trailing unowned payload refused");
 b=sample;SIZE_T p=2+std::strlen("[MeshSerializer_v1.100]\n");b[p+6]=1;Check(!Parser(&b[0],b.size()).Read(info),"skeleton dependency refused before native import");
 b=sample;unsigned large=0xffffffff;std::memcpy(&b[p+2],&large,4);Check(!Parser(&b[0],b.size()).Read(info),"chunk length overflow refused");Lifecycle();std::printf("Private LOD policy PASS %u checks;282actual meshes\n",checks);return 0;}catch(...){return 1;}}
