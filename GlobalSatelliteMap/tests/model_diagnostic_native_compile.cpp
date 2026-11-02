#include "../GsmModelDiagnostic.h"
typedef char DiagnosticReportBound[sizeof(GsmModelDiagnostic::Report)<65536?1:-1];
typedef char DiagnosticStateBound[sizeof(GsmModelDiagnostic::Once)<65536?1:-1];
HRESULT CompileModelDiagnostic(GsmModelDiagnostic::Once&once,Town*town,Ogre::SceneManager*scene,LONG generation,volatile LONG*live,bool verified,bool*newReport){return once.Capture(true,town,scene,generation,live,verified,newReport);}
