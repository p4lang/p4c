

#include "backends/p4tools/common/lib/util.h"

#include <cxxabi.h>

#include <lib/null.h>

#include <chrono>  // NOLINT cpplint throws a warning because Google has a similar library...
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <sstream>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_int/add.hpp>
#include <boost/multiprecision/cpp_int/divide.hpp>
#include <boost/multiprecision/detail/et_ops.hpp>
#include <boost/multiprecision/number.hpp>
#include <boost/none.hpp>
#include <boost/random/uniform_int_distribution.hpp>

namespace P4Tools {

boost::optional<uint32_t> TestgenUtils::currentSeed = boost::none;

boost::random::mt19937 TestgenUtils::rng;

std::string TestgenUtils::getTimeStamp() {
    // get current time
    auto now = std::chrono::system_clock::now();
    // get number of milliseconds for the current second
    // (remainder after division into seconds)
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    // convert to std::time_t in order to convert to std::tm (broken time)
    auto timer = std::chrono::system_clock::to_time_t(now);
    // convert to broken time
    std::tm* bt = std::localtime(&timer);
    CHECK_NULL(bt);
    std::stringstream oss;
    oss << std::put_time(bt, "%Y-%m-%d-%H:%M:%S");  // HH:MM:SS
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

void TestgenUtils::setRandomSeed(int seed) {
    currentSeed = seed;
    rng.seed(seed);
}

boost::optional<uint32_t> TestgenUtils::getCurrentSeed() { return currentSeed; }

uint64_t TestgenUtils::getRandInt(uint64_t max) {
    if (!currentSeed) {
        return 0;
    }
    boost::random::uniform_int_distribution<uint64_t> dist(0, max);
    return dist(rng);
}

uint64_t TestgenUtils::getRandPct() { return getRandInt(100); }

big_int TestgenUtils::getRandBigInt(big_int max) {
    if (!currentSeed) {
        return 0;
    }
    boost::random::uniform_int_distribution<big_int> dist(0, max);
    return dist(rng);
}

}  // namespace P4Tools
