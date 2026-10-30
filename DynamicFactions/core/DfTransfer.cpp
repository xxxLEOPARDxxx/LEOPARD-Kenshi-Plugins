#include "core/DfTransfer.h"
#include <climits>
#include <float.h>

DfGoodsState DfGoodsResult(int s0, int d0, int s1, int d1)
{
    if (s0 <= 0 || d0 < 0 || s1 < 0 || d1 < 0 || d0 == INT_MAX)
        return DF_GOODS_UNCERTAIN;
    if (s1 == s0 && d1 == d0)
        return DF_GOODS_UNCHANGED;
    if (s1 == s0 - 1 && d1 == d0 + 1)
        return DF_GOODS_MOVED;
    if (s1 == s0 - 1 && d1 == d0)
        return DF_GOODS_DETACHED;
    return DF_GOODS_UNCERTAIN;
}

bool DfCanPay(int buyer, int seller, int price)
{
    return price > 0 && buyer >= price && seller >= 0 && seller <= INT_MAX - price;
}

bool DfCanHarvestOwner(const void* owner, const void* worker, const void* emptyFaction)
{
    return worker != 0 && (owner == 0 || owner == worker || (emptyFaction != 0 && owner == emptyFaction));
}

DfPaymentState DfPaymentResult(int b0, int s0, int price, int b1, int s1)
{
    if (!DfCanPay(b0, s0, price) || b1 < 0 || s1 < 0)
        return DF_PAYMENT_UNCERTAIN;
    if (b1 == b0 - price && s1 == s0 + price)
        return DF_PAYMENT_COMPLETE;
    if (b1 == b0 - price && s1 == s0)
        return DF_PAYMENT_DEBITED;
    if (b1 == b0 && s1 == s0)
        return DF_PAYMENT_UNCHANGED;
    return DF_PAYMENT_UNCERTAIN;
}

namespace
{
    bool Positive(float x) { return _finite(x) && x > 0.0f; }
    int Truncate(float x)
    {
        // Comparing to INT_MAX as float rounds up; use double before casting.
        return _finite(x) && x > 0.0f && static_cast<double>(x) <= INT_MAX ? static_cast<int>(x) : 0;
    }
}

int DfOreUnitPrice(int average, float town, float charges, float full, float trader, float culture)
{
    if (average <= 0 || !Positive(town) || !Positive(charges) || !Positive(full) ||
        charges > full || !Positive(trader) || !Positive(culture))
        return 0;
    const int local = Truncate(static_cast<float>(average) * town);
    const int charged = Truncate((charges / full) * static_cast<float>(local));
    return Truncate(static_cast<float>(charged) * (trader * culture));
}

DfUnitResult DfTransferOne(DfTransferPort& port)
{
    int s0 = 0, d0 = 0, s1 = 0, d1 = 0;
    if (!port.Read(s0, d0) || s0 <= 0 || d0 < 0 || d0 == INT_MAX) return DF_UNIT_STOP;
    void* detached = 0;
    const bool removed = port.Remove(detached);
    if (!port.Read(s1, d1)) return DF_UNIT_FAULT;
    const DfGoodsState afterRemove = DfGoodsResult(s0, d0, s1, d1);
    if (!removed || detached == 0)
        return removed && afterRemove == DF_GOODS_UNCHANGED ? DF_UNIT_STOP : DF_UNIT_FAULT;
    if (afterRemove != DF_GOODS_DETACHED || !port.Detached(detached)) return DF_UNIT_FAULT;
    bool accepted = false;
    const bool added = port.Add(detached, accepted);
    if (!port.Read(s1, d1)) return DF_UNIT_FAULT;
    const DfGoodsState afterAdd = DfGoodsResult(s0, d0, s1, d1);
    if (afterAdd == DF_GOODS_MOVED) return DF_UNIT_MOVED;
    // A successful add consumes its pointer, even if a hook merged/deleted it.
    if (!added || accepted || afterAdd != DF_GOODS_DETACHED || !port.Detached(detached))
        return DF_UNIT_FAULT;
    bool restored = false;
    port.Restore(detached, restored); // never retry restoration after a fault
    if (!port.Read(s1, d1)) return DF_UNIT_FAULT;
    return DfGoodsResult(s0, d0, s1, d1) == DF_GOODS_UNCHANGED ? DF_UNIT_STOP : DF_UNIT_FAULT;
}

bool DfPayOne(DfPaymentPort& port, int price)
{
    int b0 = 0, s0 = 0, b1 = 0, s1 = 0;
    if (!port.Read(b0, s0) || !DfCanPay(b0, s0, price)) return false;
    port.WriteBuyer(b0 - price);
    if (!port.Read(b1, s1)) return false;
    DfPaymentState state = DfPaymentResult(b0, s0, price, b1, s1);
    if (state == DF_PAYMENT_DEBITED)
    {
        port.WriteSeller(s0 + price);
        if (!port.Read(b1, s1)) return false;
        state = DfPaymentResult(b0, s0, price, b1, s1);
        if (state == DF_PAYMENT_DEBITED)
        {
            // Credit demonstrably did not occur. Exactly one absolute restore,
            // never add(price), retry debit, or touch any uncertain wallet.
            port.WriteBuyer(b0);
            if (!port.Read(b1, s1)) return false;
            // false is intentional: payment was cancelled, not completed.
            return false;
        }
    }
    return state == DF_PAYMENT_COMPLETE;
}

DfTradeResult DfTradeOne(DfTradePort& port, int price)
{
    int b0 = 0, s0 = 0, b1 = 0, s1 = 0;
    int goodsSeller = 0, goodsBuyer = 0, gs1 = 0, gb1 = 0;
    if (!port.Read(b0, s0) || !DfCanPay(b0, s0, price) ||
        !port.ReadGoods(goodsSeller, goodsBuyer) || goodsSeller <= 0 || goodsBuyer < 0 || goodsBuyer == INT_MAX)
        return DF_TRADE_STOP;
    const DfUnitResult delivered = port.Deliver();
    if (delivered == DF_UNIT_FAULT) return DF_TRADE_FAULT;
    if (!port.ReadGoods(gs1, gb1) || !port.Read(b1, s1)) return DF_TRADE_FAULT;
    if (b1 != b0 || s1 != s0) return DF_TRADE_FAULT;
    const DfGoodsState goods = DfGoodsResult(goodsSeller, goodsBuyer, gs1, gb1);
    if (delivered == DF_UNIT_STOP)
        return goods == DF_GOODS_UNCHANGED ? DF_TRADE_STOP : DF_TRADE_FAULT;
    if (goods != DF_GOODS_MOVED) return DF_TRADE_FAULT;
    if (DfPayOne(port, price)) return DF_TRADE_COMPLETE;
    // No credit to seller and no net charge to buyer must be PROVEN before
    // moving the merchant's real item back. An unknown result is not a refund.
    if (!port.Read(b1, s1) || b1 != b0 || s1 != s0) return DF_TRADE_FAULT;
    if (!port.ReturnGoods()) return DF_TRADE_FAULT;
    if (!port.ReadGoods(gs1, gb1) || !port.Read(b1, s1)) return DF_TRADE_FAULT;
    return gs1 == goodsSeller && gb1 == goodsBuyer && b1 == b0 && s1 == s0
        ? DF_TRADE_RESTORED : DF_TRADE_FAULT;
}
