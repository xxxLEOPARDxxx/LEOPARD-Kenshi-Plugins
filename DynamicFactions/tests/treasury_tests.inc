struct FakeTreasury : public DfTreasuryPort
{
    int balance, reads, sets, mode;
    FakeTreasury(int b,int m=0):balance(b),reads(0),sets(0),mode(m){}
    virtual bool Read(int& value) {++reads;if(mode==3 || (mode==4&&reads>1))return false;value=balance;return true;}
    virtual bool Set(int value) {++sets;if(mode==1)return false;balance=mode==5?value-1:value;return mode!=2;}
};
static void RunTreasuryRestoreTests()
{
    FakeTreasury fresh(0);CHECK(DfRestoreTreasuryBalance(fresh,14000));CHECK(fresh.balance==14000&&fresh.sets==1);
    CHECK(DfRestoreTreasuryBalance(fresh,14000));CHECK(fresh.balance==14000&&fresh.sets==1);
    FakeTreasury native(14000);CHECK(DfRestoreTreasuryBalance(native,14000));CHECK(native.sets==0);
    FakeTreasury zero(20);CHECK(DfRestoreTreasuryBalance(zero,0));CHECK(zero.balance==0);
    FakeTreasury legacy(70);CHECK(DfRestoreTreasuryBalance(legacy,-1));CHECK(legacy.balance==70&&legacy.reads==0&&legacy.sets==0);
    CHECK(!DfRestoreTreasuryBalance(legacy,-2));CHECK(legacy.balance==70&&legacy.sets==0);
    FakeTreasury before(0,1);CHECK(!DfRestoreTreasuryBalance(before,14000));CHECK(before.balance==0&&before.sets==1);
    FakeTreasury late(0,2);CHECK(DfRestoreTreasuryBalance(late,14000));CHECK(late.balance==14000&&late.sets==1);
    FakeTreasury unread(0,3);CHECK(!DfRestoreTreasuryBalance(unread,14000));CHECK(unread.sets==0);
    FakeTreasury postUnread(0,4);CHECK(!DfRestoreTreasuryBalance(postUnread,14000));CHECK(postUnread.balance==14000&&postUnread.sets==1);
    FakeTreasury wrong(0,5);CHECK(!DfRestoreTreasuryBalance(wrong,14000));CHECK(wrong.balance==13999&&wrong.sets==1);
    FakeTreasury invalid(-1);CHECK(!DfRestoreTreasuryBalance(invalid,14000));CHECK(invalid.balance==-1&&invalid.sets==0);
    int saved=14000;
    FakeTreasury unrestored(0,1);CHECK(!DfRestoreTreasuryBalance(unrestored,saved));
    const int readsBefore=unrestored.reads;CHECK(DfCaptureTreasuryBalance(unrestored,true,saved));
    CHECK(saved==14000&&unrestored.reads==readsBefore);
    int missing=-1;CHECK(!DfCaptureTreasuryBalance(unrestored,true,missing));CHECK(missing==-1);
    FakeTreasury capture(14240);CHECK(DfCaptureTreasuryBalance(capture,false,saved));CHECK(saved==14240);
    FakeTreasury missingWallet(0,3);CHECK(!DfCaptureTreasuryBalance(missingWallet,false,saved));CHECK(saved==14240);
    FakeTreasury negativeWallet(-3);CHECK(!DfCaptureTreasuryBalance(negativeWallet,false,saved));CHECK(saved==14240);
    FakeTreasury spent(0);CHECK(DfCaptureTreasuryBalance(spent,false,saved));CHECK(saved==0);
    std::vector<DfFactionState> state(1),loaded;state[0].recordId="df";state[0].founded=true;
    int totalSales=0;FakeTreasury wallet(14000);
    for(int round=0;round<3;++round)
    {
        wallet.balance+=240;totalSales+=240;CHECK(DfCaptureTreasuryBalance(wallet,false,state[0].treasury));
        const std::string save=DfSerialize(state);wallet.balance=0; // Native NPC wallet is NOT serialized.
        CHECK(DfDeserialize(save,loaded));CHECK(loaded.size()==1);
        CHECK(DfRestoreTreasuryBalance(wallet,loaded[0].treasury));
        CHECK(wallet.balance==14000+totalSales);
        CHECK(DfRestoreTreasuryBalance(wallet,loaded[0].treasury));CHECK(wallet.balance==14000+totalSales);
    }
}
static void RunTreasuryTests()
{
    RunTreasuryRestoreTests();
    std::vector<DfFactionState> out;
    const std::string prefix="[faction 0]\nid=df\nfounded=1\n";
    CHECK(DfDeserialize(prefix+"treasury=14000\n",out));
    CHECK(DfSerialize(out).find("treasury=14000\n")!=std::string::npos);
    CHECK(DfDeserialize(prefix+"treasury=0\n",out));
    CHECK(DfSerialize(out).find("treasury=0\n")!=std::string::npos);
    CHECK(DfDeserialize(prefix+"treasury=2147483647\n",out));
    CHECK(DfSerialize(out).find("treasury=2147483647\n")!=std::string::npos);
    DfStateValidationTests::Reject(prefix+"treasury=-1\n");
    DfStateValidationTests::Reject(prefix+"treasury=2147483648\n");
    DfStateValidationTests::Reject(prefix+"treasury=nan\n");
    DfStateValidationTests::Reject(prefix+"treasury=14000\ntreasury=14000\n");
    DfStateValidationTests::Reject("[faction 0]\nid=df\nfounded=0\ntreasury=14000\n");
    CHECK(DfDeserialize(prefix,out));
    CHECK(DfSerialize(out).find("treasury=")==std::string::npos);
}
