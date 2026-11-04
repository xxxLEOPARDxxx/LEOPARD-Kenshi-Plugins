#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
static int sdkCalls=0,errorCalls=0,debugCalls=0;static bool throwSdk=false;
namespace KenshiLib{struct BinaryVersion{std::string ToString(){return "1.0.65 GOG";}};static BinaryVersion GetKenshiVersion(){++sdkCalls;if(throwSdk)throw 1;return BinaryVersion();}}
static void DebugLog(const char*){++debugCalls;}static void ErrorLog(const char*){++errorCalls;}
#define KEO_STARTUP_HOST_TEST
#include "GogStartupTest.cpp"
static int checks=0,beginCalls=0;static std::vector<std::string> sections,rows;
static MCM_GetIntFn langGet=NULL,debugGet=NULL;static MCM_SetIntFn langSet=NULL,debugSet=NULL;
static void Check(bool ok,const char*why){++checks;if(!ok){printf("FAIL %s\n",why);throw 1;}}
static void __cdecl Begin(MCM_Api*,const char*id,const char*,const char*ini,MCM_ApplyFn apply){++beginCalls;Check(!strcmp(id,"KEOGogStartupTest"),"distinct test page id");Check(!ini&&!apply,"no config/native apply");}
static void __cdecl Section(MCM_Api*,const char*text){sections.push_back(text);}
static void __cdecl Info(MCM_Api*,const char*){}
static void __cdecl Choice(MCM_Api*,const char*,const char*,MCM_GetIntFn get,MCM_SetIntFn set,void*ud,const int*values,const char*const*,int count,unsigned flags){Check(!ud&&!flags&&count==3&&values[0]==0&&values[2]==2,"three languages without native state");langGet=get;langSet=set;rows.push_back("language");}
static void __cdecl Toggle(MCM_Api*,const char*,const char*,MCM_GetIntFn get,MCM_SetIntFn set,void*ud,unsigned flags){Check(!ud&&!flags,"debug cached no restart");debugGet=get;debugSet=set;rows.push_back("debug");}
int main(){
 try{
  MCM_Api api;memset(&api,0,sizeof(api));api.version=3;api.beginMod=Begin;api.section=Section;api.info=Info;api.choiceFn=Choice;api.toggleFn=Toggle;
  MCM_Describe(NULL);api.version=2;MCM_Describe(&api);Check(beginCalls==0,"unsupported MCM refused");api.version=3;
  MCM_Describe(&api);Check(sections.size()==3&&sections[0].find("not completed")!=std::string::npos,"startup not falsely completed");Check(sdkCalls==0,"Describe cannot query SDK");
  startPlugin();startPlugin();Check(sdkCalls==1&&GogStartup::Read(&GogStartup::state)==2,"startup once");Check(debugCalls==0&&errorCalls==0,"success quiet until Debug");
  sections.clear();rows.clear();MCM_Describe(&api);Check(sdkCalls==1,"no repeat SDK query on page");Check(sections[0].find("successfully")!=std::string::npos,"startup status");Check(sections[1]=="KenshiLib: 1.0.65 GOG","platform status");Check(sections[2].find("OFF")!=std::string::npos,"optimization state honest");Check(rows.size()==2&&rows.back()=="debug","debug always last");
  int value=-1;char err[8]={0};Check(langGet(NULL,&value)==0&&value==0,"English default");Check(langSet(NULL,1,err,sizeof(err))==0,"RU accepted");Check(langSet(NULL,2,err,sizeof(err))==0,"ZH accepted");Check(langGet(NULL,&value)==0&&value==2,"language persisted in own memory");Check(langSet(NULL,3,err,sizeof(err))==1&&GogStartup::Read(&GogStartup::language)==2,"invalid language refused");Check(langGet((void*)1,&value)==1&&langGet(NULL,NULL)==1,"invalid getter args");
  Check(debugGet(NULL,&value)==0&&value==0,"debug default OFF");Check(debugSet(NULL,1,err,sizeof(err))==0&&debugCalls==2,"debug emits saved startup summary");Check(debugSet(NULL,0,err,sizeof(err))==0&&debugCalls==2,"disable quiet");Check(debugSet(NULL,2,err,sizeof(err))==1,"invalid debug refused");
  InterlockedExchange(&GogStartup::state,0);throwSdk=true;startPlugin();Check(errorCalls==1&&GogStartup::Read(&GogStartup::state)==-1,"SDK failure visible and safe");
  printf("PASS %d actual production startup/MCM host checks; no game SDK called\n",checks);return 0;
 }catch(...){return 1;}
}
