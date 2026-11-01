#pragma once
#include <Debug.h>

// One state per DLL, shared by every translation unit. Settings callbacks and
// localization loading run on the UI thread, just like Localization.h itself.
// No INI reads, callbacks, allocations or log checks in Tr()/TrFmt().
namespace KLocLog
{
    struct State
    {
        bool enabled;
        char summary[256];
        State() : enabled(false) { summary[0] = '\0'; }
    };
    inline State& Current() { static State state; return state; }
    inline void SetEnabled(bool enabled)
    {
        State& state = Current();
        const bool rising = enabled && !state.enabled;
        state.enabled = enabled;
        // Also support turning Debug on after MCM has already translated the
        // settings page. Report the existing catalogue, without reloading it.
        if (rising && state.summary[0]) DebugLog(state.summary);
    }
    inline void SetSummary(const char* text)
    {
        State& state = Current();
        unsigned int i = 0;
        if (text) for (; i + 1 < sizeof(state.summary) && text[i]; ++i)
            state.summary[i] = text[i];
        state.summary[i] = '\0';
        if (state.enabled && state.summary[0]) DebugLog(state.summary);
    }
    // Config parsers/setters may return early or roll back a failed write.
    // Follow their final, committed flag instead of the intermediate value.
    class FollowConfiguration
    {
        const bool& enabled;
        FollowConfiguration(const FollowConfiguration&);
        FollowConfiguration& operator=(const FollowConfiguration&);
    public:
        explicit FollowConfiguration(const bool& value) : enabled(value) {}
        ~FollowConfiguration() { SetEnabled(enabled); }
    };
}
