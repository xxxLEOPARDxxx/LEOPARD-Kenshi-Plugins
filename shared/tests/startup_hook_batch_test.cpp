#include "../StartupHookBatch.h"
#include <cstdio>
#include <cstdlib>

using namespace StartupHooks;

namespace {
    const size_t Owner = 0x4200;
    int checks = 0, actions[12], calls = 0, failMask = 0, appliedMask = 0;
    bool badOriginal = false, throwApply = false, failApply = false;
    void* originals[4];

    void Check(bool passed, const char* message)
    {
        ++checks;
        if (!passed) { std::fprintf(stderr, "FAIL %s\n", message); std::exit(1); }
    }

    void* Address(size_t value) { return reinterpret_cast<void*>(value); }
    int Index(void* target) { return static_cast<int>(reinterpret_cast<size_t>(target)) - 1; }

    bool Immediate(void* target, void*, void** original)
    {
        const int i = Index(target);
        actions[calls++] = 40+i;
        if (failMask & (1<<i)) return false;
        *original = Address(50+i);
        return true;
    }

    int Queue(size_t id, void* target, void*, void** original)
    {
        Check(id == Owner, "queue uses owning module");
        const int i = Index(target);
        actions[calls++] = i;
        if (failMask & (1<<i)) return 1;
        if (!badOriginal || i != 1) *original = Address(50+i);
        return 0;
    }

    int Apply(size_t id)
    {
        Check(id == Owner, "apply uses owning module");
        actions[calls++] = 20;
        if (throwApply) throw 1;
        appliedMask = failApply ? 1 : 15 & ~failMask;
        return failApply ? 1 : 0;
    }

    void Reset()
    {
        calls = failMask = appliedMask = 0;
        badOriginal = throwApply = failApply = false;
        for (int i = 0; i != 4; ++i) originals[i] = NULL;
    }

    void Run(int scenario)
    {
        Reset();
        Hook rows[] = {
            Hook(Address(1), Address(11), &originals[0]),
            Hook(Address(2), Address(12), &originals[1]),
            Hook(Address(3), Address(13), &originals[2]),
            Hook(Address(4), Address(14), &originals[3]),
        };
        Backend backend(Owner, Queue, Apply);
        if (scenario == 1) backend = Backend();
        if (scenario == 2) failMask = 2;
        if (scenario == 3) failApply = true;
        if (scenario == 4) badOriginal = true;
        if (scenario == 5) rows[2].target = rows[0].target;
        if (scenario == 6) originals[2] = Address(222);
        if (scenario == 7) throwApply = true;
        if (scenario == 8) rows[1].target = NULL;
        if (scenario == 9) failMask = 15;
        if (scenario == 10) rows[1].enabled = false;
        if (scenario == 11) rows[1].original = rows[0].original;
        InstallWithBackend(rows, sizeof(rows)/sizeof(rows[0]), backend, Immediate);

        if (scenario == 1 || scenario == 5 || scenario == 11)
        {
            const bool repeatedSlot = scenario == 11;
            Check(calls == (repeatedSlot ? 3 : 4), "fallback runs eligible immediate hooks only");
            Check(actions[0] == 40 && actions[calls-1] == 43, "fallback preserves order");
            Check(actions[2] == (scenario == 5 ? 40 : repeatedSlot ? 43 : 42), "fallback target order");
        }
        else
        {
            const bool skip = scenario == 6 || scenario == 8 || scenario == 10;
            const bool apply = scenario != 4 && scenario != 9;
            Check(calls == 4 - (skip ? 1 : 0) + (apply ? 1 : 0), "single apply; no immediate retry");
            Check(actions[0] == 0 && actions[1] == ((scenario == 8 || scenario == 10) ? 2 : 1), "queue order");
            Check(actions[calls-1] == (apply ? 20 : 3), "apply follows queue construction");
        }

        if (scenario == 2) Check(rows[0].installed && !rows[1].installed && rows[2].installed, "queue failure stays local");
        if (scenario == 3 || scenario == 7)
        {
            Check(appliedMask == (scenario == 3 ? 1 : 0), "simulate partial activation or exception");
            for (int i = 0; i != 4; ++i)
                Check(originals[i] == Address(50+i) && !rows[i].installed, "preserve originals and withhold success");
        }
        if (scenario == 4) Check(!rows[0].installed && !rows[1].installed && originals[0], "missing trampoline stops apply");
        if (scenario == 6) Check(!rows[2].installed && originals[2] == Address(222), "never replace a live original");
        if (scenario == 8 || scenario == 10) Check(!rows[1].installed && !originals[1], "missing or disabled row untouched");
        if (scenario == 11) Check(!rows[1].installed && originals[0] == Address(50), "reused original slot is protected");
    }
}

int main()
{
    for (int scenario = 0; scenario != 12; ++scenario) Run(scenario);
    std::printf("PASS %d startup hook checks\n", checks);
    return 0;
}
