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
};
template<class T>struct lektor:std::vector<T>{};
typedef int StatsEnumerated;
struct CharStats {float value;CharStats():value(1.5f){}float getStat(int,bool base){if(!base)abort();return value;}};
struct Character {
 Ogre::Vector3 base,bones[3];int boneCalls,positionCalls;bool positionFault,boneFault[3];
 std::string displayName;unsigned serial;bool dead;CharStats stats;
 Character():base(100,7,200),boneCalls(0),positionCalls(0),positionFault(false),displayName("character"),serial(1),dead(false){
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
struct PlayerInterface {lektor<Character*>playerCharacters;bool isObjectSelected(Character*){return true;}};
struct GameWorld {PlayerInterface*player;void showPlayerAMessage_withLog(const std::string&,bool){}};
GameWorld*ou=NULL;
struct ScreenLabel {
 enum{LS_MEDIUM,RS_NORMAL};Ogre::Vector3 offset;hand tracked;int trackingCalls;
 ScreenLabel():offset(0,0,0),trackingCalls(0){}
 void setTracking(hand h,Ogre::Vector3 v){tracked=h;offset=v;++trackingCalls;}
};
size_t labels=0;
struct ForgottenGUI {
 ScreenLabel label;bool failLabel;ForgottenGUI():failLabel(false){}
 ScreenLabel*createScreenLabel(const std::string&,MyGUI::Colour c,int size,int speed){
  if(c.r!=1||c.g!=.84f||c.b!=.35f||size!=ScreenLabel::LS_MEDIUM||speed!=ScreenLabel::RS_NORMAL)abort();
  ++labels;return failLabel?NULL:&label;
 }
};
ForgottenGUI*gui=NULL;
struct CharacterStatsWindow {struct Stat {std::string name;};static Stat*getStat(int){return NULL;}};
int failures=0;
void check(bool ok,const char*description){printf("%s %s\n",ok?"PASS":"FAIL",description);if(!ok)++failures;}
