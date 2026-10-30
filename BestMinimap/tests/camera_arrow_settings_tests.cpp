#include <cstdio>
#include <string>
#include "../CameraRangePolicy.h"
int stored=0,pending=-1,writes=0,g_dot=10,g_cameraArrowSize=11;
int ReadInt(const std::string&,const char*,int,int lo,int hi){return stored<lo?lo:stored>hi?hi:stored;}
void WriteInt(const char*,int value){pending=value;}
void FlushWrites(){if(pending>=0){stored=pending;pending=-1;++writes;}}
void Apply(){FlushWrites();std::string ini;
#include "camera_arrow_settings_under_test.h"
}
int main(){int failures=0,checks=0;
#define C(x) do{++checks;if(!(x)){++failures;std::printf("FAIL %d\n",__LINE__);}}while(0)
 Apply();C(g_cameraArrowSize==22&&stored==22&&writes==1);
 stored=8;Apply();C(stored==8&&g_cameraArrowSize==8&&writes==1);
 Apply();C(writes==1);
 stored=8;Apply();C(g_cameraArrowSize==8&&stored==8&&writes==1);
 stored=4;Apply();C(g_cameraArrowSize==4&&writes==1);
 stored=2;Apply();C(g_cameraArrowSize==4&&stored==4&&writes==2);
 stored=0;g_dot=2;Apply();C(g_cameraArrowSize==8&&stored==8);
 stored=0;g_dot=5;Apply();C(g_cameraArrowSize==11&&stored==11);
 std::printf("%d production arrow-setting checks, %d failures\n",checks,failures);return failures?1:0;
}
