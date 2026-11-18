#include <Windows.h>
#include <cstdio>
#include <stdexcept>
#include <EngineerSurplusPolicy.h>
static unsigned checks=0,failures=0;
static void Check(bool b,const char* s){++checks;if(!b){++failures;std::printf("FAIL %s\n",s);}}
struct Identity{unsigned serial;Identity(unsigned n=0):serial(n){}};
struct Item {int quantity;bool isEquipped;Identity handle;float chargesLeft,originalFullChargeAmount;};
struct Character{};struct GameData{};struct Owned{int units;};
struct Episode{long generation;unsigned token;Owned credit;};
static long currentEpoch=3;static bool reservation=true,reserved=false,receipt=false,failSplit=false,failBack=false,failDest=false,resetSplit=false,resetDest=false,throwDest=false,offSplit=false,feature=true;
static int sourceQuantity=0,storedQuantity=0,endCalls=0,commitCalls=0,errors=0,backCalls=0,pieceQuantity=0;static Item* piece=NULL;
static Identity pieceReceipt;
static Item* Make(int n){Item* p=static_cast<Item*>(VirtualAlloc(NULL,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));p->quantity=n;p->isEquipped=false;p->handle=Identity(9);p->chargesLeft=p->originalFullChargeAmount=0;return p;}
static void FreePiece(){if(piece)VirtualFree(piece,0,MEM_RELEASE);piece=NULL;pieceQuantity=0;}
struct Inventory
{
 bool hasItem(Item* p){return p&&p->quantity>0;}
 Item* removeItemDontDestroy_returnsItem(Item* p,int count,bool copy)
 {
   Check(count>0&&copy,"native split uses returning-copy contract");if(failSplit)return NULL;
   int n=count<p->quantity?count:p->quantity;sourceQuantity-=n;p->quantity-=n;piece=Make(n);pieceQuantity=n;
   if(offSplit)feature=false;
   if(resetSplit){++currentEpoch;FreePiece();return reinterpret_cast<Item*>(0x12345000);}
   return piece;
 }
 bool addItem(Item* p,int count,bool drop,bool destroy)
 {
   ++backCalls;Check(count==1&&!drop&&!destroy,"rollback never drops/destroys");if(failBack)return false;
   sourceQuantity+=p->quantity;FreePiece();return true;
 }
};
struct InventorySection
{
 bool addItem(Item* p,int count)
 {
   Check(count==1&&p->quantity==1,"destination receives exactly one unit");
   if(throwDest)throw std::runtime_error("native callback");
   if(resetDest)++currentEpoch;
   if(failDest)return false;
   storedQuantity+=p->quantity;FreePiece();return true;
 }
};
static long Epoch(){return currentEpoch;}
static bool Live(long e){return feature&&currentEpoch==e;}
static bool BeginTransaction(unsigned){if(!reservation||reserved)return false;reserved=true;return true;}
static void EndTransaction(unsigned){++endCalls;reserved=false;}
static Identity Of(const Identity& x){return x;}
static void Pending(unsigned,const Identity& p,const Identity&){receipt=true;pieceReceipt=p;}
static void ClearPending(unsigned){receipt=false;pieceReceipt=Identity();}
static int Total(Character*,GameData*){return sourceQuantity;}
namespace EngineerMaterialsRuntime{static void Returned(const Owned&,int n,int){Check(n==1,"credit receipt one unit");++commitCalls;}}
static void ErrorLog(const char*){++errors;}
// @PRODUCTION@
static void Reset()
{
 FreePiece();currentEpoch=3;reservation=true;reserved=receipt=failSplit=failBack=failDest=resetSplit=resetDest=throwDest=false,offSplit=false,feature=true;
 sourceQuantity=5;storedQuantity=endCalls=commitCalls=errors=backCalls=0;
}
int main()
{
 Inventory inv;InventorySection dest;Character c;GameData data;Item carried;carried.quantity=5;carried.isEquipped=false;carried.handle=Identity(4);carried.chargesLeft=carried.originalFullChargeAmount=0;
 NativeTransfer op;
 #define PREP Reset();carried.quantity=5;carried.isEquipped=false;carried.chargesLeft=carried.originalFullChargeAmount=0;op=NativeTransfer();op.episode.generation=3;op.episode.token=7;op.actor=&c;op.source=&inv;op.destination=&dest;op.item=&carried;op.data=&data
 PREP;Check(MoveOne(op)==1,"owned split delivers");Check(sourceQuantity==4&&storedQuantity==1&&!receipt&&commitCalls==1&&!reserved,"native receipt conserved after merging destroys piece");
 PREP;reservation=false;Check(MoveOne(op)==0&&sourceQuantity==5&&endCalls==0&&!receipt,"saturated receipt capacity prevents detach and foreign reservation release");
 PREP;reserved=true;Check(MoveOne(op)==0&&reserved&&endCalls==0,"cannot release another transaction reservation");
 PREP;failDest=true;Check(MoveOne(op)==0&&sourceQuantity==5&&storedQuantity==0&&!receipt&&commitCalls==0&&backCalls==1,"failed destination restored same native piece");
 PREP;failDest=failBack=true;Check(MoveOne(op)==0&&sourceQuantity==4&&pieceQuantity==1&&receipt&&pieceReceipt.serial==9&&commitCalls==0,"failed rollback keeps serial receipt and unit");
 Check(sourceQuantity+storedQuantity+pieceQuantity==5,"no loss when rollback fails");
 PREP;offSplit=failBack=true;Check(MoveOne(op)==0&&receipt&&pieceReceipt.serial==9&&backCalls==1&&storedQuantity==0,"feature OFF during detach retains full piece identity and attempts same-world rollback");
 PREP;offSplit=true;Check(MoveOne(op)==0&&!receipt&&sourceQuantity==5&&storedQuantity==0&&backCalls==1,"feature OFF during detach restores source without new destination delivery");
 PREP;resetSplit=true;Check(MoveOne(op)==0&&receipt&&pieceReceipt.serial==0&&backCalls==0&&commitCalls==0,"world change during split never dereferences poisoned old pointer");
 PREP;resetDest=failDest=true;Check(MoveOne(op)==0&&receipt&&pieceReceipt.serial==9&&backCalls==0&&commitCalls==0,"world change during destination prevents old source writes");
 PREP;failSplit=true;Check(MoveOne(op)==0&&sourceQuantity==5&&!receipt&&endCalls==1,"failed native split releases only reserved metadata");
 PREP;carried.isEquipped=true;Check(MoveOne(op)==0&&sourceQuantity==5&&endCalls==0,"personal equipment excluded");
 PREP;carried.chargesLeft=.5f;carried.originalFullChargeAmount=1;Check(MoveOne(op)==0&&sourceQuantity==5&&endCalls==0,"partial-charge item never split/reset for return");
 PREP;throwDest=true;bool threw=false;try{MoveOne(op);}catch(const std::runtime_error&){threw=true;}Check(threw&&receipt&&pieceQuantity==1&&!reserved&&commitCalls==0,"native exception preserves detached receipt and propagates");
 FreePiece();std::printf("%u checks, %u failures\n",checks,failures);return failures?1:0;
}
