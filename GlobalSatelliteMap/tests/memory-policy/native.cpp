#define BOOST_ALL_NO_LIB
#define NOMINMAX
#include "../../atlas-terrain/world-native-rebind/NativeSession.h"
#include "../../atlas-terrain/fine-coordinator/FineCoordinator.h"
#include "../../GsmFineGpu.h"
typedef WorldNativeRebind::Session<WorldNative::StageAdapter<WorldNative::NativeStage> > NativeSession;
typedef FineCoordinator::Controller<NativeSession> NativeController;
FineWrite::RamStats CompileNativeStats(const NativeController&c){return c.MemoryStats();}
FineStore::U64 CompileNativeDebt(const NativeController&c){return c.RamBytes()+c.RetainedRamBytes()+c.RetainedCpuBytes()+c.RetainedGpuBytes();}
bool CompileNativeAdmission(const FineCoordinator::Residency&r){return FineCoordinator::StartupFits(r);}
