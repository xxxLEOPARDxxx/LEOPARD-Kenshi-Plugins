#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include PAINTED_MIP_POLICY_HEADER
static unsigned checks=0;
static void Check(bool ok,const char* message)
{++checks;if(!ok){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
// The adapter models the actual Resource contract: notify adds the texture's
// base-level size, setToLoaded changes state only, unload debits loaded objects.
// Native calls cannot run in a standalone test; the real sequencing seam does.
struct NativeBoundary
{
    bool ready,loaded,uploadFail,verifyFail;
    unsigned throwAt,preflights,uploads,notifies,finishes,order,managerBytes;
    NativeBoundary():ready(true),loaded(false),uploadFail(false),verifyFail(false),throwAt(0),
        preflights(0),uploads(0),notifies(0),finishes(0),order(0),managerBytes(0){}
    bool Preflight(){++preflights;return ready&&!loaded;}
    bool Upload(){++uploads;order=order*10+1;if(throwAt==1)throw 1;return !uploadFail;}
    void Notify(){++notifies;managerBytes+=256;order=order*10+2;if(throwAt==2)throw 2;}
    void SetLoaded(){++finishes;loaded=true;order=order*10+3;if(throwAt==3)throw 3;}
    bool Loaded(){order=order*10+4;if(throwAt==4)throw 4;return loaded&&!verifyFail;}
    void Destroy(){if(loaded){Check(managerBytes>=256,"no accounting underflow");managerBytes-=256;loaded=false;}}
};
int main()
{
    using namespace PaintedMipPolicy;
    {
        NativeBoundary b;bool uncertain=false;
        Check(CompleteUpload(b,uncertain),"native completion succeeds");
        Check(!uncertain&&b.loaded&&b.managerBytes==256,"manager credited once and resource Loaded");
        Check(b.order==1234&&b.uploads==1&&b.notifies==1&&b.finishes==1,"upload-notify-setLoaded-verify exact order");
        Check(!CompleteUpload(b,uncertain)&&b.managerBytes==256&&b.uploads==1&&b.notifies==1,"loaded resource never credited/uploaded twice");
        b.Destroy();Check(b.managerBytes==0,"successful native cleanup balances one credit/debit");
    }
    {NativeBoundary b;b.ready=false;bool u=false;Check(!CompleteUpload(b,u)&&!u&&!b.uploads&&!b.managerBytes,"preflight refusal permits clean destruction without writes");}
    {NativeBoundary b;b.loaded=true;b.managerBytes=256;bool u=false;Check(!CompleteUpload(b,u)&&!u&&!b.uploads&&b.managerBytes==256,"preloaded resource refused before writes/credit");}
    {
        NativeBoundary b;b.uploadFail=true;bool u=false;
        Check(!CompleteUpload(b,u)&&u&&b.uploads==1&&!b.notifies&&!b.finishes,"failed GPU upload quarantines without credit");
        b.uploadFail=false;Check(!CompleteUpload(b,u)&&b.preflights==1&&b.uploads==1,"uncertain upload cannot be retried");
    }
    for(unsigned stage=1;stage<=4;++stage)
    {
        NativeBoundary b;b.throwAt=stage;bool u=false,thrown=false;
        try{CompleteUpload(b,u);}catch(int value){thrown=value==int(stage);}
        Check(thrown&&u,"exception at any submitted/completion stage preserves quarantine");
        Check(b.uploads==1&&b.notifies==(stage>=2?1u:0u)&&b.finishes==(stage>=3?1u:0u),"exception stops later native stages");
        unsigned credit=b.managerBytes;b.throwAt=0;
        Check(!CompleteUpload(b,u)&&b.preflights==1&&b.uploads==1&&b.managerBytes==credit,"partial native credit never retried");
    }
    {NativeBoundary b;b.verifyFail=true;bool u=false;Check(!CompleteUpload(b,u)&&u&&b.managerBytes==256&&b.finishes==1,"unverified Loaded state retains credited resource");}
    Plan p[2];
    Check(Select(0,1024,1024,12,p[0]),"overlay BGRA full-chain plan");
    Check(p[0].additionalMips==10&&p[0].levels==11&&p[0].dxgiFormat==87&&p[0].gpuBytes==22369616,"overlay plan literal native dimensions/bytes");
    Check(Select(1,2048,2048,13,p[1]),"colour RGBA full-chain plan");
    Check(p[1].additionalMips==11&&p[1].levels==12&&p[1].dxgiFormat==28&&p[1].gpuBytes==89478480,"colour plan literal native dimensions/bytes");
    size_t budget=0;Check(Budget(p,budget)&&budget==111848096,"both arrays exact full-chain byte budget");
    Plan untouched=p[0];
    Check(!Select(0,2048,2048,12,untouched)&&untouched.gpuBytes==p[0].gpuBytes,"wrong overlay size leaves plan unchanged");
    Check(!Select(1,1024,2048,13,untouched),"non-square colour refused");
    Check(!Select(2,2048,2048,13,untouched),"third kind refused");
    Check(!Select(1,2048,2048,14,untouched),"pixel conversion/unsupported format refused");
    Check(!Select(0,size_t(-1),1024,12,untouched),"oversized/truncated width refused");
    p[1].gpuBytes=128u*1024u*1024u;budget=99;
    Check(!Budget(p,budget)&&budget==99,"GPU cap refusal leaves budget unchanged");
    Check(Select(1,2048,2048,12,p[1])&&p[1].dxgiFormat==87,"colour BGRA mapping");
    Check(Select(0,1024,1024,13,p[0])&&p[0].dxgiFormat==28,"overlay RGBA mapping");
    std::printf("%u production painted full-mip/accounting checks passed\n",checks);
}
