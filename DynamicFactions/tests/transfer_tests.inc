#include "core/DfTransfer.h"
#include <climits>

struct FakeDfTransfer : DfTransferPort
{
    int source, destination, addMode, removes, adds, restores, readCalls, failRead;
    bool failRemove, failRestore, attached;
    FakeDfTransfer() : source(5), destination(2), addMode(0), removes(0), adds(0), restores(0),
        readCalls(0), failRead(0), failRemove(false), failRestore(false), attached(false) {}
    bool Read(int& s, int& d)
    {
        ++readCalls;
        if (readCalls == failRead) return false;
        s = source; d = destination; return true;
    }
    bool Remove(void*& p)
    {
        ++removes;
        --source;
        if (failRemove) return false; // native fault AFTER mutation, pointer unknown
        p = this; return true;
    }
    bool Detached(void*) { return !attached; }
    bool Add(void*, bool& accepted)
    {
        ++adds;
        if (addMode == 1) { accepted = false; return true; } // normal rejection
        if (addMode == 2) { accepted = true; return true; } // hook consumed, unknown destination
        if (addMode == 3) { attached = true; accepted = false; return true; }
        ++destination;
        attached = true;
        accepted = addMode != 4; // hook reports false AFTER actual merge
        return addMode != 5; // native fault AFTER actual merge
    }
    bool Restore(void*, bool& accepted)
    {
        ++restores;
        if (failRestore) return false;
        ++source; accepted = true; return true;
    }
};

struct FakeDfPayment : DfPaymentPort
{
    int buyer, seller, debits, credits, reads, failRead;
    bool ignoreDebit, ignoreCredit, corruptDebit, ignoreRefund, corruptCredit;
    FakeDfPayment() : buyer(100), seller(200), debits(0), credits(0), reads(0), failRead(0),
        ignoreDebit(false), ignoreCredit(false), corruptDebit(false), ignoreRefund(false), corruptCredit(false) {}
    bool Read(int& b, int& s)
    {
        ++reads;
        if (reads == failRead) return false;
        b = buyer; s = seller; return true;
    }
    void WriteBuyer(int target)
    {
        ++debits;
        if (!ignoreDebit && !(ignoreRefund && debits == 2)) buyer = target - (corruptDebit ? 1 : 0);
    }
    void WriteSeller(int target) { ++credits; if (!ignoreCredit) seller = target - (corruptCredit ? 1 : 0); }
};

struct FakeDfReturn : DfTransferPort
{
    FakeDfTransfer& goods;
    bool rejectAdd;
    FakeDfReturn(FakeDfTransfer& g, bool reject) : goods(g), rejectAdd(reject) {}
    bool Read(int& source, int& destination) { source=goods.destination;destination=goods.source;return true; }
    bool Remove(void*& p) { --goods.destination;p=this;return true; }
    bool Detached(void*) { return true; }
    bool Add(void*, bool& accepted)
    {
        accepted=!rejectAdd;
        if (accepted) ++goods.source;
        return true;
    }
    bool Restore(void*, bool& accepted) { ++goods.destination;accepted=true;return true; }
};

struct FakeDfTrade : DfTradePort
{
    FakeDfPayment money;
    FakeDfTransfer goods;
    int returnCalls;
    bool rejectReturn;
    FakeDfTrade() : returnCalls(0), rejectReturn(false) {}
    bool Read(int& b,int& s) { return money.Read(b,s); }
    void WriteBuyer(int n) { money.WriteBuyer(n); }
    void WriteSeller(int n) { money.WriteSeller(n); }
    bool ReadGoods(int& s,int& b) { s=goods.source;b=goods.destination;return true; }
    DfUnitResult Deliver() { return DfTransferOne(goods); }
    bool ReturnGoods()
    {
        ++returnCalls;
        FakeDfReturn reverse(goods,rejectReturn);
        return DfTransferOne(reverse)==DF_UNIT_MOVED;
    }
};

// The merchant's visual shop inventory is a copy snapshot, and personal pockets
// are a different persistent inventory. Only actual backing stock authorizes pay.
struct FakeDfShopTrade : FakeDfTrade
{
    int destinationMode, proxyCopies, personalPockets, hiddenSection;
    FakeDfShopTrade() : destinationMode(0), proxyCopies(2), personalPockets(0), hiddenSection(0) {}
    DfUnitResult Deliver()
    {
        if (destinationMode==3) return DF_UNIT_STOP; // all real sections full
        if (destinationMode==0) return FakeDfTrade::Deliver();
        --goods.source;
        if (destinationMode==1) ++proxyCopies;
        if (destinationMode==2) ++personalPockets;
        if (destinationMode==4) ++hiddenSection; // same parent, NOT its published backing section
        return DF_UNIT_MOVED; // claimed success is disproved by backing counts
    }
    void ReopenShopWindow() { proxyCopies=goods.destination; }
    int WholeParentStock() const { return goods.destination+hiddenSection; }
};

