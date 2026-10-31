// The runner injects production functions here; only external UI/input/game dependencies are fake.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <intrin.h>
#include <map>
#include <new>
#include <string>

static int checks=0, failures=0, parentCalls=0, allocations=0;
void* operator new(size_t n) { ++allocations; void* p=malloc(n); if(!p) throw std::bad_alloc(); return p; }
void operator delete(void* p) { free(p); }
void Check(bool value,const char* label) { ++checks; if(!value) { ++failures; printf("FAIL: %s\n",label); } }
typedef unsigned long DWORD;
enum { SM_SWAPBUTTON=23, VK_LBUTTON=1, VK_RBUTTON=2 };
static bool buttonDown=false, swapped=false;
static int keyRead=0;
int GetSystemMetrics(int) { return swapped?1:0; }
short GetAsyncKeyState(int key) { keyRead=key; return buttonDown?(short)0x8000:0; }
DWORD GetTickCount() { return 2000; }
namespace MyGUI {
struct Widget {
    Widget* parent;
    std::string name;
    Widget(const char* n="",Widget* p=NULL):parent(p),name(n) {}
    Widget* getParent() { ++parentCalls; return parent; }
    const std::string& getName() const { return name; }
};
struct InputManager {
    static InputManager* instance;
    Widget* focus;
    InputManager():focus(NULL) {}
    static InputManager* getInstancePtr() { return instance; }
    Widget* getMouseFocusWidget() { return focus; }
};
InputManager* InputManager::instance=NULL;
}
struct InventoryGUI {
    MyGUI::Widget* win;
    InventoryGUI* childInventory;
    InventoryGUI(MyGUI::Widget* w=NULL):win(w),childInventory(NULL) {}
};
struct ForgottenGUI { std::map<int,InventoryGUI*> inventoryWindowsOpen; };
static ForgottenGUI* gui=NULL;
struct PlayerInterface {};
struct RootObject {};
struct Character {};
struct Building {};
struct hand {};
enum TaskType { TRADE=17, MOVE=18 };
namespace Ogre { struct Vector3 { float x,y,z; Vector3():x(1),y(2),z(3) {} }; }
static DWORD g_lastDropLogMs=0;
static void* g_dropCaller=NULL;
static int logs=0;
void DebugLog(const std::string&) { ++logs; }
std::string CallerOf(void*) { return "fixture"; }

// EXTRACT_PRODUCTION

