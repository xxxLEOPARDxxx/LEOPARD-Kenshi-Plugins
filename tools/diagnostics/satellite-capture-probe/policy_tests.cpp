#include "ProbePolicy.h"
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
    std::cout << "13 fixed-grid/projection/cleanup checks passed\n";
}
