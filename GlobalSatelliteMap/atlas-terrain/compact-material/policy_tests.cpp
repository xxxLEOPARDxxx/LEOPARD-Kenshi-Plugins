#include "CompactMaterial.h"
#include <cstdio>
int main(){using namespace CompactMaterial;Plan p; if(!FixedPlan(p)||p.layers[0]!=6||p.layers[1]!=6||p.layers[2]!=1||p.layers[3]!=1||p.selector[0]!=18||p.selector[1]!=4||p.bytes!=83885956ULL)return 1;if(!Fits(90ULL*1024*1024-p.bytes)||Fits(90ULL*1024*1024-p.bytes+1))return 2;std::puts("PASS compact fixed-case real plan/budget");}