static void RunTransferTests()
{
    int workerFaction = 1, defaultFaction = 2, foreignOwner = 3;
    // Actual natural veins can use the engine's nofac sentinel, while another
    // neutral or non-real faction must not be mistaken for unowned resources.
    CHECK(DfCanHarvestOwner(NULL, &workerFaction));
    CHECK(!DfCanHarvestOwner(&defaultFaction, &workerFaction));
    CHECK(DfCanHarvestOwner(&workerFaction, &workerFaction));
    CHECK(!DfCanHarvestOwner(&foreignOwner, &workerFaction));
    CHECK(!DfCanHarvestOwner(NULL, NULL));
    CHECK(!DfCanHarvestOwner(&workerFaction, NULL));
    CHECK(DfCanHarvestOwner(&defaultFaction, &workerFaction, &defaultFaction));
    CHECK(!DfCanHarvestOwner(&foreignOwner, &workerFaction, &defaultFaction));
    CHECK(!DfCanHarvestOwner(&defaultFaction, NULL, &defaultFaction));
    CHECK(DfGoodsResult(5, 2, 4, 3) == DF_GOODS_MOVED);
    CHECK(DfGoodsResult(5, 2, 5, 2) == DF_GOODS_UNCHANGED);
    CHECK(DfGoodsResult(5, 2, 4, 2) == DF_GOODS_DETACHED);
    CHECK(DfGoodsResult(5, 2, 5, 3) == DF_GOODS_UNCERTAIN);
    CHECK(DfGoodsResult(5, 2, 3, 3) == DF_GOODS_UNCERTAIN);
    CHECK(DfGoodsResult(-1, 2, 0, 3) == DF_GOODS_UNCERTAIN);
    CHECK(DfGoodsResult(5, INT_MAX, 4, INT_MAX) == DF_GOODS_UNCERTAIN);
    CHECK(DfPaymentResult(20, 100, 7, 13, 107) == DF_PAYMENT_COMPLETE);
    CHECK(DfPaymentResult(20, 100, 7, 13, 100) == DF_PAYMENT_DEBITED);
    CHECK(DfPaymentResult(20, 100, 7, 20, 100) == DF_PAYMENT_UNCHANGED);
    CHECK(DfPaymentResult(20, 100, 7, 20, 107) == DF_PAYMENT_UNCERTAIN);
    CHECK(DfPaymentResult(20, 100, 7, 12, 107) == DF_PAYMENT_UNCERTAIN);
    CHECK(!DfCanPay(6, 100, 7));
    CHECK(!DfCanPay(7, INT_MAX - 6, 7));
    CHECK(!DfCanPay(-1, 0, 1));
    CHECK(!DfCanPay(10, -1, 1));
    CHECK(!DfCanPay(10, 10, 0));
    CHECK(DfCanPay(7, INT_MAX - 7, 7));
    // Explicit trader/town pricing uses native truncation order; no GUI globals.
    CHECK(DfOreUnitPrice(100, 1.25f, 1.0f, 1.0f, 0.8f, 1.5f) == 150);
    CHECK(DfOreUnitPrice(101, 1.25f, 1.0f, 1.0f, 0.8f, 1.5f) == 151);
    CHECK(DfOreUnitPrice(101, 1.25f, 0.5f, 1.0f, 0.8f, 1.5f) == 75);
    CHECK(DfOreUnitPrice(100, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f) == 0);
    CHECK(DfOreUnitPrice(100, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f) == 0);
    CHECK(DfOreUnitPrice(INT_MAX, 2.0f, 1.0f, 1.0f, 1.0f, 1.0f) == 0);
    CHECK(DfOreUnitPrice(100, std::numeric_limits<float>::infinity(), 1, 1, 1, 1) == 0);
    CHECK(DfOreUnitPrice(100, 1, std::numeric_limits<float>::quiet_NaN(), 1, 1, 1) == 0);
    CHECK(DfOreUnitPrice(100, 1, 2, 1, 1, 1) == 0);
    CHECK(DfOreUnitPrice(100, 1, 1, 1, -1, 1) == 0);
    FakeDfTransfer normal;
    CHECK(DfTransferOne(normal) == DF_UNIT_MOVED);
    CHECK(normal.source == 4 && normal.destination == 3 && normal.adds == 1 && normal.restores == 0);
    FakeDfTransfer reject;
    reject.addMode = 1;
    CHECK(DfTransferOne(reject) == DF_UNIT_STOP);
    CHECK(reject.source == 5 && reject.destination == 2 && reject.restores == 1);
    FakeDfTransfer consumed;
    consumed.addMode = 2;
    CHECK(DfTransferOne(consumed) == DF_UNIT_FAULT);
    CHECK(consumed.adds == 1 && consumed.restores == 0);
    FakeDfTransfer attached;
    attached.addMode = 3;
    CHECK(DfTransferOne(attached) == DF_UNIT_FAULT);
    CHECK(attached.restores == 0);
    FakeDfTransfer merged;
    merged.addMode = 4;
    CHECK(DfTransferOne(merged) == DF_UNIT_MOVED);
    CHECK(merged.adds == 1 && merged.restores == 0);
    FakeDfTransfer addFault;
    addFault.addMode = 5;
    CHECK(DfTransferOne(addFault) == DF_UNIT_MOVED);
    CHECK(addFault.adds == 1 && addFault.restores == 0);
    FakeDfTransfer removeFault;
    removeFault.failRemove = true;
    CHECK(DfTransferOne(removeFault) == DF_UNIT_FAULT);
    CHECK(removeFault.adds == 0 && removeFault.restores == 0);
    FakeDfTransfer countFault;
    countFault.failRead = 3;
    CHECK(DfTransferOne(countFault) == DF_UNIT_FAULT);
    CHECK(countFault.adds == 1 && countFault.restores == 0);
    FakeDfTransfer restoreFault;
    restoreFault.addMode = 1;
    restoreFault.failRestore = true;
    CHECK(DfTransferOne(restoreFault) == DF_UNIT_FAULT);
    CHECK(restoreFault.restores == 1);
    FakeDfTransfer empty;
    empty.source = 0;
    CHECK(DfTransferOne(empty) == DF_UNIT_STOP);
    CHECK(empty.removes == 0);
    FakeDfPayment payment;
    CHECK(DfPayOne(payment, 25));
    CHECK(payment.buyer == 75 && payment.seller == 225 && payment.buyer + payment.seller == 300);
    CHECK(payment.debits == 1 && payment.credits == 1);
    FakeDfPayment missingDebit;
    missingDebit.ignoreDebit = true;
    CHECK(!DfPayOne(missingDebit, 25));
    CHECK(missingDebit.debits == 1 && missingDebit.credits == 0);
    FakeDfPayment missingCredit;
    missingCredit.ignoreCredit = true;
    CHECK(!DfPayOne(missingCredit, 25));
    CHECK(missingCredit.debits == 2 && missingCredit.credits == 1);
    CHECK(missingCredit.buyer == 100 && missingCredit.seller == 200);
    FakeDfPayment corrupt;
    corrupt.corruptDebit = true;
    CHECK(!DfPayOne(corrupt, 25));
    CHECK(corrupt.debits == 1 && corrupt.credits == 0);
    FakeDfPayment unreadable;
    unreadable.failRead = 2;
    CHECK(!DfPayOne(unreadable, 25));
    CHECK(unreadable.debits == 1 && unreadable.credits == 0);
    FakeDfPayment poor;
    CHECK(!DfPayOne(poor, 101));
    CHECK(poor.debits == 0 && poor.credits == 0);
    FakeDfPayment failedRefund;
    failedRefund.ignoreCredit = failedRefund.ignoreRefund = true;
    CHECK(!DfPayOne(failedRefund,25));
    CHECK(failedRefund.buyer==75 && failedRefund.seller==200 && failedRefund.debits==2);
    FakeDfPayment partialCredit;
    partialCredit.corruptCredit=true;
    CHECK(!DfPayOne(partialCredit,25));
    CHECK(partialCredit.buyer==75 && partialCredit.seller==224 && partialCredit.debits==1);
    FakeDfTrade committed;
    CHECK(DfTradeOne(committed,25)==DF_TRADE_COMPLETE);
    CHECK(committed.goods.source==4 && committed.goods.destination==3 && committed.returnCalls==0);
    CHECK(committed.money.buyer==75 && committed.money.seller==225);
    FakeDfTrade restored;
    restored.money.ignoreCredit=true;
    CHECK(DfTradeOne(restored,25)==DF_TRADE_RESTORED);
    CHECK(restored.goods.source==5 && restored.goods.destination==2 && restored.returnCalls==1);
    CHECK(restored.money.buyer==100 && restored.money.seller==200);
    CHECK(restored.money.debits==2 && restored.money.credits==1);
    FakeDfTrade noDebit;
    noDebit.money.ignoreDebit=true;
    CHECK(DfTradeOne(noDebit,25)==DF_TRADE_RESTORED);
    CHECK(noDebit.returnCalls==1 && noDebit.goods.source==5 && noDebit.money.buyer==100);
    FakeDfTrade uncertainCredit;
    uncertainCredit.money.corruptCredit=true;
    CHECK(DfTradeOne(uncertainCredit,25)==DF_TRADE_FAULT);
    CHECK(uncertainCredit.returnCalls==0 && uncertainCredit.money.debits==1);
    FakeDfTrade refundFailed;
    refundFailed.money.ignoreCredit=refundFailed.money.ignoreRefund=true;
    CHECK(DfTradeOne(refundFailed,25)==DF_TRADE_FAULT);
    CHECK(refundFailed.returnCalls==0 && refundFailed.money.debits==2);
    FakeDfTrade returnRejected;
    returnRejected.money.ignoreCredit=true;
    returnRejected.rejectReturn=true;
    CHECK(DfTradeOne(returnRejected,25)==DF_TRADE_FAULT);
    CHECK(returnRejected.returnCalls==1 && returnRejected.goods.source==4 && returnRejected.goods.destination==3);
    CHECK(returnRejected.money.buyer==100 && returnRejected.money.seller==200);
    FakeDfTrade transferRejected;
    transferRejected.goods.addMode=1;
    CHECK(DfTradeOne(transferRejected,25)==DF_TRADE_STOP);
    CHECK(transferRejected.money.debits==0 && transferRejected.returnCalls==0);
    FakeDfShopTrade shopSale;
    CHECK(DfTradeOne(shopSale,25)==DF_TRADE_COMPLETE);
    CHECK(shopSale.goods.source==4 && shopSale.goods.destination==3 && shopSale.personalPockets==0);
    CHECK(shopSale.proxyCopies==2); // closed-window snapshot is not authoritative
    shopSale.ReopenShopWindow();
    CHECK(shopSale.proxyCopies==3 && shopSale.money.buyer==75 && shopSale.money.seller==225);
    FakeDfShopTrade copyOnly;
    copyOnly.destinationMode=1;
    CHECK(DfTradeOne(copyOnly,25)==DF_TRADE_FAULT);
    CHECK(copyOnly.goods.destination==2 && copyOnly.proxyCopies==3);
    CHECK(copyOnly.money.debits==0 && copyOnly.money.credits==0);
    FakeDfShopTrade pocketsOnly;
    pocketsOnly.destinationMode=2;
    CHECK(DfTradeOne(pocketsOnly,25)==DF_TRADE_FAULT);
    CHECK(pocketsOnly.goods.destination==2 && pocketsOnly.personalPockets==1);
    CHECK(pocketsOnly.money.debits==0 && pocketsOnly.money.credits==0);
    FakeDfShopTrade fullStock;
    fullStock.destinationMode=3;
    CHECK(DfTradeOne(fullStock,25)==DF_TRADE_STOP);
    CHECK(fullStock.goods.source==5 && fullStock.goods.destination==2 && fullStock.money.debits==0);
    FakeDfShopTrade shopRefund;
    shopRefund.money.ignoreCredit=true;
    CHECK(DfTradeOne(shopRefund,25)==DF_TRADE_RESTORED);
    CHECK(shopRefund.goods.source==5 && shopRefund.goods.destination==2 && shopRefund.personalPockets==0);
    CHECK(shopRefund.money.buyer==100 && shopRefund.money.seller==200 && shopRefund.returnCalls==1);
    FakeDfShopTrade redirectedWithinParent;
    redirectedWithinParent.destinationMode=4;
    CHECK(DfTradeOne(redirectedWithinParent,25)==DF_TRADE_FAULT);
    CHECK(redirectedWithinParent.WholeParentStock()==3 && redirectedWithinParent.goods.destination==2);
    CHECK(redirectedWithinParent.money.debits==0 && redirectedWithinParent.money.credits==0);
    CHECK(redirectedWithinParent.returnCalls==0);
}
