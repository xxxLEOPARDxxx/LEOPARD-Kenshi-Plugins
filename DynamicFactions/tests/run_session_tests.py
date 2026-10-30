from pathlib import Path
import subprocess, os, sys

root = Path(__file__).resolve().parents[1]
out = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else root.parents[1]/'build/DynamicFactionsSessionCheck'
out.mkdir(parents=True, exist_ok=True)
hire = (root/'game/DfHire.cpp').read_text(encoding='utf-8-sig')
recruit = (root/'game/DfRecruit.cpp').read_text(encoding='utf-8-sig')
main = (root/'DynamicFactions.cpp').read_text(encoding='utf-8-sig')
labour = (root/'game/DfLabour.cpp').read_text(encoding='utf-8-sig')

def function(text, signature):
    start = text.index(signature)
    brace = text.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]

code = r'''
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <string>
#include <map>
#include <cstdio>
#include <cmath>
#include <algorithm>
namespace Ogre { struct Vector3 {float x,y,z;Vector3(float a=0,float b=0,float c=0):x(a),y(b),z(c){} Vector3 operator+(const Vector3& b)const{return Vector3(x+b.x,y+b.y,z+b.z);} float squaredDistance(const Vector3& b)const {return (x-b.x)*(x-b.x)+(y-b.y)*(y-b.y)+(z-b.z)*(z-b.z);} }; }
struct Faction;
struct Ownerships { int money, mode, calls; Ownerships():money(100),mode(0),calls(0){}
 void setMoney(int value) { ++calls; if(mode==1 || (mode==5 && calls==2)) RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,0); money=value; if(mode==2) RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,0); if(mode==3) money-=3; }
};
struct Faction { Ownerships* factionOwnerships; };
struct Platoon { Faction* owner; };
struct ActivePlatoon { Platoon* me; };
int resolutions=0, cleared=0;
struct hand;
struct Character { Faction* faction; ActivePlatoon* platoon; int mode; bool player;
 Ogre::Vector3 position;Ogre::Vector3 getPosition(){return position;}
 Character():faction(0),platoon(0),mode(0),player(false){}
 bool isDead(){return false;} bool isUnconcious(){return false;} bool isDown(){return false;}
 bool isPlayerCharacter(){return player;} Faction* getFaction(){return faction;}
 ActivePlatoon* getPlatoon(){return platoon;}
 hand getHandle();
 void setFaction(Faction* f,ActivePlatoon* p){ if(mode==1) RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,0); faction=f; if(mode!=3)platoon=p; if(mode==2)RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,0); }
};
struct hand { unsigned int type,container,containerSerial,index,serial; Character* character;
 hand():type(1),container(2),containerSerial(3),index(4),serial(5),character(0){}
 Character* getCharacter() const { ++resolutions; return character; }
};
hand Character::getHandle(){hand h;h.character=this;return h;}
enum DfHirePhase { DF_HIRE_IDLE,DF_HIRE_GO,DF_HIRE_TALK };
struct DfHireTask { unsigned long session; int price; Faction* recruitFaction; std::string why; DfHirePhase phase; hand envoy;
 bool busy()const{return phase!=DF_HIRE_IDLE;} };
unsigned long g_session=1, now=0;
unsigned long DfRecruitSession(){return g_session;}
unsigned long DfHireNow(){return now;}
bool g_joinUncertain=false;
void ErrorLog(const char*){}
void ClearOrder(Character*){++cleared;}
'''
code += r'''
struct DfMemberWork {bool moving;DfMemberWork():moving(false){}};
struct DfLabour {unsigned int scoutStep;unsigned long scoutAt;DfLabour():scoutStep(0),scoutAt(0){}};
int moves=0;void Move(Character*,DfMemberWork& w,const Ogre::Vector3&){++moves;w.moving=true;}
'''
code += function(labour,'void Scout(')+'\n'
for signature in ['bool Able(', 'bool SetMoneySafely(', 'bool SetFactionSafely(', 'bool Join(', 'void DfHireCancel(']:
    code += function(hire, signature)+'\n'
