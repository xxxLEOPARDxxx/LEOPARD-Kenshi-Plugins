from pathlib import Path
import subprocess, os, sys

root = Path(__file__).resolve().parents[1]
out = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else root.parents[1]/'build/DynamicFactionsSessionCheck'
out.mkdir(parents=True, exist_ok=True)
hire = (root/'game/DfHire.cpp').read_text(encoding='utf-8-sig')
recruit = (root/'game/DfRecruit.cpp').read_text(encoding='utf-8-sig')
main = (root/'DynamicFactions.cpp').read_text(encoding='utf-8-sig')
labour = (root/'game/DfLabour.cpp').read_text(encoding='utf-8-sig')
workinfo = (root/'ui/DfWorkInfo.cpp').read_text(encoding='utf-8-sig')

def function(text, signature):
    start = text.index(signature)
    brace = text.index('{', start)
    while ';' in text[start:brace]:
        start = text.index(signature, text.index(';', start)+1)
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
#include <vector>
namespace Ogre { struct Vector3 {float x,y,z;Vector3(float a=0,float b=0,float c=0):x(a),y(b),z(c){} Vector3 operator+(const Vector3& b)const{return Vector3(x+b.x,y+b.y,z+b.z);} float squaredDistance(const Vector3& b)const {return (x-b.x)*(x-b.x)+(y-b.y)*(y-b.y)+(z-b.z)*(z-b.z);} }; }
typedef int StatsEnumerated;const int STAT_NONE=0;const int STAT_LABOURING=3;
struct DatapanelGUI {};
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
 StatsEnumerated currentSkillUsing;
 Character():currentSkillUsing(0),faction(0),platoon(0),mode(0),player(false){}
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
struct GameData {std::string stringID;};
struct DfKnownNode {float x,y,z;std::string itemSid;int stored;};
struct DfFactionState {std::vector<DfKnownNode> nodes;};
struct DfNode {GameData* item;int itemValue;Ogre::Vector3 pos;};
Ogre::Vector3 Pos(const DfKnownNode& n){return Ogre::Vector3(n.x,n.y,n.z);}
'''
code += function(labour,'int Known(')+'\n'
code += r'''
bool g_ready=false,g_debug=false;DWORD g_readyAt=0;int readyRestores=0;
void RestoreTreasuries(){++readyRestores;}
void UpdateOwnFactions(){}
void DebugLog(const std::string&){}
'''
code += function(main,'void MarkReady(')+'\n'
code += r'''
bool infoEligible=true;int infoShown=-1,infoMode=0;
bool SafeSkill(Character* c,StatsEnumerated& shown){shown=STAT_LABOURING;return infoEligible&&c->currentSkillUsing==STAT_NONE;}
void FakeGuiData(Character* c,DatapanelGUI*,int){infoShown=c->currentSkillUsing;c->currentSkillUsing=infoMode==2?4:0;if(infoMode==1)RaiseException(EXCEPTION_ACCESS_VIOLATION,0,0,0);}
void (*g_original)(Character*,DatapanelGUI*,int)=FakeGuiData;
'''
code += function(workinfo,'void GuiData_hook(')+'\n'
code += r'''
bool GuiInfoFault(Character* c){bool fault=false;__try{GuiData_hook(c,0,0);}__except(EXCEPTION_EXECUTE_HANDLER){fault=true;}return fault;}
'''


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
 {Character c;infoEligible=true;infoMode=0;GuiData_hook(&c,0,0);
 check(infoShown==STAT_LABOURING&&c.currentSkillUsing==STAT_NONE,"offscreen work row is supplied only during native GUI rendering");
 c.currentSkillUsing=4;GuiData_hook(&c,0,0);check(infoShown==4&&c.currentSkillUsing==0,"native nonempty work stat retains native consumption");
 infoEligible=false;GuiData_hook(&c,0,0);check(infoShown==0&&c.currentSkillUsing==0,"nonworker or unmanaged character is not given a work row");
 infoEligible=true;infoMode=1;const bool fault=GuiInfoFault(&c);
 check(fault&&c.currentSkillUsing==0,"temporary UI stat is restored after native GUI fault");
 infoMode=2;GuiData_hook(&c,0,0);check(c.currentSkillUsing==4,"a different native state written during rendering is preserved");}

 g_ready=false;readyRestores=0;MarkReady("loaded");
 check(g_ready&&readyRestores==1,"treasury restored before first ready boundary");
 MarkReady("duplicate hook completion");check(readyRestores==1,"repeated ready callback cannot restore cash again");
 {Character worker;worker.position=Ogre::Vector3(0,0,0);std::vector<Character*> people(1,&worker);
 GameData data;data.stringID="ore";DfNode node;node.item=&data;node.itemValue=90;node.pos=Ogre::Vector3(1694,50,0);DfFactionState state;
 check(Known(state,node,people)==0&&state.nodes.size()==1,"public local vein is available immediately without scouting circles");
 check(Known(state,node,people)==0&&state.nodes.size()==1,"same real vein is not duplicated");
 node.item=0;check(Known(state,node,people)==-1,"missing output is not a known mine");node.item=&data;
 node.itemValue=0;check(Known(state,node,people)==-1,"invalid output value is not a known mine");}
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

assert function(main,'void MarkReady(').index('RestoreTreasuries(')<function(main,'void MarkReady(').index('g_ready = true')
assert function(main,'void RestoreTreasuries(').index('g_restoreTreasury = false')<function(main,'void RestoreTreasuries(').index('DfRestoreTreasury(')
assert 'g_restoreTreasury = false' in function(main,'void BeginSession(')
assert function(main,'void WriteState(').index('DfCaptureTreasury(')<function(main,'void WriteState(').index('DfSerialize(')
print('Treasury save/restore boundary contracts passed')

assert 'g_runtime[i].treasuryRestoreFailed=true' in function(main,'void RestoreTreasuries(')
assert 'g_runtime[i].treasuryRestoreFailed,s.treasury' in function(main,'void WriteState(')
