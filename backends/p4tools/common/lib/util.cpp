#include "backends/p4tools/common/lib/util.h"

#include <lib/null.h>

#include <chrono>  // NOLINT cpplint throws a warning because Google has a similar library...
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <optional>
#include <ratio>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_int/add.hpp>
#include <boost/multiprecision/detail/et_ops.hpp>
#include <boost/multiprecision/number.hpp>
#include <boost/random/uniform_int_distribution.hpp>

#include "ir/id.h"
#include "ir/irutils.h"
#include "ir/vector.h"
#include "lib/exceptions.h"

namespace P4Tools {

/* =========================================================================================
 *  Seeds, timestamps, randomness.
 * ========================================================================================= */

std::optional<uint32_t> Utils::currentSeed = std::nullopt;

boost::random::mt19937 Utils::rng(0);

std::string Utils::getTimeStamp() {
    // get current time
    auto now = std::chrono::system_clock::now();
    // get number of milliseconds for the current second
    // (remainder after division into seconds)
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    // convert to std::time_t in order to convert to std::tm (broken time)
    auto timer = std::chrono::system_clock::to_time_t(now);
    // convert to broken time
    std::tm *bt = std::localtime(&timer);
    CHECK_NULL(bt);
    std::stringstream oss;
    oss << std::put_time(bt, "%Y-%m-%d-%H:%M:%S");  // HH:MM:SS
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

void Utils::setRandomSeed(int seed) {
    if (currentSeed.has_value()) {
        BUG("Seed already initialized with %1%.", currentSeed.value());
    }
    currentSeed = seed;
    rng.seed(seed);
}

std::optional<uint32_t> Utils::getCurrentSeed() { return currentSeed; }

uint64_t Utils::getRandInt(uint64_t max) {
    if (!currentSeed) {
        return 0;
    }
    boost::random::uniform_int_distribution<uint64_t> dist(0, max);
    return dist(rng);
}

big_int Utils::getRandBigInt(big_int max) {
    if (!currentSeed) {
        return 0;
    }
    boost::random::uniform_int_distribution<big_int> dist(0, max);
    return dist(rng);
}

const IR::Constant *Utils::getRandConstantForWidth(int bitWidth) {
    auto maxVal = IR::getMaxBvVal(bitWidth);
    auto randInt = Utils::getRandBigInt(maxVal);
    const auto *constType = IR::getBitType(bitWidth);
    return IR::getConstant(constType, randInt);
}

const IR::Constant *Utils::getRandConstantForType(const IR::Type_Bits *type) {
    auto maxVal = IR::getMaxBvVal(type->width_bits());
    auto randInt = Utils::getRandBigInt(maxVal);
    return IR::getConstant(type, randInt);
}

/* =========================================================================================
 *  Other.
 * ========================================================================================= */

const IR::MethodCallExpression *Utils::generateInternalMethodCall(
    cstring methodName, const std::vector<const IR::Expression *> &argVector,
    const IR::Type *returnType) {
    auto *args = new IR::Vector<IR::Argument>();
    for (const auto *expr : argVector) {
        args->push_back(new IR::Argument(expr));
    }
    return new IR::MethodCallExpression(
        returnType,
        new IR::Member(new IR::Type_Method(new IR::ParameterList(), methodName),
                       new IR::PathExpression(new IR::Type_Extern("*"), new IR::Path("*")),
                       methodName),
        args);
}

}  // namespace P4Tools
