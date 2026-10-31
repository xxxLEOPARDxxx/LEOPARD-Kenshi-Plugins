#pragma once
#include <string>
#include "core/DfSupply.h"

// Market knowledge only: no Item/GameData/Inventory pointers survive a visit.
struct DfStockPool
{
    unsigned int type,container,containerSerial,index,serial;
    DfStockPool():type(0),container(0),containerSerial(0),index(0),serial(0) {}
    bool Known()const {return index!=0 || container!=0;}
    bool operator==(const DfStockPool& b)const
    {return type==b.type && container==b.container && containerSerial==b.containerSerial && index==b.index && serial==b.serial;}
};
struct DfSupplyQuote
{
    DfStockPool pool;
    DfSupplyKind kind;
    std::string sid;
    int price;
    int quantity;
    float amount;
    DfBagSpec bag;
    DfSupplyQuote():kind(DF_SUPPLY_FOOD),price(0),quantity(0),amount(0) {}
};
