namespace FineStore {struct Identity{};struct Ledger{};}
struct RouteProducer {
 WorldCohort::Job job;WorldCohort::Status state;unsigned view,borrows,releases;bool borrowing,invalidRelease;WorldPixels::Accepted accepted;
 RouteProducer(const WorldCohort::Job&j):job(j),state(WorldCohort::Complete),view(0),borrows(0),releases(0),borrowing(false),invalidRelease(false){}
 WorldCohort::Status State(){return state;}unsigned CurrentView(){return view;}HRESULT Pump(bool,ID3D11Device*,const SourceReceipts::Scope&,const WorldBank::Ledger&){return borrowing?E_FAIL:S_OK;}
 const WorldRaster::Frame*Result(unsigned n,bool,const SourceReceipts::Scope&,const WorldRaster::Frame&f){return n==view&&n<job.count&&WorldRaster::SameFrame(f,job.frame[n])?&job.frame[n]:NULL;}
 bool Borrow(unsigned n,bool enabled,const SourceReceipts::Scope&s,const WorldRaster::Frame&f,WorldCohort::Borrowed&out){if(!Result(n,enabled,s,f)||borrowing)return false;borrowing=true;++borrows;out.accepted=&accepted;return true;}
 HRESULT ReleaseView(unsigned n,bool enabled,const SourceReceipts::Scope&s,const WorldRaster::Frame&f){if(borrowing){invalidRelease=true;return E_FAIL;}if(!Result(n,enabled,s,f))return E_FAIL;++releases;if(++view==job.count)state=WorldCohort::Idle;return S_OK;}
};
struct RouteWorker {
 RouteProducer*p;bool pending,ack;unsigned puts;FineWindows::Key keys[4];
 RouteWorker(RouteProducer&producer):p(&producer),pending(false),ack(false),puts(0){}
 bool SubmitPut(const WorldCohort::Borrowed&borrow,const FineWindows::Key&key,const FineStore::Identity&,const FineStore::Ledger&){if(pending||!borrow.accepted||!p->borrowing||puts>=4)return false;keys[puts++]=key;pending=true;return true;}bool Quarantined(){return false;}
};
struct RouteControl {
 struct Owner {FineJob::Plan plan;WorldCohort::Job job;SourceReceipts::Scope scope;FineStore::Identity identity;RouteWorker worker;unsigned pair,stored;bool queued,writing,soft;FineWindows::Key writingKey;Owner(const FineJob::Plan&plan_,const WorldCohort::Job&job_,RouteProducer&p):plan(plan_),job(job_),scope(plan_.demand.scope),worker(p),pair(0),stored(0),queued(true),writing(false),soft(false){}}owner;
 RouteProducer producer;bool failed,quarantined_,drained;FineJob::Budget budget;
 // Producer is declared after Owner for no production layout significance;
 // Owner's constructor stores its address without dereferencing it.
 RouteControl(const FineJob::Plan&plan,const WorldCohort::Job&job):owner(plan,job,producer),producer(job),failed(false),quarantined_(false),drained(false){std::memset(&budget,0,sizeof(budget));budget.viewerGpu=FineJob::ViewerAllowance;budget.producerCpu=FineJob::Internal::ProducerCpuFloor();}
 void Hard(HRESULT){failed=true;}bool PollWritten(){if(owner.worker.pending&&owner.worker.ack){owner.worker.pending=false;owner.writing=false;producer.borrowing=false;++owner.stored;}return true;}
 int DrainStep(const WorldBank::Ledger&){drained=!producer.borrowing;return drained?2:0;}bool CpuLedger(bool,const FineStore::Ledger&,FineStore::Ledger&){return true;}void Retain(){quarantined_=true;}
 int Step(){Owner&o=owner;RouteProducer&p=producer;const FineJob::Demand&now=o.plan.demand;ID3D11Device*device=NULL;WorldBank::Ledger gpu={};FineStore::Ledger external;
 // EXTRACTED_WRITING
 // EXTRACTED_QUEUED_GATE
 // EXTRACTED_COMPLETE
 return 0;}
};
static void Routes(const FineJob::Plan&plan,const WorldCohort::Job&job){
 RouteControl route(plan,job);C(route.Step()==0&&route.owner.worker.puts==1,"first exact output borrowed for cache write");
 C(route.Step()==0&&route.producer.releases==0&&route.owner.worker.puts==1,"pending file consumer prevents source release and next borrow");
 route.owner.worker.ack=true;for(unsigned n=0;n<8&&route.producer.state!=WorldCohort::Idle;++n)route.Step();
 unsigned wanted=0;for(unsigned n=0;n<job.count;++n)if(plan.pairs[n/2].wanted[n%2]){C(wanted<route.owner.worker.puts&&FineWindows::Equal(route.owner.worker.keys[wanted],plan.pairs[n/2].key[n%2]),"borrowed output maps exact original pair/key including second pair");++wanted;}
 C(!route.failed&&!route.producer.invalidRelease&&route.producer.releases==job.count&&route.producer.borrows==wanted&&route.owner.worker.puts==wanted&&route.owner.stored==wanted&&!route.owner.queued,"all native views acknowledged once; only wanted outputs written");
 RouteControl soft(plan,job);soft.Step();soft.owner.soft=true;C(soft.Step()==0&&!soft.drained&&soft.producer.borrowing,"viewport cancellation preserves borrowed output until consumer acknowledgment");soft.owner.worker.ack=true;C(soft.Step()==2&&soft.drained&&!soft.producer.borrowing&&soft.producer.releases==0,"viewport drain begins after borrowed consumer acknowledgment");
 RouteControl tight(plan,job);const FineJob::U64 availableBytes=AtlasView::GpuCap-tight.budget.viewerGpu-WorldCohort::GpuReservation(2);tight.budget.occupied=unsigned(availableBytes/FineJob::WindowBytes);if(tight.budget.occupied>FineJob::ActiveSlots())tight.budget.occupied=FineJob::ActiveSlots();tight.budget.fineGpu=FineJob::U64(tight.budget.occupied)*FineJob::WindowBytes;
 C(tight.Step()==0&&tight.failed&&tight.producer.borrows==0,"queued four-view tick enforces actual reservation before borrowing");
 RouteControl writing(plan,job);writing.Step();writing.owner.worker.ack=true;writing.budget=tight.budget;C(writing.Step()==0&&writing.failed&&writing.producer.releases==0,"write acknowledgment rechecks four-view reservation before ReleaseView");
}
