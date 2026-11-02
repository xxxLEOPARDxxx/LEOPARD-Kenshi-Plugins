#include "../GsmTownValues.h"
#include <kenshi/Globals.h>
typedef char TownCatalogValueAbi[sizeof(GsmTownValues::TownValue)==24?1:-1];
typedef char TownCatalogResultBound[sizeof(GsmTownValues::Result)<128*1024?1:-1];
HRESULT CompileTownValues(SharedKing*king,HMODULE engine,LONG generation,volatile LONG*live,GsmTownValues::Result&out){return GsmTownValues::Read(true,true,king,engine,generation,live,out);}
