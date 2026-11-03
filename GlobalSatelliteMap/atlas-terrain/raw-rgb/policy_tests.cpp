#include "RawMaterial.h"
#include <cstdio>
int main(){using namespace TerrainRaw; if(PrivateBytes!=1310720ULL||PrivateBytes-PackedBytes!=262144ULL||!Fits(Limit-PrivateBytes)||Fits(Limit-PrivateBytes+1))return 1;std::puts("PASS rawterrain fixed256 payload/extra262144/inclusive90MiB guard");}
