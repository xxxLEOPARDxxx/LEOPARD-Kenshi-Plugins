#pragma once

// Pure reconciliation for one actual unit. Never infer success from an exception
// or an addItem return value alone, and never repeat a debit after confirmation.
enum DfGoodsState
{
    DF_GOODS_UNCERTAIN,
    DF_GOODS_UNCHANGED,
    DF_GOODS_DETACHED,
    DF_GOODS_MOVED
};
enum DfPaymentState
{
    DF_PAYMENT_UNCERTAIN,
    DF_PAYMENT_UNCHANGED,
    DF_PAYMENT_DEBITED,
    DF_PAYMENT_COMPLETE
};
DfGoodsState DfGoodsResult(int sourceBefore, int destinationBefore, int sourceAfter, int destinationAfter);
bool DfCanPay(int buyerMoney, int sellerMoney, int price);
DfPaymentState DfPaymentResult(int buyerBefore, int sellerBefore, int price, int buyerAfter, int sellerAfter);
// Pass raw owner and the engine's exact empty-faction sentinel. Public veins
// can use either NULL or nofac; a different neutral faction is still an owner.
bool DfCanHarvestOwner(const void* actualOwner, const void* workerFaction,
    const void* emptyFaction = 0);

// Kenshi's NPC receiving price, with the actual trader/town passed explicitly.
// Native getValueSingle(false) reads the player's GLOBAL trading GUI context.
// This reproduces its verified truncation order without touching that context.
int DfOreUnitPrice(int average, float townMult, float charges, float fullCharges,
                   float traderMult, float cultureMult);

// The native adapter catches faults at EACH operation and snapshots actual
// counts. This same coordinator is exercised with fault-injecting fake ports.
class DfTransferPort
{
public:
    virtual ~DfTransferPort() {}
    virtual bool Read(int& source, int& destination) = 0;
    virtual bool Remove(void*& detached) = 0;
    virtual bool Detached(void* item) = 0;
    virtual bool Add(void* item, bool& accepted) = 0;
    virtual bool Restore(void* item, bool& accepted) = 0;
};
enum DfUnitResult { DF_UNIT_STOP, DF_UNIT_MOVED, DF_UNIT_FAULT };
DfUnitResult DfTransferOne(DfTransferPort& port);

class DfPaymentPort
{
public:
    virtual ~DfPaymentPort() {}
    virtual bool Read(int& buyer, int& seller) = 0;
    virtual void WriteBuyer(int target) = 0;
    virtual void WriteSeller(int target) = 0;
};
// Called once after goods delivery. Only exact debit + unchanged seller permits
// one absolute buyer restoration; caller must inspect wallets after false.
bool DfPayOne(DfPaymentPort& port, int price);

class DfTradePort : public DfPaymentPort
{
public:
    virtual bool ReadGoods(int& seller, int& buyer) = 0;
    virtual DfUnitResult Deliver() = 0;
    virtual bool ReturnGoods() = 0;
};
enum DfTradeResult { DF_TRADE_STOP, DF_TRADE_COMPLETE, DF_TRADE_RESTORED, DF_TRADE_FAULT };
// One bounded physical deal. Compensation is allowed only with exact observed
// original wallets; goods are returned through the same real-transfer primitive.
DfTradeResult DfTradeOne(DfTradePort& port, int price);

class DfTreasuryPort
{
public:
    virtual ~DfTreasuryPort() {}
    virtual bool Read(int& amount) = 0;
    virtual bool Set(int amount) = 0;
};
// Load boundary only: absolute restoration, never add a grant or replay revenue.
// Missing legacy snapshots do not modify the native wallet. The session owner
// consumes restoration once, before economic work can resume.
bool DfRestoreTreasuryBalance(DfTreasuryPort& port, int saved);
// A failed load restoration must not overwrite the authoritative snapshot with
// the native wallet that has not yet received it. Read failures are not zero.
bool DfCaptureTreasuryBalance(DfTreasuryPort& port, bool restoreFailed, int& saved);
