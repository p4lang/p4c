#include "backends/p4tools/common/lib/timer.h"

#include <algorithm>
#include <chrono>  // NOLINT linter forbids using chrono, but we don't have alternatives
#include <memory>
#include <unordered_map>
#include <utility>

namespace P4Tools {

namespace {

using Clock = std::chrono::high_resolution_clock;

/// Represents one specific time counter. The time counter entries form a tree - every non-root
/// time counter has a parent, which is the most inner counter currently active when this
/// counter has been created. Conversely, every counter can have any number of child counters,
/// which are counters invoked when this counter was also active.
struct CounterEntry {
    const char* name;
    std::unordered_map<std::string, std::unique_ptr<CounterEntry>> counters;
    Clock::duration duration;

    /// Lookup existing or create new child counter.
    CounterEntry* open_subcounter(const char* name) {
        auto it = counters.find(name);
        if (it == counters.end()) {
            it = counters.emplace(name, new CounterEntry(name)).first;
        }
        return it->second.get();
    }

    /// Adds specified duration to the current counter.
    void add(Clock::duration d) { duration += d; }

    explicit CounterEntry(const char* n) : name(n) {}
};

struct RootCounter {
    /// The topmost counter.
    CounterEntry counter;
    /// The most inner currently active counter.
    CounterEntry* current;
    Clock::time_point start;

    static RootCounter& get() {
        // TODO: not threadsafe currently. Instance of RootCounter could be thread_local:
        //
        // thread_local RootCounter root;
        //
        // however libgc cannot scan thread local data, which can lead to premature object
        // garbage collection.
        static RootCounter root;
        root.counter.duration = Clock::now() - root.start;
        return root;
    }

    CounterEntry* getCurrent() const { return current; }

    void setCurrent(CounterEntry* c) { current = c; }

 private:
    RootCounter() : counter("") {
        current = &counter;
        start = Clock::now();
    }
};

}  // namespace

// RAII helper which manages lifetime of one timer invocation.
struct ScopedTimer::Ctx {
    CounterEntry* parent = nullptr;
    CounterEntry* self = nullptr;
    Clock::time_point start_time;

    explicit Ctx(const char* counter_name) {
        start_time = Clock::now();
        // Push new active counter - the current active counter becomes the parent of this
        // counter, and this counter becomes the current active counter.
        parent = RootCounter::get().getCurrent();
        self = parent->open_subcounter(counter_name);
        RootCounter::get().setCurrent(self);
    }
    ~Ctx() {
        // Close the current timer invocation, measure time and add it to the counter.
        auto duration = Clock::now() - start_time;
        self->add(duration);
        // Restore previous counter as current.
        RootCounter::get().setCurrent(parent);
    }
};

ScopedTimer::ScopedTimer(const char* name) : ctx(new ScopedTimer::Ctx(name)) {}
ScopedTimer::~ScopedTimer() = default;

void withTimer(const char* counter_name, std::function<void()> fn) {
    ScopedTimer timer(counter_name);
    fn();
}

// Helper function, walks recursively tree of counter entries, collects counter values
// and serializes them into a list.
static void formatCounters(std::vector<TimerEntry>& out, CounterEntry& current,
                           std::string& namePrefix, size_t parentDurationMs) {
    auto prevNamePrefix = namePrefix;
    if (!namePrefix.empty()) {
        namePrefix = '\t' + namePrefix + '.';
    }
    namePrefix += current.name;
    auto currentTotalDuration =
        std::chrono::duration_cast<std::chrono::milliseconds>(current.duration);
    TimerEntry entry;
    entry.timerName = namePrefix;
    entry.milliseconds = currentTotalDuration.count();
    if (parentDurationMs == 0) {
        entry.relativeToParent = 1;
    } else {
        entry.relativeToParent = static_cast<float>(entry.milliseconds) / parentDurationMs;
    }
    out.emplace_back(entry);
    for (const auto& subcounter : current.counters) {
        formatCounters(out, *subcounter.second, namePrefix, entry.milliseconds);
    }
    namePrefix = prevNamePrefix;
}

std::vector<TimerEntry> getTimers() {
    std::vector<TimerEntry> ret;
    std::string namePrefix = "";
    formatCounters(ret, RootCounter::get().counter, namePrefix, 0);
    return ret;
}

}  // namespace P4Tools
