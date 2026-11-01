#include <string>
#include <vector>
#include <cstdio>
static std::vector<std::string> messages;
void DebugLog(const std::string& message) { messages.push_back(message); }
#include <LocalizationLog.h>
extern void OtherTranslationUnit(bool enabled);
static int checks=0;
static void Check(bool ok) { ++checks; if(!ok) { std::fprintf(stderr,"failed check %d\n",checks); std::exit(1); } }
int main()
{
    KLocLog::SetSummary("locale id='ru_RU', strings loaded=18");
    Check(messages.empty()); // Debug=0, including first startup.
    OtherTranslationUnit(true);
    Check(messages.size()==1 && messages.back().find("ru_RU")!=std::string::npos);
    KLocLog::SetEnabled(true);
    Check(messages.size()==1); // Reapplying unchanged settings does not spam.
    OtherTranslationUnit(false);
    KLocLog::SetSummary("locale id='en_GB', strings loaded=22");
    Check(messages.size()==1);
    KLocLog::SetEnabled(true);
    Check(messages.size()==2 && messages.back().find("en_GB")!=std::string::npos);
    KLocLog::SetSummary("locale id='zh_CN', strings loaded=23");
    Check(messages.size()==3 && messages.back().find("zh_CN")!=std::string::npos);
    KLocLog::SetSummary(std::string(10000,'a').c_str());
    Check(messages.back().size()==255);
    KLocLog::SetEnabled(false);
    for(int i=0;i<100000;++i)KLocLog::SetEnabled(false);
    Check(messages.size()==4);
    bool flag=false;
    { KLocLog::FollowConfiguration finalFlag(flag); flag=true; flag=false; }
    Check(messages.size()==4); // Failed save rolls back before scope exits.
    { KLocLog::FollowConfiguration finalFlag(flag); flag=true; }
    Check(messages.size()==5); // Successful save enables the cached report.
    { KLocLog::FollowConfiguration finalFlag(flag); flag=false; }
    Check(messages.size()==5);
    std::printf("%d localization log checks passed\n",checks);
}
