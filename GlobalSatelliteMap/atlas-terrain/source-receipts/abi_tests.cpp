#define _CRT_SECURE_NO_WARNINGS
#include "NativeMetadata.h"
#include <cstdio>
#include <map>
#include <string>
using namespace SourceReceipts;
static unsigned checks=0;static void C(bool b){++checks;if(!b)throw 1;}
struct ThreadInput{boost::recursive_mutex*mutex;bool acquired;};
static DWORD WINAPI Other(LPVOID p){ThreadInput&x=*static_cast<ThreadInput*>(p);TryLock lock;x.acquired=lock.Lock(*x.mutex);return 0;}
int main(){try{
 C(GroupAccess::Layout());C(ArchiveAccess::Layout());C(ResourceAccess::Layout());C(sizeof(GroupAccess::Getter)==16);C(sizeof(boost::recursive_mutex)==24);
 typedef boost::detail::basic_recursive_mutex B;C(offsetof(B,recursion_count)==0);C(offsetof(B,locking_thread_id)==4);C(offsetof(B,mutex)==8);C(offsetof(boost::detail::basic_timed_mutex,active_count)==0);C(offsetof(boost::detail::basic_timed_mutex,event)==8);
 boost::recursive_mutex mutex;{TryLock one,two;C(one.Lock(mutex));C(two.Lock(mutex));C(mutex.recursion_count==2&&DWORD(mutex.locking_thread_id)==GetCurrentThreadId());ThreadInput input={&mutex,true};HANDLE thread=CreateThread(NULL,0,Other,&input,0,NULL);C(thread!=NULL);C(WaitForSingleObject(thread,5000)==WAIT_OBJECT_0);CloseHandle(thread);C(!input.acquired);two.Clear();C(mutex.recursion_count==1);}C(mutex.recursion_count==0&&mutex.locking_thread_id==0);
 for(unsigned n=0;n<80;++n){Ogre::String s(n,'a');char copied[TextLimit];C(NativeString(s,copied,TextLimit,true));C(strlen(copied)==n);}
 typedef std::map<std::string,U64>Map;Map exact,lower;exact["DATA/Test.dds"]=1;exact["DATA/Test.dds"]=2;lower["data/test.dds"]=3;unsigned kind=0;const std::string name="DATA/Test.dds";C(IndexedWinner(exact,lower,name,kind)==2&&kind==Exact);exact.clear();C(IndexedWinner(exact,lower,name,kind)==3&&kind==Insensitive);C(name=="DATA/Test.dds");C(IndexedWinner(exact,lower,std::string("absent"),kind)==0);
 printf("PASS %u actual SDK layout/recursive-lock/local-string/index-order checks; no native module calls\n",checks);return 0;
 }catch(...){fprintf(stderr,"FAIL ABI/lock/index check%u\n",checks);return 1;}}
