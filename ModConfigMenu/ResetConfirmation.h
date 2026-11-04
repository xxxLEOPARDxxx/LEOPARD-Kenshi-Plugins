#pragma once
#include <string>

// UI-thread request identity only: never retain a ModEntry or widget pointer.
namespace McmReset {
class Confirmation {
    std::string id_;
    unsigned long long generation_;
    bool pending_, decided_, confirmed_;
public:
    Confirmation() : generation_(0), pending_(false), decided_(false), confirmed_(false) {}
    bool IsPending() const { return pending_; }
    bool IsDecided() const { return decided_; }
    bool Matches(const std::string& id, unsigned long long generation) const {
        return pending_ && id_ == id && generation_ == generation;
    }
    bool Request(const std::string& id, unsigned long long generation) {
        if (pending_ || id.empty()) return false;
        id_ = id; generation_ = generation;
        pending_ = true; decided_ = confirmed_ = false;
        return true;
    }
    void Decide(bool confirm) {
        if (pending_ && !decided_) { decided_ = true; confirmed_ = confirm; }
    }
    void Cancel() {
        pending_ = decided_ = confirmed_ = false;
        generation_ = 0; id_.clear();
    }
    bool Take(const std::string& id, unsigned long long generation) {
        const bool reset = Matches(id, generation) && decided_ && confirmed_;
        Cancel(); // consume before any plugin callback can re-enter the UI
        return reset;
    }
};
}
