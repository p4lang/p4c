#include "lib/timer.h"

#include <chrono>  // NOLINT linter forbids using chrono, but we don't have alternatives
#include <memory>
#include <unordered_map>
#include <utility>

namespace Util {

namespace {

using Clock = std::chrono::high_resolution_clock;

/// Represents one specific time counter. The time counter entries form a tree - every non-root
/// time counter has a parent, which is the most inner counter currently active when this
/// counter has been created. Conversely, every counter can have any number of child counters,
/// which are counters invoked when this counter was also active.
struct CounterEntry {
    const char *name;
    std::unordered_map<std::string, std::unique_ptr<CounterEntry>> counters;
    Clock::duration duration{};

    /// Lookup existing or create new child counter.
    CounterEntry *openSubcounter(const char *name) {
        auto it = counters.find(name);
        if (it == counters.end()) {
            it = counters.emplace(name, new CounterEntry(name)).first;
        }
        return it->second.get();
    }

    /// Adds specified duration to the current counter.
    void add(Clock::duration d) { duration += d; }

    explicit CounterEntry(const char *n) : name(n) {}
};

struct RootCounter {
    /// The topmost counter.
    CounterEntry counter;
    /// The most inner currently active counter.
    CounterEntry *current;
    Clock::time_point start;

    static RootCounter &get() {
        // TODO: not threadsafe currently. Instance of RootCounter could be thread_local:
        //
        // thread_local RootCounter root;
        //
        // however libgc cannot scan thread local data, which can lead to premature object
        // garbage collection.
        static RootCounter ROOT;
        ROOT.counter.duration = Clock::now() - ROOT.start;
        return ROOT;
    }

    CounterEntry *getCurrent() const { return current; }

    void setCurrent(CounterEntry *c) { current = c; }

 private:
    RootCounter() : counter(""), current(&counter) { start = Clock::now(); }
};

}  // namespace

#pragma GCC diagnostic push
// Ignore a bogus subobject-linkage warning, which can occur in unity builds.
// the #if pragma is a little awkward because some preprocessors do not like ||
#if defined(__has_warning)
#if __has_warning("-Wsubobject-linkage")
#pragma GCC diagnostic ignored "-Wsubobject-linkage"
#endif
#else
#pragma GCC diagnostic ignored "-Wsubobject-linkage"
#endif
// RAII helper which manages lifetime of one timer invocation.
struct ScopedTimerCtx {
    CounterEntry *parent = nullptr;
    CounterEntry *self = nullptr;
    Clock::time_point startTime;

    explicit ScopedTimerCtx(const char *timerName)
        : parent(RootCounter::get().getCurrent()), self(parent->openSubcounter(timerName)) {
        startTime = Clock::now();
        // Push new active counter - the current active counter becomes the parent of this
        // counter, and this counter becomes the current active counter.
        RootCounter::get().setCurrent(self);
    }
    ~ScopedTimerCtx() {
        // Close the current timer invocation, measure time and add it to the counter.
        auto duration = Clock::now() - startTime;
        self->add(duration);
        // Restore previous counter as current.
        RootCounter::get().setCurrent(parent);
    }
};
#pragma GCC diagnostic pop

ScopedTimer::ScopedTimer(const char *name) : ctx(new ScopedTimerCtx(name)) {}

ScopedTimer::~ScopedTimer() = default;

void withTimer(const char *timerName, const std::function<void()> &fn) {
    ScopedTimer timer(timerName);
    fn();
}

// Helper function, walks recursively tree of counter entries, collects counter values
// and serializes them into a list.
static void formatCounters(std::vector<TimerEntry> &out, CounterEntry &current,
                           std::string &namePrefix, size_t parentDurationMs) {
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
    for (const auto &subcounter : current.counters) {
        formatCounters(out, *subcounter.second, namePrefix, entry.milliseconds);
    }
    namePrefix = prevNamePrefix;
}

std::vector<TimerEntry> getTimers() {
    std::vector<TimerEntry> ret;
    std::string namePrefix;
    formatCounters(ret, RootCounter::get().counter, namePrefix, 0);
    return ret;
}

}  // namespace Util
