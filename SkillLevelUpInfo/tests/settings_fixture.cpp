int recordedDefault=0,recordedMin=0,recordedMax=0;
void Begin(MCM_Api*,const char*,const char*,const char*,MCM_ApplyFn){}
void Toggle(MCM_Api*,const char*,const char*,const char*,const char*,int,unsigned){}
void Number(MCM_Api*,const char*,const char*,const char*,const char*,float,float,float,int,unsigned){}
void Integer(MCM_Api*,const char*,const char*key,const char*,const char*,int def,int lo,int hi,unsigned){
 if(std::string(key)=="MaxRows"){recordedDefault=def;recordedMin=lo;recordedMax=hi;}}
int main(){
 InitializeCriticalSection(&g_lock);const std::string path=PluginSettings::Path();
 if(path.find("_generated")==std::string::npos)abort();
 WritePrivateProfileStringA("SkillLevelUpInfo","MaxRows","3",path.c_str());
 WritePrivateProfileStringA("Unrelated","Preserve","untouched",path.c_str());
 LoadSettings();check(g_maxRows==2,"legacy MaxRows3 clamps to2 at actual LoadSettings");
 check(PluginSettings::Text("SkillLevelUpInfo","MaxRows","",path)=="3"&&PluginSettings::Text("Unrelated","Preserve","",path)=="untouched",
       "loading legacy MaxRows does not rewrite user INI or unrelated values");
 WritePrivateProfileStringA("SkillLevelUpInfo","MaxRows","0",path.c_str());LoadSettings();check(g_maxRows==1,"runtime MaxRows lower bound remains1");
 WritePrivateProfileStringA("SkillLevelUpInfo","MaxRows",NULL,path.c_str());LoadSettings();check(g_maxRows==2,"missing MaxRows preserves default2");
 MCM_Api api={};api.version=1;api.beginMod=Begin;api.toggle=Toggle;api.integer=Integer;api.number=Number;MCM_Describe(&api);
 check(recordedDefault==2&&recordedMin==1&&recordedMax==2,"actual MCM_Describe exposes range1..2 with default2");
 printf("actual settings/MCM regressions: %s\n",failures?"FAILED":"PASSED");return failures?1:0;
}
