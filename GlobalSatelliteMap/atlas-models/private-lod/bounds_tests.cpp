#ifdef PRIVATE_BLOB_HEADER
#include PRIVATE_BLOB_HEADER
#else
#include "BlobPolicy.h"
#endif
#include <vector>
#include <cstdio>
using namespace PrivateTownLod;
static unsigned checks=0;
static void Check(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}}
static unsigned U16(const std::vector<BYTE>&b,SIZE_T p){unsigned short v;std::memcpy(&v,&b[p],2);return v;}
static unsigned U32(const std::vector<BYTE>&b,SIZE_T p){unsigned v;std::memcpy(&v,&b[p],4);return v;}
// Independent file-chunk walk, not a second copy of the production parser.
static SIZE_T BoundsChunk(const std::vector<BYTE>&b){
 SIZE_T p=2;while(p<b.size()&&b[p]!='\n')++p;++p;
 if(p+7>b.size()||U16(b,p)!=0x3000)return 0;p+=7;
 while(p+6<=b.size()){unsigned kind=U16(b,p),size=U32(b,p+2);if(size<6||size>b.size()-p)return 0;if(kind==0x9000)return size==34?p+6:0;p+=size;}
 return 0;
}
int main(int argc,char**argv){try{
 Check(argc==2,"actual asset directory supplied");
 BlobInfo empty;float zeros[6]={0};Check(!std::memcmp(empty.localBounds,zeros,24),"empty bounds contain only zero values");
 char path[MAX_PATH];_snprintf_s(path,sizeof(path),_TRUNCATE,"%s/distant_*.mesh",argv[1]);WIN32_FIND_DATAA row;HANDLE h=FindFirstFileA(path,&row);Check(h!=INVALID_HANDLE_VALUE,"actual assets exist");unsigned files=0;std::vector<BYTE>sample;
 do{
  _snprintf_s(path,sizeof(path),_TRUNCATE,"%s/%s",argv[1],row.cFileName);FILE*f=NULL;fopen_s(&f,path,"rb");Check(f!=NULL,"open actual mesh");fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);Check(n>0&&n<=long(MaxBytes),"actual fixture stays within input cap");std::vector<BYTE>b(n);Check(fread(&b[0],1,n,f)==SIZE_T(n),"read exact real-file bytes");fclose(f);
  SIZE_T at=BoundsChunk(b);Check(at!=0,"independent real-file bounds chunk");float expected[6];std::memcpy(expected,&b[at],24);BlobInfo info;Check(Parser(&b[0],b.size()).Read(info),"actual production parser admits real fixture");Check(!std::memcmp(info.localBounds,expected,24),"actual native bounds copied exactly");
  if(!std::strcmp(row.cFileName,"distant_2.mesh")){Check(info.vertices==2818&&info.indices==3201&&info.GpuReservation()==323550,"actual Hub payload and reservation unchanged");sample=b;}
  ++files;
 }while(FindNextFileA(h,&row));FindClose(h);Check(files==282&&!sample.empty(),"all282 assets and Hub covered");
 const SIZE_T at=BoundsChunk(sample);BlobInfo info;
 for(unsigned i=0;i<7;++i){std::vector<BYTE>b=sample;unsigned nan=0x7fc00001;std::memcpy(&b[at+i*4],&nan,4);Check(!Parser(&b[0],b.size()).Read(info),"nonfinite native bounds refused");Check(!std::memcmp(info.localBounds,zeros,24),"invalid bounds are not published");}
 for(unsigned axis=0;axis<3;++axis){std::vector<BYTE>b=sample;float lo,hi;std::memcpy(&lo,&b[at+axis*4],4);std::memcpy(&hi,&b[at+(axis+3)*4],4);std::memcpy(&b[at+axis*4],&hi,4);std::memcpy(&b[at+(axis+3)*4],&lo,4);Check(!Parser(&b[0],b.size()).Read(info),"inverted bounds refused");}
 Check(!Parser(NULL,32).Read(info)&&!std::memcmp(info.localBounds,zeros,24),"entry refusal resets copied bounds");
 std::printf("Private native bounds PASS %u checks;282actual files\n",checks);return 0;
 }catch(...){return 1;}}
