#ifndef LIB_TIMER_H_
#define LIB_TIMER_H_

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Util {

/// Runs specified function and measures it's execution duration under specified timer name.
/// Timers can be nested. Measured durations are added under given timer name. In case of nested
/// timers, the durations are counter separately for each unique ordered list of nested
/// timer invocations, e.g.
///
/// withTimer("A", [&] {
///     withTimer("C", [&] { f1() });
/// });
/// withTimer("B", [&] {
///     withTimer("C", [&] { f2() });
/// });
///
/// uses two separate counters denoted as "A.C" and "B.C".
void withTimer(const char *timerName, const std::function<void()> &fn);

struct TimerEntry {
    /// Counter name. If a timer "Y" was invoked inside timer "X", its timer name is "X.Y".
    std::string timerName;
    /// Total duration in milliseconds.
    size_t milliseconds;
    /// Portion of this timer's name relative to the parent timer.
    float relativeToParent;
};

/// Returns list of all timers for and their current values.
std::vector<TimerEntry> getTimers();

// Internal implementation.
struct ScopedTimerCtx;

/// Similar to withTimer function, measures execution time elapsed from instance creation to
/// destruction.
class ScopedTimer {
 public:
    explicit ScopedTimer(const char *name);
    ScopedTimer(const ScopedTimer &) = delete;
    ~ScopedTimer();

 private:
    std::unique_ptr<ScopedTimerCtx> ctx;
};

}  // namespace Util

#endif /* LIB_TIMER_H_ */
