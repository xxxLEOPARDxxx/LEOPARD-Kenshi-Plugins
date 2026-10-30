// Native inventories/wallets are saved by Kenshi. The mod only serializes its
// own metadata. Restart hauling after each possible phase from those inventories.
static void RunEconomyReloadTests()
{
    for (int reloadStep=0;reloadStep<8;++reloadStep)
    {
        int ore=5,cargo=0,stock=0,buyer=7,seller=0;
        bool atNode=false,atTrader=false,greeted=false;
        DfFactionState metadata=Founded(0.0);
        metadata.recordId="reload-test";
        DfKnownNode node;node.x=node.y=node.z=0;node.itemSid="modded-ore";node.stored=17;
        metadata.nodes.push_back(node);
        for (int step=0;step<24;++step)
        {
            if (step==reloadStep)
            {
                // This snapshot is between synchronous units, as native saves
                // are before the GUI tick. No task cash/item counter is saved.
                const int savedOre=ore,savedCargo=cargo,savedStock=stock;
                const int savedBuyer=buyer,savedSeller=seller;
                std::vector<DfFactionState> loaded;
                CHECK(DfDeserialize(DfSerialize(std::vector<DfFactionState>(1,metadata)),loaded));
                CHECK(loaded.size()==1);
                if(loaded.empty()) return;
                metadata=loaded[0];greeted=false;
                ore=savedOre;cargo=savedCargo;stock=savedStock;buyer=savedBuyer;seller=savedSeller;
            }
            DfHaulSnapshot s={};s.able=true;s.carried=cargo;s.nodeLoaded=true;
            s.atNode=atNode;s.nodeHasOre=ore>0;s.hasRoom=cargo<2;
            s.hasTrader=buyer>=3;s.atTrader=atTrader;s.greeted=greeted;
            switch(DfChooseHaulAction(s))
            {
            case DF_HAUL_GO_NODE:atNode=true;atTrader=false;break;
            case DF_HAUL_TAKE:
            {
                FakeDfTransfer move;move.source=ore;move.destination=cargo;
                CHECK(DfTransferOne(move)==DF_UNIT_MOVED);
                ore=move.source;cargo=move.destination;break;
            }
            case DF_HAUL_GO_TRADER:atNode=false;atTrader=true;break;
            case DF_HAUL_TALK:greeted=true;break;
            case DF_HAUL_SELL:
            {
                FakeDfTransfer move;move.source=cargo;move.destination=stock;
                CHECK(DfTransferOne(move)==DF_UNIT_MOVED);
                FakeDfPayment pay;pay.buyer=buyer;pay.seller=seller;
                CHECK(DfPayOne(pay,3));
                cargo=move.source;stock=move.destination;buyer=pay.buyer;seller=pay.seller;
                break;
            }
            default:break;
            }
            CHECK(ore+cargo+stock==5);
            CHECK(buyer+seller==7);
            CHECK(metadata.nodes[0].stored==17); // legacy virtual stock never cashed/materialized
        }
        CHECK(stock==2 && seller==6 && buyer==1);
    }
    DfFactionState fault=Founded(0.0);fault.recordId="quarantine";fault.workFault=true;
    std::vector<DfFactionState> loaded;
    CHECK(DfDeserialize(DfSerialize(std::vector<DfFactionState>(1,fault)),loaded));
    CHECK(loaded.size()==1 && loaded[0].workFault);
    // Full workers cannot hold the only empty-hauler reservation indefinitely.
    std::vector<DfMember> members;
    members.push_back(Member(0,DF_ROLE_HAUL,0,0));members[0].canHaul=false;
    members.push_back(Member(1,DF_ROLE_MINE,0,0));
    std::vector<DfNodeState> nodes(1,Node(100,10,40,2));
    DfAssignRoles(members,nodes,true,true);
    CHECK(members[0].role!=DF_ROLE_HAUL && members[1].role==DF_ROLE_HAUL);
    members[0].carried=2;
    DfAssignRoles(members,nodes,true,false);
    CHECK(members[0].role==DF_ROLE_HAUL); // carried goods survive pickup cooldown
}
