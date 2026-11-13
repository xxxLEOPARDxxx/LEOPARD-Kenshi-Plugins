#include <Windows.h>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>
#include <cstdio>
#include <RestPolicy.h>
// @GLOBALS@
char fixturePath[MAX_PATH]={};
const char* fixtureIni=fixturePath;
std::string IniPath(){return fixtureIni;}
namespace KLocLog {void SetEnabled(bool){}}
// @LOAD@
int checks=0,failures=0;
void Check(bool condition,const char* message){++checks;if(!condition){++failures;printf("FAIL: %s\n",message);}}
std::string Bytes(){std::ifstream f(fixtureIni,std::ios::binary);return std::string(std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>());}
int main(){
    if(!GetFullPathNameA("unstuck-delay-settings-fixture.ini",MAX_PATH,fixturePath,NULL))return 2;
    DeleteFileA(fixtureIni);
    WritePrivateProfileStringA("AIFixed","ForeignUserKey","keep-exact",fixtureIni);
    WritePrivateProfileStringA("AIFixed","RecoverStuckNpc","1",fixtureIni);
    const std::string oldIni=Bytes();g_unstuckDelayHours=99;LoadSettings();
    Check(g_unstuckDelayHours==1,"old personal INI missing the new key defaults to one hour");
    Check(g_unstuck,"loading new wait does not reset the existing recovery toggle");
    Check(Bytes()==oldIni,"actual settings load does not rewrite personal INI contents");
    const char* values[]={"0","1","2","3","4","5"};const int expected[]={1,1,2,3,4,4};
    for(unsigned i=0;i<6;++i){
        WritePrivateProfileStringA("AIFixed","UnstuckDelayHours",values[i],fixtureIni);
        const std::string before=Bytes();LoadSettings();
        Check(g_unstuckDelayHours==expected[i],"actual INI loader clamps1..4 and applies each live integer value");
        Check(Bytes()==before,"live delay load preserves all existing user keys and values");
    }
    DeleteFileA(fixtureIni);
    printf("AIFixed extracted delay settings: %s (%d checks)\n",failures?"FAIL":"PASS",checks);
    return failures?1:0;
}
