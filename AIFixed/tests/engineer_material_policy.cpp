#include "../EngineerMaterialsPolicy.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace AIFixedEngineer;
static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"CHECK line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static Identity Id(unsigned n){Identity x;x.type=1;x.container=7;x.containerSerial=8;x.index=n;x.serial=9;return x;}
static Material Mat(const char* s){Material x;x.type=4;strcpy_s(x.id,s);return x;}
static Request Req(unsigned actor=1){Request x;x.actor=Id(actor);x.project=Id(20);x.source=Id(30);x.material=Mat("building_materials");x.need=100;x.available=100;x.epoch=3;x.now=100;return x;}
int main()
{
    Coordinator c;c.Reset(3);Request a=Req();
    CHECK(StorageCapacity(40,10,6)==5);CHECK(StorageCapacity(1000,0,6)==32);
    CHECK(StorageCapacity(7,0,6)==1);CHECK(StorageCapacity(5,8,6)==2);
    CHECK(StorageCapacity(40,10,0)==2);CHECK(StorageCapacity(40,-1,6)==2);
    CHECK(StorageCapacity(std::numeric_limits<double>::quiet_NaN(),0,6)==2);
    CHECK(StorageCapacity(40,std::numeric_limits<double>::infinity(),6)==2);
    CHECK(StorageCapacity(40,10,std::numeric_limits<double>::infinity())==2);
    Request cargo=Req();cargo.need=10;cargo.carried=3;CHECK(c.Cargo(cargo)==3);
    Request iron=cargo;iron.material=Mat("iron");iron.need=8;iron.carried=5;CHECK(c.Cargo(iron)==5);
    Request steel=cargo;steel.material=Mat("steel");steel.need=4;steel.carried=4;CHECK(c.Cargo(steel)==4);
    CHECK(c.Demand(Id(2),cargo.project,cargo.material,10,100,3)==7);
    CHECK(c.Demand(Id(2),iron.project,iron.material,8,100,3)==3);
    CHECK(c.Demand(Id(2),steel.project,steel.material,4,100,3)==0);
    cargo.source=Id(30);cargo.available=100;CHECK(c.Commit(cargo)==7);
    CHECK(c.Demand(Id(2),cargo.project,cargo.material,10,100,3)==0);
    CHECK(c.Cargo(cargo)==10); // a later cargo snapshot must not erase pending source units
    Request sourceProbe=Req(2);sourceProbe.project=Id(99);sourceProbe.available=8;CHECK(c.Preview(sourceProbe)==1);
    c.Cancel(cargo.actor);CHECK(c.Demand(Id(2),iron.project,iron.material,8,100,3)==8);
    c.Reset(3);
    CHECK(c.Cargo(iron)==5);CHECK(c.Cargo(steel)==4);CHECK(c.Cargo(cargo)==3);CHECK(c.Commit(cargo)==7);
    c.Acquired(cargo,2,3,5);Owned mixed[8];CHECK(c.ListOwned(mixed,8)==1&&mixed[0].owned==2&&mixed[0].baseline==3);
    c.Delivered(iron.actor,iron.material,5,4);
    CHECK(c.Demand(Id(2),iron.project,iron.material,7,100,3)==3);
    CHECK(c.Demand(Id(2),steel.project,steel.material,4,100,3)==0);
    CHECK(c.Demand(Id(2),cargo.project,cargo.material,10,100,3)==0);
    c.Finish(cargo,5);CHECK(c.Demand(Id(2),cargo.project,cargo.material,10,100,3)==5);
    c.Reset(3);
    for(unsigned i=0;i<Coordinator::Capacity;++i)
    {Request x=Req();char name[32];sprintf_s(name,"cargo_%u",i);x.material=Mat(name);x.carried=1;CHECK(c.Cargo(x)==1);}
    Request extra=Req();extra.material=Mat("extra_cargo");extra.carried=1;CHECK(c.Cargo(extra)==0);
    c.Cancel(extra.actor);CHECK(c.Cargo(extra)==1);c.Reset(3);
    // A selected owner's source witness survives a long native walk without
    // renewing its resource/demand lease. Unobserved witnesses expire fully.
    Request witness=Req();witness.ground=true;witness.available=1;witness.need=1;
    CHECK(c.Commit(witness)==1);Request observed;
    CHECK(c.SourceWatch(witness.actor,witness.project,witness.source,100,3,observed));
    CHECK(!c.SourceWatch(witness.actor,witness.project,witness.source,200,3,observed));
    CHECK(c.SourceWatch(witness.actor,witness.project,witness.source,120099,3,observed));
    c.Expire(120101,3);Request later=Req(2);later.ground=true;later.available=1;later.need=1;later.now=120101;
    CHECK(c.Preview(later)==1);witness.now=120101;CHECK(!c.Has(witness));
    CHECK(c.SourceWatch(witness.actor,witness.project,witness.source,120400,3,observed));
    c.Expire(240401,3);CHECK(!c.SourceWatch(witness.actor,witness.project,witness.source,240401,3,observed));
    c.Reset(3);
    Request shipment=Req();shipment.need=10;shipment.available=10;
    CHECK(c.Commit(shipment)==10);
    CHECK(c.SourceWatch(shipment.actor,shipment.project,shipment.source,350,3,observed));
    shipment.available=3;shipment.now=350;
    CHECK(c.RevalidateSource(observed,shipment)==3);
    CHECK(c.Demand(Id(2),shipment.project,shipment.material,10,350,3)==7);
    CHECK(c.SourceWatch(shipment.actor,shipment.project,shipment.source,600,3,observed));
    shipment.available=10;shipment.now=600;
    CHECK(c.RevalidateSource(observed,shipment)==10);
    CHECK(c.Demand(Id(2),shipment.project,shipment.material,10,600,3)==7);
    CHECK(c.Demand(Id(2),shipment.project,shipment.material,10,120101,3)==10);
    c.Reset(3);
    CHECK(c.Preview(a)==32);CHECK(c.Preview(a)==32);
    CHECK(c.Commit(a)==32);Request b=Req(2);CHECK(c.Commit(b)==32);
    b=Req(3);CHECK(c.Commit(b)==32);b=Req(4);CHECK(c.Commit(b)==4);
    b=Req(5);CHECK(c.Commit(b)==0);
    CHECK(c.Demand(b.actor,b.project,b.material,100,b.now,b.epoch)==0);
    c.Cancel(Id(2));CHECK(c.Commit(b)==32);
    a.carried=5;a.need=10;CHECK(c.Grant(a)==0); // other project cargo already covers demand
    c.Reset(3);a=Req();a.need=10;a.carried=3;CHECK(c.Commit(a)==7);
    b=Req(2);b.project=Id(21);b.available=8;CHECK(c.Commit(b)==1); // source shared across projects
    c.Reset(3);a=Req();a.ground=true;a.available=1000;a.need=3;CHECK(c.Commit(a)==3);
    b=Req(2);b.ground=true;CHECK(c.Commit(b)==0); // whole ground stack is exclusive
    b.source.serial++;CHECK(c.Commit(b)==32); // serial-bearing source identity
    c.Reset(3);a=Req();CHECK(c.Commit(a)==32);a.now+=Coordinator::LeaseMs+1;c.Expire(a.now,3);
    b=Req(2);b.need=100;b.available=100;CHECK(c.Commit(b)==32);
    c.Reset(4);CHECK(c.Commit(a)==0);a.epoch=4;CHECK(c.Commit(a)==32);
    Request old=a;old.epoch=3;CHECK(!c.Has(old)&&c.Has(a));
    c.Reset(3);a=Req();a.ground=true;a.need=3;CHECK(c.Commit(a)==3);
    c.Acquired(a,100,5,105);Owned rows[128];CHECK(c.ListOwned(rows,128)==1);
    CHECK(rows[0].owned==100&&rows[0].baseline==5&&rows[0].expectedTotal==105);
    int allowed=0;CHECK(c.CheckOwned(rows[0],105,allowed)&&allowed==100);
    c.Returned(rows[0],30,75);CHECK(c.ListOwned(rows,128)==1);CHECK(rows[0].owned==70);
    CHECK(!c.CheckOwned(rows[0],74,allowed));CHECK(c.ListOwned(rows,128)==0); // ambiguous external mutation
    c.Reset(3);a=Req();a.need=32;a.available=32;CHECK(c.Commit(a)==32);
    for(int unit=0;unit<32;++unit)
    {
        c.Acquired(a,1,unit,unit+1);Request other=Req(2);other.available=31-unit;
        CHECK(c.Preview(other)==0); // pending source units remain leased between native giveItem calls
        other.source=Id(99);other.need=32;CHECK(c.Preview(other)==0); // all in-flight demand stays reserved
    }
    c.Finish(a,32);b=Req(2);b.project=Id(21);b.available=1;CHECK(c.Preview(b)==1);
    c.Reset(3);a=Req();a.need=10;a.carried=3;CHECK(c.Commit(a)==7);
    b=Req(2);b.available=100;b.need=10;CHECK(c.Preview(b)==0); // existing carried cargo is demand too
    c.Reset(3);a=Req();CHECK(c.Commit(a)==32);c.Acquired(a,3,7,10);CHECK(c.ListOwned(rows,128)==1);
    Owned stale=rows[0];c.Cancel(a.actor);CHECK(c.ListOwned(rows,128)==1); // cancelling preserves proven surplus
    c.Reset(3);CHECK(!c.CheckOwned(stale,10,allowed));
    for(unsigned i=0;i<Coordinator::Capacity;++i){Request x=Req(i+1);x.project=Id(i+1000);x.source=Id(i+2000);CHECK(c.Commit(x)==32);}
    CHECK(c.Commit(Req(999))==0); // bounded table, no eviction of live actor
    std::printf("engineer materials policy: %u checks PASS\n",checks);return 0;
}
