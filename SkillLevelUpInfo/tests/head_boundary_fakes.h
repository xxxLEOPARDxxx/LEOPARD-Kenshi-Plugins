#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ogre/OgreVector3.h>

DWORD fakeNow=10000;
DWORD ProbeTick(){return fakeNow;}
#define GetTickCount ProbeTick
void DebugLog(const std::string&){}
const char* Tr(const char* s){return s;}
namespace PluginDiagnostics {bool Enabled(){return false;}void Write(const char*){}}
namespace MyGUI {struct Colour {float r,g,b;Colour(float a,float c,float d):r(a),g(c),b(d){}};}
class Character;
struct GameSaveState {};
struct SaveFileSystem {};
struct hand {
 Character* ptr;unsigned serial;
 hand():ptr(NULL),serial(0){}hand(Character* p);
 bool operator<(const hand& h)const{return ptr<h.ptr||(ptr==h.ptr&&serial<h.serial);}
 bool operator==(const hand& h)const{return ptr==h.ptr&&serial==h.serial;}
 Character* getCharacter()const;
};
template<class T>struct lektor:std::vector<T>{};
typedef int StatsEnumerated;
struct CharStats {
 float value;Character* me;float refs[64];int firstRef,alias[64],unsupported;bool independent;
 CharStats():value(1.5f),me(NULL),firstRef(-1),unsupported(-1),independent(false){for(int i=0;i<64;++i){refs[i]=0;alias[i]=-1;}}
 float getStat(int index,bool base){if(!base)abort();return independent?getStatRef(index):value;}
 float& getStatRef(int index){static float outside=0;if(index==unsupported)return outside;if(alias[index]>=0)index=alias[index];if(firstRef<0)firstRef=index;return index==firstRef?value:refs[index];}
};
struct Character {
 Ogre::Vector3 base,bones[3];int boneCalls,positionCalls;bool positionFault,boneFault[3];
 std::string displayName;unsigned serial;bool dead;CharStats stats;
 Character():base(100,7,200),boneCalls(0),positionCalls(0),positionFault(false),displayName("character"),serial(1),dead(false){
  stats.me=this;
  for(int i=0;i<3;++i){bones[i]=base;boneFault[i]=false;}
 }
 bool isDead(){return dead;}bool isPlayerCharacter(){return true;}CharStats* getStats(){return &stats;}
 Ogre::Vector3 getPosition(){++positionCalls;if(positionFault)RaiseException(0xE1234567,0,0,NULL);return base;}
 Ogre::Vector3 getBoneWorldPosition(const std::string& name){
  ++boneCalls;int i=name=="Bip01 Head"?0:name=="Head"?1:2;
  if(boneFault[i])RaiseException(0xE1234567,0,0,NULL);return bones[i];
 }
};
hand::hand(Character* p):ptr(p),serial(p?p->serial:0){}
Character*hand::getCharacter()const{return ptr&&ptr->serial==serial?ptr:NULL;}
struct PlayerInterface {lektor<Character*>playerCharacters;bool selected;PlayerInterface():selected(true){}bool isObjectSelected(Character*){return selected;}};
struct GameWorld {PlayerInterface*player;void showPlayerAMessage_withLog(const std::string&,bool){}};
GameWorld*ou=NULL;
struct ScreenLabel {
 enum{LS_MEDIUM,RS_NORMAL,LS_SMALL};Ogre::Vector3 offset;hand tracked;int trackingCalls;
 ScreenLabel():offset(0,0,0),trackingCalls(0){}
 void setTracking(hand h,Ogre::Vector3 v){tracked=h;offset=v;++trackingCalls;}
};
size_t labels=0;
std::string lastCaption;int lastSize=-1;bool lastGold=false;
struct ForgottenGUI {
 ScreenLabel label;bool failLabel;ForgottenGUI():failLabel(false){}
 ScreenLabel*createScreenLabel(const std::string& text,MyGUI::Colour c,int size,int speed){
  bool gold=c.r==1&&c.g==.84f&&c.b==.35f&&size==ScreenLabel::LS_MEDIUM;
  bool blue=c.r==.62f&&c.g==.86f&&c.b==1&&size==ScreenLabel::LS_SMALL;
  if((!gold&&!blue)||speed!=ScreenLabel::RS_NORMAL)abort();lastGold=gold;lastSize=size;lastCaption=text;
  ++labels;return failLabel?NULL:&label;
 }
};
ForgottenGUI*gui=NULL;
struct CharacterStatsWindow {struct Stat {std::string name;};static Stat*getStat(int){return NULL;}};
int failures=0;
void check(bool ok,const char*description){printf("%s %s\n",ok?"PASS":"FAIL",description);if(!ok)++failures;}
