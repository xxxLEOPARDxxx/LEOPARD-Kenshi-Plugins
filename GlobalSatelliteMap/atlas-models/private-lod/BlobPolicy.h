#pragma once
#include <Windows.h>
#include <cstring>
#include <float.h>
namespace PrivateTownLod {
const SIZE_T MaxBytes=512*1024;
inline bool Budget(unsigned __int64 other,SIZE_T candidate,unsigned __int64 cap){return candidate<=cap&&other<=cap-candidate;}
struct BlobInfo {unsigned vertices,indices,vertexBytes,indexBytes,signature;float localBounds[6];BlobInfo(){std::memset(this,0,sizeof(*this));}SIZE_T GpuBytes()const{return SIZE_T(vertexBytes)+indexBytes;}SIZE_T GpuReservation()const{return GpuBytes()*3;}};
struct Chunk {unsigned kind;SIZE_T body,end;};
class Parser {
 const BYTE*b_;SIZE_T size_;bool U16(SIZE_T p,unsigned&v)const{if(p>size_||size_-p<2)return false;unsigned short n;std::memcpy(&n,b_+p,2);v=n;return true;}bool U32(SIZE_T p,unsigned&v)const{if(p>size_||size_-p<4)return false;std::memcpy(&v,b_+p,4);return true;}
 bool Next(SIZE_T&p,SIZE_T end,Chunk&c)const{unsigned n;if(end>size_||p>end||end-p<6||!U16(p,c.kind)||!U32(p+2,n)||n<6||n>end-p)return false;c.body=p+6;c.end=p+n;p+=n;return true;}
 bool Geometry(const Chunk&g,BlobInfo&out)const{
  if(!U32(g.body,out.vertices)||!out.vertices||out.vertices>65536)return false;SIZE_T at=g.body+4;unsigned declaration=0,buffer=0,mask=0;
  while(at<g.end){Chunk c;if(!Next(at,g.end,c))return false;
   if(c.kind==0x5100){if(declaration++)return false;SIZE_T p=c.body;while(p<c.end){Chunk e;if(!Next(p,c.end,e)||e.kind!=0x5110||e.end-e.body!=10)return false;unsigned source,type,semantic,offset,index;if(!U16(e.body,source)||!U16(e.body+2,type)||!U16(e.body+4,semantic)||!U16(e.body+6,offset)||!U16(e.body+8,index)||source||index)return false;unsigned bit=0;if(semantic==1&&type==2&&offset==0)bit=1;else if(semantic==4&&type==2&&offset==12)bit=2;else if(semantic==5&&(type==4||type==10||type==11)&&offset==32)bit=4;else if(semantic==7&&type==1&&offset==24)bit=8;if(!bit||(mask&bit))return false;mask|=bit;}}
   else if(c.kind==0x5200){if(buffer++)return false;unsigned slot,stride;if(!U16(c.body,slot)||!U16(c.body+2,stride)||slot||stride!=36)return false;SIZE_T p=c.body+4;Chunk data;if(!Next(p,c.end,data)||p!=c.end||data.kind!=0x5210||data.end-data.body!=SIZE_T(out.vertices)*36)return false;out.vertexBytes=unsigned(data.end-data.body);}
   else return false;
  }return declaration==1&&buffer==1&&mask==15;
 }
 bool Submesh(const Chunk&s,BlobInfo&out)const{
  SIZE_T p=s.body;if(p>=s.end||b_[p++]!='\n'||p>=s.end||b_[p++]!=0||!U32(p,out.indices)||!out.indices||out.indices>196608||out.indices%3)return false;p+=4;if(p>=s.end||b_[p]>1)return false;unsigned width=b_[p++]?4:2;SIZE_T bytes=SIZE_T(out.indices)*width;if(bytes>s.end-p)return false;out.indexBytes=unsigned(bytes);p+=bytes;unsigned geometry=0,operation=0;
  while(p<s.end){Chunk c;if(!Next(p,s.end,c))return false;if(c.kind==0x5000){if(geometry++||!Geometry(c,out))return false;}else if(c.kind==0x4010){unsigned op;if(operation++||c.end-c.body!=2||!U16(c.body,op)||op!=4)return false;}else return false;}return geometry==1;
 }
public:Parser(const void*b,SIZE_T n):b_(static_cast<const BYTE*>(b)),size_(n){}
 bool Read(BlobInfo&out)const{
  out=BlobInfo();const char version[]="[MeshSerializer_v1.100]\n";unsigned header;if(!b_||size_>MaxBytes||size_<2+sizeof(version)-1||!U16(0,header)||header!=0x1000||std::memcmp(b_+2,version,sizeof(version)-1))return false;
  SIZE_T p=2+sizeof(version)-1;Chunk mesh;if(!Next(p,size_,mesh)||p!=size_||mesh.kind!=0x3000||mesh.body>=mesh.end||b_[mesh.body])return false;p=mesh.body+1;unsigned sub=0,bounds=0,names=0;
  while(p<mesh.end){Chunk c;if(!Next(p,mesh.end,c))return false;if(c.kind==0x4000){if(sub++||!Submesh(c,out))return false;}else if(c.kind==0x9000){if(bounds++||c.end-c.body!=28)return false;float f[7];std::memcpy(f,b_+c.body,28);for(unsigned i=0;i<7;++i)if(!_finite(f[i]))return false;if(f[0]>f[3]||f[1]>f[4]||f[2]>f[5]||f[6]<0)return false;std::memcpy(out.localBounds,f,24);}
   else if(c.kind==0xa000){if(names++)return false;SIZE_T at=c.body;Chunk name;unsigned index;if(!Next(at,c.end,name)||at!=c.end||name.kind!=0xa100||!U16(name.body,index)||index)return false;SIZE_T n=name.end-name.body;if(n<4||n>11||b_[name.end-1]!='\n')return false;unsigned value=0;for(SIZE_T q=name.body+2;q<name.end-1;++q){unsigned ch=b_[q],digit=ch>='0'&&ch<='9'?ch-'0':ch>='a'&&ch<='f'?ch-'a'+10:ch>='A'&&ch<='F'?ch-'A'+10:16;if(digit>15)return false;value=(value<<4)|digit;}out.signature=value;}
   else return false;
  }return sub==1&&bounds==1&&names==1&&out.GpuBytes()<=MaxBytes;
 }
};
struct Ownership {bool mesh;Ownership():mesh(false){}};
// Exact order is shared by the native owner and exception/partial fixtures.
template<class Ops>bool Retire(Ops&ops,Ownership&owned){ops.ClearPacket();if(owned.mesh){if(!ops.Identity())return false;ops.Unload();ops.DestroyMesh();owned.mesh=false;}ops.DropLoader();return true;}
struct Gate {const void*owner;bool poisoned;Gate():owner(NULL),poisoned(false){}bool Acquire(const void*p){if(poisoned||(owner&&owner!=p))return false;owner=p;return true;}void Release(const void*p){if(owner==p&&!poisoned)owner=NULL;}void Poison(){poisoned=true;}};
}
