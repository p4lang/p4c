#ifndef BACKENDS_P4TOOLS_COMMON_LIB_UTIL_H_
#define BACKENDS_P4TOOLS_COMMON_LIB_UTIL_H_

#include <stdint.h>

#include <string>
#include <utility>

#include <boost/format.hpp>
#include <boost/optional/optional.hpp>
#include <boost/random/mersenne_twister.hpp>

#include "lib/big_int_util.h"

namespace P4Tools {

/// Helper function for @printFeature
inline std::string logHelper(boost::format& f) { return f.str(); }

/// Helper function for @printFeature
template <class T, class... Args>
std::string logHelper(boost::format& f, T&& t, Args&&... args) {
    return logHelper(f % std::forward<T>(t), std::forward<Args>(args)...);
}

/// A helper function that allows us to configure logging for a particular feature. This code is
/// taken from
// https://stackoverflow.com/a/25859856
template <typename... Arguments>
void printFeature(const std::string& label, int level, const std::string& fmt,
                  Arguments&&... args) {
    boost::format f(fmt);

    auto result = logHelper(f, std::forward<Arguments>(args)...);

    LOG_FEATURE(label.c_str(), level, result);
}

/// General utility functions that are not present in the compiler framework.
class TestgenUtils {
 private:
    /// The random generator of this project. It is initialized with the input seed.
    static boost::random::mt19937 rng;

    /// Stores the state of the PRNG.
    static boost::optional<uint32_t> currentSeed;

 public:
    /// Return the current timestamp with millisecond accuracy.
    /// Format: year-month-day-hour:minute:second.millisecond
    /// Borrowed from https://stackoverflow.com/a/35157784
    static std::string getTimeStamp();

    /// Initialize the random generator with an integer seed. This also seeds @var currentSeed.
    /// Uses boost's mersenne twister.
    static void setRandomSeed(int seed);

    /// @returns currentSeed.
    static boost::optional<uint32_t> getCurrentSeed();

    /// @returns a random integer in the range [0, @param max]. Always return 0 if no seed is set.
    static uint64_t getRandInt(uint64_t max);

    /// @returns a random big integer in the range [0, @param max]. Always return 0 if no seed is
    /// set.
    static big_int getRandBigInt(big_int max);
};

}  // namespace P4Tools

#endif /* BACKENDS_P4TOOLS_COMMON_LIB_UTIL_H_ */
