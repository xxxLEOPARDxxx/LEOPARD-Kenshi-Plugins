struct FakeDfPurchase : FakeDfTrade
{
    int deliveries;
    bool paidBeforeDelivery;
    bool failSellerRefund;
    FakeDfPurchase():deliveries(0),paidBeforeDelivery(false),failSellerRefund(false) {}
    DfUnitResult Deliver()
    {
        ++deliveries;paidBeforeDelivery=money.buyer==75 && money.seller==225;
        return FakeDfTrade::Deliver();
    }
    void WriteSeller(int n)
    {
        if(failSellerRefund && money.credits==1) {++money.credits;return;}
        FakeDfTrade::WriteSeller(n);
    }
};
static void RunPurchaseTests()
{
    FakeDfPurchase buy;
    CHECK(DfPurchaseOne(buy,25)==DF_TRADE_COMPLETE);
    CHECK(buy.deliveries==1 && buy.paidBeforeDelivery && buy.returnCalls==0);
    CHECK(buy.money.buyer==75 && buy.money.seller==225);
    CHECK(buy.goods.source==4 && buy.goods.destination==3);
    FakeDfPurchase rejected;rejected.goods.addMode=1;
    CHECK(DfPurchaseOne(rejected,25)==DF_TRADE_RESTORED);
    CHECK(rejected.money.buyer==100 && rejected.money.seller==200);
    CHECK(rejected.goods.source==5 && rejected.goods.destination==2 && rejected.returnCalls==0);
    FakeDfPurchase noDebit;noDebit.money.ignoreDebit=true;
    CHECK(DfPurchaseOne(noDebit,25)==DF_TRADE_STOP);
    CHECK(noDebit.deliveries==0 && noDebit.money.seller==200 && noDebit.returnCalls==0);
    FakeDfPurchase noCredit;noCredit.money.ignoreCredit=true;
    CHECK(DfPurchaseOne(noCredit,25)==DF_TRADE_STOP);
    CHECK(noCredit.deliveries==0 && noCredit.money.buyer==100 && noCredit.money.seller==200);
    FakeDfPurchase corrupt;corrupt.money.corruptCredit=true;
    CHECK(DfPurchaseOne(corrupt,25)==DF_TRADE_FAULT);CHECK(corrupt.deliveries==0);
    FakeDfPurchase merged;merged.goods.addMode=4;
    CHECK(DfPurchaseOne(merged,25)==DF_TRADE_COMPLETE);
    CHECK(merged.paidBeforeDelivery && merged.goods.destination==3 && merged.returnCalls==0);
    FakeDfPurchase ambiguous;ambiguous.goods.addMode=2;
    CHECK(DfPurchaseOne(ambiguous,25)==DF_TRADE_FAULT);
    CHECK(ambiguous.money.buyer==75 && ambiguous.money.seller==225 && ambiguous.returnCalls==0);
    FakeDfPurchase sellerRefundFail;sellerRefundFail.goods.addMode=1;sellerRefundFail.failSellerRefund=true;
    CHECK(DfPurchaseOne(sellerRefundFail,25)==DF_TRADE_FAULT);
    CHECK(sellerRefundFail.money.buyer==75 && sellerRefundFail.money.seller==225);
    CHECK(sellerRefundFail.money.debits==1 && sellerRefundFail.money.credits==2);
    FakeDfPurchase buyerRefundFail;buyerRefundFail.goods.addMode=1;buyerRefundFail.money.ignoreRefund=true;
    CHECK(DfPurchaseOne(buyerRefundFail,25)==DF_TRADE_FAULT);
    CHECK(buyerRefundFail.money.buyer==75 && buyerRefundFail.money.seller==200);
    CHECK(buyerRefundFail.money.debits==2 && buyerRefundFail.returnCalls==0);
    FakeDfPurchase invalid;
    CHECK(DfPurchaseOne(invalid,0)==DF_TRADE_STOP);CHECK(invalid.deliveries==0);
    invalid.money.seller=INT_MAX;
    CHECK(DfPurchaseOne(invalid,25)==DF_TRADE_STOP);CHECK(invalid.deliveries==0);
}
