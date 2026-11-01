#include <string>
void DebugLog(const std::string& message);
#include <LocalizationLog.h>
void OtherTranslationUnit(bool enabled) { KLocLog::SetEnabled(enabled); }
