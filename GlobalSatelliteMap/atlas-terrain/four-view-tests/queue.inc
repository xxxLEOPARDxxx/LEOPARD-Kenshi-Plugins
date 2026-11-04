struct QueueNative {
 struct CohortOwner {unsigned submissions,count;CohortOwner():submissions(0),count(0){}WorldCohort::Status State(){return WorldCohort::Idle;}FineJob::U64 ReservedGpuBytes(){return 0;}HRESULT Submit(bool,const WorldCohort::Job&job,const WorldStream::CapturedSources&,const SourceReceipts::Scope&){++submissions;count=job.count;return S_OK;}}cohort;
 HRESULT result;unsigned captures;QueueNative():result(S_OK),captures(0){}CohortOwner&Cohort(){return cohort;}HRESULT RebindCapture(const WorldCohort::Job&,WorldStream::CapturedSources&){++captures;return result;}
};
struct QueueControl:Exact {
 struct Residency{};
 struct Owner {FineJob::Plan plan;unsigned pair;WorldCohort::Job job;FineStore::Ledger external;bool soft,queued;BYTE shaders[21920];QueueNative native;WorldStream::CapturedSources tokens;SourceReceipts::Scope scope;Owner():pair(0),soft(false),queued(false){}}owner,*owner_;
 bool retryPending_,failed;FineJob::Budget budget;
 QueueControl(const FineJob::Plan&plan):owner_(&owner),retryPending_(false),failed(false){owner.scope=plan.demand.scope;std::memset(&budget,0,sizeof(budget));budget.viewerGpu=FineJob::ViewerAllowance;budget.producerCpu=FineJob::Internal::ProducerCpuFloor();}
 bool Main(){return true;}bool Ready(){return !owner.queued;}bool CurrentBase(const FineJob::Demand&,const FineStore::Identity&){return true;}bool MakeBudget(const Residency&,const FineStore::Ledger&,FineJob::Budget&out){out=budget;return true;}void Hard(HRESULT){failed=true;}
 // EXTRACTED_QUEUE
};
static void Queues(const FineJob::Plan&plan,const WorldCohort::Job&job){
 QueueControl queue(plan);QueueControl::Residency residency;FineStore::Ledger external;FineStore::Identity identity;
 const FineJob::Budget base=queue.budget;const FineJob::U64 availableBytes=AtlasView::GpuCap-base.viewerGpu-WorldCohort::GpuReservation(2);queue.budget.occupied=unsigned(availableBytes/FineJob::WindowBytes);if(queue.budget.occupied>FineJob::ActiveSlots())queue.budget.occupied=FineJob::ActiveSlots();queue.budget.fineGpu=FineJob::U64(queue.budget.occupied)*FineJob::WindowBytes;
 C(!queue.Queue(plan,0,plan.demand,identity,job,residency,external)&&!queue.owner.native.captures&&!queue.failed&&!queue.retryPending_,"four-view Queue rejects two-view-only headroom before native rebind");
 queue.budget=base;queue.owner.native.result=HRESULT_FROM_WIN32(ERROR_BUSY);C(!queue.Queue(plan,0,plan.demand,identity,job,residency,external)&&queue.retryPending_&&!queue.failed&&!queue.owner.native.cohort.submissions,"native metadata BUSY preserves empty retryable four-view boundary");
 const FineJob::Budget admitted=queue.budget;queue.budget.occupied=unsigned(availableBytes/FineJob::WindowBytes);if(queue.budget.occupied>FineJob::ActiveSlots())queue.budget.occupied=FineJob::ActiveSlots();queue.budget.fineGpu=FineJob::U64(queue.budget.occupied)*FineJob::WindowBytes;C(!queue.Queue(plan,0,plan.demand,identity,job,residency,external)&&queue.owner.native.captures==1&&!queue.retryPending_,"retry rechecks four-view reservation when residency changes");queue.budget=admitted;
 queue.owner.native.result=S_OK;C(queue.Queue(plan,0,plan.demand,identity,job,residency,external)&&!queue.retryPending_&&queue.owner.native.cohort.submissions==1&&queue.owner.native.cohort.count==4&&queue.owner.queued,"successful retry submits exact four-view job after renewed admission");
}
