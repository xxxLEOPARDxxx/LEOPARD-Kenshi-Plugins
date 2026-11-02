#include <Windows.h>
#include <cstdio>
#include "../GsmLocalDiagnostic.h"
static unsigned checks=0,calls=0,resets=0;
static HRESULT response=S_OK;static bool receivedVerified=false;static const void*receivedScene=NULL;
struct Reader {
    void Reset(){++resets;}
    HRESULT Read(bool on,LONG generation,const void*scene,bool verified,const volatile LONG*live,DWORD){
        ++calls;receivedVerified=verified;receivedScene=scene;
        return on&&generation==*live?response:E_ABORT;
    }
};
static void Check(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}}
int main(){try{
    volatile LONG generation=7;GsmLocalDiagnostic::Bridge<Reader>b;const void*scene=reinterpret_cast<const void*>(1);
    Check(b.Tick(false,false,7,scene,&generation,0)==S_FALSE&&calls==0,"OFF does not read native scene/light data");
    Check(b.Tick(true,true,7,scene,&generation,1)==S_OK&&calls==1&&receivedVerified&&receivedScene==scene,"verified current scene is passed unchanged");
    response=E_FAIL;Check(FAILED(b.Tick(true,true,7,scene,&generation,2))&&calls==2,"source refusal returned once");
    Check(b.Tick(true,true,7,scene,&generation,3)==S_FALSE&&calls==2,"same failed scene is latched without repeat native work");
    response=S_OK;scene=reinterpret_cast<const void*>(2);
    Check(b.Tick(true,true,7,scene,&generation,4)==S_OK&&calls==3&&receivedScene==scene,"scene replacement retires old snapshot and allows new source");
    generation=8;Check(b.Tick(true,true,8,scene,&generation,5)==S_OK&&calls==4,"generation replacement retires old snapshot");
    Check(b.Tick(true,false,8,scene,&generation,6)==S_OK&&!receivedVerified,"caller never invents successful source verification");
    response=E_FAIL;Check(FAILED(b.Tick(true,true,8,scene,&generation,7)),"second refusal observed");
    b.Tick(false,false,8,scene,&generation,8);response=S_OK;
    Check(b.Tick(true,true,8,scene,&generation,9)==S_OK,"OFF resets failure and permits explicit retry");
    Check(resets>=5,"reset covers OFF and source identity transitions");
    std::printf("local diagnostic: %u checks PASS\n",checks);return 0;
}catch(...){return 1;}}
