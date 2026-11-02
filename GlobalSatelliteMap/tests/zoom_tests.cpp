#include "../GsmZoomPolicy.h"
#include <cstdio>
unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){printf("FAIL line%u %s\n",__LINE__,#x);return 1;}}while(0)
int main(){GsmZoom::Lease lease;GsmZoom::Action a;
 CHECK(lease.Plan(false,4,.12f,1,2,2,16384,16384,a)==GsmZoom::Unchanged);
 CHECK(lease.Plan(true,1,.12f,1,2,2,16384,16384,a)==GsmZoom::Unchanged);
 CHECK(lease.Plan(true,4,.12f,1,2,2,16384,16384,a)==GsmZoom::Apply&&a.maximum==8&&!a.clamp);
 for(unsigned i=0;i<100;++i)CHECK(lease.Plan(true,4,.12f,1,8,2,16384,16384,a)==GsmZoom::Unchanged);
 // Ordinary world clear resets current to minimum, not maximum. Never compound.
 CHECK(lease.Plan(true,4,.12f,1,8,.12f,983,983,a)==GsmZoom::Unchanged);
 CHECK(lease.Plan(true,4,.12f,1,8,.12f,65536,65536,a)==GsmZoom::Unchanged);
 CHECK(lease.Plan(true,2,.12f,1,8,8,65536,65536,a)==GsmZoom::Apply&&a.maximum==4&&a.clamp);
 CHECK(lease.Plan(false,2,.12f,1,4,4,32768,32768,a)==GsmZoom::Apply&&a.maximum==2&&a.clamp);
 CHECK(lease.Plan(false,2,.12f,1,2,2,16384,16384,a)==GsmZoom::Unchanged);
 CHECK(lease.Plan(true,8,.12f,1,2,2,16384,16384,a)==GsmZoom::Apply&&a.maximum==16);
 // Leave another plugin's later maximum untouched; do not fight every frame.
 CHECK(lease.Plan(true,8,.12f,1,6,2,16384,16384,a)==GsmZoom::ForeignChange);
 CHECK(lease.Plan(true,8,.12f,1,6,2,16384,16384,a)==GsmZoom::Unchanged);
 CHECK(lease.Plan(false,8,.12f,1,6,2,16384,16384,a)==GsmZoom::Unchanged);
 lease.Reset();CHECK(lease.Plan(true,2,.12f,1,6,2,16384,16384,a)==GsmZoom::Apply&&a.maximum==4);
 CHECK(lease.Plan(false,2,.12f,1,4,2,16384,16384,a)==GsmZoom::Apply&&a.maximum==6);
 for(int factor=0;factor<=9;factor+=9){lease.Reset();CHECK(lease.Plan(true,factor,.12f,1,2,2,16384,16384,a)==GsmZoom::Refused);}
 lease.Reset();CHECK(lease.Plan(true,8,.12f,1,2,2,1000000,1000000,a)==GsmZoom::Refused);
 CHECK(lease.Plan(true,2,0,1,2,2,16384,16384,a)==GsmZoom::Refused);
 CHECK(lease.Plan(true,2,.12f,0,2,2,16384,16384,a)==GsmZoom::Refused);
 CHECK(lease.Plan(true,2,.12f,1,2,0,16384,16384,a)==GsmZoom::Refused);
 CHECK(lease.Plan(true,2,.12f,1,2,2,0,16384,a)==GsmZoom::Refused);
 CHECK(lease.Plan(true,2,.12f,1,2,2,16384,0,a)==GsmZoom::Refused);
 // Matching maximum was not written by this lease; OFF must not undo it.
 lease.Reset();CHECK(lease.Plan(true,4,.12f,1,8,2,16384,16384,a)==GsmZoom::Unchanged);
 CHECK(lease.Plan(false,4,.12f,1,8,2,16384,16384,a)==GsmZoom::Unchanged);
 for(unsigned f=1;f<=8;++f){lease.Reset();int r=lease.Plan(true,int(f),.12f,1,2,2,16384,16384,a);CHECK(f==1?r==GsmZoom::Unchanged:r==GsmZoom::Apply&&a.maximum==2*f);}
 printf("Maximum zoom policy %u checks PASS\n",checks);return 0;}