static int orderCalls[5]={};
static bool approach=false;
static PlayerInterface player;
static Character character;
static Building building;
static RootObject subject;
static hand target;
static Ogre::Vector3 position;
static TaskType receivedTask=MOVE;
static const hand* receivedTarget=NULL;
static const Ogre::Vector3* receivedPosition=NULL;
static bool receivedFlag=false;
void OriginalAdd(PlayerInterface*,Building*,TaskType,RootObject*,bool,bool,const Ogre::Vector3&) { ++orderCalls[0]; }
void OriginalWaypoint(PlayerInterface*,const Ogre::Vector3&,Building*) { ++orderCalls[2]; }
void OriginalMove(PlayerInterface*,const Ogre::Vector3& pos,Building*) { ++orderCalls[3]; receivedPosition=&pos; }
void OriginalChar(Character*,Building*,RootObject*,const Ogre::Vector3&) { ++orderCalls[4]; }
void OriginalTask(PlayerInterface* self,TaskType task,const hand& h,Building* indoors,const Ogre::Vector3& click,bool flag) {
    ++orderCalls[1]; receivedTask=task; receivedTarget=&h; receivedFlag=flag;
    if(approach) { PlayerMove_hook(self,click,indoors); CharMoveOrder_hook(&character,indoors,&subject,click); }
}
void Reset(MyGUI::InputManager& input,MyGUI::Widget* focus) {
    input.focus=focus; buttonDown=false; swapped=false; parentCalls=0;
    g_rmbWasDown=false; g_rmbStale=false; g_rmbWhere.clear(); g_lastDropLogMs=0;
    memset(orderCalls,0,sizeof(orderCalls)); approach=false;
    receivedTask=MOVE; receivedTarget=NULL; receivedPosition=NULL; receivedFlag=false;
}
void AllHooks() {
    AddOrder_hook(&player,&building,TRADE,&subject,true,false,position);
    NewTask_hook(&player,TRADE,target,&building,position,true);
    MoveWaypoint_hook(&player,position,&building);
    PlayerMove_hook(&player,position,&building);
    CharMoveOrder_hook(&character,&building,&subject,position);
}
void Policy(MyGUI::InputManager& input,MyGUI::Widget* focus,bool blocked,const char* label) {
    input.focus=focus; std::string where; const int before=allocations; parentCalls=0;
    Check(PressOverInterface(where)==blocked,label);
    Check(parentCalls<=64,"one bounded ancestor walk");
    if(!blocked) Check(allocations==before,"outside inventory allocates no memory");
    if(blocked) Check(where==focus->getName(),"matched focus name retained for existing diagnostics");
}
int main() {
    g_origAddOrder=OriginalAdd; g_origNewTask=OriginalTask; g_origMoveWaypoint=OriginalWaypoint;
    g_origPlayerMove=OriginalMove; g_origCharMoveOrder=OriginalChar;
    ForgottenGUI manager; gui=&manager; MyGUI::InputManager input; MyGUI::InputManager::instance=&input;
    MyGUI::Widget common("GUI"), inv("Inventory",&common), content("Content",&inv), client("Client",&content);
    MyGUI::Widget frame("Frame",&inv), icon("ItemIcon",&client), portrait("Portrait C",&common);
    MyGUI::Widget menu("PortraitContext",&common), trade("Trade",&menu), map("LargeMap",&common);
    MyGUI::Widget bag("DetachedBag",&common), bagIcon("BagItem",&bag);
    InventoryGUI inventory(&inv), backpack(&bag); inventory.childInventory=&backpack;
    manager.inventoryWindowsOpen[1]=&inventory; manager.inventoryWindowsOpen[2]=NULL;
    Policy(input,&inv,true,"inventory root blocked"); Policy(input,&frame,true,"inventory frame blocked");
    Policy(input,&icon,true,"nested inventory item blocked"); Policy(input,&bagIcon,true,"detached child backpack blocked");
    Policy(input,&portrait,false,"portrait while other trade open is allowed");
    Policy(input,&trade,false,"portrait context Trade is allowed"); Policy(input,&map,false,"large map remains allowed");
    Policy(input,&common,false,"common UI ancestor is not an inventory window");
    Policy(input,NULL,false,"no focus allowed");
    MyGUI::Widget unrelated("Inventory",&common); InventoryGUI foreign(&unrelated);
    for(int i=100;i<164;++i) manager.inventoryWindowsOpen[i]=&foreign;
    Policy(input,&portrait,false,"many unrelated inventory roots do not claim portrait");
    Check(parentCalls==2,"ancestor collection happens once across all open inventory entries");
    for(int i=100;i<164;++i) manager.inventoryWindowsOpen.erase(i);
    MyGUI::Widget replacement("Inventory",&common),replacementIcon("ItemIcon",&replacement);
    inventory.win=&replacement;
    Policy(input,&icon,false,"replaced inventory root is not cached");
    Policy(input,&replacementIcon,true,"replacement root is recognized immediately");
    inventory.win=&inv;
    MyGUI::Widget replacementBag("DetachedBag",&common),replacementBagIcon("BagItem",&replacementBag);
    InventoryGUI replacementBackpack(&replacementBag); inventory.childInventory=&replacementBackpack;
    Policy(input,&bagIcon,false,"old child backpack is not cached");
    Policy(input,&replacementBagIcon,true,"new child backpack is recognized immediately");
    inventory.childInventory=&backpack;
    input.focus=&portrait; std::string where; MyGUI::InputManager::instance=NULL;
    Check(!PressOverInterface(where),"missing InputManager allowed"); MyGUI::InputManager::instance=&input;
    gui=NULL; Check(!PressOverInterface(where),"missing GUI allowed"); gui=&manager;
    MyGUI::Widget cycleA("Cycle A"),cycleB("Cycle B"); cycleA.parent=&cycleB; cycleB.parent=&cycleA;
    Policy(input,&cycleA,false,"foreign cyclic ancestry terminates and allows");
    MyGUI::Widget chain[65];
    for(int i=0;i<65;++i) chain[i].parent=i<64?&chain[i+1]:NULL;
    MyGUI::Widget* savedWin=inventory.win; inventory.childInventory=NULL;
    inventory.win=&chain[63]; Policy(input,&chain[0],true,"root at ancestor bound is recognized");
    inventory.win=&chain[64]; Policy(input,&chain[0],false,"root past ancestry bound falls through");
    inventory.win=NULL; Policy(input,&icon,false,"null inventory window ignored");
    inventory.win=savedWin; inventory.childInventory=&backpack;
    // Three original inventory routes must still suppress every existing task/move hook.
    MyGUI::Widget* protectedFocus[3]={&frame,&icon,&bagIcon};
    for(int i=0;i<3;++i) {
        Reset(input,protectedFocus[i]); buttonDown=true; AllHooks();
        for(int j=0;j<5;++j) Check(orderCalls[j]==0,"inventory press blocks each original hook");
        const int walked=parentCalls; AllHooks();
        Check(parentCalls==walked,"held button does not repeat ancestry walk");
    }
    // Exact reported route: fresh RMB over C followed by a context-menu Trade command.
    Reset(input,&portrait); buttonDown=true; TrackRightButton(); input.focus=&trade; buttonDown=false;
    approach=true; NewTask_hook(&player,TRADE,target,&building,position,true);
    Check(orderCalls[1]==1,"portrait Trade reaches original NewTask");
    Check(orderCalls[3]==1&&orderCalls[4]==1,"original Trade auto-approach move hooks survive");
    Check(receivedTask==TRADE&&receivedTarget==&target&&receivedFlag,"Trade original target/task/flag preserved");
    Check(receivedPosition==&position,"original auto-approach position preserved");
    Reset(input,&trade); buttonDown=true; AllHooks();
    for(int j=0;j<5;++j) Check(orderCalls[j]==1,"fresh context menu press passes each hook");
    Reset(input,&map); buttonDown=true; PlayerMove_hook(&player,position,&building);
    Check(orderCalls[3]==1,"map click issues original move with inventory open");
    // Release outside or close cannot replay the inventory-origin click, even across multiple hooks.
    Reset(input,&icon); buttonDown=true; TrackRightButton(); input.focus=NULL; buttonDown=false;
    AllHooks(); for(int j=0;j<5;++j) Check(orderCalls[j]==0,"release outside keeps inventory click suppressed");
    manager.inventoryWindowsOpen.clear(); AllHooks();
    for(int j=0;j<5;++j) Check(orderCalls[j]==0,"closing inventory does not replay stale click");
    buttonDown=true; AllHooks(); for(int j=0;j<5;++j) Check(orderCalls[j]==1,"fresh world press clears stale inventory latch");
    Check(!g_rmbStale,"fresh world click resets stale state");
    Policy(input,&portrait,false,"empty inventory map allows other interface");
    manager.inventoryWindowsOpen[1]=&inventory;
    Reset(input,&portrait); buttonDown=true; TrackRightButton(); const int walked=parentCalls;
    const int before=allocations;
    for(int i=0;i<1000;++i) { TrackRightButton(); NewTask_hook(&player,TRADE,target,&building,position,true); }
    Check(parentCalls==walked,"1000 held frames/hooks have zero parent queries");
    Check(allocations==before,"1000 held frames/hooks have zero allocations");
    Check(orderCalls[1]==1000,"1000 allowed commands preserve original calls");
    Reset(input,&icon); swapped=true; buttonDown=true; TrackRightButton();
    Check(keyRead==VK_LBUTTON&&g_rmbStale,"swapped mouse buttons preserve logical RMB protection");
    printf("RMB production regression: %d checks, %d failures\n",checks,failures);
    return failures?1:0;
}
