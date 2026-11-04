#define BOOST_ALL_NO_LIB
#define NOMINMAX
#include "../../atlas-terrain/world-native-rebind/NativeSession.h"
#include "../../atlas-terrain/fine-coordinator/FineCoordinator.h"
typedef WorldNativeRebind::Session<WorldNative::StageAdapter<WorldNative::NativeStage> > ActualSession;
typedef FineCoordinator::Controller<ActualSession> ActualController;
FineCoordinator::Progress CompileActualNativeProgress(const ActualController&owner){return owner.ObserveProgress();}
