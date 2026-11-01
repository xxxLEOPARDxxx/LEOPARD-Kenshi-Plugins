#include "NativeWorkerHandle.h"
#include <iostream>
#include <cstdlib>
unsigned checks=0;
void Check(bool ok,const char *name){++checks;if(!ok){std::cerr<<name<<'\n';std::exit(1);}}
DWORD WINAPI WaitForEvent(void *event){WaitForSingleObject(static_cast<HANDLE>(event),INFINITE);return 0;}
int main()
{
    DWORD before=0,after=0;GetProcessHandleCount(GetCurrentProcess(),&before);
    {
        NativeWorkerHandle worker;bool closed=false;volatile BYTE canary[3]={0xA5,1,0x5A};
        Check(!worker.Adopt(NULL,1),"null worker refused");
        Check(!worker.Complete(closed),"no owner completion refused");
        HANDLE event=CreateEvent(NULL,TRUE,FALSE,NULL);DWORD tid=0;
        HANDLE thread=CreateThread(NULL,0,WaitForEvent,event,0,&tid);
        Check(thread&&event,"isolated test worker created");
        Check(!worker.Adopt(event,tid),"non-thread object refused");
        Check(!worker.Adopt(thread,tid+1),"wrong thread identity refused");
        Check(worker.Adopt(thread,tid),"own worker adopted");
        Check(worker.Adopt(thread,tid),"repeat adoption allocates no duplicate");
        Check(!worker.StopAndJoin(&canary[1],0)&&canary[1]==0,"running thread cannot be freed on polling flag alone");
        Check(canary[0]==0xA5&&canary[2]==0x5A,"stop signal changes only the owned request byte");
        Check(!worker.Complete(closed),"completion before kernel join refused");
        SetEvent(event);
        Check(worker.StopAndJoin(&canary[1],2000),"kernel join waits for actual termination");
        Check(worker.Complete(closed)&&closed,"native omitted handle close repaired once");
        DWORD flags=0;Check(!GetHandleInformation(thread,&flags),"native worker handle actually closed");
        Check(!worker.Complete(closed),"double completion refused");
        CloseHandle(event);
    }
    {
        NativeWorkerHandle worker;bool closed=false;volatile BYTE flag=1;
        HANDLE event=CreateEvent(NULL,TRUE,FALSE,NULL);DWORD tid=0;
        HANDLE thread=CreateThread(NULL,0,WaitForEvent,event,0,&tid);
        Check(worker.Adopt(thread,tid),"second worker adopted");
        SetEvent(event);Check(worker.StopAndJoin(&flag,2000),"second worker joined");
        CloseHandle(thread);
        HANDLE replacement=CreateEvent(NULL,TRUE,FALSE,NULL);
        std::cout<<"native handle slot reused by non-thread fixture="<<(replacement==thread)<<'\n';
        Check(worker.Complete(closed)&&!closed,"upstream close/reused non-thread handle not closed again");
        DWORD flags=0;Check(GetHandleInformation(replacement,&flags)!=0,"replacement object remains valid");
        CloseHandle(replacement);CloseHandle(event);
    }
    {
        NativeWorkerHandle reusable;
        for(unsigned cell=0;cell<16;++cell)
        {
            bool closed=false;volatile BYTE flag=1;DWORD tid=0;
            HANDLE event=CreateEvent(NULL,TRUE,FALSE,NULL);
            HANDLE thread=CreateThread(NULL,0,WaitForEvent,event,0,&tid);
            Check(event&&thread&&reusable.Adopt(thread,tid),"same private worker helper adopts each fresh cell thread");
            SetEvent(event);
            Check(reusable.StopAndJoin(&flag,2000),"every recycled-cell worker kernel-joined");
            Check(reusable.Complete(closed)&&closed,"recycled helper closes then resets ownership for next cell");
            CloseHandle(event);
        }
    }
    GetProcessHandleCount(GetCurrentProcess(),&after);
    Check(before==after,"no test worker/event/duplicate/native handles leak");
    std::cout<<checks<<" production Win32 worker identity/join/handle checks passed\n";
}