start = recruit.index('    struct RecruitKey')
end = recruit.index('    // -1 - ', start)
code += recruit[start:end]+'\n'
code += r'''
int failures=0, checks=0;
void check(bool ok,const char* name){++checks;if(!ok){++failures;printf("FAIL %s\n",name);}}
void hireCase(int moneyMode,int factionMode,bool expected,int balance,bool member,bool blocked,const char* name){
 Ownerships o; o.mode=moneyMode; Faction f,old; f.factionOwnerships=&o; Platoon p; p.owner=&f; ActivePlatoon ap; ap.me=&p;
 Character envoy,recruit; envoy.faction=&f;envoy.platoon=&ap;recruit.faction=&old;recruit.mode=factionMode;
 DfHireTask t; t.session=g_session;t.price=30;t.recruitFaction=&old;g_joinUncertain=false;
 const bool result=Join(t,&f,&envoy,&recruit);
 check(result==expected && o.money==balance && (recruit.faction==&f)==member && g_joinUncertain==blocked,name);
}
int main(){
 {Character scout;DfMemberWork w;DfLabour r;Ogre::Vector3 home(0,100,0);scout.position=Ogre::Vector3(300,152,0);moves=0;
 Scout(&scout,w,r,home,3000,1000);
 check(r.scoutStep==1&&moves==0,"scout reaches horizontal waypoint despite terrain elevation");
 DfLabour r2;scout.position=Ogre::Vector3(250,100,0);moves=0;
 Scout(&scout,w,r2,home,3000,1000);
 check(r2.scoutStep==0&&moves==1,"scout still walks when horizontally outside arrival range");}
 hireCase(0,0,true,70,true,false,"hire success");
 hireCase(0,1,false,100,false,false,"join faults before mutation: refund");
 hireCase(0,2,true,70,true,false,"join faults after mutation: no duplicate/refund");
 hireCase(1,0,false,100,false,false,"debit faults before mutation: no free recruit");
 hireCase(2,0,true,70,true,false,"debit faults after mutation: inspect actual debit");
 hireCase(3,0,false,67,false,true,"unexpected debit: quarantine");
 hireCase(5,1,false,70,false,true,"refund fails: quarantine");
 hireCase(0,3,true,70,true,false,"paid faction member with partial squad result remains hired");
 {
 Ownerships o;Faction f,old,other;f.factionOwnerships=&o;Platoon p;p.owner=&f;ActivePlatoon ap;ap.me=&p;Character e,r;e.faction=&f;e.platoon=&ap;r.faction=&other;
 DfHireTask t;t.session=g_session;t.price=30;t.recruitFaction=&old;g_joinUncertain=false;
 check(!Join(t,&f,&e,&r)&&o.money==100,"third-party faction change prevents debit");
 t.phase=DF_HIRE_GO;t.session=g_session-1;t.envoy.character=&e;resolutions=cleared=0;DfHireCancel(t);
 check(t.phase==DF_HIRE_IDLE&&resolutions==0&&cleared==0,"stale cancellation never resolves handle");
 }
 hand a,b; check(!(RecruitKey(a)<RecruitKey(b))&&!(RecruitKey(b)<RecruitKey(a)),"same full identity");
 b.serial++;check(RecruitKey(a)<RecruitKey(b),"serial prevents reused index collision");b=a;b.containerSerial++;check(RecruitKey(a)<RecruitKey(b),"container serial prevents collision");
 b=a;b.type++;check(RecruitKey(a)<RecruitKey(b),"type is part of identity");
 RecruitKey oldKey(a);++g_session;check(oldKey<RecruitKey(a),"session prevents identical native identity reuse");
 printf("%d checks, %d failures\n",checks,failures); return failures;
}
'''
(out/'session_tests.cpp').write_text(code, encoding='utf-8')
vc = Path(r'C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC')
sdk = Path(r'C:\Program Files\Microsoft SDKs\Windows\v7.1')
env = dict(os.environ)
env['INCLUDE'] = str(vc/'include')+';'+str(sdk/'Include')
env['LIB'] = str(vc/'lib/amd64')+';'+str(sdk/'Lib/x64')
env['PATH'] = str(vc/'bin/amd64')+';'+env['PATH']
subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/W3',str(out/'session_tests.cpp'),'/Fo'+str(out/'session_tests.obj'),'/Fe'+str(out/'session_tests.exe')],env=env,check=True)
subprocess.run([str(out/'session_tests.exe')],check=True)

# Structural contracts complement the native-fault harness. They do not claim
# to prove the game's native hook ordering or success status semantics.
for hook, original in [('void SfsLoad_hook(', 'g_origSfsLoad('), ('void SfsNew_hook(', 'g_origSfsNew('), ('int LoadGame_hook(', 'g_origLoadGame('), ('void ResetGame_hook(', 'g_origResetGame(')]:
    body = function(main,hook)
    assert body.index('BeginSession(') < body.index(original), hook
assert 'DfHireCancel' not in function(main,'void SetStates(')
assert function(main,'void GuiUpdate_hook(').index('!g_ready') < function(main,'void GuiUpdate_hook(').index('PollKey(')
assert function(main,'bool SfsSave_hook(').index('WriteState(') < function(main,'bool SfsSave_hook(').index('g_origSfsSave(')
assert 'g_completedSfs == SESSION_LOAD' in function(main,'int LoadGame_hook(')
assert 'g_completedSfs == SESSION_NEW' in function(main,'void InitNewWorld_hook(')
assert function(main,'void ReadState(').index('g_states.clear()') < function(main,'void ReadState(').index('fileExists(')
print('Lifecycle/save ordering source contracts passed')
