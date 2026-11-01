#include "ProbePolicy.h"
#include "FixedProgramCache.h"
#include <iostream>
#include <cstdlib>
void Check(bool ok, const char *message)
{
    if (!ok)
    {
        std::cerr << message << "\n";
        std::exit(1);
    }
}
int main()
{
    using namespace ProbePolicy;
    RequestLatch request;
    Check(!request.Changed(17),"startup token must not capture");
    Check(!request.Changed(17),"unchanged token ignored");
    Check(request.Changed(18),"new token captured once");
    Check(!request.Changed(18),"repeat token ignored");
    Check(!request.Changed(0),"zero token cancels without capture");
    Check(request.Changed(1),"token after zero accepted");
    Check(request.Changed(0xffffffffUL),"maximum token accepted");
    Check(!request.Changed(0)&&request.Changed(1),"token wrap accepted");
    Check(!PollDue(100,200)&&PollDue(100,350),"poll capped to four per second");
    Check(PollDue(0xffffff80UL,0x80UL),"poll clock wrap");
    int owned = 0, other = 0;
    const int *many[65];
    for (int i = 0; i < 65; ++i)
        many[i] = &other;
    many[64] = &owned;
    Check(Contains(many, 65, &owned), "cleanup membership beyond64");
    Check(!Contains(many, 64, &owned), "cleanup missing identity");
    Check(!Contains(static_cast<const int *const *>(NULL), 0, &owned), "empty cleanup list");
    Check(Cell(0) == 0, "origin");
    Check(Cell(1024) == 1, "positive boundary");
    Check(Cell(-1) == -1, "negative floor");
    Check(Cell(-1024) == -1, "negative boundary");
    Check(Centre(0) == 512, "centre");
    Check(Centre(-1) == -512, "negative centre");
    Check(!SafeCell(1001) && SafeCell(-1000), "cell cap");
    Check(ValidMode(0) && ValidMode(4) && !ValidMode(5), "mode cap");
    Check(std::fabs(NativeReconstructedX(512, 4000, 10000) - 512) > 250,
          "installed perspective reconstruction cannot preserve orthographic X");
    Check(std::fabs(NativeReconstructedX(0, 4000, 10000)) < 1e-12,
          "perspective model centre masks mismatch");
    Check(SafeAuxiliaryCleanup(true,&owned,&owned),"owned auxiliary same manager cleanup");
    Check(!SafeAuxiliaryCleanup(true,&owned,&other),"different live manager must retain auxiliary resources");
    Check(!SafeAuxiliaryCleanup(true,&owned,NULL),"unknown manager must retain auxiliary resources");
    Check(SafeAuxiliaryCleanup(false,&owned,NULL),"no auxiliary workspace needs no manager proof");
    Check(ValidShaderSource("float4 main(){}",15),"ASCII shader source accepted");
    Check(!ValidShaderSource("",0),"empty shader refused");
    Check(!ValidShaderSource(NULL,1),"null shader refused");
    Check(!ValidShaderSource("abc\0def",7),"embedded shader NUL refused");
    Check(!ValidShaderSource("\xFF",1),"non-ASCII shader refused");
    Check(!ValidShaderSource("x",32769),"oversized source refused before reading bytes");
    FixedProgramCache programs;
    Check(programs.Inspect(&owned,0,NULL,"vs")==FixedProgramCache::Create,"fresh shader slot create");
    Check(programs.Inspect(&owned,0,&other,"vs")==FixedProgramCache::Refuse,"foreign existing shader refused");
    Check(!programs.Record(&owned,0,NULL,"vs"),"null program cannot be cached");
    Check(programs.Record(&owned,0,&other,"vs"),"owned shader recorded");
    Check(programs.Inspect(&owned,0,&other,"vs")==FixedProgramCache::Reuse,"unchanged shader reused");
    Check(programs.Inspect(&owned,0,&other,"changed")==FixedProgramCache::Refuse,"source change requires diagnostic restart");
    Check(programs.Inspect(&owned,0,NULL,"vs")==FixedProgramCache::Refuse,"externally removed shader refused");
    Check(programs.Inspect(&other,0,&other,"vs")==FixedProgramCache::Refuse,"changed compositor owner refused");
    Check(!programs.Record(&owned,0,&owned,"vs"),"cache identity cannot be overwritten");
    Check(programs.Record(&owned,1,&owned,"ps"),"second fixed shader recorded");
    Check(programs.Record(&owned,2,&programs,"sun"),"third fixed shader recorded");
    Check(programs.Count()==3&&!programs.Record(&owned,3,&programs,"extra"),"cache bounded to three programs");
    Check(programs.Owns(0,&other)&&!programs.Owns(1,&other)&&!programs.Owns(3,&other),"cleanup recognizes exact owned slot only");
    Check(programs.Inspect(NULL,0,&other,"vs")==FixedProgramCache::Refuse,"unknown manager refused");
    FixedProgramCache empty;
    Check(empty.Inspect(&owned,0,NULL,"")==FixedProgramCache::Refuse,"empty cache source refused");
    Check(empty.Inspect(&owned,0,NULL,std::string(32769,'x'))==FixedProgramCache::Refuse,"cache source bytes bounded");
    std::cout << "49 fixed-grid/projection/cleanup/source/request/cache checks passed\n";
}
